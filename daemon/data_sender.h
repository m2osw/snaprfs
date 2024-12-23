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
#pragma once

// self
//
#include    "file_listener.h"



// eventdispatcher
//
#include    <eventdispatcher/tcp_server_client_connection.h>


// murmur3
//
#include    <murmur3/stream.h>


// C++
//
#include    <fstream>
#include    <set>



namespace rfs_daemon
{



constexpr murmur3::seed_t const  DATA_SEED_H1 = 0x0e2e6c7ea1639275ULL;
constexpr murmur3::seed_t const  DATA_SEED_H2 = 0x1811764757f36729ULL;


struct data_header
{
    std::uint8_t        f_magic[4] = { 'D', 'A', 'T', 'A' };
    std::uint32_t       f_id = 0;
    std::uint64_t       f_mtime_sec = 0;            // a timespec uses time_t and long, here we make sure it is 64 bits always
    std::uint64_t       f_mtime_nsec = 0;
    std::uint32_t       f_size = 0;
    std::uint16_t       f_mode = 0;
    std::uint8_t        f_username_length = 0;
    std::uint8_t        f_groupname_length = 0;
    std::uint8_t        f_login_name_length = 0;
    std::uint8_t        f_password_length = 0;
    std::uint8_t        f_padding[6] = {};          // uint64 means we need a multiple of 8 bytes
};


struct data_footer
{
    std::uint8_t        f_murmur3[murmur3::HASH_SIZE] = {};
    std::uint8_t        f_end[4] = { 'E', 'N', 'D', '!' };
};


struct file_request
{
    std::uint8_t        f_magic[4] = { 'F', 'I', 'L', 'E' };
    std::uint32_t       f_id = 0;
};


class server;


class data_sender
    : public ed::tcp_server_client_connection
{
public:
    typedef std::shared_ptr<data_sender>    pointer_t;

                        data_sender(
                              server * s
                            , ed::tcp_bio_client::pointer_t client);
                        data_sender(data_sender const &) = delete;
    data_sender &       operator = (data_sender const &) = delete;

    void                set_login_info(std::string const & login_name, std::string const & password);
    bool                open();

    // tcp_client_connection implementation
    //
    bool                is_writer() const override;
    virtual void        process_write() override;
    void                process_read() override;

private:
    server *            f_server = nullptr;
    std::string         f_login_name = std::string();
    std::string         f_password = std::string();
    std::ifstream       f_input = std::ifstream();
    murmur3::stream     f_murmur3 = murmur3::stream(DATA_SEED_H1, DATA_SEED_H2);
    file_request        f_file_request = file_request();
    std::string         f_filename = std::string();
    std::size_t         f_received_bytes = 0;
    std::uint8_t        f_buffer[1024 * 4] = {};
    std::size_t         f_size = 0;
    std::size_t         f_position = 0;
    bool                f_sent_footer = false;
};



} // namespace rfs_daemon
// vim: ts=4 sw=4 et
