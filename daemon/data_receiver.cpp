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
#include    "data_receiver.h"

#include    "server.h"


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
//#include    <snapdev/glob_to_list.h>
//#include    <snapdev/tokenize_string.h>


// C++
//
//#include    <list>


// last include
//
#include    <snapdev/poison.h>



namespace rfs_daemon
{



data_receiver::data_receiver(
          server * s
        , ed::tcp_bio_client::pointer_t client)
    : tcp_server_client_connection(client)
    , f_server(s)
{
}


void data_receiver::process_read()
{
    int r(0);

    if(f_received_bytes < sizeof(f_header))
    {
        r = read(&f_header + f_received_bytes, sizeof(f_header));
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
        if(f_received_bytes >= sizeof(f_header))
        {
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

            f_incoming_file = f_server->get_file(f_header.f_id);
            if(f_incoming_file != nullptr)
            {
                SNAP_LOG_ERROR
                    << "file id \""
                    << f_header.f_id
                    << "\" not found on this server."
                    << SNAP_LOG_SEND;
                process_error();
                return;
            }
            f_filename = f_incoming_file->get_filename();
            if(f_filename.empty())
            {
                SNAP_LOG_ERROR
                    << "file id \""
                    << f_header.f_id
                    << "\" not found on this server."
                    << SNAP_LOG_SEND;
                process_error();
                return;
            }
            // while receiving, use parts (right now, we don't expect to
            // have to use more than one part)
            //
            f_receiving_filename = f_filename + ".part1";

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
    }

    if(f_received_bytes >= sizeof(f_header))
    {
        while(f_received_bytes - sizeof(f_header) < f_header.f_size)
        {
            std::uint8_t buf[1024 * 4];
            std::size_t const size_left(f_received_bytes - sizeof(f_header));
            r = read(buf, std::min(size_left, sizeof(buf)));
            if(r == -1)
            {
                SNAP_LOG_ERROR
                    << "an I/O error occurred while reading the file data."
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
    }

    if(f_received_bytes - sizeof(f_header) - f_header.f_size < sizeof(f_footer))
    {
        r = read(&f_footer + f_received_bytes - sizeof(f_header) - f_header.f_size, sizeof(f_footer));
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
        if(f_received_bytes - sizeof(f_header) - f_header.f_size >= sizeof(f_footer))
        {
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

            r = unlink(f_filename.c_str());
            if(r != 0
            && errno != ENOENT)
            {
                int const e(errno);
                SNAP_LOG_RECOVERABLE_ERROR
                    << "removal of old file \""
                    << f_filename
                    << "\" failed with errno: "
                    << e
                    << ", "
                    << strerror(e)
                    << "."
                    << SNAP_LOG_SEND;
            }

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

            remove_from_communicator();

            f_incoming_file->set_received();
        }
    }

    // process next level too
    //
    tcp_server_client_connection::process_read();
}



} // namespace rfs_daemon
// vim: ts=4 sw=4 et
