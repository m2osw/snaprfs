// Copyright (c) 2019-2023  Made to Order Software Corp.  All Rights Reserved
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
#include    <snapdev/mounts.h>
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
          advgetopt::Name("temp-dirs")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("list of directories where transferred files are saved temporarilly.")
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
          advgetopt::Name("transfer-after-sec")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("number of seconds after which a modified file gets transferred even if not closed.")
        , advgetopt::DefaultValue("10")
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

                                modified_timer(server * s, std::uint32_t transfer_after_sec);
                                modified_timer(modified_timer const &) = delete;
    modified_timer &            operator = (modified_timer const &) = delete;

    void                        add_file(shared_file::pointer_t file);
    void                        remove_file(shared_file::pointer_t file);

    // timer implementation
    virtual void                process_timeout() override;

private:
    server *                    f_server = nullptr;
    shared_file::set_t          f_modified_files = shared_file::set_t();
    snapdev::timespec_ex        f_transfer_after_sec = snapdev::timespec_ex();
};


modified_timer::pointer_t       g_modified_timer = modified_timer::pointer_t();


modified_timer::modified_timer(server * s, std::uint32_t transfer_after_sec)
    : timer(1'000'000LL)
    , f_server(s)
    , f_transfer_after_sec(std::max(3L, std::int64_t(transfer_after_sec)), 0) // minimum is 3 seconds
{
    set_name("modified_timer");
}


void modified_timer::add_file(shared_file::pointer_t file)
{
    f_modified_files.insert(file);

    set_enable(true);
}


void modified_timer::remove_file(shared_file::pointer_t file)
{
    auto it(f_modified_files.find(file));
    if(it == f_modified_files.end())
    {
        return;
    }

    f_modified_files.erase(it);

    if(f_modified_files.empty())
    {
        set_enable(false);
    }
}


void modified_timer::process_timeout()
{
    // we have two timestamp and a duration:
    //   - when file was last updated (LU)
    //   - now (N)
    //   - transfer after seconds (TA)
    //
    // this is used as:
    //
    //   LU + TA <= N
    //
    // to avoid as much math as possible inside the loop, we compute
    // a parameter called threshold (T) from the equation above:
    //
    //   LU <= N - TA
    //
    // so:
    //
    //   T = N - TA
    //
    // and as a result, the loop just does:
    //
    //   LU <= T
    //
    snapdev::timespec_ex const threshold(snapdev::timespec_ex::gettime() - f_transfer_after_sec);
    for(auto m(f_modified_files.begin()); m != f_modified_files.end(); )
    {
        // needs to be transferred at all?
        //
        if((*m)->was_updated())
        {
            if((*m)->get_last_updated() <= threshold)
            {
                f_server->broadcast_file_changed(*m);
                m = f_modified_files.erase(m);
            }
            else
            {
                // not time yet
                //
                // Note: this means if updated within f_transfer_after_sec,
                //       it does not get transferred
                //
                //       this goes on until such changes stops for at least
                //       f_transfer_after_sec
                //
                //       at some point we may want to consider adding a way
                //       to share the file sooner if so many changes happen
                //       so quickly (but then we may transfer a file which
                //       changes as we transfer it...)
                //
                ++m;
            }
        }
        else
        {
            // somehow it is not marked as updated, forget about it immediately
            //
            m = f_modified_files.erase(m);
        }
    }
}








snapdev::mounts *               g_mounts = nullptr;


snapdev::mounts const & get_mounts()
{
    // TODO: add a duration for the mounts cache so it gets refreshed once
    //       in a while (especially if snaprfs starts before all the mount
    //       points are up)
    //
    if(g_mounts == nullptr)
    {
        g_mounts = new snapdev::mounts();
    }
    return *g_mounts;
}



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


snapdev::timespec_ex const & shared_file::get_last_updated() const
{
    return f_last_updated;
}


bool shared_file::set_start_sharing()
{
    f_start_sharing = snapdev::timespec_ex::gettime();

    if(stat(f_filename.c_str(), &f_stat) != 0)
    {
        // this can happen if the file is created, updated a few times,
        // then deleted, all of which happens without closing the file first
        //
        SNAP_LOG_WARNING
            << "could not find \""
            << f_filename
            << "\"; cannot start sharing."
            << SNAP_LOG_SEND;
        return false;
    }

    return true;
}


