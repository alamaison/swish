/**
    @file

    SFTP backend interfaces.

    @if license

    Copyright (C) 2010, 2011, 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include <boost/filesystem/path.hpp> // wpath
//#include <boost/range/any_range.hpp>

#include <comet/interface.h> // comtype
#include <comet/ptr.h> // com_ptr

#include <OleAuto.h> // SysAllocStringLen, SysStringLen, SysFreeString,
                     // VarBstrCmp

#include <string> // wstring
#include <vector>

class ISftpConsumer : public IUnknown
{
public:

    virtual HRESULT OnPasswordRequest(
        BSTR bstrRequest,
        BSTR *pbstrPassword
    ) = 0;
    virtual HRESULT OnKeyboardInteractiveRequest(
        BSTR bstrName,
        BSTR bstrInstruction,
        SAFEARRAY* saPrompts,
        SAFEARRAY* saShowResponses,
        SAFEARRAY* *psaResponses
    ) = 0;
    virtual HRESULT OnPrivateKeyFileRequest(
        BSTR *pbstrPrivateKeyFile
    ) = 0;
    virtual HRESULT OnPublicKeyFileRequest(
        BSTR *pbstrPublicKeyFile
    ) = 0;
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

typedef boost::filesystem::wpath sftp_provider_path;

/**
 * An entry in a remote SFTP directory.
 */
class SmartListing
{
public:

    SmartListing()
    {
        bstrFilename = NULL;
        uPermissions = 0U;
        bstrOwner = NULL;
        bstrGroup = NULL;
        uUid = 0U;
        uGid = 0U;
        uSize = 0U;
        dateModified = 0;
        dateAccessed = 0;
        fIsDirectory = FALSE;
        fIsLink = FALSE;
    }

    SmartListing(const SmartListing& other)
    {
        bstrFilename = ::SysAllocStringLen(
            other.bstrFilename, ::SysStringLen(other.bstrFilename));
        uPermissions = other.uPermissions;
        bstrOwner = ::SysAllocStringLen(
            other.bstrOwner, ::SysStringLen(other.bstrOwner));
        bstrGroup = ::SysAllocStringLen(
            other.bstrGroup, ::SysStringLen(other.bstrGroup));
        uUid = other.uUid;
        uGid = other.uGid;
        uSize = other.uSize;
        dateModified = other.dateModified;
        dateAccessed = other.dateAccessed;
        fIsDirectory = other.fIsDirectory;
        fIsLink = other.fIsLink;
    }

    friend void swap(SmartListing& lhs, SmartListing& rhs)
    {
        std::swap(lhs.bstrFilename, rhs.bstrFilename);
        std::swap(lhs.uPermissions, rhs.uPermissions);
        std::swap(lhs.bstrOwner, rhs.bstrOwner);
        std::swap(lhs.bstrGroup, rhs.bstrGroup);
        std::swap(lhs.uUid, rhs.uUid);
        std::swap(lhs.uGid, rhs.uGid);
        std::swap(lhs.uSize, rhs.uSize);
        std::swap(lhs.dateModified, rhs.dateModified);
        std::swap(lhs.dateAccessed, rhs.dateAccessed);
        std::swap(lhs.fIsDirectory, rhs.fIsDirectory);
        std::swap(lhs.fIsLink, rhs.fIsLink);
    }

    SmartListing& operator=(const SmartListing& other)
    {
        SmartListing copy(other);
        swap(*this, copy);
        return *this;
    }

    ~SmartListing()
    {
        ::SysFreeString(bstrFilename);
        ::SysFreeString(bstrGroup);
        ::SysFreeString(bstrOwner);
    }

    bool operator<(const SmartListing& other) const
    {
        if (bstrFilename == 0)
            return other.bstrFilename != 0;

        if (other.bstrFilename == 0)
            return false;

        return ::VarBstrCmp(
            bstrFilename, other.bstrFilename,
            ::GetThreadLocale(), 0) == VARCMP_LT;
    }

    bool operator==(const SmartListing& other) const
    {
        if (bstrFilename == 0 && other.bstrFilename == 0)
            return true;

        return ::VarBstrCmp(
            bstrFilename, other.bstrFilename,
            ::GetThreadLocale(), 0) == VARCMP_EQ;
    }

    bool operator==(const comet::bstr_t& name) const
    {
        return bstrFilename == name;
    }

    BSTR bstrFilename;    ///< Directory-relative filename (e.g. README.txt)
    ULONG uPermissions;   ///< Unix file permissions
    BSTR bstrOwner;       ///< The user name of the file's owner
    BSTR bstrGroup;       ///< The name of the group to which the file belongs
    ULONG uUid;           ///< Numerical ID of file's owner
    ULONG uGid;           ///< Numerical ID of group to which the file belongs
    ULONGLONG uSize;      ///< The file's size in bytes
    DATE dateModified;    ///< The date and time at which the file was 
                          ///< last modified in automation-compatible format
    DATE dateAccessed;    ///< The date and time at which the file was 
                          ///< last accessed in automation-compatible format
    BOOL fIsDirectory;    ///< This filesystem item can be listed for items
                          ///< under it.
    BOOL fIsLink;         ///< This file is a link to another file or directory
};

//typedef boost::any_range<
//    SmartListing, boost::forward_traversal_tag, SmartListing&, std::ptrdiff_t>
//    directory_listing;
typedef std::vector<SmartListing> directory_listing;

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

    /**
     * @name Deletion methods
     * We use two methods rather than one for safety.  This makes it explicit 
     * what the intended consequence was. It's possible for a user to ask
     * for a file to be deleted but, meanwhile, it has been changed to a 
     * directory by someone else.  We do not want to delete the directory 
     * without the user knowing.
     */
    // @{

    virtual void delete_file(ISftpConsumer* consumer, BSTR path) = 0;

    virtual void delete_directory(ISftpConsumer* consumer, BSTR path) = 0;

    // @}

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

    virtual SmartListing stat(
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
