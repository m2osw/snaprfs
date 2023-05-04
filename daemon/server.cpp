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
 *
 * \msc
 *   width = "2000";
 *   file_listener, server, messenger, sender,
 *   communicatord, remote_communicatord,
 *   remote_messenger, remote_server, remote_receiver;
 * 
 *   file_listener rbox communicatord [label = "source"];
 *   remote_communicatord rbox remote_receiver [label = "destination"];
 *   server=>messenger [label = "create"];
 *   messenger->communicatord [label = "connect"];
 *   server=>file_listener [label = "create"];
 *   server=>sender [label = "create"];
 *   --- [label = "ready"];
 *   file_listener=>server [label = "File Changed"];
 *   server=>messenger [label = "RFS_FILE_CHANGED"];
 *   messenger->communicatord [label = "RFS_FILE_CHANGED"];
 *   communicatord->remote_communicatord [label = "RFS_FILE_CHANGED"];
 *   remote_communicatord->remote_messenger [label = "RFS_FILE_CHANGED"];
 *   remote_messenger=>remote_server [label = "receive_file()"];
 *   remote_server=>remote_receiver [label = "create"];
 *   remote_receiver->sender [label = "connect"];
 *   --- [label = "start send loop"];
 *   sender->remote_receiver [label = "send file data"];
 *   remote_receiver=>remote_receiver [label = "save file (.part1)"];
 *   --- [label = "end send loop"];
 *   remote_receiver=>remote_receiver [label = "rename file (remove .part1)"];
 * \endmsc
 */

// self
//
#include    "server.h"

#include    "data_receiver.h"


// snaprfs
//
#include    <snaprfs/exception.h>
#include    <snaprfs/names.h>
#include    <snaprfs/version.h>


// communicatord
//
#include    <communicatord/names.h>


// advgetopt
//
#include    <advgetopt/exception.h>
#include    <advgetopt/options.h>
#include    <advgetopt/utils.h>


// eventdispatcher
//
#include    <eventdispatcher/signal_handler.h>


// libexcept
//
#include    <libexcept/file_inheritance.h>


// edhttp
//
#include    <edhttp/uri.h>


// snaplogger
//
#include    <snaplogger/message.h>
#include    <snaplogger/options.h>


// snapdev
//
#include    <snapdev/pathinfo.h>
#include    <snapdev/stringize.h>


// C
//
#include    <sys/random.h>


// last include
//
#include    <snapdev/poison.h>



namespace rfs_daemon
{


namespace
{



advgetopt::option const g_command_line_options[] =
{
    // COMMANDS
    //

    // OPTIONS
    //
    advgetopt::define_option(
          advgetopt::Name("listen")
        , advgetopt::Flags(advgetopt::all_flags<
                      advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("plain listen URL for the snaprfs data channel.")
    ),
    advgetopt::define_option(
          advgetopt::Name("certificate")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("certificate for the data server connection.")
    ),
    advgetopt::define_option(
          advgetopt::Name("private-key")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("private key for the data server connection.")
    ),
    advgetopt::define_option(
          advgetopt::Name("secure-listen")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("URL to listen on with TLS for the snaprfs data channel.")
    ),
    advgetopt::define_option(
          advgetopt::Name("watch-dirs")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("one or more colon (:) separated directory names where configuration files are found.")
        , advgetopt::DefaultValue("/usr/share/snaprfs/watch-dirs:/var/lib/snaprfs/watch-dirs")
    ),

    // END
    //
    advgetopt::end_options()
};


char const * g_configuration_directories[] =
{
    "/etc/snaprfs",
    nullptr
};


advgetopt::group_description const g_group_descriptions[] =
{
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_COMMANDS)
        , advgetopt::GroupName("command")
        , advgetopt::GroupDescription("Commands:")
    ),
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_OPTIONS)
        , advgetopt::GroupName("option")
        , advgetopt::GroupDescription("Options:")
    ),
    advgetopt::end_groups()
};


