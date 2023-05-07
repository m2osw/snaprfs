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


// self
//
#include    "snaprfs/connection.h"
#include    "snaprfs/exception.h"


// libaddr
//
#include    <libaddr/addr.h>
#include    <libaddr/addr_parser.h>


// eventdispatcher
//
#include    <eventdispatcher/message.h>
#include    <eventdispatcher/tcp_client_message_connection.h>


// last include
//
#include    <snapdev/poison.h>



namespace rfs
{


namespace detail
{

class connection_impl
{
public:
                                                    connection_impl();

    void                                            set_snaprfs_host(std::string const & host);
    ed::tcp_client_message_connection::pointer_t    get_connection();
    void                                            send_order(order::pointer_t o);

private:
    addr::addr                                      f_addr = addr::addr();
    ed::tcp_client_message_connection::pointer_t    f_connection = ed::tcp_client_message_connection::pointer_t();
};


connection_impl::connection_impl()
{
    set_snaprfs_host(std::string());
}


void connection_impl::set_snaprfs_host(std::string const & host)
{
    f_addr = addr::string_to_addr(host, "127.0.0.1", 4043, "tcp");
}


ed::tcp_client_message_connection::pointer_t connection_impl::get_connection()
{
    if(f_connection == nullptr)
    {
        f_connection = std::make_shared<ed::tcp_client_message_connection>(f_addr);
    }

    return f_connection;
}


void connection_impl::send_order(order::pointer_t o)
{
    if(o == nullptr)
    {
        throw logic_error("the send_order() function cannot be called with a null pointer");
    }

    ed::tcp_client_message_connection::pointer_t connection(get_connection());
    if(connection != nullptr)
    {
        ed::message msg;
        msg.set_command("ORDER");
        msg.add_parameter("command",     o->get_command());
        msg.add_parameter("source",      o->get_source());
        msg.add_parameter("destination", o->get_destination());
        msg.add_parameter("flags",       o->flags_as_string());
        connection->send_message(msg);
    }
}


}
// namespace detail



connection::connection()
    : f_impl(std::make_shared<detail::connection_impl>())
{
}


void connection::set_snaprfs_host(std::string const & host)
{
    f_impl->set_snaprfs_host(host);
}


void connection::send_order(order::pointer_t o)
{
    f_impl->send_order(o);
}





} // namespace rfs
// vim: ts=4 sw=4 et
