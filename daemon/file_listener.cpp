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


// self
//
#include    "file_listener.h"

#include    "server.h"


// snaprfs
//
#include    <snaprfs/exception.h>


// advgetopt
//
#include    <advgetopt/conf_file.h>
#include    <advgetopt/exception.h>


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


/** \brief Count the number of matching segments.
 *
 * This function compares \p path with this path_info object path
 * (f_path). It counts the number of segments that match.
 *
 * \warning
 * At this time, the function expects both paths to be canonicalized. This
 * means that no two slashes (/) follow each other and no "/./" or "/../"
 * are found in the path.
 *
 * \param[in] path  The path to match against this path_info f_path.
 */
int path_info::match_path(std::string const & path) const
{
    int result(0);
    for(std::size_t idx(0); ; ++idx)
    {
        if(idx >= path.length()
        && idx >= f_path.length())
        {
            if(idx > 0 && path[idx - 1] != '/')
            {
                ++result;
            }
            return result;
        }

        if(idx >= path.length()
        || idx >= f_path.length())
        {
            return result;
        }

        if(path[idx] != f_path[idx])
        {
            return result;
        }

        if(path[idx] == '/' && idx > 0)
        {
            ++result;
        }
    }
}


void path_info::set_path_mode(path_mode_t path_mode)
{
    f_path_mode = path_mode;
}


path_mode_t path_info::get_path_mode() const
{
    return f_path_mode;
}


void path_info::set_delete_mode(delete_mode_t delete_mode)
{
    f_delete_mode = delete_mode;
}


delete_mode_t path_info::get_delete_mode() const
{
    return f_delete_mode;
}


void path_info::set_path_part(std::string const & path_part)
{
    f_path_part = path_part;
}


std::string const & path_info::get_path_part() const
{
    return f_path_part;
}


bool path_info::operator < (path_info const & rhs) const
{
    return f_path < rhs.f_path;
}





file_listener::file_listener(server * s, std::string const & watch_dirs)
    : f_server(s)
{
    set_name("file_listener");

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

    SNAP_LOG_CONFIGURATION
        << "found "
        << f_count_paths
        << " directory path"
        << (f_count_paths == 1 ? "" : "s")
        << " to manage, "
        << f_count_listens
        << " of which we are listening to for changes on this system."
        << SNAP_LOG_SEND;

    if(f_count_paths == 0)
    {
        SNAP_LOG_FATAL
            << "absolutely no configuration found; you need at least one path before you can start the snaprfs daemon."
            << SNAP_LOG_SEND;
        throw advgetopt::getopt_exit(
                      "no paths were found in your configuration files; the daemon would no be able to do anything."
                    , advgetopt::CONFIGURATION_EXIT_CODE);
    }
}