// until we have C++20, remove warnings this way
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "snaprfs",
    .f_group_name = nullptr,
    .f_options = g_command_line_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "SNAPRFS",
    .f_environment_variable_intro = nullptr,
    .f_section_variables_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = "snaprfs.conf",
    .f_configuration_directories = g_configuration_directories,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [--<opt>]\n"
                     "where --<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = SNAPRFS_VERSION_STRING,
    .f_license = "GNU GPL v2",
    .f_copyright = "Copyright (c) 2020-"
                   SNAPDEV_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME,
    .f_groups = g_group_descriptions
};
#pragma GCC diagnostic pop



class modified_timer
    : public ed::timer
{
public:
    typedef std::shared_ptr<modified_timer> pointer_t;

                                modified_timer();

private:
};



} // no name namespace






shared_file::shared_file(std::string const & filename)
    : f_filename(filename)
{
    // get a random number for the identifier of this file
    //
    if(getrandom(&f_id, sizeof(f_id), 0) != sizeof(f_id))
    {
        throw rfs::no_random_data_available("no random data available fo shared_file() identifier");
    }
}


std::string const & shared_file::get_filename() const
{
    return f_filename;
}


std::uint32_t shared_file::get_id() const
{
    return f_id;
}


void shared_file::set_received()
{
    f_received = time(nullptr);
}


snapdev::timespec_ex shared_file::get_received() const
{
    return f_received;
}


void shared_file::set_last_updated()
{
    f_last_updated = snapdev::timespec_ex::gettime();
}


void shared_file::set_start_sharing()
{
    f_start_sharing = snapdev::timespec_ex::gettime();
}


bool shared_file::was_updated() const
{
    return f_last_updated > f_start_sharing;
}





server::server(int argc, char * argv[])
    : f_opts(g_options_environment)
    , f_communicator(ed::communicator::instance())
    , f_messenger(std::make_shared<messenger>(this, f_opts))
{
    snaplogger::add_logger_options(f_opts);

    f_opts.finish_parsing(argc, argv);

    if(!snaplogger::process_logger_options(f_opts, "/etc/snaprfs/logger"))
    {
        // exit on any error
        //
        throw advgetopt::getopt_exit("logger options generated an error.", 1);
    }

    f_messenger->process_communicatord_options();
}


int server::run()
{
    f_communicator->run();
    return f_force_restart ? 1 : 0;
}


