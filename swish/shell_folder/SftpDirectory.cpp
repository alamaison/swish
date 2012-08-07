/*  Manage remote directory as a collection of PIDLs.

    Copyright (C) 2007, 2008, 2009, 2010, 2011, 2012
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
*/

#include "SftpDirectory.h"

#include "swish/host_folder/host_pidl.hpp" // host_itemid_view, create_host_item
#include "swish/remote_folder/remote_pidl.hpp" // remote_itemid_view,
                                               // create_remote_itemid
#include "swish/remote_folder/swish_pidl.hpp" // absolute_path_from_swish_pidl

#include <winapi/shell/pidl_iterator.hpp> // pidl_iterator, find_host_itemid
#include <winapi/trace.hpp> // trace

#include <comet/datetime.h> // datetime_t
#include <comet/error.h> // com_error
#include <comet/interface.h> // comtype
#include <comet/smart_enum.h> // make_smart_enumeration

#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/make_shared.hpp> // make_shared
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <exception> // exception
#include <vector>

using swish::remote_folder::absolute_path_from_swish_pidl;
using swish::remote_folder::create_remote_itemid;
using swish::remote_folder::remote_itemid_view;
using swish::SmartListing;

using swish::host_folder::create_host_itemid;
using swish::host_folder::find_host_itemid;
using swish::host_folder::host_itemid_view;

using winapi::shell::pidl::apidl_t;
using winapi::shell::pidl::cpidl_t;
using winapi::shell::pidl::pidl_iterator;
using winapi::shell::pidl::raw_pidl_iterator;
using winapi::trace;

using comet::auto_attach;
using comet::bstr_t;
using comet::com_error;
using comet::com_error_from_interface;
using comet::com_ptr;
using comet::datetime_t;
using comet::enum_iterator;
using comet::make_smart_enumeration;

using boost::filesystem::wpath;
using boost::make_shared;
using boost::shared_ptr;

using std::exception;
using std::vector;
using std::wstring;

namespace comet {

template<> struct comtype<IEnumIDList>
{
    static const IID& uuid() throw() { return IID_IEnumIDList; }
    typedef IUnknown base;
};

template<> struct enumerated_type_of<IEnumIDList>
{ typedef PITEMID_CHILD is; };

/**
 * Copy-policy for use by enumerators of child PIDLs.
 */
template<> struct impl::type_policy<PITEMID_CHILD>
{
    static void init(PITEMID_CHILD& t, const cpidl_t& s) 
    {
        s.copy_to(t);
    }

    static void clear(PITEMID_CHILD& t)
    {
        ::ILFree(t);
    }    
};

}

/**
 * Creates and initialises directory instance from a PIDL. 
 *
 * @param directory_pidl  PIDL to the directory this object represents.  Must
 *                        start at or before a HostItemId.
 */
CSftpDirectory::CSftpDirectory(
    const apidl_t& directory_pidl,
    com_ptr<ISftpProvider> provider, com_ptr<ISftpConsumer> consumer)
: 
m_provider(provider), m_consumer(consumer), m_directory_pidl(directory_pidl),
m_directory(absolute_path_from_swish_pidl(directory_pidl)) {}

namespace {

    bool is_directory(const SmartListing& lt)
    { return lt.get().fIsDirectory != FALSE; }

    bool is_link(const SmartListing& lt)
    { return lt.get().fIsLink != FALSE; }

    bool is_dotted(const SmartListing& lt)
    { return lt.get().bstrFilename[0] == OLECHAR('.'); }

    cpidl_t to_pidl(const SmartListing& lt)
    {
        return create_remote_itemid(
            lt.get().bstrFilename,
            lt.get().fIsDirectory != FALSE,
            lt.get().fIsLink != FALSE,
            lt.get().bstrOwner,
            lt.get().bstrGroup,
            lt.get().uUid,
            lt.get().uGid,
            lt.get().uPermissions,
            lt.get().uSize,
            datetime_t(lt.get().dateModified),
            datetime_t(lt.get().dateAccessed));
    }
    
    /**
     * Notify the shell that a new directory was created.
     *
     * Primarily, this will cause Explorer to show the new folder in any 
     * windows displaying the parent folder.
     *
     * IMPORTANT: this will only happen if the parent folder is listening for
     * SHCNE_MKDIR notifications.
     *
     * We wait for the event to flush because setting the edit text afterwards
     * depends on this.
     */
    void notify_shell_created_directory(const apidl_t& folder_pidl)
    {
        assert(folder_pidl);

        ::SHChangeNotify(
            SHCNE_MKDIR, SHCNF_IDLIST | SHCNF_FLUSH, folder_pidl.get(), NULL);
    }
    
