# Copyright (c) 2019  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/snaprfs
# contact@m2osw.com
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# - Try to find SnapRFS
#
# Once done this will define
#
# SNAPRFS_FOUND        - System has SnapRFS
# SNAPRFS_INCLUDE_DIR  - The SnapRFS include directories
# SNAPRFS_LIBRARY      - The libraries needed to use SnapRFS (none)
# SNAPRFS_DEFINITIONS  - Compiler switches required for using SnapRFS (none)
#

find_path(
    SNAPRFS_INCLUDE_DIR
        snaprfs/rfs.h

    PATHS
        $ENV{SNAPRGS_INCLUDE_DIR}
)

find_library(
    SNAPRFS_LIBRARY
        snaprfs

    PATHS
        $ENV{SNAPRFS_LIBRARY}
)

mark_as_advanced(
    SNAPRFS_INCLUDE_DIR
    SNAPRFS_LIBRARY
)

set(SNAPRFS_INCLUDE_DIRS ${SNAPRFS_INCLUDE_DIR})
set(SNAPRFS_LIBRARIES    ${SNAPRFS_LIBRARY}    )

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set SNAPRFS_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(
    SnapRFS
    DEFAULT_MSG
    SNAPRFS_INCLUDE_DIR
    SNAPRFS_LIBRARY
)

# vim: ts=4 sw=4 et