void server::ready()
{
    // we receive the READY message each time we reconnect
    //
    // TBD: we probably want to look at disconnecting when we lose the
    //      connection to the communicatord service; then remove this
    //      test since we will then be able to reconnect
    //
    if(f_file_listener != nullptr)
    {
        return;
    }

    // start listening for file changes only once we are connected
    // to the communicator daemon
    //
    std::string const watch_dirs(f_opts.get_string("watch-dirs"));
    f_file_listener = std::make_shared<file_listener>(this, watch_dirs);
    if(f_file_listener->valid_socket())
    {
        // only add if the socket is valid (i.e. we are listening for changes
        // in at least one directory or file)
        //
        f_communicator->add_connection(f_file_listener);
    }

    edhttp::uri u;
    if(!u.set_uri(f_opts.get_string("listen"), false, true))
    {
        SNAP_LOG_ERROR
            << "the \"listen=...\" parameter \""
            << f_opts.get_string("listen")
            << "\" is not a valid URI: "
            << u.get_last_error_message()
            << "."
            << SNAP_LOG_SEND;
        stop(false);
        return;
    }
    if(u.scheme() != "rfs")
    {
        SNAP_LOG_RECOVERABLE_ERROR
            << "the \"listen=...\" parameter must have an address with the scheme set to \"rfs\" not \""
            << u.scheme()
            << "\". \""
            << f_opts.get_string("listen")
            << "\" is not supported."
            << SNAP_LOG_SEND;
    }
    else
    {
        addr::addr_range::vector_t const & ranges(u.address_ranges());
        if(ranges.size() != 1
        || !ranges[0].has_from()
        || ranges[0].has_to())
        {
            SNAP_LOG_ERROR
                << "the \"listen=...\" parameter must have an address with the scheme set to \"rfs\". \""
                << f_opts.get_string("listen")
                << "\" is not supported."
                << SNAP_LOG_SEND;
            stop(false);
            return;
        }
        f_data_server = std::make_shared<data_server>(
                              this
                            , ranges[0].get_from()
                            , std::string()
                            , std::string()
                            , ed::mode_t::MODE_PLAIN
                            , -1
                            , true);
        f_communicator->add_connection(f_data_server);
    }

    // also create a secure listener if the user specified a
    // certificate and a private key and a secure-listen URI
    //
    if(!f_opts.is_defined("secure-listen")
    && !f_opts.is_defined("certificate")
    && !f_opts.is_defined("private-key"))
    {
        std::string const secure_listen(f_opts.get_string("secure-listen"));
        std::string const certificate(f_opts.get_string("certificate"));
        std::string const private_key(f_opts.get_string("private-key"));
        if(!secure_listen.empty() && !certificate.empty() && !private_key.empty())
        {
            if(!u.set_uri(secure_listen))
            {
                SNAP_LOG_ERROR
                    << "the \"listen=...\" parameter \""
                    << f_opts.get_string("listen")
                    << "\" is not a valid URI: "
                    << u.get_last_error_message()
                    << "."
                    << SNAP_LOG_SEND;
                stop(false);
                return;
            }
            if(u.scheme() != "rfss")
            {
                SNAP_LOG_RECOVERABLE_ERROR
                    << "the \"secure_listen=...\" parameter must have an address with the scheme set to \"rfss\" not \""
                    << u.scheme()
                    << "\". \""
                    << secure_listen
                    << "\" is not supported."
                    << SNAP_LOG_SEND;
            }
            else
            {
                addr::addr_range::vector_t const & ranges(u.address_ranges());
                if(ranges.size() != 1
                || ranges[0].size() != 1
                || !ranges[0].has_from())
                {
                    SNAP_LOG_ERROR
                        << "the \"secure_listen=...\" parameter must have an address with the scheme set to \"rfs\". \""
                        << secure_listen
                        << "\" is not supported."
                        << SNAP_LOG_SEND;
                    stop(false);
                    return;
                }
                else
                {
                    f_secure_data_server = std::make_shared<data_server>(
                                          this
                                        , ranges[0].get_from()
                                        , certificate
                                        , private_key
                                        , ed::mode_t::MODE_ALWAYS_SECURE
                                        , -1
                                        , true);
                    f_communicator->add_connection(f_secure_data_server);
                }
            }
        }
    }
}


void server::restart()
{
    f_force_restart = true;
    stop(false);
}


void server::stop(bool quitting)
{
    SNAP_LOG_INFO
        << (quitting ? "quitting" : "stopping")
        << " snaprfs service."
        << SNAP_LOG_SEND;

    if(f_messenger != nullptr)
    {
        f_messenger->unregister_communicator(quitting);
    }

    if(f_communicator != nullptr)
    {
        f_communicator->remove_connection(f_file_listener);
        f_file_listener.reset();
    }
}


shared_file::pointer_t server::get_file(std::uint32_t id)
{
    auto const & it(f_files.find(id));
    if(it != f_files.end())
    {
        return shared_file::pointer_t();
    }

    return it->second;
}


