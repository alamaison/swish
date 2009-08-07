/**
    @file

    Unit tests for the ShellDataObject wrapper.

    @if licence

    Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "swish/atl.hpp"

#include "swish/shell_folder/ShellDataObject.h"  // Test subject
#include "swish/exception.hpp"  // com_exception

#include "test/common_boost/fixtures.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

using swish::exception::com_exception;
using test::common_boost::ComFixture;
using test::common_boost::SandboxFixture;
using boost::filesystem::wpath;
using ATL::CComPtr;

namespace { // private

	/**
	 * Implementation of SHParseDisplayName() missing from pre-XP Windows.
	 */
	HRESULT SHParseDisplayName(
		LPCWSTR pszName, IBindCtx* pbc, PIDLIST_ABSOLUTE* ppidl, 
		SFGAOF sfgaoIn, SFGAOF* psfgaoOut)
	{
		IShellFolder* psfDesktop;
		HRESULT hr = E_FAIL;
		ULONG dwAttr = sfgaoIn;

		if (!pszName || !ppidl || !psfgaoOut)
			return E_INVALIDARG;

		hr = ::SHGetDesktopFolder(&psfDesktop);
		if (FAILED(hr))
			return hr;

		hr = psfDesktop->ParseDisplayName(
			NULL, pbc, const_cast<LPWSTR>(pszName), NULL, 
			reinterpret_cast<PIDLIST_RELATIVE*>(ppidl), &dwAttr);

		psfDesktop->Release();

		if (SUCCEEDED(hr))
			*psfgaoOut = dwAttr;

		return hr;
	}

	/**
	 * Return an IDataObject representing a file on the local filesystem.
	 */
	CComPtr<IDataObject> GetDataObjectOfLocalFile(wpath local)
	{
		HRESULT hr;
		CComPtr<IDataObject> spdo;

		PIDLIST_ABSOLUTE pidl;
		SFGAOF sfgao;
		hr = SHParseDisplayName(
			local.file_string().c_str(), NULL, &pidl, 0, &sfgao);
		if (SUCCEEDED(hr))
		{
			CComPtr<IShellFolder> spsf;
			PCUITEMID_CHILD pidlChild; // TODO: free this?
			hr = ::SHBindToParent(
				pidl, __uuidof(IShellFolder), (void**)&spsf, &pidlChild);
			if (SUCCEEDED(hr))
			{
				hr = spsf->GetUIObjectOf(
					NULL, 1, &pidlChild, __uuidof(IDataObject), NULL, 
					(void**)&spdo);
			}

			::CoTaskMemFree(pidl);
		}

		if (FAILED(hr))
			throw com_exception(hr);

		return spdo;
	}

	class DataObjectFixture : public ComFixture, public SandboxFixture
	{
	public:
		CComPtr<IDataObject> data_object_to_sandbox_file()
		{
			return GetDataObjectOfLocalFile(NewFileInSandbox());
		}
	};
}

BOOST_FIXTURE_TEST_SUITE(shell_data_object, DataObjectFixture)

/**
 * Create and destroy an instance of the StorageMedium helper object.
 * Check a few member to see if they are initialsed properly.
 */
BOOST_AUTO_TEST_CASE( storage_medium_lifecycle )
{
	{
		StorageMedium medium;

		BOOST_REQUIRE(medium.empty());
		BOOST_REQUIRE_EQUAL(medium.get().hGlobal, static_cast<HGLOBAL>(NULL));
		BOOST_REQUIRE_EQUAL(medium.get().pstm, static_cast<IStream*>(NULL));
		BOOST_REQUIRE_EQUAL(
			medium.get().pUnkForRelease, static_cast<IUnknown*>(NULL));
	}
}

/**
 * Get a PIDL from the shell data object.
 */
BOOST_AUTO_TEST_CASE( cfstr_shellidlist )
{
	ShellDataObject data_object(data_object_to_sandbox_file());

	BOOST_REQUIRE(data_object.has_pidl_format());
	BOOST_REQUIRE_EQUAL(data_object.pidl_count(), 1U);

//	CAbsolutePidl folder_pidl
}

BOOST_AUTO_TEST_SUITE_END()
