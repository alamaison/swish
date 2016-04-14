// Copyright 2008, 2009, 2010, 2012, 2013, 2016 Alexander Lamaison

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef SWISH_PROVIDER_PROVIDER_HPP
#define SWISH_PROVIDER_PROVIDER_HPP
#pragma once

#include "swish/connection/session_manager.hpp" // session_reservation
#include "swish/provider/sftp_provider.hpp"

#include <memory>

namespace swish
{
namespace provider
{

class provider;

class CProvider : public sftp_provider
{
public:
    explicit CProvider(swish::connection::session_reservation&& session_ticket);

    virtual directory_listing listing(const ssh::filesystem::path& directory);

    virtual comet::com_ptr<IStream>
    get_file(const ssh::filesystem::path& file_path,
             std::ios_base::openmode open_mode);

    virtual VARIANT_BOOL rename(ISftpConsumer* consumer,
                                const ssh::filesystem::path& from_path,
                                const ssh::filesystem::path& to_path);

    virtual void remove_all(const ssh::filesystem::path& path);

    virtual void create_new_directory(const ssh::filesystem::path& path);

    virtual ssh::filesystem::path
    resolve_link(const ssh::filesystem::path& link_path);

    virtual sftp_filesystem_item stat(const ssh::filesystem::path& path,
                                      bool follow_links);

private:
    std::unique_ptr<provider> m_provider;
};
}
} // namespace swish::provider

#endif
