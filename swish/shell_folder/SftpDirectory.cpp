/*  Manage remote directory as a collection of PIDLs.

    Copyright (C) 2007, 2008, 2009, 2010, 2011
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

#include "swish/host_folder/host_pidl.hpp" // absolute_path_from_swish_pidl

#include <comet/error.h> // com_error
#include <comet/interface.h> // comtype
#include <comet/smart_enum.h> // make_smart_enumeration

#include <boost/make_shared.hpp> // make_shared
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <vector>

using swish::host_folder::absolute_path_from_swish_pidl;
using swish::SmartListing;

using winapi::shell::pidl::apidl_t;

using comet::bstr_t;
using comet::com_error;
using comet::com_error_from_interface;
using comet::com_ptr;
using comet::enum_iterator;
using comet::make_smart_enumeration;

using boost::filesystem::wpath;
using boost::make_shared;
using boost::shared_ptr;

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
	static void init(PITEMID_CHILD& t, const CChildPidl& s) 
	{
		t = s.CopyTo();
	}

	static void clear(PITEMID_CHILD& t)
	{
		::ILFree(t);
	}	
};

}


#define S_IFMT     0170000 /* type of file */
#define S_IFDIR    0040000 /* directory 'd' */
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

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

	bool directory(const SmartListing& lt)
	{ return S_ISDIR(lt.get().uPermissions); }

	bool dotted(const SmartListing& lt)
	{ return lt.get().bstrFilename[0] == OLECHAR('.'); }

	CRemoteItem to_pidl(const SmartListing& lt)
	{
		return CRemoteItem(
			bstr_t(lt.get().bstrFilename).c_str(),
			S_ISDIR(lt.get().uPermissions),
			bstr_t(lt.get().bstrOwner).c_str(),
			bstr_t(lt.get().bstrGroup).c_str(),
			lt.get().uUid,
			lt.get().uGid,
			false,
			lt.get().uPermissions,
			lt.get().uSize,
			lt.get().dateModified,
			lt.get().dateAccessed);
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

	com_ptr<IEnumListing> directory_enum;
	HRESULT hr = m_provider->GetListing(
		m_consumer.in(), bstr_t(m_directory.string()).in(),
		directory_enum.out());
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(com_error_from_interface(m_provider, hr));

	shared_ptr< vector<CChildPidl> > pidls = 
		make_shared< vector<CChildPidl> >();

	do {
		SmartListing lt;
		ULONG fetched = 0;
		hr = directory_enum->Next(1, lt.out(), &fetched);
		if (hr == S_OK)
		{
			if (!include_folders && directory(lt))
				continue;
			if (!include_non_folders && !directory(lt))
				continue;
			if (!include_hidden && dotted(lt))
				continue;

			pidls->push_back(to_pidl(lt));
		}
	} while (hr == S_OK);

	return make_smart_enumeration<IEnumIDList>(pidls).get();
}


enum_iterator<IEnumListing, SmartListing> CSftpDirectory::begin() const
{
	com_ptr<IEnumListing> directory_enum;
	HRESULT hr = m_provider->GetListing(
		m_consumer.in(), bstr_t(m_directory.string()).in(),
		directory_enum.out());
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(com_error_from_interface(m_provider, hr));

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
CSftpDirectory CSftpDirectory::GetSubdirectory(CRemoteItemHandle directory)
{
	if (!directory.IsFolder())
		BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));

	apidl_t sub_directory =
		m_directory_pidl + reinterpret_cast<PCITEMID_CHILD>(directory.Get());
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
com_ptr<IStream> CSftpDirectory::GetFile(
	CRemoteItemHandle file, bool writeable)
{
	bstr_t file_path = (m_directory / file.GetFilename()).string();

	com_ptr<IStream> stream;
	HRESULT hr = m_provider->GetFile(
		m_consumer.in(), file_path.in(), writeable, stream.out());
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(com_error_from_interface(m_provider, hr));
	return stream;
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
	bstr_t file_path = (m_directory / file).string();

	com_ptr<IStream> stream;
	HRESULT hr = m_provider->GetFile(
		m_consumer.in(), file_path.in(), writeable, stream.out());
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(com_error_from_interface(m_provider, hr));
	return stream;
}

bool CSftpDirectory::Rename(
	CRemoteItemHandle old_file, const wstring& new_filename)
{
	VARIANT_BOOL was_target_overwritten = VARIANT_FALSE;
	bstr_t old_file_path = (m_directory / old_file.GetFilename()).string();
	bstr_t new_file_path = (m_directory / new_filename).string();

	HRESULT hr = m_provider->Rename(
		m_consumer.in(), old_file_path.in(), new_file_path.in(),
		&was_target_overwritten);
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(com_error_from_interface(m_provider, hr));

	return (was_target_overwritten == VARIANT_TRUE);
}

void CSftpDirectory::Delete(CRemoteItemHandle file)
{
	bstr_t target_path = (m_directory / file.GetFilename()).string();
	
	HRESULT hr;
	if (file.IsFolder())
		hr = m_provider->DeleteDirectory(m_consumer.in(), target_path.in());
	else
		hr = m_provider->Delete(m_consumer.in(), target_path.in());
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(com_error_from_interface(m_provider, hr));
}

void CSftpDirectory::CreateDirectory(const wstring& name)
{
	bstr_t target_path = (m_directory / name).string();

	HRESULT hr = m_provider->CreateNewDirectory(
		m_consumer.in(), target_path.in());
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(com_error_from_interface(m_provider, hr));
}
