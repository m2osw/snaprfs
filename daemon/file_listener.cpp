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


// self
//
#include    "file_listener.h"

#include    "server.h"


// advgetopt
//
#include    <advgetopt/conf_file.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/glob_to_list.h>
#include    <snapdev/tokenize_string.h>


// C++
//
#include    <list>


// last include
//
#include    <snapdev/poison.h>



namespace rfs_daemon
{



path_info::path_info(std::string const & path)
    : f_path(path)
{
}


std::string const & path_info::get_path() const
{
    return f_path;
}


void path_info::set_mode(path_mode_t mode)
{
    f_mode = mode;
}


path_mode_t path_info::get_mode() const
{
    return f_mode;
}


bool path_info::operator < (path_info const & rhs) const
{
    return f_path < rhs.f_path;
}





file_listener::file_listener(server *s, std::string const & watch_dirs)
    : f_server(s)
{
    std::list<std::string> paths;
    snapdev::tokenize_string(
          paths
        , watch_dirs
        , ":"
        , false);
    for(auto const & p : paths)
    {
        load_setup(p);
    }
}


void file_listener::load_setup(std::string const & dir)
{
    std::string const pattern(dir + "/*.conf");
    snapdev::glob_to_list<std::list<std::string>> filenames;
    if(!filenames.read_path<
            snapdev::glob_to_list_flag_t::GLOB_FLAG_EMPTY>(pattern))
    {
        SNAP_LOG_MINOR
            << "no configuration files found for the file_listener."
            << SNAP_LOG_SEND;
        return;
    }
    for(auto const & name : filenames)
    {
        advgetopt::conf_file_setup setup(name);
        advgetopt::conf_file::pointer_t settings(advgetopt::conf_file::get_conf_file(setup));
        advgetopt::variables::pointer_t variables(std::make_shared<advgetopt::variables>());
        settings->section_to_variables("variables", variables);
        settings->set_variables(variables);
        advgetopt::conf_file::sections_t sections(settings->get_sections());
        for(auto const & s : sections)
        {
            std::string const path_name(s + "::path");
            if(settings->has_parameter(path_name))
            {
                std::string const path(settings->get_parameter(path_name));
                path_info new_path_info(path);
                std::string const mode_name(s + "::mode");
                if(settings->has_parameter(mode_name))
                {
                    std::string const mode(settings->get_parameter(path_name));
                    if(mode.empty()
                    || mode == "send-only")
                    {
                        new_path_info.set_mode(path_mode_t::PATH_MODE_SEND_ONLY);
                    }
                    else if(mode == "receive-only")
                    {
                        new_path_info.set_mode(path_mode_t::PATH_MODE_RECEIVE_ONLY);
                    }
                    else if(mode == "latest")
                    {
                        new_path_info.set_mode(path_mode_t::PATH_MODE_LATEST);
                    }
                    else
                    {
                        SNAP_LOG_RECOVERABLE_ERROR
                            << "ignoring path \""
                            << path
                            << "\" since its mode ("
                            << mode
                            << ") was not recognized."
                            << SNAP_LOG_SEND;
                        continue;
                    }
                }
                //else -- keep default since "send-only" is the safest

                auto const inserted(f_path_info.insert(new_path_info));
                if(inserted.second)
                {
                    if(new_path_info.get_mode() != path_mode_t::PATH_MODE_RECEIVE_ONLY)
                    {
                        // watch the files in this directory
                        //
                        // the UPDATED is used because that tells us the
                        // file was opened, updated (write/truncate) and
                        // then closed--at the moment we do not deal with
                        // files that get and stay opened (i.e. logs like
                        // files will not work well with snaprfs)
                        //
                        // TODO: listen for WRITE events and react after a
                        //       small amount of time (i.e. after say 5 sec.
                        //       still emit a copy event)
                        //
                        watch_file(
                                  new_path_info.get_path()
                                , ed::SNAP_FILE_CHANGED_EVENT_UPDATED
                                | ed::SNAP_FILE_CHANGED_EVENT_WRITE);
                    }
                }
                else
                {
                    SNAP_LOG_RECOVERABLE_ERROR
                        << "ignoring second definition of path \""
                        << path
                        << "\" found in file \""
                        << name
                        << "\"."
                        << SNAP_LOG_SEND;
                    continue;
                }
            }
        }
    }
}


void file_listener::process_event(ed::file_event const & watch_event)
{
std::cerr << "--- received event: "
<< watch_event.get_watched_path()
<< " -- "
<< std::hex << watch_event.get_events() << std::dec
<< " -- "
<< watch_event.get_filename()
<< "\n";

    bool const updated((watch_event.get_events() & ed::SNAP_FILE_CHANGED_EVENT_UPDATED) != 0);
    bool const modified((watch_event.get_events() & ed::SNAP_FILE_CHANGED_EVENT_WRITE) != 0);
    if(updated || modified)
    {
        f_server->updated_file(
                  watch_event.get_watched_path()
                , watch_event.get_filename()
                , updated);
    }
}



} // namespace rfs_daemon
// vim: ts=4 sw=4 et
