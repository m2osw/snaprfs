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

/** \file
 * \brief Snap RFS exceptions.
 *
 * This files declares a few exceptions that the RFS environment uses
 * when a parameter or other runtime error occurs.
 *
 * It also defines a logic error in case something wrong which should not
 * happen is detected. Logic errors must be fixed before production.
 */


// libexcept lib
//
#include    <libexcept/exception.h>


namespace rfs
{



DECLARE_LOGIC_ERROR(rfs_logic_error);

DECLARE_MAIN_EXCEPTION(rfserror);
DECLARE_MAIN_EXCEPTION(rfs_error);

DECLARE_EXCEPTION(rfs_error, duplicate_error);
DECLARE_EXCEPTION(rfs_error, invalid_variable);
DECLARE_EXCEPTION(rfs_error, invalid_parameter);
DECLARE_EXCEPTION(rfs_error, invalid_severity);
DECLARE_EXCEPTION(rfs_error, not_a_message);



}
// namespace rfs
// vim: ts=4 sw=4 et