    /**
     * Notify the shell that a file or directory was deleted.
     *
     * Primarily, this will cause Explorer to remove the item from the parent
     * folder view.
     *
     * This function works out whether it was a file or folder removed by
     * extracting the flag from the remote item ID.
     */
    void notify_shell_of_deletion(
        const apidl_t& parent_folder, const cpidl_t& file_or_folder)
    {
        bool is_folder = remote_itemid_view(file_or_folder).is_folder();
        ::SHChangeNotify(
            (is_folder) ? SHCNE_RMDIR : SHCNE_DELETE,
            SHCNF_IDLIST | SHCNF_FLUSHNOWAIT,
            (parent_folder + file_or_folder).get(), NULL);
    }

}

/**
 * Retrieve an IEnumIDList to enumerate this directory's contents.
 *
 * This function returns an enumerator which can be used to iterate through
 * the contents of this directory as a series of PIDLs.  This listing is a
 * @b copy of the one obtained from the server and will not update to reflect
 * changes.  In order to obtain an up-to-date listing, this function must be 
 * called again to get a new enumerator.
 *
 * @param flags  Flags specifying nature of files to fetch.
 *
 * @returns  Smart pointer to the IEnumIDList.
 * @throws  com_error if an error occurs.
 */
com_ptr<IEnumIDList> CSftpDirectory::GetEnum(SHCONTF flags)
{
    // Interpret supported SHCONTF flags
    bool include_folders = (flags & SHCONTF_FOLDERS) != 0;
    bool include_non_folders = (flags & SHCONTF_NONFOLDERS) != 0;
    bool include_hidden = (flags & SHCONTF_INCLUDEHIDDEN) != 0;

    com_ptr<IEnumListing> directory_enum = m_provider->get_listing(
        m_consumer.in(), m_directory.string());

    shared_ptr< vector<cpidl_t> > pidls = make_shared< vector<cpidl_t> >();

    HRESULT hr;
    do {
        SmartListing lt;
        ULONG fetched = 0;
        hr = directory_enum->Next(1, lt.out(), &fetched);
        if (hr == S_OK)
        {
            if (!include_hidden && is_dotted(lt))
                continue;

            bool is_dir;
            if (is_link(lt))
            {
                // Links don't indicate anything about their target such as
                // whether it is a file or folder so we have to interrogate
                // its target
                bstr_t link_path =
                    (m_directory / lt.get().bstrFilename).string();

                try
                {
                    SmartListing ltTarget;
                    // HACK: Make listings manage their own memory
                    *(ltTarget.out()) = m_provider->stat(
                        m_consumer.in(), link_path.in(), TRUE);

                    // TODO: consider what other properties we might want to
                    // take from the target instead of the link.  Currently
                    // we only take on folderness.
                    lt.out()->fIsDirectory = ltTarget.get().fIsDirectory;
                }
                catch(const exception&)
                {
                    // Broken links are treated like files.  There isn't really
                    // anything else sensible to do with them.
                    lt.out()->fIsDirectory = false;
                }

                assert(lt.get().fIsLink);
            }

            is_dir = is_directory(lt);

            if (!include_folders && is_dir)
                continue;
            if (!include_non_folders && !is_dir)
                continue;

            pidls->push_back(to_pidl(lt));
        }
    } while (hr == S_OK);

    return make_smart_enumeration<IEnumIDList>(pidls).get();
}


enum_iterator<IEnumListing, SmartListing> CSftpDirectory::begin() const
{
    com_ptr<IEnumListing> directory_enum = m_provider->get_listing(
        m_consumer.in(), m_directory.string());

    return enum_iterator<IEnumListing, SmartListing>(directory_enum);
}

enum_iterator<IEnumListing, SmartListing> CSftpDirectory::end() const
{
    return enum_iterator<IEnumListing, SmartListing>();
}

/**
 * Get instance of CSftpDirectory for a subdirectory of this directory.
 *
 * @param directory  Child PIDL of directory within this directory.
 *
 * @returns  Instance of the subdirectory.
 * @throws  com_error if error.
 */
CSftpDirectory CSftpDirectory::GetSubdirectory(const cpidl_t& directory)
{
    if (!remote_itemid_view(directory).is_folder())
        BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));

    apidl_t sub_directory = m_directory_pidl + directory;
    return CSftpDirectory(sub_directory, m_provider, m_consumer);
}

/**
 * Get IStream interface to the remote file specified by the given PIDL.
 *
 * This 'file' may also be a directory but the IStream does not give access
 * to its subitems.
 *
 * @param file  Child PIDL of item within this directory.
 *
 * @returns  Smart pointer of an IStream interface to the file.
 * @throws  com_error if error.
 */
com_ptr<IStream> CSftpDirectory::GetFile(const cpidl_t& file, bool writeable)
{
    wstring file_path =
        (m_directory / remote_itemid_view(file).filename()).string();

    return m_provider->get_file(m_consumer, file_path, writeable);
}