void file_listener::load_setup(std::string const & dir)
{
    if(dir.empty()
    || dir == "/")
    {
        // note that "/" is perfectly valid, we just think that's most
        // probably in error and do not want to support it in snaprfs
        //
        throw advgetopt::getopt_exit(
                      "the root directory (/) and an empty string are not valid paths for the inotify configuration file directory."
                    , advgetopt::CONFIGURATION_EXIT_CODE);
    }

    std::string const pattern(dir + "/*.conf");
    snapdev::glob_to_list<std::list<std::string>> filenames;
    if(!filenames.read_path<
            snapdev::glob_to_list_flag_t::GLOB_FLAG_EMPTY>(pattern))
    {
        SNAP_LOG_MINOR
            << "could not read directory with pattern \""
            << pattern
            << "\"."
            << SNAP_LOG_SEND;
        return;
    }

    if(filenames.empty())
    {
        SNAP_LOG_CONFIGURATION
            << "no configuration files found in \""
            << dir
            << "\"."
            << SNAP_LOG_SEND;
        return;
    }

    for(auto const & name : filenames)
    {
        SNAP_LOG_CONFIGURATION
            << "loading configuration \""
            << name
            << "\" for list of directories to listen to."
            << SNAP_LOG_SEND;

        advgetopt::conf_file_setup setup(name);
        advgetopt::conf_file::pointer_t settings(advgetopt::conf_file::get_conf_file(setup));
        advgetopt::variables::pointer_t variables(std::make_shared<advgetopt::variables>());
        settings->section_to_variables("variables", variables);
        settings->set_variables(variables);
        advgetopt::conf_file::sections_t sections(settings->get_sections());
        for(auto const & s : sections)
        {
            std::string const path_name(s + "::path");
            if(!settings->has_parameter(path_name))
            {
                SNAP_LOG_CONFIGURATION
                    << "ignoring section \""
                    << s
                    << "\" since it has no \"path=...\" parameter."
                    << SNAP_LOG_SEND;
                continue;
            }

            std::string const path(settings->get_parameter(path_name));
            if(path.empty()
            || path == "/")
            {
                SNAP_LOG_RECOVERABLE_ERROR
                    << path_name
                    << ": ignoring path \""
                    << path
                    << "\" since its an empty string or \"/\" which are not considered valid for inotify."
                    << SNAP_LOG_SEND;
                continue;
            }

            path_info new_path_info(path);
            std::string const path_mode_name(s + "::path_mode");
            if(settings->has_parameter(path_mode_name))
            {
                std::string const path_mode(settings->get_parameter(path_mode_name));
                if(path_mode.empty()
                || path_mode == "send-only")
                {
                    new_path_info.set_path_mode(path_mode_t::PATH_MODE_SEND_ONLY);
                }
                else if(path_mode == "receive-only")
                {
                    new_path_info.set_path_mode(path_mode_t::PATH_MODE_RECEIVE_ONLY);
                }
                else if(path_mode == "latest")
                {
                    new_path_info.set_path_mode(path_mode_t::PATH_MODE_LATEST);
                }
                else
                {
                    SNAP_LOG_RECOVERABLE_ERROR
                        << "ignoring path \""
                        << path
                        << "\" since its path_mode ("
                        << path_mode
                        << ") was not recognized."
                        << SNAP_LOG_SEND;
                    continue;
                }
            }
            //else -- keep default since "send-only" is the safest

            std::string const delete_name(s + "::delete_mode");
            if(settings->has_parameter(delete_name))
            {
                std::string const delete_mode(settings->get_parameter(delete_name));
                if(delete_mode.empty()
                || delete_mode == "ignore")
                {
                    new_path_info.set_delete_mode(delete_mode_t::DELETE_MODE_IGNORE);
                }
                else if(delete_mode == "apply")
                {
                    new_path_info.set_delete_mode(delete_mode_t::DELETE_MODE_APPLY);
                }
                else
                {
                    SNAP_LOG_RECOVERABLE_ERROR
                        << "unrecognized delete mode \""
                        << delete_mode
                        << "\" ignored."
                        << SNAP_LOG_SEND;
                    continue;
                }
            }

            std::string const path_part_name(s + "::path_part");
            if(settings->has_parameter(path_part_name))
            {
                new_path_info.set_path_part(settings->get_parameter(path_part_name));
            }

            auto const inserted(f_path_info.insert(new_path_info));
            if(!inserted.second)
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

            if(new_path_info.get_path_mode() != path_mode_t::PATH_MODE_RECEIVE_ONLY)
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
                ed::file_event_mask_t flags(
                          ed::SNAP_FILE_CHANGED_EVENT_UPDATED
                        | ed::SNAP_FILE_CHANGED_EVENT_WRITE);
                if(new_path_info.get_delete_mode() == delete_mode_t::DELETE_MODE_APPLY)
                {
                    flags |= ed::SNAP_FILE_CHANGED_EVENT_DELETED;
                }
                watch_files(new_path_info.get_path(), flags);
                ++f_count_listens;
            }
            ++f_count_paths;
        }
    }
}


path_info const * file_listener::find_path_info(std::string const & path) const
{
    std::set<path_info>::iterator it;
    int best_match(0);
    //for(auto const & p : f_path_info)
    for(std::set<path_info>::iterator p(f_path_info.begin()); p != f_path_info.end(); ++p)
    {
        int const count(p->match_path(path));
        if(count > best_match)
        {
            best_match = count;
            it = p;
        }
    }
    if(best_match > 0)
    {
        // found a best match, return its pointer
        //
        return &*it;
    }

    return nullptr;
}


void file_listener::process_event(ed::file_event const & watch_event)
{
std::cerr << "--- received event: " << watch_event.get_watched_path()
<< " -- " << std::hex << watch_event.get_events() << std::dec
<< " -- " << watch_event.get_filename()
<< "\n";

    std::string const fullpath(snapdev::pathinfo::canonicalize(
                                      watch_event.get_watched_path()
                                    , watch_event.get_filename()));

    struct stat s;
    if(stat(fullpath.c_str(), &s) == 0)
    {
        switch(s.st_mode & (S_IFMT))
        {
        case S_IFREG:
        case S_IFDIR:
        case S_IFLNK:
            break;

        default:
            // ignore character, block, fifo, ... type of files
            //
            // last time I checked the format numbers were defined in:
            //     /usr/include/x86_64-linux-gnu/bits/stat.h
            //
            SNAP_LOG_WARNING
                << "found a non-regular file, directory, or symbolic link \""
                << watch_event.get_filename()
                << "\" in directory \""
                << watch_event.get_watched_path()
                << "\" which is snaprfs cannot handle (type: "
                << std::oct << (s.st_mode & (S_IFMT))
                << " in octal)."
                << SNAP_LOG_SEND;
            return;

        }
    }

    bool const updated((watch_event.get_events() & ed::SNAP_FILE_CHANGED_EVENT_UPDATED) != 0);
    bool const modified((watch_event.get_events() & ed::SNAP_FILE_CHANGED_EVENT_WRITE) != 0);
    if(updated || modified)
    {
        f_server->updated_file(fullpath, updated);
    }

    if((watch_event.get_events() & ed::SNAP_FILE_CHANGED_EVENT_DELETED) != 0)
    {
        f_server->deleted_file(fullpath);
    }
}



} // namespace rfs_daemon
// vim: ts=4 sw=4 et
