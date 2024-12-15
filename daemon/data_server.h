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


// eventdispatcher
//
#include    <eventdispatcher/tcp_server_connection.h>
#include    <eventdispatcher/communicator.h>



namespace rfs_daemon
{



class server;


class data_server
    : public ed::tcp_server_connection
{
public:
    typedef std::shared_ptr<data_server>    pointer_t;

    static constexpr int const  DATA_SERVER_PORT = 4044;

                        data_server(
                              server * s
                            , addr::addr const & addr
                            , std::string const & certificate
                            , std::string const & private_key
                            , ed::mode_t mode = ed::mode_t::MODE_PLAIN
                            , int max_connections = -1
                            , bool reuse_addr = false);
                        data_server(data_server const &) = delete;
    data_server &       operator = (data_server const &) = delete;

    virtual void        process_accept() override;

    void                set_login_info(std::string const & login_name, std::string const & password);

private:
    server *            f_server = nullptr;
    ed::communicator::pointer_t
                        f_communicator = ed::communicator::pointer_t();
    std::string         f_login_name = std::string();
    std::string         f_password = std::string();
};



} // namespace rfs_daemon
// vim: ts=4 sw=4 et
