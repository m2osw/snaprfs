// Copyright (c) 2019-2024  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snaprfs
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


// self
//
#include    "data_receiver.h"

#include    "server.h"



// snaprfs
//
#include    <snaprfs/exception.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/as_root.h>
#include    <snapdev/chownnm.h>
#include    <snapdev/pathinfo.h>


// C
//
#include    <fcntl.h>


// last include
//
#include    <snapdev/poison.h>



namespace rfs_daemon
{
namespace
{
int         g_identifier = 0;
}



data_receiver::data_receiver(
          server * s
        , std::string const & filename
        , std::uint32_t id
        , std::string const & temp_path
        , addr::addr const & address
        , ed::mode_t mode)
    : tcp_client_connection(address, mode)
    , f_server(s)
    , f_filename(filename)
    , f_id(id)
    , f_path_part(temp_path)
{
    set_name("data_receiver");

    non_blocking();

    if(f_filename.empty())
    {
        throw rfs::missing_parameter("filename cannot be empty in data_receiver");
    }
    if(f_path_part.empty())
    {
        throw rfs::missing_parameter("temp_path cannot be empty in data_receiver");
    }
    if(f_path_part.back() != '/')
    {
        f_path_part += '/';
    }

    // send info about the file we want to download to the sender
    //
    // (since the sender can send multiple messages about changing
    // files simultaneously)
    //
    file_request request;
    request.f_id = f_id;

    char const * d(reinterpret_cast<char const *>(&request));
    f_request.insert(f_request.end(), d, d + sizeof(request));
}


void data_receiver::set_login_info(std::string const & login_name, std::string const & password)
{
    f_login_name = login_name;
    f_password = password;
}


ssize_t data_receiver::write(void const * data, std::size_t length)
{
    if(get_socket() == -1)
    {
        errno = EBADF;
        return -1;
    }

    if(data != nullptr && length > 0)
    {
        char const * d(reinterpret_cast<char const *>(data));
        f_request.insert(f_request.end(), d, d + length);
        return length;
    }

    return 0;
}


bool data_receiver::is_writer() const
{
    return get_socket() != -1 && !f_request.empty();
}


void data_receiver::process_read()
{
    // use RAII to process the next level on exit wherever it happens
    //
    class on_exit
    {
    public:
        on_exit(data_receiver::pointer_t receiver)
            : f_receiver(receiver)
        {
        }

        ~on_exit()
        {
            f_receiver->tcp_client_connection::process_read();
        }

    private:
        data_receiver::pointer_t    f_receiver;
    };
    on_exit raii_on_exit(std::dynamic_pointer_cast<data_receiver>(shared_from_this()));

    if(get_socket() == -1)
    {
        return;
    }

    // read the header
    //
    int r(0);
    if(f_received_bytes < sizeof(f_header))
    {
        while(f_received_bytes < sizeof(f_header))
        {
            r = read(&f_header + f_received_bytes, sizeof(f_header) - f_received_bytes);
            if(r == -1)
            {
                SNAP_LOG_ERROR
                    << "an I/O error occurred while reading data header."
                    << SNAP_LOG_SEND;
                process_error();
                return;
            }
            if(r == 0)
            {
                return;
            }
            f_received_bytes += r;
        }

        if(f_header.f_magic[0] != 'D'
        || f_header.f_magic[1] != 'A'
        || f_header.f_magic[2] != 'T'
        || f_header.f_magic[3] != 'A')
        {
            SNAP_LOG_ERROR
                << "header magic is not 'DATA'."
                << SNAP_LOG_SEND;
            process_error();
            return;
        }
        if(f_id != f_header.f_id)
        {
            SNAP_LOG_ERROR
                << "file id mismatched, expected \""
                << f_id
                << "\", receiving \""
                << f_header.f_id
                << "\" instead."
                << SNAP_LOG_SEND;
            process_error();
            return;
        }

        // while receiving, use a temporary file
        //
        // the f_path_part has an ending '/' (see constructor)
        //
        ++g_identifier;
        f_receiving_filename = f_path_part;
        f_receiving_filename += snapdev::pathinfo::basename(f_filename);
        f_receiving_filename += '-';
        f_receiving_filename += std::to_string(g_identifier);
        f_receiving_filename += ".tmp";

        // we need to also read the user & group names
        //
        f_header_size = sizeof(f_header)
                        + f_header.f_username_length
                        + f_header.f_groupname_length
                        + f_header.f_login_name_length
                        + f_header.f_password_length;
    }

    // read the user and group name which directly follow the header
    //
    if(f_received_bytes < f_header_size)
    {
        while(f_received_bytes < f_header_size)
        {
            // note: the f_names buffer is allocated once on creation with
            //       1024 bytes since the maximum size for each name is
            //       255 it will always be enough
            //
            std::size_t const size_left(f_header_size - f_received_bytes);
            r = read(f_names.data(), std::min(size_left, f_names.size()));
            if(r == -1)
            {
                SNAP_LOG_ERROR
                    << "an I/O error occurred while receiving file data for \""
                    << f_filename
                    << "\"."
                    << SNAP_LOG_SEND;
                process_error();
                return;
            }
            if(r == 0)
            {
                return;
            }
            f_received_bytes += r;
        }

        // verify the login/password
        //
        if(!f_login_name.empty()
        || !f_password.empty())
        {
            std::string const login_name(f_names.data() + f_header.f_username_length + f_header.f_groupname_length, f_header.f_login_name_length);
            std::string const password(f_names.data() + f_header.f_username_length + f_header.f_groupname_length + f_header.f_login_name_length, f_header.f_password_length);
            if(f_login_name != login_name
            || f_password != password)
            {
                SNAP_LOG_ERROR
                    << "sender does not know the correct login name and/or password."
                    << SNAP_LOG_SEND;
                process_error();
                return;
            }
        }

        // note: we receive the file as the snaprfs user
        //
        f_output.open(
                  f_receiving_filename
                , std::ios_base::trunc | std::ios_base::binary | std::ios_base::ate);
        if(!f_output.is_open())
        {
            int const e(errno);
            SNAP_LOG_ERROR
                << "could not open output file \""
                << f_receiving_filename
                << "\" for writing (errno: "
                << e
                << ", "
                << strerror(e)
                << ")."
                << SNAP_LOG_SEND;
            process_error();
            return;
        }
    }

    // read the file contents
    //
    while(f_received_bytes < f_header_size + f_header.f_size)
    {
        std::uint8_t buf[1024 * 4];
        std::size_t const size_left(f_header_size + f_header.f_size - f_received_bytes);
        r = read(buf, std::min(size_left, sizeof(buf)));
        if(r == -1)
        {
            SNAP_LOG_ERROR
                << "an I/O error occurred while receiving file data for \""
                << f_filename
                << "\"."
                << SNAP_LOG_SEND;
            process_error();
            return;
        }
        if(r == 0)
        {
            return;
        }
        f_murmur3.add_data(buf, r);
        f_output.write(reinterpret_cast<char const *>(buf), r);
        f_received_bytes += r;
    }

    // read the footer
    //
    if(f_received_bytes < f_header_size + f_header.f_size + sizeof(f_footer))
    {
        while(f_received_bytes < f_header_size + f_header.f_size + sizeof(f_footer))
        {
            std::size_t const bytes_read(f_received_bytes - f_header_size - f_header.f_size);
            r = read(&f_footer + bytes_read, sizeof(f_footer) - bytes_read);
            if(r == -1)
            {
                SNAP_LOG_ERROR
                    << "an I/O error occurred while reading data footer."
                    << SNAP_LOG_SEND;
                process_error();
                return;
            }
            if(r == 0)
            {
                return;
            }
            f_received_bytes += r;
        }

        f_output.close();

        if(f_footer.f_end[0] != 'E'
        || f_footer.f_end[1] != 'N'
        || f_footer.f_end[2] != 'D'
        || f_footer.f_end[3] != '!')
        {
            SNAP_LOG_ERROR
                << "footer magic is not 'END!'."
                << SNAP_LOG_SEND;
            process_error();
            return;
        }

        murmur3::hash const h(f_murmur3.flush());
        murmur3::hash received;
        received.set(f_footer.f_murmur3);
        if(h != received)
        {
            SNAP_LOG_ERROR
                << "murmur3 hashes do not match (received: "
                << received.to_string()
                << ", computed: "
                << h.to_string()
                << ")."
                << SNAP_LOG_SEND;
            process_error();
            return;
        }

        // we may not own the file (we are "snaprfs", after all), so we become
        // root and change the ownership and mode, and finally rename the
        // file so it gets copied to a new location; we can then
        // drop back as "snaprfs"
        //
        snapdev::as_root safe_root;

        // TODO: I think that the user/group names should be changed
        //       just before the rename (just before the utimensat()
        //       actually) since the file is considered invalid until
        //       verified otherwise by the murmur3 checksum
        //
        std::string const username(f_names.data(), f_header.f_username_length);
        std::string const groupname(f_names.data() + f_header.f_username_length, f_header.f_groupname_length);
        if(snapdev::chownnm(f_receiving_filename, username, groupname) != 0)
        {
            int const e(errno);
            SNAP_LOG_RECOVERABLE_ERROR
                << "could not change user and/or group name of output file \""
                << f_receiving_filename
                << "\" (errno: "
                << e
                << ", "
                << strerror(e)
                << ")."
                << SNAP_LOG_SEND;
            // continue in this case, although the file may not be readable
            // by the service owning this file as a result...
        }

        if(chmod(f_receiving_filename.c_str(), f_header.f_mode) != 0)
        {
            int const e(errno);
            SNAP_LOG_RECOVERABLE_ERROR
                << "could not change mode (chmod) of output file \""
                << f_receiving_filename
                << "\" (errno: "
                << e
                << ", "
                << strerror(e)
                << ")."
                << SNAP_LOG_SEND;
            // continue in this case, although the file may not be readable
            // by the service owning this file as a result...
        }

        timespec times[2] = {
            // atime
            {
                .tv_sec = 0,
                .tv_nsec = UTIME_OMIT,
            },
            // mtime
            {
                .tv_sec = static_cast<time_t>(f_header.f_mtime_sec),
                .tv_nsec = static_cast<long int>(f_header.f_mtime_nsec),
            },
        };
        if(utimensat(AT_FDCWD, f_receiving_filename.c_str(), times, 0) != 0)
        {
            int const e(errno);
            SNAP_LOG_MAJOR
                << "could not change modification time of output file \""
                << f_receiving_filename
                << "\" (errno: "
                << e
                << ", "
                << strerror(e)
                << ")."
                << SNAP_LOG_SEND;
            // continue in this case, although the file may not be readable
            // by the service owning this file as a result...
        }

        // rename(2) is atomic and does not require us to first delete
        // the destination file
        //
        r = rename(f_receiving_filename.c_str(), f_filename.c_str());
        if(r != 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR
                << "renaming of received file \""
                << f_receiving_filename
                << "\" to \""
                << f_filename
                << "\" failed with error: "
                << e
                << ", "
                << strerror(e)
                << "."
                << SNAP_LOG_SEND;
            process_error();
            return;
        }

        f_server->refresh_file(f_filename);

        remove_from_communicator();
    }
}


void data_receiver::process_write()
{
    if(get_socket() != -1)
    {
        errno = 0;
        ssize_t const r(tcp_client_connection::write(&f_request[f_position], f_request.size() - f_position));
        if(r > 0)
        {
            // some data was written
            //
            f_position += r;
            if(f_position >= f_request.size())
            {
                f_request.clear();
                f_position = 0;
                process_empty_buffer();
            }
        }
        else if(r < 0 && errno != 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            // connection is considered bad, generate an error
            //
            int const e(errno);
            SNAP_LOG_ERROR
                << "an error occurred while writing to socket of \""
                << get_name()
                << "\" (errno: "
                << e
                << " -- "
                << strerror(e)
                << ")."
                << SNAP_LOG_SEND;
            process_error();
            return;
        }
    }

    // process next level too
    tcp_client_connection::process_write();
}


void data_receiver::process_error()
{
    if(!f_receiving_filename.empty())
    {
        int const r(unlink(f_receiving_filename.c_str()));
        if(r != 0
        && errno != ENOENT)
        {
            int const e(errno);
            SNAP_LOG_RECOVERABLE_ERROR
                << "an error occurred trying to delete \""
                << f_receiving_filename
                << "\" (errno: "
                << e
                << " -- "
                << strerror(e)
                << ")."
                << SNAP_LOG_SEND;
        }
    }

    tcp_client_connection::process_error();
}



} // namespace rfs_daemon
// vim: ts=4 sw=4 et
