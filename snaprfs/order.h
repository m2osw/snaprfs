// Copyright (c) 2019-2022  Made to Order Software Corp.  All Rights Reserved
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


// libaddr lib
//
#include    <libaddr/addr.h>


// C++
//
#include    <iostream>
#include    <memory>
#include    <set>
#include    <string>



namespace rfs
{



enum class order_flag_t
{
    ORDER_FLAG_OVERWRITE,
    ORDER_FLAG_RECURSIVE,
    ORDER_FLAG_ACKNOWLEDGE
};

typedef std::set<order_flag_t>      order_flag_set_t;


class order
{
public:
    typedef std::shared_ptr<order>      pointer_t;

                            order(std::string const & command);

    std::string const &     get_command() const;
    void                    set_source(std::string const & source);
    std::string             get_source() const;
    void                    set_destination(std::string const & destination);
    std::string             get_destination() const;

    void                    add_flag(order_flag_t flag);
    void                    add_flags(std::string const & flags);
    order_flag_set_t        get_flags() const;
    std::string             flags_as_string() const;

private:
    std::string const       f_command;
    std::string             f_source = std::string();
    std::string             f_destination = std::string();
    order_flag_set_t        f_flags = order_flag_set_t();
};



}
// namespace rfs



template<typename CharT, typename Traits>
inline std::basic_ostream<CharT, Traits> &
operator << (std::basic_ostream<CharT, Traits> & os, rfs::order_flag_t flag)
{
    switch(flag)
    {
    case rfs::order_flag_t::ORDER_FLAG_OVERWRITE:
        os << "overwrite";
        break;

    case rfs::order_flag_t::ORDER_FLAG_RECURSIVE:
        os << "recursive";
        break;

    case rfs::order_flag_t::ORDER_FLAG_ACKNOWLEDGE:
        os << "acknowledge";
        break;

    default:
        os << "(unknown flag: "
           << static_cast<int>(flag)
           << ")";
        break;

    }

    return os;
}



// vim: ts=4 sw=4 et
