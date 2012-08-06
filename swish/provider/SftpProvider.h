/**
    @file

    Wrapped C++ version of ISftpProvider et. al.

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

#ifndef SWISH_INTERFACES_SFTP_PROVIDER_H
#define SWISH_INTERFACES_SFTP_PROVIDER_H
#pragma once

//#include "swish/interfaces/_SftpProvider.h" // MIDL-generated definitions

#include <oaidl.h>
#ifdef __cplusplus

#include <comet/enum_common.h> // enumerated_type_of
#include <comet/interface.h> // comtype
#include <comet/ptr.h> // com_ptr

#include <OleAuto.h> // SysAllocStringLen, SysStringLen, SysFreeString,
                     // VarBstrCmp

#include <string> // wstring

/**
 * The record structure returned by the GetListing() method of the SFTPProvider.
 *
 * This structure represents a single file contained in the directory 
 * specified to GetListing().
 */

struct Listing {
    BSTR bstrFilename;    ///< Directory-relative filename (e.g. README.txt)
    ULONG uPermissions;   ///< Unix file permissions
    BSTR bstrOwner;       ///< The user name of the file's owner
    BSTR bstrGroup;       ///< The name of the group to which the file belongs
    ULONG uUid;           ///< Numerical ID of file's owner
    ULONG uGid;           ///< Numerical ID of group to which the file belongs
    ULONGLONG uSize;      ///< The file's size in bytes
    ULONG cHardLinks;     ///< The number of hard links referencing this file
    DATE dateModified;    ///< The date and time at which the file was 
                          ///< last modified in automation-compatible format
    DATE dateAccessed;    ///< The date and time at which the file was 
                          ///< last accessed in automation-compatible format
    BOOL fIsDirectory;    ///< This filesystem item can be listed for items
                          ///< under it.
    BOOL fIsLink;         ///< This file is a link to another file or directory
};

class IEnumListing : public IUnknown
{
public:

    virtual HRESULT Next (
        ULONG celt,
        struct Listing *rgelt,
        ULONG *pceltFetched) = 0;
    virtual HRESULT Skip (ULONG celt) = 0;
    virtual HRESULT Reset () = 0;
    virtual HRESULT Clone (IEnumListing **ppEnum) = 0;
};

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

class provider_interface
{
public:
    virtual ~provider_interface() {}

    virtual IEnumListing* get_listing(
        ISftpConsumer* consumer, BSTR directory) = 0;

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

    virtual Listing stat(
        ISftpConsumer* consumer, BSTR path, BOOL follow_links) = 0;
};

}}

class ISftpProvider : public IUnknown, public swish::provider::provider_interface
{
public:
    virtual HRESULT GetListing(
        ISftpConsumer *pConsumer,
        BSTR bstrDirectory,
        IEnumListing **ppEnum
    ) = 0;
};

namespace comet {

template<> struct comtype<ISftpProvider>
{
    static const IID& uuid() throw()
    {
        static comet::uuid_t iid("E2D6A1D6-48EB-4F38-9D00-3C4536416C49");
        return iid;
    }
    typedef IUnknown base;
};

template<> struct comtype<ISftpConsumer>
{
    static const IID& uuid() throw()
    {
        static comet::uuid_t iid("304982B4-4FB1-4C2E-A892-3536DF59ACF5");
        return iid;
    }
    typedef IUnknown base;
};

template<> struct comtype<IEnumListing>
{
    static const IID& uuid() throw()
    {
        static comet::uuid_t iid("304982B4-4FB1-4C2E-A892-3536DF59ACF5");
        return iid;
    }
    typedef IUnknown base;
};

template<> struct enumerated_type_of<IEnumListing>
{ typedef Listing is; };

}

namespace swish {

namespace detail {

inline Listing copy_listing(const Listing& other)
{
    Listing lt;

    lt.bstrFilename = ::SysAllocStringLen(
        other.bstrFilename, ::SysStringLen(other.bstrFilename));
    lt.uPermissions = other.uPermissions;
    lt.bstrOwner = ::SysAllocStringLen(
        other.bstrOwner, ::SysStringLen(other.bstrOwner));
    lt.bstrGroup = ::SysAllocStringLen(
        other.bstrGroup, ::SysStringLen(other.bstrGroup));
    lt.uUid = other.uUid;
    lt.uGid = other.uGid;
    lt.uSize = other.uSize;
    lt.cHardLinks = other.cHardLinks;
    lt.dateModified = other.dateModified;
    lt.dateAccessed = other.dateAccessed;
    lt.fIsDirectory = other.fIsDirectory;
    lt.fIsLink = other.fIsLink;

    return lt;
}

}

/**
 * Wrapped version of Listing that cleans up its string resources on
 * destruction.
 */
class SmartListing
{
public:

    SmartListing() : lt(Listing()) {}

    SmartListing(const SmartListing& other) : lt(detail::copy_listing(other.lt))
    {}

    SmartListing(const Listing& other) : lt(detail::copy_listing(other)) {}

    SmartListing& operator=(const SmartListing& other)
    {
        SmartListing copy(other);
        std::swap(this->lt, copy.lt);
        return *this;
    }

    ~SmartListing()
    {
        ::SysFreeString(lt.bstrFilename);
        ::SysFreeString(lt.bstrGroup);
        ::SysFreeString(lt.bstrOwner);
        std::memset(&lt, 0, sizeof(Listing));
    }

    bool operator<(const SmartListing& other) const
    {
        if (lt.bstrFilename == 0)
            return other.lt.bstrFilename != 0;

        if (other.lt.bstrFilename == 0)
            return false;

        return ::VarBstrCmp(
            lt.bstrFilename, other.lt.bstrFilename,
            ::GetThreadLocale(), 0) == VARCMP_LT;
    }

    bool operator==(const SmartListing& other) const
    {
        if (lt.bstrFilename == 0 && other.lt.bstrFilename == 0)
            return true;

        return ::VarBstrCmp(
            lt.bstrFilename, other.lt.bstrFilename,
            ::GetThreadLocale(), 0) == VARCMP_EQ;
    }

    bool operator==(const comet::bstr_t& name) const
    {
        return lt.bstrFilename == name;
    }

    Listing detach()
    {
        Listing out = lt;
        std::memset(&lt, 0, sizeof(Listing));
        return out;
    }

    Listing* out() { return &lt; }
    const Listing& get() const { return lt; }

private:
    Listing lt;
};

} // namespace swish


namespace comet {

/**
 * Copy-policy for use by enumerators of Listing items.
 */
template<> struct impl::type_policy<Listing>
{
    static void init(Listing& t, const swish::SmartListing& s) 
    {
        swish::SmartListing copy = s;
        t = copy.detach();
    }

    static void init(Listing& t, const Listing& s) 
    {
        t = swish::detail::copy_listing(s);
    }

    static void init(swish::SmartListing& t, const Listing& s) 
    {
        t = swish::SmartListing(s);
    }

    static void clear(Listing& t)
    {
        ::SysFreeString(t.bstrFilename);
        ::SysFreeString(t.bstrOwner);
        ::SysFreeString(t.bstrGroup);
        t = Listing();
    }

    static void clear(swish::SmartListing&) {}
};

} // namespace comet

#endif // __cplusplus

#endif SWISH_INTERFACES_SFTP_PROVIDER_H
