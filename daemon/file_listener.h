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
#pragma once


// eventdispatcher
//
#include    <eventdispatcher/file_changed.h>


// C++
//
#include    <set>



namespace rfs_daemon
{



enum class path_mode_t
{
    PATH_MODE_SEND_ONLY,        // read-only, ignore if other computer sends a copy to us (default)
    PATH_MODE_RECEIVE_ONLY,     // local changes to this file are ignored, we accept copies from other(s)
    PATH_MODE_LATEST,           // send/receive to keep the latest file from anywhere
};


enum class delete_mode_t
{
    DELETE_MODE_IGNORE,         // do nothing when a file gets deleted (default)
    DELETE_MODE_APPLY,          // apply the deletion on other systems
};


class server;


class path_info
{
public:
    typedef std::set<path_info>    set_t;

                        path_info(std::string const & path);

    std::string const & get_path() const;
    void                set_path_mode(path_mode_t mode);
    path_mode_t         get_path_mode() const;
    void                set_delete_mode(delete_mode_t mode);
    delete_mode_t       get_delete_mode() const;

    bool                operator < (path_info const & rhs) const;

private:
    std::string         f_path = std::string();
    path_mode_t         f_path_mode = path_mode_t::PATH_MODE_SEND_ONLY;
    delete_mode_t       f_delete_mode = delete_mode_t::DELETE_MODE_IGNORE;
};


class file_listener
    : public ed::file_changed
{
public:
                        file_listener(
                              server * s
                            , std::string const & watch_dir);
                        file_listener(file_listener const &) = delete;
    file_listener const &
                        operator = (file_listener const &) = delete;

    // file_changed implementation
    //
    virtual void        process_event(ed::file_event const & watch_event) override;

private:
    void                load_setup(std::string const & dir);

    server *            f_server = nullptr;
    path_info::set_t    f_path_info = path_info::set_t();
    std::size_t         f_count_paths = 0;
    std::size_t         f_count_listens = 0;
};



} // namespace rfs_daemon
// vim: ts=4 sw=4 et
