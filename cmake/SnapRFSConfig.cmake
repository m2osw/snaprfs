# - Find SnapRFS (Remote File System)
#
# SNAPRFS_FOUND        - System has SnapRFS
# SNAPRFS_INCLUDE_DIR  - The SnapRFS include directories
# SNAPRFS_LIBRARY      - The libraries needed to use SnapRFS
# SNAPRFS_DEFINITIONS  - Compiler switches required for using SnapRFS
#
# License:
#
# Copyright (c) 2011-2023  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/snaprfs
# contact@m2osw.com
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

find_path(
    SNAPRFS_INCLUDE_DIR
        snaprfs/rfs.h

    PATHS
        ENV SNAPRFS_INCLUDE_DIR
)

find_library(
    SNAPRFS_LIBRARY
        snaprfs

    PATHS
        ${SNAPRFS_LIBRARY_DIR}
        ENV SNAPRFS_LIBRARY
)

mark_as_advanced(
    SNAPRFS_INCLUDE_DIR
    SNAPRFS_LIBRARY
)

set(SNAPRFS_INCLUDE_DIRS ${SNAPRFS_INCLUDE_DIR})
set(SNAPRFS_LIBRARIES    ${SNAPRFS_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    SnapRFS
    REQUIRED_VARS
        SNAPRFS_INCLUDE_DIR
        SNAPRFS_LIBRARY
)

# vim: ts=4 sw=4 et
