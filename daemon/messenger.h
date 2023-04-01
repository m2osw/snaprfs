// Copyright (c) 2022-2023  Made to Order Software Corp.  All Rights Reserved
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
 * \brief The declaration of the messenger class.
 *
 * This file is the declaration of the messenger class.
 *
 * The messenger is used to connect to the communicator daemon and listen
 * for messages from other snaprfs tools.
 *
 * Note that orders can also be sent to the snaprfs environment using the
 * rfs command line tool.
 */

// eventdispatcher
//
#include    <eventdispatcher/dispatcher.h>


// communicatord
//
#include    <communicatord/communicatord.h>





namespace rfs_daemon
{



class server;


class messenger
    : public communicatord::communicator
{
public:
    typedef std::shared_ptr<messenger>      pointer_t;

                        messenger(
                              server * s
                            , advgetopt::getopt & opts);
                        messenger(messenger const &) = delete;
    virtual             ~messenger() override;
    messenger &         operator = (messenger const &) = delete;

    // connection_with_send_message implementation
    //
    virtual void        ready(ed::message & msg) override;
    virtual void        restart(ed::message & msg) override;
    virtual void        stop(bool quitting) override;

    void                msg_configuration_filenames(ed::message & msg);
    void                msg_copy(ed::message & msg);
    void                msg_duplicate(ed::message & msg);
    void                msg_list(ed::message & msg);
    void                msg_move(ed::message & msg);
    void                msg_ping(ed::message & msg);
    void                msg_remove(ed::message & msg);
    void                msg_stat(ed::message & msg);
    void                msg_version(ed::message & msg);

private:
    server *            f_server = nullptr;
    ed::dispatcher::pointer_t
                        f_dispatcher = ed::dispatcher::pointer_t();
};



} // namespace rfs_daemon
// vim: ts=4 sw=4 et
