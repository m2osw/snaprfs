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


// snaprfs
//
#include    <snaprfs/connection.h>
#include    <snaprfs/version.h>


// advgetopt
//
#include    <advgetopt/advgetopt.h>
#include    <advgetopt/exception.h>
#include    <advgetopt/options.h>
#include    <advgetopt/utils.h>


// snaplogger
//
#include    <snaplogger/message.h>
#include    <snaplogger/options.h>


// snapdev
//
#include    <snapdev/stringize.h>


// last include
//
#include    <snapdev/poison.h>


namespace
{


advgetopt::option const g_options[] =
{
    // COMMANDS
    //
    advgetopt::define_option(
          advgetopt::Name("configuration-filenames")
        , advgetopt::Flags(advgetopt::standalone_all_flags<
                      advgetopt::GETOPT_FLAG_GROUP_COMMANDS
                    , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE>())
        , advgetopt::Help("requests the configuration information from snaprfs services.")
    ),
    advgetopt::define_option(
          advgetopt::Name("duplicate")
        , advgetopt::Flags(advgetopt::standalone_all_flags<
                      advgetopt::GETOPT_FLAG_GROUP_COMMANDS
                    , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE>())
        , advgetopt::Help("send one file to one or more snaprfs destinations.")
    ),
    advgetopt::define_option(
          advgetopt::Name("copy")
        , advgetopt::Flags(advgetopt::standalone_all_flags<
                      advgetopt::GETOPT_FLAG_GROUP_COMMANDS
                    , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE>())
        , advgetopt::Help("send one or more files to a snaprfs destination.")
    ),
    advgetopt::define_option(
          advgetopt::Name("info")
        , advgetopt::Flags(advgetopt::standalone_all_flags<
                      advgetopt::GETOPT_FLAG_GROUP_COMMANDS
                    , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE>())
        , advgetopt::Help("retrieve information, such as the hostname and version, of the known snaprfs services.")
    ),
    advgetopt::define_option(
          advgetopt::Name("list")
        , advgetopt::Flags(advgetopt::standalone_all_flags<
                      advgetopt::GETOPT_FLAG_GROUP_COMMANDS
                    , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE>())
        , advgetopt::Help("list files managed by the specified snaprfs hosts.")
    ),
    advgetopt::define_option(
          advgetopt::Name("mode")
        , advgetopt::Flags(advgetopt::standalone_all_flags<
                      advgetopt::GETOPT_FLAG_GROUP_COMMANDS
                    , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE>())
        , advgetopt::Help("allow the sending of otherwise unknown commands with this specific mode (0, 1, * twice separated by a colon, for example *:1 means many sources to one destination; *:* is not allowed).")
    ),
    advgetopt::define_option(
          advgetopt::Name("move")
        , advgetopt::Flags(advgetopt::standalone_all_flags<
                      advgetopt::GETOPT_FLAG_GROUP_COMMANDS
                    , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE>())
        , advgetopt::Help("send one or more files to a snaprfs destination and remove the sources once done.")
    ),
    advgetopt::define_option(
          advgetopt::Name("ping")
        , advgetopt::Flags(advgetopt::standalone_all_flags<
                      advgetopt::GETOPT_FLAG_GROUP_COMMANDS
                    , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE>())
        , advgetopt::Help("send a ping to a snaprfs service to verify that it is alive.")
    ),
    advgetopt::define_option(
          advgetopt::Name("rm")
        , advgetopt::Flags(advgetopt::standalone_all_flags<
                      advgetopt::GETOPT_FLAG_GROUP_COMMANDS
                    , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE>())
        , advgetopt::Help("remove one or more files.")
    ),
    advgetopt::define_option(
          advgetopt::Name("stat")
        , advgetopt::Flags(advgetopt::standalone_all_flags<
                      advgetopt::GETOPT_FLAG_GROUP_COMMANDS
                    , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE>())
        , advgetopt::Help("get statistics about one or more files on the snaprfs cluster.")
    ),
    advgetopt::define_option(
          advgetopt::Name("stop")
        , advgetopt::Flags(advgetopt::standalone_all_flags<
                      advgetopt::GETOPT_FLAG_GROUP_COMMANDS
                    , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE>())
        , advgetopt::Help("send the STOP command to a snaprfs service.")
    ),

    // OPTIONS
    //
    advgetopt::define_option(
          advgetopt::Name("force")
        , advgetopt::ShortName('f')
        , advgetopt::Flags(advgetopt::any_flags<
                      advgetopt::GETOPT_FLAG_GROUP_OPTIONS
                    , advgetopt::GETOPT_FLAG_COMMAND_LINE
                    , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE>())
        , advgetopt::Help("copy even if a destination file of the same name exists.")
    ),
    advgetopt::define_option(
          advgetopt::Name("recursive")
        , advgetopt::ShortName('r')
        , advgetopt::Flags(advgetopt::standalone_all_flags<
                      advgetopt::GETOPT_FLAG_GROUP_OPTIONS
                    , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE>())
        , advgetopt::Help("copy directories recursively.")
    ),

    // COMMAND + SOURCE[S] + DESTINATION[S]
    //
    advgetopt::define_option(
          advgetopt::Name("--")
        , advgetopt::Flags(advgetopt::command_flags<
                      advgetopt::GETOPT_FLAG_GROUP_COMMANDS
                    , advgetopt::GETOPT_FLAG_MULTIPLE
                    , advgetopt::GETOPT_FLAG_DEFAULT_OPTION>())
    ),

    // END
    //
    advgetopt::end_options()
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


constexpr char const * const g_configuration_directories[] = {
    "/etc/snaprfs",
    nullptr
};

// until we have C++20, remove warnings this way
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "snaprfs",
    .f_group_name = nullptr,
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "RFS_OPTIONS",
    .f_environment_variable_intro = nullptr,
    .f_section_variables_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = "rfs.conf",
    .f_configuration_directories = g_configuration_directories,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [--<opt>] <command> {<source>} {<destination>}\n"
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





enum class mode_t
{
    MODE_NO_INPUT,
    MODE_ONE_SOURCE,
    MODE_MANY_SOURCES,
    MODE_ONE_DESTINATION,
    MODE_MANY_DESTINATIONS,
    MODE_ONE_SOURCE_ONE_DESTINATION,
    MODE_ONE_SOURCE_MANY_DESTINATIONS,
    MODE_MANY_SOURCES_ONE_DESTINATION
};


class tools
{
public:
                                    tools(int argc, char * argv[]);

    int                             run();

private:
    advgetopt::getopt               f_opts;
    rfs::order::pointer_t           f_order = rfs::order::pointer_t();
};


tools::tools(int argc, char * argv[])
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


int tools::run()
{
    size_t const max(f_opts.size("--"));
    if(max == 0)
    {
        std::cerr << "error: the <command> parameter is missing; try --help for more info."
                  << std::endl;
        return 1;
    }

    std::string const command(f_opts.get_string("--", 0));
    f_order = std::make_shared<rfs::order>(command);

    if(f_opts.is_defined("force"))
    {
        f_order->add_flag(rfs::order_flag_t::ORDER_FLAG_OVERWRITE);
    }

    if(f_opts.is_defined("recursive"))
    {
        f_order->add_flag(rfs::order_flag_t::ORDER_FLAG_RECURSIVE);
    }

    // how to handle the sources and destinations depend on the command
    // so we need a clean way to check the prerequisites; since other
    // systems only use at most one source and one destination, we
    // can have specialized code here
    //
    mode_t mode(mode_t::MODE_MANY_SOURCES);
    if(command == "version"
    || command == "configuration-filenames")
    {
        mode = mode_t::MODE_NO_INPUT;
    }
    else if(command == "ping")
    {
        mode = mode_t::MODE_ONE_SOURCE;
    }
    else if(command == "stop")
    {
        mode = mode_t::MODE_ONE_DESTINATION;
    }
    else if(command == "cp"
         || command == "mv")
    {
        mode = mode_t::MODE_MANY_SOURCES_ONE_DESTINATION;
    }
    else if(command == "dup")
    {
        mode = mode_t::MODE_ONE_SOURCE_MANY_DESTINATIONS;
    }
    else if(command == "stat"
         || command == "list")
    {
        mode = mode_t::MODE_MANY_SOURCES;
    }
    else if(command == "rm")
    {
        mode = mode_t::MODE_MANY_DESTINATIONS;
    }
    else
    {
        if(!f_opts.is_defined("mode"))
        {
            std::cerr << "error: unknown command \""
                      << command
                      << "\"; if you still want to forward that command, try with --mode <mode>."
                      << std::endl;
            return 1;
        }
        std::string const user_mode(f_opts.get_string("mode"));
std::cerr << "---[" << user_mode << "]---\n";
        char const *m(user_mode.c_str());
        char const s(*m);
        if(s == ':')
        {
            std::cerr << "error: invalid mode \""
                      << user_mode
                      << "\"; the source was not specified, try with '0'."
                      << std::endl;
            return 1;
        }
        ++m;
        if(*m != ':')
        {
            std::cerr << "error: invalid mode \""
                      << user_mode
                      << "\"; the colon is missing."
                      << std::endl;
            return 1;
        }
        ++m;
        char const d(*m);
#define MODE_VALUE(a, b)    ((a << 8) + b)
        switch(MODE_VALUE(s, d))
        {
        case MODE_VALUE('0', '0'):
            mode = mode_t::MODE_NO_INPUT;
            break;

        case MODE_VALUE('1', '0'):
            mode = mode_t::MODE_ONE_SOURCE;
            break;

        case MODE_VALUE('0', '1'):
            mode = mode_t::MODE_ONE_DESTINATION;
            break;

        case MODE_VALUE('1', '1'):
            mode = mode_t::MODE_ONE_SOURCE_ONE_DESTINATION;
            break;

        case MODE_VALUE('1', '*'):
            mode = mode_t::MODE_ONE_SOURCE_MANY_DESTINATIONS;
            break;

        case MODE_VALUE('*', '1'):
            mode = mode_t::MODE_MANY_SOURCES_ONE_DESTINATION;
            break;

        case MODE_VALUE('*', '0'):
            mode = mode_t::MODE_MANY_SOURCES;
            break;

        case MODE_VALUE('0', '*'):
            mode = mode_t::MODE_MANY_DESTINATIONS;
            break;

        default:
            std::cerr << "error: invalid mode \""
                      << user_mode
                      << "\"; the source and destination must be one of: 0, 1, or *; note that \"*:*\" is not allowed."
                      << std::endl;
            return 1;

        }
    }

    rfs::connection c;
    c.set_snaprfs_host("127.0.0.1");

    switch(mode)
    {
    case mode_t::MODE_NO_INPUT:
        if(max != 1)
        {
            std::cerr << "error: command \""
                      << command
                      << "\" does not expect any source or destination."
                      << std::endl;
            return 1;
        }
        c.send_order(f_order);
        break;

    case mode_t::MODE_ONE_SOURCE:
        if(max != 2)
        {
            std::cerr << "error: command \""
                      << command
                      << "\" expects exactly one <source>."
                      << std::endl;
            return 1;
        }
        f_order->set_source(f_opts.get_string("--", 1));
        c.send_order(f_order);
        break;

    case mode_t::MODE_MANY_SOURCES:
        if(max < 2)
        {
            std::cerr << "error: command \""
                      << command
                      << "\" expects at least one <source>."
                      << std::endl;
            return 1;
        }
        for(size_t idx(1); idx < max; ++idx)
        {
            f_order->set_source(f_opts.get_string("--", idx));
            c.send_order(f_order);
        }
        break;

    case mode_t::MODE_ONE_DESTINATION:
        if(max != 2)
        {
            std::cerr << "error: command \""
                      << command
                      << "\" expects exactly one <destination>."
                      << std::endl;
            return 1;
        }
        f_order->set_destination(f_opts.get_string("--", 1));
        c.send_order(f_order);
        break;

    case mode_t::MODE_MANY_DESTINATIONS:
        if(max < 2)
        {
            std::cerr << "error: command \""
                      << command
                      << "\" expects at least one <destination>."
                      << std::endl;
            return 1;
        }
        for(size_t idx(1); idx < max; ++idx)
        {
            f_order->set_destination(f_opts.get_string("--", idx));
            c.send_order(f_order);
        }
        break;

    case mode_t::MODE_ONE_SOURCE_ONE_DESTINATION:
        if(max != 3)
        {
            std::cerr << "error: command \""
                      << command
                      << "\" expects exactly one <source> and one <destination>."
                      << std::endl;
            return 1;
        }
        f_order->set_source(f_opts.get_string("--", 1));
        f_order->set_destination(f_opts.get_string("--", 2));
        c.send_order(f_order);
        break;

    case mode_t::MODE_ONE_SOURCE_MANY_DESTINATIONS:
        if(max < 3)
        {
            std::cerr << "error: command \""
                      << command
                      << "\" expects exactly one <source> and one or more <destinations>."
                      << std::endl;
            return 1;
        }
        f_order->set_source(f_opts.get_string("--", 1));
        for(size_t idx(2); idx < max; ++idx)
        {
            f_order->set_destination(f_opts.get_string("--", idx));
            c.send_order(f_order);
        }
        break;

    case mode_t::MODE_MANY_SOURCES_ONE_DESTINATION:
        if(max < 3)
        {
            std::cerr << "error: command \""
                      << command
                      << "\" expects one or more <sources> and at least one <destination>."
                      << std::endl;
            return 1;
        }
        f_order->set_destination(f_opts.get_string("--", max - 1));
        for(size_t idx(1); idx < max - 1; ++idx)
        {
            f_order->set_source(f_opts.get_string("--", idx));
            c.send_order(f_order);
        }
        break;

    }

    return 0;
}




}
// no name namespace


int main(int argc, char * argv[])
{
    try
    {
        tools command(argc, argv);
        return command.run();
    }
    catch(advgetopt::getopt_exit const &)
    {
        return 1;
    }
    catch(std::exception const & e)
    {
        std::cerr << "error: an exception occurred: " << e.what() << std::endl;
        return 1;
    }
}


// vim: ts=4 sw=4 et
