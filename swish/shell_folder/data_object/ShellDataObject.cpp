/**
    @file

    Classes to handle the typical Explorer 'Shell DataObject'.

    @if licence

    Copyright (C) 2008, 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "ShellDataObject.hpp"

#include "GlobalLocker.hpp" // GlobalLocker
#include "swish/exception.hpp"  // com_exception

#include "boost/shared_ptr.hpp"  // share_ptr

using swish::exception::com_exception;
using swish::shell_folder::data_object::GlobalLocker;
using boost::shared_ptr;

namespace swish {
namespace shell_folder {
namespace data_object {

namespace { // private

	const CLIPFORMAT CF_SHELLIDLIST = 
		static_cast<CLIPFORMAT>(::RegisterClipboardFormat(CFSTR_SHELLIDLIST));

	/**
	 * Lifetime-management class for a CIDA held in global memory in a
	 * STGMEDIUM.
	 *
	 * The lifetimes of a STGMEDIUM holding an HGLOBAL, a lock on that
	 * HGLOBAL and the pointer to the memory it contains.  The pointer
	 * is only valid for the duration of the lock which, in turn, can only
	 * exist while the global memory in the STGMEDIUM is allocated.
	 *
	 * Therefore, this class exists to make it easy to manage the lifetimes
	 * of these three items together.  A caller to get() is free to use the
	 * CIDA returned as long as the instance of this class remains in scope.
	 * Copying is explicity prevented as that reallocates the STGMEDIUM
	 * invalidating both the lock and the pointer to the original memory.
	 */
	class GlobalCida
	{
	public:
		GlobalCida(const StorageMedium& medium) : 
		  m_medium(medium), m_lock(m_medium.get().hGlobal)
		{
		}

		const CIDA& get() const
		{
			CIDA* pcida = m_lock.get();
			if (!pcida)
				throw com_exception(E_UNEXPECTED);
			return *pcida;
		}

	private:
		GlobalCida(const GlobalCida& cida); // Disabled
		GlobalCida& operator=(const GlobalCida& cida); // Disabled
		StorageMedium m_medium;
		GlobalLocker<CIDA> m_lock;
	};

	/**
	 * Return a STGMEDIUM with a list of PIDLs in global memory.
	 */
	StorageMedium cfstr_shellidlist_from_data_object(IDataObject* pdo)
	{
		FORMATETC fetc = {
			CF_SHELLIDLIST, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
		};

		StorageMedium medium;
		HRESULT hr = pdo->GetData(&fetc, medium.out());
		if (FAILED(hr))
			throw com_exception(hr);

		assert(medium.get().hGlobal);
		return medium;
	}

#pragma region CIDA Accessors

	/**
	 * Return a pointer to the ith PIDL in the CIDA.
	 */
	PCIDLIST_RELATIVE pidl_from_cida(const CIDA& cida, int i)
	{
		unsigned int offset = cida.aoffset[i];
		const BYTE* position = reinterpret_cast<const BYTE*>(&cida) + offset;

		return reinterpret_cast<PCIDLIST_RELATIVE>(position);
	}

	/**
	 * Return a pointer to the PIDL corresponding to the parent folder of the 
	 * other PIDLs.
	 */
	PCIDLIST_ABSOLUTE parent_from_cida(const CIDA& cida)
	{
		return static_cast<PCIDLIST_ABSOLUTE>(pidl_from_cida(cida, 0));
	}

	/**
	 * Return a pointer to the ith child PIDL in the CIDA (i+1th PIDL).
	 */
	PCIDLIST_RELATIVE child_from_cida(const CIDA& cida, int i)
	{
		return pidl_from_cida(cida, i + 1);
	}

#pragma endregion
}

ShellDataObject::ShellDataObject( IDataObject *pDataObj ) :
	m_spDataObj(pDataObj)
{
}

ShellDataObject::~ShellDataObject()
{
}

/**
 * Does the data object have the CFSTR_SHELLIDLIST format?
 *
 * This must not call GetData() on the data object in order to make this
 * operation cheap and to prevent premature rendering of delay-rendered data.
 */
bool ShellDataObject::has_pidl_format() const
{
	FORMATETC fetc = {
		CF_SHELLIDLIST, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
	};

	return m_spDataObj->QueryGetData(&fetc) == S_OK;
}

CAbsolutePidl ShellDataObject::GetParentFolder()
{
	GlobalCida global_cida(cfstr_shellidlist_from_data_object(m_spDataObj));

	CAbsolutePidl pidl = parent_from_cida(global_cida.get());
	return pidl;
}

CRelativePidl ShellDataObject::GetRelativeFile(UINT i)
{
	GlobalCida global_cida(cfstr_shellidlist_from_data_object(m_spDataObj));
	if (i >= global_cida.get().cidl)
		throw std::range_error(
			"The index is greater than the number of PIDLs in the Data Object");

	CRelativePidl pidl = child_from_cida(global_cida.get(), i);
	return pidl;
}

CAbsolutePidl ShellDataObject::GetFile(UINT i)
{
	CAbsolutePidl pidlFolder(GetParentFolder(), GetRelativeFile(i));
	return pidlFolder;
}

/**
 * Return the number of PIDLs in the CFSTR_SHELLIDLIST format of the data
 * object.
 */
UINT ShellDataObject::pidl_count()
{
	GlobalCida global_cida(cfstr_shellidlist_from_data_object(m_spDataObj));
	return global_cida.get().cidl;
}

}}} // namespace swish::shell_folder::data_object
