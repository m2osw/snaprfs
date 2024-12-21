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

/** \file
 * \brief Snap RFS connection.
 *
 * This files declares a class used to connect to the snaprfs local client.
 * This allows you to send orders which start copying data as required.
 */


// self
//
#include    <snaprfs/order.h>


// C++
//
#include    <memory>
#include    <set>
#include    <string>



namespace rfs
{


namespace detail
{

class connection_impl;

} // namespace detail


class connection
{
public:
                        connection();

    void                set_snaprfs_host(std::string const & host);

    void                send_order(order::pointer_t o);

private:
    std::shared_ptr<detail::connection_impl>
                        f_impl = std::shared_ptr<detail::connection_impl>();
};



} // namespace rfs
// vim: ts=4 sw=4 et