void server::updated_file(
      std::string const & path
    , std::string const & filename
    , bool updated)
{
    shared_file::pointer_t file;

    std::string fullpath(snapdev::pathinfo::canonicalize(path, filename));
    auto it(std::find_if(
          f_files.begin()
        , f_files.end()
        , [fullpath](auto const & f)
        {
            return f.second->get_filename() == fullpath;
        }));
    if(it == f_files.end())
    {
        // it does not exist in our list, just prepare it and let other
        // snaprfs know it was updated
        //
        file = std::make_shared<shared_file>(fullpath);
        f_files[file->get_id()] = file;
    }
    else
    {
        // it exists, we need to re-send from scratch since the file
        // changed
        //
        file = it->second;
    }
    file->set_last_updated();

    if(updated)
    {
        // on an update, we start right away, otherwise the timer will
        // call the start whenever the "last updated" is N seconds in
        // the past
        //
        broadcast_file_changed(file);
    }
}


void server::deleted_file(
      std::string const & path
    , std::string const & filename)
{
    shared_file::pointer_t file;

    std::string fullpath(snapdev::pathinfo::canonicalize(path, filename));
    auto it(std::find_if(
          f_files.begin()
        , f_files.end()
        , [fullpath](auto const & f)
        {
            return f.second->get_filename() == fullpath;
        }));
    if(it != f_files.end())
    {
        // it exists in our list, remove it, it's gone now
        //
        f_files.erase(it);
    }

    ed::message msg;
    msg.set_command(snaprfs::g_name_snaprfs_cmd_rfs_file_deleted);
    msg.set_service(communicatord::g_name_communicatord_service_public_broadcast); // see communicatord/TODO.md -- how to only broadcast to snaprfs services
    msg.add_parameter(snaprfs::g_name_snaprfs_param_filename, filename);
std::cerr << "--- sending message [" << msg.get_command() << "]\n";
    f_messenger->send_message(msg);
}


void server::broadcast_file_changed(shared_file::pointer_t file)
{
    // broadcast to others about the fact that file was modified so they
    // can download the file from us
    //
    ed::message msg;
    msg.set_command(snaprfs::g_name_snaprfs_cmd_rfs_file_changed);
    msg.set_service(communicatord::g_name_communicatord_service_public_broadcast); // see communicatord/TODO.md -- how to only broadcast to snaprfs services
    msg.add_parameter(snaprfs::g_name_snaprfs_param_filename, file->get_filename());
    msg.add_parameter(snaprfs::g_name_snaprfs_param_id, file->get_id());
    msg.add_parameter(snaprfs::g_name_snaprfs_param_my_address, f_messenger->get_my_address());
std::cerr << "--- sending message [" << msg.get_command() << "]\n";
    f_messenger->send_message(msg);
}


void server::receive_file(std::string const & filename, std::uint32_t id, addr::addr const & address)
{
    data_receiver::pointer_t receiver(std::make_shared<data_receiver>(
          filename
        , id
        , address
        , ed::mode_t::MODE_PLAIN));
    f_communicator->add_connection(receiver);
}


void server::delete_local_file(
      std::string const & filename)
{
    int const r(unlink(filename.c_str()));
    if(r != 0 && errno != ENOENT)
    {
        int const e(errno);
        SNAP_LOG_MINOR
            << "could not delete \""
            << filename
            << "\" (errno: "
            << e
            << ", "
            << strerror(e)
            << ")."
            << SNAP_LOG_SEND;
        return;
    }

    auto it(std::find_if(
          f_files.begin()
        , f_files.end()
        , [filename](auto const & f)
        {
            return f.second->get_filename() == filename;
        }));
    if(it != f_files.end())
    {
        // it exists in our list, remove it, it's gone now
        //
        f_files.erase(it);
    }
}




} // namespace rfs_daemon



int main(int argc, char * argv[])
{
    ed::signal_handler::create_instance();
    libexcept::verify_inherited_files();

    try
    {
        rfs_daemon::server rfs(argc, argv);
        return rfs.run();
    }
    catch(advgetopt::getopt_exit const & e)
    {
        return e.code();
    }
    catch(std::exception const & e)
    {
        std::cerr << "error: an exception occurred: "
                  << e.what()
                  << std::endl;
        return 1;
    }
}



// vim: ts=4 sw=4 et
