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
 * \brief Snap RFS exceptions.
 *
 * This files declares a few exceptions that the RFS environment uses
 * when a parameter or other runtime error occurs.
 *
 * It also defines a logic error in case something wrong which should not
 * happen is detected. Logic errors must be fixed before production.
 */


// libexcept
//
#include    <libexcept/exception.h>


namespace rfs
{



DECLARE_LOGIC_ERROR(logic_error);

DECLARE_MAIN_EXCEPTION(rfs_error);

DECLARE_EXCEPTION(rfs_error, duplicate_file);
DECLARE_EXCEPTION(rfs_error, missing_parameter);
DECLARE_EXCEPTION(rfs_error, no_random_data_available);
DECLARE_EXCEPTION(rfs_error, unsupported_file);



}
// namespace rfs
// vim: ts=4 sw=4 et