/**
 * Get IStream interface to the remote file specified by a relative path.
 *
 * This 'file' may also be a directory but the IStream does not give access
 * to its subitems.
 *
 * @param file  Path of item relative to this directory (may be at a level
 *              below this directory).
 *
 * @returns  Smart pointer of an IStream interface to the file.
 * @throws  com_error if error.
 */
com_ptr<IStream> CSftpDirectory::GetFileByPath(
    const wpath& file, bool writeable)
{
    return m_provider->get_file(
        m_consumer, (m_directory / file).string(), writeable);
}

bool CSftpDirectory::exists(const cpidl_t& file)
{
    wstring file_path =
        (m_directory / remote_itemid_view(file).filename()).string();

    try
    {
        m_provider->get_file(m_consumer, file_path, false);
    }
    catch (const exception&)
    {
        return false;
    }

    return true;
}

bool CSftpDirectory::Rename(
    const cpidl_t& old_file, const wstring& new_filename)
{
    bstr_t old_file_path =
        (m_directory / remote_itemid_view(old_file).filename()).string();
    bstr_t new_file_path = (m_directory / new_filename).string();

    return m_provider->rename(
        m_consumer.in(), old_file_path.in(), new_file_path.in())
        == VARIANT_TRUE;
}

void CSftpDirectory::Delete(const cpidl_t& file)
{
    bstr_t target_path =
        (m_directory / remote_itemid_view(file).filename()).string();
    
    if (remote_itemid_view(file).is_folder())
        m_provider->delete_directory(m_consumer.in(), target_path.in());
    else
        m_provider->delete_file(m_consumer.in(), target_path.in());

    try
    {
        // Must not report a failure after this point.  The item was deleted
        // even if notifying the shell fails.

        notify_shell_of_deletion(m_directory_pidl, file);
    }
    catch (const exception& e)
    {
        trace("WARNING: Couldn't notify shell of deletion: %s") % e.what();
    }

}

cpidl_t CSftpDirectory::CreateDirectory(const wstring& name)
{
    bstr_t target_path = (m_directory / name).string();

    cpidl_t sub_directory = create_remote_itemid(
        name, true, false, L"", L"", 0, 0, 0, 0, datetime_t::now(),
        datetime_t::now());

    m_provider->create_new_directory(m_consumer.in(), target_path.in());

    try
    {
        // Must not report a failure after this point.  The folder was created 
        // even if notifying the shell fails.

        // TODO: stat new folder for actual parameters

        notify_shell_created_directory(m_directory_pidl + sub_directory);
    }
    catch (const exception& e)
    {
        trace("WARNING: Couldn't notify shell of new folder: %s") % e.what();
    }

    return sub_directory;
}

apidl_t CSftpDirectory::ResolveLink(const cpidl_t& item)
{
    remote_itemid_view symlink(item);
    bstr_t link_path = (m_directory / symlink.filename()).string();
    bstr_t target_path(
        auto_attach(m_provider->resolve_link(m_consumer.in(), link_path.in())));

    // XXX: HACK:
    // Currently, we create the new PIDL for the resolved path by copying all
    // the items up to (not including) the host itemid, then appending a new
    // host itemid containing the full resolved path.  This is a horrible hack
    // and is likely to fail miserably if the resolved target is a file rather
    // than a directory.
    //
    // The proper solution would be to have three types of Item ID (PIDL items):
    // - Server items that just maintain the details of the server connection.
    //   They don't store any path information.
    // - Remote items that hold the details of one segment of the remote path.
    //   Combined in a list after a server item, they identify an absolute
    //   path to a file or directory on a remote server.
    // - Host items that are just shortcuts that resolve to a server item and
    //   one or more remote items.  They hold server information and a starting
    //   path.
    // ... or something like this.  A host item could, in some magical, way hold
    // an absolute PIDL that contains a server items followed by several remote
    // items.  Symlink items could even be a fourth type of item.

    pidl_iterator itemids(m_directory_pidl);
    apidl_t pidl_to_link_target;
    raw_pidl_iterator host_itemid = find_host_itemid(m_directory_pidl);
    while (itemids != host_itemid)
    {
        pidl_to_link_target += *itemids++;
    }

    host_itemid_view old_item(*host_itemid);
    cpidl_t new_host_item = create_host_itemid(
        old_item.host(), old_item.user(), L"", old_item.port(),
        old_item.label());
    
    apidl_t resolved_target = pidl_to_link_target + new_host_item;
    BOOST_FOREACH(const wpath& segment, wpath(target_path.w_str()))
    {
        resolved_target += create_remote_itemid(
            segment.filename(), true, false, L"", L"", 0, 0, 0, 0,
            datetime_t(), datetime_t());
    }

    return resolved_target;
}
