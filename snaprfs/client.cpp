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

/** \file
 * \brief `client` is a help class used to connect to the snaprfs.
 *
 * This file is the implementation of a messenger expected to be used
 * by your clients if they don't already have their own communicatord
 * class.
 */

// self
//
#include    "client.h"

#include    "exception.h"


// advgetopt
//
#include    <advgetopt/validator_double.h>
#include    <advgetopt/validator_integer.h>


// libaddr
//
#include    <libaddr/addr_parser.h>


// last include
//
#include    <snapdev/poison.h>



namespace rfs_daemon
{




client::client(advgetopt::getopt & opts, std::string const & service_name)
    : communicator(opts, "snaprfs")
    , f_service_name(service_name)
    , f_dispatcher(std::make_shared<ed::dispatcher>(this))
{
    if(f_service_name.empty())
    {
        throw rfs::missing_parameter("service name missing creating a client object.");
    }

#ifdef _DEBUG
    f_dispatcher->set_trace();
#endif
    set_dispatcher(f_dispatcher);

    f_dispatcher->add_matches({
        DISPATCHER_MATCH("RFS_RECEIVED", &client::msg_received),
        DISPATCHER_MATCH("RFS_SUCCESS", &client::msg_success),
        DISPATCHER_MATCH("RFS_FAILURE", &client::msg_failure),
    });

    f_dispatcher->add_communicator_commands();
}


client::~client()
{
}


msg_id_t client::send_configuration_filenames(std::string const & hostname_opt)
{
    ed::message msg;
    msg.set_service(f_service_name);
    msg.set_command("RFS_CONFIGURATION_FILENAMES");
    if(!hostname_opt.empty())
    {
        msg.set_server(hostname_opt);
    }
    msg_id_t const id(set_message_id(msg));
    send_message(msg);
    return id;
}


msg_id_t client::send_copy(std::string const & source, std::string const & destination)
{
    ed::message msg;
    msg.set_service(f_service_name);
    msg.set_command("RFS_COPY");
    msg.add_parameter("source", source);
    msg.add_parameter("destination", destination);
    msg_id_t const id(set_message_id(msg));
    send_message(msg);
    return id;
}


msg_id_t client::send_duplicate(std::string const & source, advgetopt::string_list_t const & destinations)
{
    ed::message msg;
    msg.set_service(f_service_name);
    msg.set_command("RFS_DUPLICATE");
    msg.add_parameter("source", source);
    std::size_t const max(destinations.size());
    for(std::size_t i(0); i < max; ++i)
    {
        std::string param("destination");
        param += std::to_string(i + 1);
        msg.add_parameter(param, destinations[i]);
    }
    msg_id_t const id(set_message_id(msg));
    send_message(msg);
    return id;
}


msg_id_t client::send_info(std::string const & hostname_opt)
{
    ed::message msg;
    msg.set_service(f_service_name);
    msg.set_command("RFS_INFO");
    if(!hostname_opt.empty())
    {
        msg.add_parameter("hostname", hostname_opt);
    }
    msg_id_t const id(set_message_id(msg));
    send_message(msg);
    return id;
}


msg_id_t client::send_list(std::string const & source)
{
    ed::message msg;
    msg.set_service(f_service_name);
    msg.set_command("RFS_LIST");
    msg.add_parameter("source", source);
    msg_id_t const id(set_message_id(msg));
    send_message(msg);
    return id;
}


msg_id_t client::send_move(std::string const & source, std::string const & destination)
{
    ed::message msg;
    msg.set_service(f_service_name);
    msg.set_command("RFS_MOVE");
    msg.add_parameter("source", source);
    msg.add_parameter("destination", destination);
    msg_id_t const id(set_message_id(msg));
    send_message(msg);
    return id;
}


msg_id_t client::send_ping(std::string const & hostname_opt)
{
    ed::message msg;
    msg.set_service(f_service_name);
    msg.set_command("RFS_PING");
    if(!hostname_opt.empty())
    {
        msg.add_parameter("hostname", hostname_opt);
    }
    msg_id_t const id(set_message_id(msg));
    send_message(msg);
    return id;
}


msg_id_t client::send_remove(std::string const & destination)
{
    ed::message msg;
    msg.set_service(f_service_name);
    msg.set_command("RFS_REMOVE");
    msg.add_parameter("destination", destination);
    msg_id_t const id(set_message_id(msg));
    send_message(msg);
    return id;
}


msg_id_t client::send_stat(std::string const & source)
{
    ed::message msg;
    msg.set_service(f_service_name);
    msg.set_command("RFS_STAT");
    msg.add_parameter("source", source);
    msg_id_t const id(set_message_id(msg));
    send_message(msg);
    return id;
}


msg_id_t client::send_stop(std::string const & hostname_opt)
{
    ed::message msg;
    msg.set_service(f_service_name);
    msg.set_command("STOP");
    if(!hostname_opt.empty())
    {
        msg.add_parameter("hostname", hostname_opt);
    }
    msg_id_t const id(set_message_id(msg));
    send_message(msg);
    return id;
}


void client::msg_received(ed::message & msg)
{
    snapdev::NOT_USED(msg);
}


void client::msg_success(ed::message & msg)
{
    snapdev::NOT_USED(msg);
}


void client::msg_failure(ed::message & msg)
{
    snapdev::NOT_USED(msg);
}


msg_id_t client::set_message_id(ed::message & msg)
{
    ++f_message_id;
    msg.add_parameter("msg_id", f_message_id);
    return f_message_id;
}




} // namespace rfs_daemon
// vim: ts=4 sw=4 et
