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
#pragma once


// self
//
#include    "data_sender.h"


// eventdispatcher
//
#include    <eventdispatcher/tcp_server_client_connection.h>



namespace rfs_daemon
{



class shared_file;
class server;


class data_receiver
    : public ed::tcp_server_client_connection
{
public:
    typedef std::shared_ptr<data_receiver>    pointer_t;

                        data_receiver(
                              server * s
                            , ed::tcp_bio_client::pointer_t client);
                        data_receiver(data_receiver const &) = delete;
    data_receiver &     operator = (data_receiver const &) = delete;

    virtual void        process_read();

private:
    server *            f_server = nullptr;
    std::shared_ptr<shared_file>
                        f_incoming_file = std::shared_ptr<shared_file>();
    std::size_t         f_received_bytes = 0;
    data_header         f_header = {};
    data_footer         f_footer = {};
    std::string         f_filename = std::string();
    std::string         f_receiving_filename = std::string();
    std::ofstream       f_output = std::ofstream();
    murmur3::stream     f_murmur3 = murmur3::stream(DATA_SEED_H1, DATA_SEED_H2);
};



} // namespace rfs_daemon
// vim: ts=4 sw=4 et
