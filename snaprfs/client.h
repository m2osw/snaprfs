// Copyright (c) 2022-2024  Made to Order Software Corp.  All Rights Reserved
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

/** \file
 * \brief The declaration of the messenger class.
 *
 * This file is the declaration of the messenger class.
 *
 * The messenger is used to connect to the communicator daemon and listen
 * for messages from other snaprfs tools.
 *
 * Note that orders can also be sent to the snaprfs environment using the
 * rfs command line tool.
 */


// advgetopt
//
#include    <advgetopt/utils.h>


// eventdispatcher
//
#include    <eventdispatcher/dispatcher.h>


// communicatord
//
#include    <communicatord/communicator.h>





namespace rfs_daemon
{



typedef std::uint32_t           msg_id_t;


class client
    : public communicatord::communicator
{
public:
    typedef std::shared_ptr<client>      pointer_t;

                        client(advgetopt::getopt & opts, std::string const & service_name);
                        client(client const &) = delete;
    virtual             ~client() override;
    client &            operator = (client const &) = delete;

    msg_id_t            send_configuration_filenames(std::string const & hostname_opt);
    msg_id_t            send_copy(std::string const & source, std::string const & destination);
    msg_id_t            send_duplicate(std::string const & source, advgetopt::string_list_t const & destinations);
    msg_id_t            send_info(std::string const & hostname_opt);
    msg_id_t            send_list(std::string const & source);
    msg_id_t            send_move(std::string const & source, std::string const & destination);
    msg_id_t            send_ping(std::string const & hostname_opt);
    msg_id_t            send_remove(std::string const & destination);
    msg_id_t            send_stat(std::string const & source);
    msg_id_t            send_stop(std::string const & hostname_opt);

    virtual void        msg_received(ed::message & msg);
    virtual void        msg_success(ed::message & msg);
    virtual void        msg_failure(ed::message & msg);

private:
    msg_id_t            set_message_id(ed::message & msg);

    msg_id_t            f_message_id = 0;
    std::string         f_service_name = std::string();
    ed::dispatcher::pointer_t
                        f_dispatcher = ed::dispatcher::pointer_t();
};



} // namespace rfs_daemon
// vim: ts=4 sw=4 et
