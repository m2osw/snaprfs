// Copyright (c) 2019-2023  Made to Order Software Corp.  All Rights Reserved
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
#include    "data_sender.h"


// eventdispatcher
//
#include    <eventdispatcher/tcp_client_connection.h>



namespace rfs_daemon
{



class shared_file;
class server;


class data_receiver
    : public ed::tcp_client_connection
{
public:
    typedef std::shared_ptr<data_receiver>    pointer_t;

                        data_receiver(
                              server * s
                            , std::string const & filename
                            , std::uint32_t id
                            , std::string const & path_part
                            , addr::addr const & address
                            , ed::mode_t mode = ed::mode_t::MODE_PLAIN);
                        data_receiver(data_receiver const &) = delete;
    data_receiver       operator = (data_receiver const &) = delete;

    // tcp_client_connection implementation
    virtual ssize_t     write(void const * data, size_t length) override;
    virtual bool        is_writer() const override;
    virtual void        process_read() override;
    virtual void        process_write() override;
    virtual void        process_error() override;

private:
    server *            f_server = nullptr;
    std::string         f_filename = std::string();
    std::string         f_receiving_filename = std::string();
    std::vector<char>   f_request = std::vector<char>();
    std::vector<char>   f_names = std::vector<char>(512);
    std::uint32_t       f_id = 0;
    std::string         f_path_part = std::string();
    std::size_t         f_received_bytes = 0;
    std::size_t         f_position = 0;
    std::size_t         f_header_size = 0;
    data_header         f_header = {};
    data_footer         f_footer = {};
    std::ofstream       f_output = std::ofstream();
    murmur3::stream     f_murmur3 = murmur3::stream(DATA_SEED_H1, DATA_SEED_H2);
};



} // namespace rfs_daemon
// vim: ts=4 sw=4 et