bool shared_file::was_updated() const
{
    return f_last_updated > f_start_sharing;
}


/** \brief Get the file last modification time.
 *
 * The function transforms the file `stat()` last modification time in
 * a string with the Unix time with a precision of nanoseconds as found
 * in the timespec structure.
 *
 * \warning
 * The f_stat field gets updated only when the set_start_sharing() function
 * is called.
 *
 * \return The last modification time as Unix timestamp with up to 9 digits
 * after the decimal point (nanoseconds).
 */
std::string shared_file::get_mtime() const
{
    snapdev::timespec_ex t(f_stat.st_mtim);
    std::stringstream ss;
    ss << t;
    return ss.str();
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

    if(f_opts.is_defined("temp_dirs"))
    {
        snapdev::tokenize_string(f_temp_dirs, f_opts.get_string("temp_dirs"), ":");
    }
    if(f_temp_dirs.empty())
    {
        f_temp_dirs.push_back("/var/lib/snaprfs/tmp");
    }
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
    if(g_modified_timer != nullptr
    || f_file_listener != nullptr)
    {
        return;
    }

    std::uint32_t const transfer_after_sec(f_opts.get_long("transfer-after-sec"));
    g_modified_timer = std::make_shared<modified_timer>(this, transfer_after_sec);
    f_communicator->add_connection(g_modified_timer);

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
    if(u.scheme() != snaprfs::g_name_snaprfs_scheme_rfs)
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
            if(!u.set_uri(secure_listen, false, true))
            {
                SNAP_LOG_ERROR
                    << "the \"secure_listen=...\" parameter \""
                    << secure_listen
                    << "\" is not a valid URI: "
                    << u.get_last_error_message()
                    << "."
                    << SNAP_LOG_SEND;
                stop(false);
                return;
            }
            if(u.scheme() != snaprfs::g_name_snaprfs_scheme_rfss)
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
        f_communicator->remove_connection(g_modified_timer);
        f_file_listener.reset();
    }
}


shared_file::pointer_t server::get_file(std::uint32_t id)
{
    auto const & it(f_files.find(id));
    if(it == f_files.end())
    {
        return shared_file::pointer_t();
    }

    return it->second;
}


void server::updated_file(
      std::string const & fullpath
    , bool updated)
{
    shared_file::pointer_t file;

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

//auto it2 = f_files.find(file->get_id());
//std::cerr << "------ " << (updated ? "updated" : "wrote to")
//<< " a file... [" << fullpath
//<< "] with ptr " << file.get() << " and ID: " << file->get_id()
//<< " found: " << std::boolalpha << (it2 != f_files.end()) << "\n";

    if(updated)
    {
        // on an update, we start right away, otherwise the timer will
        // call the start whenever the "last updated" is N seconds in
        // the past
        //
        broadcast_file_changed(file);
    }
    else
    {
        g_modified_timer->add_file(file);
    }
}


void server::deleted_file(std::string const & fullpath)
{
    shared_file::pointer_t file;

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
    msg.set_server(communicatord::g_name_communicatord_server_remote);
    msg.set_service("snaprfs");
    msg.add_parameter(snaprfs::g_name_snaprfs_param_filename, fullpath);
//std::cerr << "--- sending message [" << msg.get_command() << "]\n";
    f_messenger->send_message(msg);
}


