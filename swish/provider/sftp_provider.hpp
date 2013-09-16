/**
    @file

    SFTP backend interfaces.

    @if license

    Copyright (C) 2010, 2011, 2012, 2013
    Alexander Lamaison <awl03@doc.ic.ac.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    @endif
*/

#ifndef SWISH_PROVIDER_SFTP_PROVIDER_H
#define SWISH_PROVIDER_SFTP_PROVIDER_H
#pragma once

#include "swish/provider/sftp_filesystem_item.hpp"
#include "swish/provider/sftp_provider_path.hpp"

#include <boost/filesystem/path.hpp> // wpath
#include <boost/optional/optional.hpp>
//#include <boost/range/any_range.hpp> USE ONCE WE UPGRADE BOOST

#include <comet/interface.h> // comtype
#include <comet/ptr.h> // com_ptr

#include <string> // wstring
#include <utility> // pair
#include <vector>

class ISftpConsumer : public IUnknown
{
public:

    /**
     * Get password from the user.
     *
     * @return
     *     Uninitialised optional if authentication should be aborted.
     *     String containing password, otherwise.
     */
    virtual boost::optional<std::wstring> prompt_for_password() = 0;

    /**
     * Get files containing private and public keys for public-key
     * authentication.
     *
     * @return
     *     Uninitialised optional if public-key authentication should not be
     *     performed using file-based keys.
     *     Pair of paths: private-key file first, public-key file second.
     */
    virtual boost::optional<
        std::pair<boost::filesystem::path, boost::filesystem::path>>
        key_files() = 0;

    /**
     * Perform a challenge-response interaction with the user.
     *
     * @return
     *     Uninitialised optional if authentication should be aborted.
     *     As many responses as there were prompts, otherwise.
     */
    virtual boost::optional<std::vector<std::string>> challenge_response(
        const std::string& title, const std::string& instructions,
        const std::vector<std::pair<std::string, bool>>& prompts) = 0;

    virtual HRESULT OnConfirmOverwrite(
        BSTR bstrOldFile,
        BSTR bstrNewFile
    ) = 0;
    virtual HRESULT OnHostkeyMismatch(
        BSTR bstrHostName,
        BSTR bstrHostKey,
        BSTR bstrHostKeyType
    ) = 0;
    virtual HRESULT OnHostkeyUnknown(
        BSTR bstrHostName,
        BSTR bstrHostKey,
        BSTR bstrHostKeyType
    ) = 0;
};

namespace swish {
namespace provider {

//typedef boost::any_range<
//    sftp_filesystem_item, boost::forward_traversal_tag,
//    sftp_filesystem_item&, std::ptrdiff_t>
//    directory_listing;
typedef std::vector<sftp_filesystem_item> directory_listing;

class sftp_provider
{
public:
    virtual ~sftp_provider() {}

    virtual directory_listing listing(
        comet::com_ptr<ISftpConsumer> consumer,
        const sftp_provider_path& directory) = 0;

    virtual comet::com_ptr<IStream> get_file(
        comet::com_ptr<ISftpConsumer> consumer, std::wstring file_path,
        bool writeable) = 0;

    virtual VARIANT_BOOL rename(
        ISftpConsumer* consumer, BSTR from_path, BSTR to_path) = 0;

    virtual void remove_all(ISftpConsumer* consumer, BSTR path) = 0;

    /**
     * @name Creation methods
     * These are the dual of the deletion methods.  `create_new_file`
     * is mainly for the test-suite.  It just creates an empty file at the
     * given path (roughly equivalent to Unix `touch`).
     */
    // @{

    virtual void create_new_file(ISftpConsumer* consumer, BSTR path) = 0;

    virtual void create_new_directory(ISftpConsumer* consumer, BSTR path) = 0;

    // @}

    /**
     * Return the canonical path of the given non-canonical path.
     *
     * While generally used to resolve symlinks, it can also be used to
     * convert paths relative to the startup directory into absolute paths.
     */
    virtual BSTR resolve_link(ISftpConsumer* consumer, BSTR link_path) = 0;

    virtual sftp_filesystem_item stat(
        comet::com_ptr<ISftpConsumer> consumer, const sftp_provider_path& path,
        bool follow_links) = 0;
};

}}

namespace comet {

template<> struct comtype<ISftpConsumer>
{
    static const IID& uuid() throw()
    {
        static comet::uuid_t iid("304982B4-4FB1-4C2E-A892-3536DF59ACF5");
        return iid;
    }
    typedef IUnknown base;
};

}

#endif
