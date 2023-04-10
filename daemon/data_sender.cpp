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


// snaprfs
//
#include    <snaprfs/exception.h>


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



data_sender::data_sender(
          std::string const & filename
        , std::uint32_t id
        , addr::addr const & address
        , ed::mode_t mode)
    : tcp_client_connection(address, mode)
    , f_filename(filename)
    , f_id(id)
{
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

    data_header header;
    header.f_id = f_id;

    f_input.seekg(0, std::ios_base::end);
    header.f_size = f_input.tellg();
    f_input.seekg(0, std::ios_base::beg);

    memcpy(f_buffer, &header, sizeof(header));
    f_size = sizeof(header);

    return true;
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
