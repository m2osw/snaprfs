// Copyright (c) 2019-2023  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snaprfs
// contact@m2osw.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


// self
//
#include    "data_sender.h"

#include    "server.h"


// snaprfs
//
#include    <snaprfs/exception.h>


// snaplogger
//
#include    <snaplogger/message.h>


// C
//
#include    <grp.h>
#include    <pwd.h>


// last include
//
#include    <snapdev/poison.h>



namespace rfs_daemon
{



data_sender::data_sender(
          server * s
        , ed::tcp_bio_client::pointer_t client)
    : tcp_server_client_connection(client)
    , f_server(s)
{
    set_name("data_sender");
}


bool data_sender::open()
{
    if(f_input.is_open())
    {
        SNAP_LOG_ERROR
            << "the data_sender() input file \""
            << f_filename
            << "\" is already opened."
            << SNAP_LOG_SEND;
        return false;
    }

    struct stat s;
    if(stat(f_filename.c_str(), &s) != 0)
    {
        int const e(errno);
        SNAP_LOG_ERROR
            << "could not get stats from file \""
            << f_filename
            << "\"; errno: "
            << e
            << ", "
            << strerror(e)
            << "."
            << SNAP_LOG_SEND;
        return false;
    }
    passwd * pw(getpwuid(s.st_uid));
    if(pw == nullptr)
    {
        int const e(errno);
        SNAP_LOG_ERROR
            << "could not get user name from uid "
            << s.st_uid
            << " of file \""
            << f_filename
            << "\"; errno: "
            << e
            << ", "
            << strerror(e)
            << "."
            << SNAP_LOG_SEND;
        return false;
    }
    group * gr(getgrgid(s.st_gid));
    if(gr == nullptr)
    {
        int const e(errno);
        SNAP_LOG_ERROR
            << "could not get group name from gid "
            << s.st_gid
            << " of file \""
            << f_filename
            << "\"; errno: "
            << e
            << ", "
            << strerror(e)
            << "."
            << SNAP_LOG_SEND;
        return false;
    }
    std::size_t const pw_len(strlen(pw->pw_name));
    std::size_t const gr_len(strlen(gr->gr_name));
    if(pw_len == 0 || pw_len > 255
    || gr_len == 0 || gr_len > 255)
    {
        SNAP_LOG_ERROR
            << "user or group name for "
            << s.st_uid
            << ':'
            << s.st_gid
            << " of file \""
            << f_filename
            << "\" could not be resolved."
            << SNAP_LOG_SEND;
        return false;
    }

    f_input.open(f_filename);
    if(!f_input.is_open())
    {
        int const e(errno);
        SNAP_LOG_ERROR
            << "error occurred trying to open \""
            << f_filename
            << "\"; errno: "
            << e
            << ", "
            << strerror(e)
            << "."
            << SNAP_LOG_SEND;
        return false;
    }

    data_header * header(reinterpret_cast<data_header *>(new char[sizeof(data_header) + pw_len + gr_len]));
    header->f_id = f_file_request.f_id;
    header->f_mode = s.st_mode;
    header->f_username_length = pw_len;
    header->f_groupname_length = gr_len;
    memcpy(&header + 1, pw->pw_name, pw_len);
    memcpy(reinterpret_cast<char *>(&header + 1) + pw_len, gr->gr_name, gr_len);

    f_input.seekg(0, std::ios_base::end);
    header->f_size = f_input.tellg();
    f_input.seekg(0, std::ios_base::beg);

    f_size = sizeof(header) + pw_len + gr_len;
    memcpy(f_buffer, &header, f_size);

    return true;
}


void data_sender::process_read()
{
    // use RAII to process the next level on exit wherever it happens
    //
    class on_exit
    {
    public:
        on_exit(data_sender::pointer_t sender)
            : f_sender(sender)
        {
        }

        ~on_exit()
        {
            f_sender->tcp_server_client_connection::process_read();
        }

    private:
        data_sender::pointer_t      f_sender;
    };
    on_exit raii_on_exit(std::dynamic_pointer_cast<data_sender>(shared_from_this()));

    if(get_socket() == -1)
    {
        return;
    }

    if(f_input.is_open())
    {
        SNAP_LOG_ERROR
            << "the data_sender() input file \""
            << f_filename
            << "\" is already opened. It cannot be receiving more data."
            << SNAP_LOG_SEND;
        return;
    }

    int r(0);
    if(f_received_bytes < sizeof(f_file_request))
    {
        r = read(&f_file_request + f_received_bytes, sizeof(f_file_request) - f_received_bytes);
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
        if(f_received_bytes >= sizeof(f_file_request))
        {
            if(f_file_request.f_magic[0] != 'F'
            || f_file_request.f_magic[1] != 'I'
            || f_file_request.f_magic[2] != 'L'
            || f_file_request.f_magic[3] != 'E')
            {
                SNAP_LOG_ERROR
                    << "file request magic is not 'FILE'."
                    << SNAP_LOG_SEND;
                process_error();
                return;
            }
            shared_file::pointer_t file(f_server->get_file(f_file_request.f_id));
            if(file == nullptr)
            {
                SNAP_LOG_ERROR
                    << "file with id \""
                    << f_file_request.f_id
                    << "\" not found."
                    << SNAP_LOG_SEND;
                process_error();
                return;
            }
            f_filename = file->get_filename();
            open();
        }
    }
}


void data_sender::process_write()
{
    int r(0);

    if(!f_input.is_open())
    {
        throw rfs::logic_error("data_sender::process_write() expects f_input to be opened. Did you call open() before adding it to the communicator?");
    }

    for(;;)
    {
        while(f_position < f_size)
        {
            r = write(f_buffer, f_size - f_position);
            if(r == -1)
            {
                int const e(errno);
                SNAP_LOG_ERROR
                    << "error occurred writing data; errno: "
                    << e
                    << ", "
                    << strerror(e)
                    << "."
                    << SNAP_LOG_SEND;
                process_error();
                return;
            }
            if(r == 0)
            {
                // could not write, just wait some more
                //
                return;
            }
            f_position += r;
        }

        if(f_input.eof())
        {
            remove_from_communicator();
            return;
        }

        f_position = 0;
        f_input.read(reinterpret_cast<char *>(f_buffer), sizeof(f_buffer));
        if(f_input.bad())
        {
            int const e(errno);
            SNAP_LOG_ERROR
                << "error occurred reading data from \""
                << f_filename
                << "\"; errno: "
                << e
                << ", "
                << strerror(e)
                << "."
                << SNAP_LOG_SEND;
            process_error();
            return;
        }
        if(r == 0)
        {
            // nothing more to read, generate the footer
            //
            data_footer footer;
            murmur3::hash const h(f_murmur3.flush());
            memcpy(footer.f_murmur3, h.get(), murmur3::HASH_SIZE);
            memcpy(f_buffer, &footer, sizeof(footer));
            f_size = sizeof(footer);
        }
        else
        {
            f_murmur3.add_data(f_buffer, r);
            f_size = r;
        }
    }
}



} // namespace rfs_daemon
// vim: ts=4 sw=4 et
