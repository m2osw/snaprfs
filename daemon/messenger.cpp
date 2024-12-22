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

/** \file
 * \brief The messenger to communicator with the communicator daemon.
 *
 * This file is the implementation of a messenger to communicator with
 * the communicatord service. This allows the snaprfs daemon to receive
 * messages from other snaprfs on your network. This is used to manage
 * the files in your entire cluster.
 */

// self
//
#include    "messenger.h"

#include    "server.h"


// snaprfs
//
#include    <snaprfs/names.h>


// communicatord
//
#include    <communicatord/names.h>


// edhttp
//
#include    <edhttp/uri.h>


// advgetopt
//
#include    <advgetopt/validator_double.h>
#include    <advgetopt/validator_integer.h>


// libaddr
//
#include    <libaddr/addr_parser.h>


// snaplogger
//
#include    <snaplogger/message.h>


// last include
//
#include    <snapdev/poison.h>



namespace rfs_daemon
{



messenger::messenger(server * s, advgetopt::getopt & opts)
    : communicator(opts, "snaprfs")
    , f_server(s)
    , f_dispatcher(std::make_shared<ed::dispatcher>(this))
{
    set_name("messenger");

#ifdef _DEBUG
    f_dispatcher->set_trace();
#endif
    set_dispatcher(f_dispatcher);

    f_dispatcher->add_matches({
        DISPATCHER_MATCH(snaprfs::g_name_snaprfs_cmd_rfs_file_changed, &messenger::msg_file_changed),
        DISPATCHER_MATCH(snaprfs::g_name_snaprfs_cmd_rfs_file_deleted, &messenger::msg_file_deleted),

        // the following are not yet implemented and maybe that was wrong
        // so I may not implement them (i.e. the copy of a specific set of
        // folders is safer than allowing "random" copies across all computers)
        //DISPATCHER_MATCH(snaprfs::g_name_snaprfs_cmd_rfs_copy, &messenger::msg_copy),
        //DISPATCHER_MATCH(snaprfs::g_name_snaprfs_cmd_rfs_duplicate, &messenger::msg_duplicate),
        //DISPATCHER_MATCH(snaprfs::g_name_snaprfs_cmd_rfs_move, &messenger::msg_move),
        //DISPATCHER_MATCH(snaprfs::g_name_snaprfs_cmd_rfs_remove, &messenger::msg_remove),

        // the following is also not yet implemented, but thos I'd like to have
        // at some point
        //DISPATCHER_MATCH(snaprfs::g_name_snaprfs_cmd_rfs_configuration_filenames, &messenger::msg_configuration_filenames),
        //DISPATCHER_MATCH(snaprfs::g_name_snaprfs_cmd_rfs_list, &messenger::msg_list),
        //DISPATCHER_MATCH(snaprfs::g_name_snaprfs_cmd_rfs_ping, &messenger::msg_ping),
        //DISPATCHER_MATCH(snaprfs::g_name_snaprfs_cmd_rfs_stat, &messenger::msg_stat),
        //DISPATCHER_MATCH(snaprfs::g_name_snaprfs_cmd_rfs_version, &messenger::msg_version),
    });

    f_dispatcher->add_communicator_commands();
}


messenger::~messenger()
{
}


void messenger::ready(ed::message & msg)
{
    snapdev::NOT_USED(msg);

    f_server->ready();
}


void messenger::restart(ed::message & msg)
{
    snapdev::NOT_USED(msg);

    f_server->restart();
}


void messenger::stop(bool quitting)
{
    f_server->stop(quitting);
}


void messenger::msg_file_changed(ed::message & msg)
{
    if(!msg.has_parameter(snaprfs::g_name_snaprfs_param_filename)
    || !msg.has_parameter(snaprfs::g_name_snaprfs_param_id)
    || !msg.has_parameter(snaprfs::g_name_snaprfs_param_my_addresses)
    || !msg.has_parameter(snaprfs::g_name_snaprfs_param_mtime))
    {
        SNAP_LOG_ERROR
            << "received RFS_FILE_CHANGED message without a filename, an id, and/or my_address: \""
            << msg
            << "\"."
            << SNAP_LOG_SEND;
        return;
    }
    std::string const filename(msg.get_parameter(snaprfs::g_name_snaprfs_param_filename));
    std::uint32_t const id(msg.get_integer_parameter(snaprfs::g_name_snaprfs_param_id));
    std::string const remote_addresses(msg.get_parameter(snaprfs::g_name_snaprfs_param_my_addresses));
    snapdev::timespec_ex const mtime(msg.get_parameter(snaprfs::g_name_snaprfs_param_mtime));

    if(filename.empty()
    || remote_addresses.empty())
    {
        SNAP_LOG_ERROR
            << "filename and remote_address in the RFS_FILE_CHANGED cannot be empty."
            << SNAP_LOG_SEND;
        return;
    }
    if(mtime <= 0.0)
    {
        SNAP_LOG_ERROR
            << "mtime in RFS_FILE_CHANGED must represent a modern time (Jan 1, 1970 00:00:01 or more recent)."
            << SNAP_LOG_SEND;
        return;
    }

    advgetopt::string_list_t addresses;
    advgetopt::split_string(remote_addresses, addresses, { "," });

    for(auto uri : addresses)
    {
        edhttp::uri u;
        if(!u.set_uri(uri, false, true))
        {
            SNAP_LOG_WARNING
                << "the \"my_addresses=...\" parameter \""
                << uri
                << "\" includes an invalid URI: "
                << u.get_last_error_message()
                << "."
                << SNAP_LOG_SEND;
            continue;
        }

        bool const plain(u.scheme() == snaprfs::g_name_snaprfs_scheme_rfs);
        bool const secure(u.scheme() == snaprfs::g_name_snaprfs_scheme_rfss);
        if(!plain && !secure)
        {
            SNAP_LOG_WARNING
                << "the \"my_addresses=...\" parameter \""
                << uri
                << "\" includes a URI with an unsupported scheme."
                << SNAP_LOG_SEND;
            continue;
        }

        addr::addr_range::vector_t const & ranges(u.address_ranges());
        if(ranges.size() != 1
        || !ranges[0].has_from()
        || ranges[0].has_to())
        {
            SNAP_LOG_WARNING
                << "the \"my_addresses=...\" parameter must have one valid IP address with the scheme set to \"rfs\" or \"rfss\". \""
                << uri
                << "\" is not supported."
                << SNAP_LOG_SEND;
            continue;
        }

        if(!secure)
        {
            bool secure_message(false);
            if(msg.has_parameter(communicatord::g_name_communicatord_param_secure_remote))
            {
                secure_message = advgetopt::is_true(msg.get_parameter(communicatord::g_name_communicatord_param_secure_remote));
            }
            if(secure_message)
            {
                // the message went through a TLS encrypted pipe, so we do
                // not want to send a file through a plain connection,
                // ignore this address
                //
                SNAP_LOG_MINOR
                    << "the file request transfer was sent through a secure communicator daemon, it has to have a secure URI to transfer the file."
                    << SNAP_LOG_SEND;
                continue;
            }
        }

        addr::addr const a(ranges[0].get_from());
        if(f_server->receive_file(filename, mtime, id, a, secure))
        {
            // we were able to connect to that address so we're done here
            //
            break;
        }
    }
}


void messenger::msg_file_deleted(ed::message & msg)
{
    if(!msg.has_parameter("filename"))
    {
        SNAP_LOG_ERROR
            << "received RFS_FILE_DELETED message without a filename: \""
            << msg
            << "\"."
            << SNAP_LOG_SEND;
        return;
    }
    std::string const filename(msg.get_parameter("filename"));

    if(filename.empty())
    {
        SNAP_LOG_ERROR
            << "filename in the RFS_FILE_DELETED cannot be empty."
            << SNAP_LOG_SEND;
        return;
    }

    if(filename[0] != '/')
    {
        SNAP_LOG_ERROR
            << "filename in the RFS_FILE_DELETED must be an absolute path."
            << SNAP_LOG_SEND;
        return;
    }

    f_server->delete_local_file(filename);
}


//void messenger::msg_configuration_filenames(ed::message & msg)
//{
//    snapdev::NOT_USED(msg);
//}
//
//
//void messenger::msg_copy(ed::message & msg)
//{
//    snapdev::NOT_USED(msg);
//}
//
//
//void messenger::msg_duplicate(ed::message & msg)
//{
//    snapdev::NOT_USED(msg);
//}
//
//
//void messenger::msg_list(ed::message & msg)
//{
//    snapdev::NOT_USED(msg);
//}
//
//
//void messenger::msg_move(ed::message & msg)
//{
//    snapdev::NOT_USED(msg);
//}
//
//
//void messenger::msg_ping(ed::message & msg)
//{
//    snapdev::NOT_USED(msg);
//}
//
//
//void messenger::msg_remove(ed::message & msg)
//{
//    snapdev::NOT_USED(msg);
//}
//
//
//void messenger::msg_stat(ed::message & msg)
//{
//    snapdev::NOT_USED(msg);
//}
//
//
//void messenger::msg_version(ed::message & msg)
//{
//    snapdev::NOT_USED(msg);
//}



} // namespace rfs_daemon
// vim: ts=4 sw=4 et
