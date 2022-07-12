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


/** \file
 * \brief RFS Service.
 *
 * This file implements the RFS Service.
 *
 * This includes the event dispatcher of the service. The connections are
 * defined in separate files.
 *
 * \li Connection -- connect using TCP port XXXX for eventdispatcher messages
 * \li Connection -- listen on TCP port XXXX for data transmission (plain)
 * \li Connection -- listen on TCP port XXXX for data transmission (encrypted)
 * \li Connection -- listen on UDP port XXXX for data transmission (plain)
 * \li File -- listen for file changes (create/delete/modify...)
 *
 * The eventdispatcher message channel is used to send requests to copy
 * files, although many requests are part of configuration files, you can
 * dynamically add a file or a folder using a message.
 *
 * That channel is also used to communicate with other computers on your
 * network.
 *
 * Finally, it is used for various administrative reasons such as sending
 * the LOG_ROTATE message to reload the logger's configuration setup.
 *
 * The transmission channels are used/selected depending on how the data
 * needs to be transferred. In most cases, we use the UDP channel. If
 * encryption is necessary, the encrypted TCP channel gets used. At this
 * time I'm not too sure why we'd want to use the plain TCP connection.
 */

// rfs library
//
#include    <snaprfs/version.h>


// advgetopt lib
//
#include    <advgetopt/advgetopt.h>
#include    <advgetopt/exception.h>
#include    <advgetopt/options.h>
#include    <advgetopt/utils.h>


// eventdispatcher
//
#include    <eventdispatcher/signal_handler.h>


// libexcept
//
#include    <libexcept/file_inheritance.h>


// snaplogger lib
//
#include    <snaplogger/message.h>
#include    <snaplogger/options.h>


// boost lib
//
#include    <boost/preprocessor/stringize.hpp>


// last include
//
#include    <snapdev/poison.h>



namespace
{



advgetopt::option const g_command_line_options[] =
{
    // COMMANDS
    //
    advgetopt::define_option(
          advgetopt::Name("detach")
        , advgetopt::Flags(advgetopt::all_flags<
                      advgetopt::GETOPT_FLAG_GROUP_COMMANDS
                    , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue("false")
        , advgetopt::Help("whether to work in the background (true) or not (false).")
    ),

    // OPTIONS
    //
    advgetopt::define_option(
          advgetopt::Name("standalone")
        , advgetopt::Flags(advgetopt::standalone_all_flags<
                      advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("do not connect to any remote devices; still useful for local memory cache.")
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
    .f_help_header = "Usage: %p [--<opt>] <config-name> ...\n"
                     "where --<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = SNAPRFS_VERSION_STRING,
    .f_license = "GNU GPL v2",
    .f_copyright = "Copyright (c) 2020-"
                   BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME,
    .f_groups = g_group_descriptions
};
#pragma GCC diagnostic pop





class snaprfs
{
public:
                                        snaprfs(int argc, char * argv[]);

    void                                run();

private:
    advgetopt::getopt                   f_opts;
};


snaprfs::snaprfs(int argc, char * argv[])
    : f_opts(g_options_environment)
{
    snaplogger::add_logger_options(f_opts);

    f_opts.finish_parsing(argc, argv);

    if(!snaplogger::process_logger_options(f_opts, "/etc/snaplogger"))
    {
        // exit on any error
        throw advgetopt::getopt_exit("logger options generated an error.", 0);
    }
}


void snaprfs::run()
{
std::cerr << "--- TODO: imlement\n";
}



}
// no name namespace



int main(int argc, char * argv[])
{
    ed::signal_handler::create_instance();
    libexcept::verify_inherited_files();

    try
    {
        snaprfs rfs(argc, argv);
        rfs.run();
        return 0;
    }
    catch(advgetopt::getopt_exit const &)
    {
        return 0;
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
