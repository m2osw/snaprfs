# Copyright (c) 2019-2023  Made to Order Software Corp.  All Rights Reserved
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

##
## snaprfs daemon
##
project(snaprfs_daemon)

add_executable(${PROJECT_NAME}
    data_receiver.cpp
    data_sender.cpp
    data_server.cpp
    file_listener.cpp
    messenger.cpp
    server.cpp
)

include_directories(
    ${ADVGETOPT_INCLUDE_DIRS}
    ${EDHTTP_INCLUDE_DIRS}
    ${MURMUR3_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    snaprfs
    ${ADVGETOPT_LIBRARIES}
    ${EDHTTP_LIBRARIES}
    ${MURMUR3_LIBRARIES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME
        snaprfs
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        sbin
)

# vim: ts=4 sw=4 et
