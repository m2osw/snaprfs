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
#include    "snaprfs/order.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// snapdev lib
//
#include    <snapdev/tokenize_string.h>


// C++ lib
//
#include    <sstream>


// last include
//
#include    <snapdev/poison.h>



namespace rfs
{



order::order(std::string const & command)
    : f_command(command)
{
}


std::string const & order::get_command() const
{
    return f_command;
}


void order::set_source(std::string const & source)
{
    f_source = source;
}


std::string order::get_source() const
{
    return f_source;
}


void order::set_destination(std::string const & destination)
{
    f_destination = destination;
}


std::string order::get_destination() const
{
    return f_destination;
}


void order::add_flag(order_flag_t flag)
{
    f_flags.insert(flag);
}


void order::remove_flag(order_flag_t flag)
{
    f_flags.erase(flag);
}


void order::add_flags(std::string const & flags)
{
    std::vector<std::string>    names;

    // break input in a list of flag name that was separated by commas
    //
    snapdev::tokenize_string(
          names
        , flags
        , ", "
        , true
        , " ");

    for(auto n : names)
    {
        if(n == "overwrite")
        {
            add_flag(order_flag_t::ORDER_FLAG_OVERWRITE);
        }
        else if(n == "recursive")
        {
            add_flag(order_flag_t::ORDER_FLAG_RECURSIVE);
        }
        else if(n == "acknowledge")
        {
            add_flag(order_flag_t::ORDER_FLAG_ACKNOWLEDGE);
        }
        else
        {
            // this is backward compatible (i.e. sending a new flag which
            // an older version does not support)
            //
            SNAP_LOG_WARNING
                << "unrecognized flag name \""
                << n
                << "\"; ignored."
                << SNAP_LOG_SEND;
        }
    }
}


order_flag_set_t order::get_flags() const
{
    return f_flags;
}


std::string order::flags_as_string() const
{
    std::stringstream ss;

    std::string sep;
    for(auto f : f_flags)
    {
        ss << sep << f;
        sep = ",";
    }

    return ss.str();
}



}
// namespace rfs
// vim: ts=4 sw=4 et