void server::broadcast_file_changed(shared_file::pointer_t file)
{
    if(!file->set_start_sharing())
    {
        // if false, the file is not available anymore
        //
        return;
    }

    // broadcast to others about the fact that file was modified so they
    // can download the file from us
    //
    ed::message msg;
    msg.set_command(snaprfs::g_name_snaprfs_cmd_rfs_file_changed);
    msg.set_server(communicatord::g_name_communicatord_server_remote);
    msg.set_service(snaprfs::g_name_snaprfs_param_service);
    msg.add_parameter(snaprfs::g_name_snaprfs_param_filename, file->get_filename());
    msg.add_parameter(snaprfs::g_name_snaprfs_param_id, file->get_id());
    msg.add_parameter(snaprfs::g_name_snaprfs_param_mtime, file->get_mtime());
    std::string my_addresses;
    if(f_data_server != nullptr)
    {
        addr::addr const a(f_data_server->get_address());
        if(!my_addresses.empty())
        {
            my_addresses += ',';
        }
        my_addresses += snaprfs::g_name_snaprfs_scheme_rfs;
        my_addresses += "://";
        my_addresses += a.to_ipv4or6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT);
    }
    if(f_secure_data_server != nullptr)
    {
        addr::addr const a(f_secure_data_server->get_address());
        if(!my_addresses.empty())
        {
            my_addresses += ',';
        }
        my_addresses += snaprfs::g_name_snaprfs_scheme_rfss;
        my_addresses += "://";
        my_addresses += a.to_ipv4or6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT);
    }
    msg.add_parameter(snaprfs::g_name_snaprfs_param_my_addresses, my_addresses);
//std::cerr << "--- sending message [" << msg.to_string() << "]\n";
    f_messenger->send_message(msg);
}


void server::receive_file(
      std::string const & filename
    , std::uint32_t id
    , addr::addr const & address
    , bool secure)
{
    // make sure we can receive this file
    //
    std::string const & path(snapdev::pathinfo::dirname(filename));
    path_info const * p(f_file_listener->find_path_info(path));
    if(p == nullptr)
    {
        SNAP_LOG_VERBOSE
            << "path info for \""
            << filename
            << "\" was not found on this computer. Ignore transfer order."
            << SNAP_LOG_SEND;
        return;
    }
    switch(p->get_path_mode())
    {
    case path_mode_t::PATH_MODE_RECEIVE_ONLY:
    case path_mode_t::PATH_MODE_LATEST:
        break;

    default:
        SNAP_LOG_VERBOSE
            << "path info for \""
            << filename
            << "\" says we cannot receive this file. Ignore transfer order."
            << SNAP_LOG_SEND;
        return;

    }
    std::string path_part(p->get_path_part());
    if(path_part.empty())
    {
        // find a mount point for that path to the file we want to transfer
        //
        snapdev::mount_entry const * m(snapdev::find_mount(get_mounts(), path));
        if(m != nullptr)
        {
            // use a part directory with the same mount point if possible
            //
            for(auto const & part : f_temp_dirs)
            {
                if(snapdev::pathinfo::is_child_path(m->get_dir(), part))
                {
                    path_part = part;
                    break;
                }
            }
        }
        if(path_part.empty())
        {
            // use default if no mount point matched
            //
            path_part = *f_temp_dirs.begin();
        }
    }

    try
    {
        data_receiver::pointer_t receiver(std::make_shared<data_receiver>(
              filename
            , id
            , path_part
            , address
            , secure
                ? ed::mode_t::MODE_ALWAYS_SECURE
                : ed::mode_t::MODE_PLAIN));
        f_communicator->add_connection(receiver);
    }
    catch(ed::event_dispatcher_exception const & e)
    {
        SNAP_LOG_ERROR
            << "could not connect to receive file \""
            << filename
            << "\" from \""
            << address
            << "\" ("
            << e.what()
            << ")."
            << SNAP_LOG_SEND;
        return;
    }
}


void server::delete_local_file(
      std::string const & filename)
{
    // make sure we can delete this file
    //
    std::string const & path(snapdev::pathinfo::dirname(filename));
    path_info const * p(f_file_listener->find_path_info(path));
    if(p == nullptr)
    {
        SNAP_LOG_VERBOSE
            << "path info for \""
            << filename
            << "\" was not found on this computer. Ignore delete order."
            << SNAP_LOG_SEND;
        return;
    }
    switch(p->get_delete_mode())
    {
    case delete_mode_t::DELETE_MODE_APPLY:
        break;

    default:
        SNAP_LOG_VERBOSE
            << "path info for \""
            << filename
            << "\" says we cannot delete this file. Ignore delete order."
            << SNAP_LOG_SEND;
        return;

    }

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
