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
#include    "data_server.h"

#include    "data_receiver.h"


// advgetopt
//
#include    <advgetopt/conf_file.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/glob_to_list.h>
#include    <snapdev/tokenize_string.h>


// C++
//
#include    <list>


// last include
//
#include    <snapdev/poison.h>



namespace rfs_daemon
{



data_server::data_server(
          server * s
        , addr::addr const & addr
        , std::string const & certificate
        , std::string const & private_key
        , ed::mode_t mode
        , int max_connections
        , bool reuse_addr)
    : tcp_server_connection(
              addr
            , certificate
            , private_key
            , mode
            , max_connections
            , reuse_addr)
    , f_server(s)
    , f_communicator(ed::communicator::instance())
{
    set_name("data_server");

    non_blocking();
}


void data_server::process_accept()
{
    // a new client just connected, create a new replicator_in
    // object and add it to the snap_communicator object.
    //
    ed::tcp_bio_client::pointer_t const new_client(accept());
    if(new_client == nullptr)
    {
        // an error occurred, report in the logs
        int const e(errno);
        SNAP_LOG_ERROR
            << "accept() failed with errno: "
            << e
            << " -- "
            << strerror(e)
            << SNAP_LOG_SEND;
        return;
    }

    data_sender::pointer_t service(std::make_shared<data_sender>(
                  f_server
                , new_client));
    if(!f_communicator->add_connection(service))
    {
        SNAP_LOG_ERROR
            << "new data_sender connection could not be added to the ed::communicator."
            << SNAP_LOG_SEND;
    }
    else
    {
        service->set_login_info(f_login_name, f_password);
    }
}


void data_server::set_login_info(std::string const & login_name, std::string const & password)
{
    f_login_name = login_name;
    f_password = password;
}



} // namespace rfs_daemon
// vim: ts=4 sw=4 et
