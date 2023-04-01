// Copyright (c) 2019-2023  Made to Order Software Corp.  All Rights Reserved
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


/** \file
 * \brief RFS Service.
 *
 * This file implements the RFS Service.
 *
 * The service connects to an instance of the communicatord service and
 * opens at least one port to receive files.
 *
 * The communicatord message channel is used to send requests to copy
 * files, although many requests are part of configuration files, you can
 * dynamically add a file or a folder using such a message.
 *
 * That channel is also used to communicate with other computers on your
 * network for other reasons:
 *
 * \li gather statistics about files
 * \li get version of each instance of the running snaprfs services
 * \li make sure * certain snaprfs instances are running
 * \li etc.
 *
 * Finally, it is used for various administrative reasons such as receiving
 * the LOG_ROTATE message to reload the logger's configuration setup.
 *
 * The transmission channels are used/selected depending on how the data
 * is to be transferred. The current implementation supports plain and
 * encrypted TCP channels. The encryption is used when sending across
 * clusters. Within one cluster and assuming it is safe to send data
 * between your computer on a local network, no encryption is used to
 * make the transfers faster.
 *
 * A future version may support UDP in order to be able to broadcast
 * the files. This is particularly useful if you have many computers
 * and want to send the files to all the other computers at once.
 * However, many network on the Internet do not properly support
 * broadcasting between computers.
 */

// self
//
#include    "messenger.h"


// advgetopt
//
#include    <advgetopt/advgetopt.h>



namespace rfs_daemon
{



class server
{
public:
                            server(int argc, char * argv[]);

    int                     run();

    void                    restart();
    void                    stop(bool quitting);

private:
    advgetopt::getopt       f_opts;
    messenger::pointer_t    f_messenger = messenger::pointer_t();
    bool                    f_force_restart = false;
};



} // namespace rfs_daemon
// vim: ts=4 sw=4 et
