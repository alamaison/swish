/**
    @file

    Unit tests for the CDropTarget implementation of IDropTarget.

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

#include "swish/shell_folder/DropTarget.hpp"  // Test subject
#include "swish/exception.hpp"  // com_exception

#include "test/shell_folder/ProviderFixture.hpp"  // ProviderFixture

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <shlobj.h>

#include <string>

using swish::shell_folder::CDropTarget;
using swish::exception::com_exception;

using test::provider::ProviderFixture;

using ATL::CComPtr;

using boost::filesystem::wpath;
using boost::filesystem::ofstream;

using std::string;

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

	const string TEST_DATA = "Lorem ipsum dolor sit amet.\nbob\r\nsally";

	/**
	 * Write some data to a local file and return it as a DataObject.
	 */
	CComPtr<IDataObject> GetTestDataObject(wpath local)
	{
		ofstream test_stream(local);
		test_stream << TEST_DATA;
		return GetDataObjectOfLocalFile(local);
	}
}

BOOST_FIXTURE_TEST_SUITE(DropTarget, ProviderFixture)

/**
 * Create an instance.
 */
BOOST_AUTO_TEST_CASE( create )
{
	CComPtr<ISftpProvider> spProvider = Provider();
	BOOST_REQUIRE(spProvider);

	CComPtr<IDropTarget> sp = CDropTarget::Create(
		spProvider, ToRemotePath(Sandbox()));
	BOOST_REQUIRE(sp);
}

/**
 * Drag enter.  
 * Simulate the user dragging a file onto our folder with the left 
 * mouse button.  The 'shell' is requesting either a copy or a link at our
 * discretion.  The folder drop target should respond S_OK and specify
 * that the effect of the operation is a copy.
 */
BOOST_AUTO_TEST_CASE( drag_enter )
{
	wpath local = NewFileInSandbox();
	CComPtr<IDataObject> spdo = GetTestDataObject(local);

	CComPtr<IDropTarget> spdt = CDropTarget::Create(
		Provider(), ToRemotePath(Sandbox()));

	POINTL pt = {0, 0};
	DWORD dwEffect = DROPEFFECT_COPY | DROPEFFECT_LINK;
	BOOST_REQUIRE_OK(spdt->DragEnter(spdo, MK_LBUTTON, pt, &dwEffect));
	BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_COPY));
}

/**
 * Drag enter.  
 * Simulate the user dragging a file onto our folder but requesting an
 * effect, link, that we don't support.  The folder drop target should 
 * respond S_OK but set the effect to DROPEFFECT_NONE to indicate the drop
 * wasn't possible.
 */
BOOST_AUTO_TEST_CASE( drag_enter_bad_effect )
{
	wpath local = NewFileInSandbox();
	CComPtr<IDataObject> spdo = GetTestDataObject(local);

	CComPtr<IDropTarget> spdt = CDropTarget::Create(
		Provider(), ToRemotePath(Sandbox()));

	POINTL pt = {0, 0};
	DWORD dwEffect = DROPEFFECT_LINK;
	BOOST_REQUIRE_OK(spdt->DragEnter(spdo, MK_LBUTTON, pt, &dwEffect));
	BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_NONE));
}

/**
 * Drag over.  
 * Simulate the situation where a user drags a file over our folder and changes
 * which operation they want as they do so.  In other words, on DragEnter they
 * chose a link which we cannot perform but as they continue the drag (DragOver)
 * they chang their request to a copy which we can do.
 *
 * The folder drop target should respond S_OK and specify that the effect of 
 * the operation is a copy.
 */
BOOST_AUTO_TEST_CASE( drag_over )
{
	wpath local = NewFileInSandbox();
	CComPtr<IDataObject> spdo = GetTestDataObject(local);

	CComPtr<IDropTarget> spdt = CDropTarget::Create(
		Provider(), ToRemotePath(Sandbox()));

	POINTL pt = {0, 0};

	// Do enter with link which should be declined (DROPEFFECT_NONE)
	DWORD dwEffect = DROPEFFECT_LINK;
	BOOST_REQUIRE_OK(spdt->DragEnter(spdo, MK_LBUTTON, pt, &dwEffect));
	BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_NONE));

	// Change request to copy which sould be accepted
	dwEffect = DROPEFFECT_COPY;
	BOOST_REQUIRE_OK(spdt->DragOver(MK_LBUTTON, pt, &dwEffect));
	BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_COPY));
}

/**
 * Drag leave.  
 * Simulate an aborted drag-drop loop where the user drags a file onto our
 * folder, moves it around, and then leaves without dropping.
 *
 * The folder drop target should respond S_OK and any subsequent 
 * DragOver calls should be declined.
 */
BOOST_AUTO_TEST_CASE( drag_leave )
{
	wpath local = NewFileInSandbox();
	CComPtr<IDataObject> spdo = GetTestDataObject(local);

	CComPtr<IDropTarget> spdt = CDropTarget::Create(
		Provider(), ToRemotePath(Sandbox()));

	POINTL pt = {0, 0};

	// Do enter with copy which sould be accepted
	DWORD dwEffect = DROPEFFECT_COPY;
	BOOST_REQUIRE_OK(spdt->DragEnter(spdo, MK_LBUTTON, pt, &dwEffect));
	BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_COPY));

	// Continue drag
	BOOST_REQUIRE_OK(spdt->DragOver(MK_LBUTTON, pt, &dwEffect));
	BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_COPY));

	// Finish drag without dropping
	BOOST_REQUIRE_OK(spdt->DragLeave());

	// Decline any further queries until next DragEnter()
	BOOST_REQUIRE_OK(spdt->DragOver(MK_LBUTTON, pt, &dwEffect));
	BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_NONE));
}

/**
 * Drag and drop.
 * Simulate a complete drag-drop loop where the user drags a file onto our
 * folder, moves it around, and then drops it.
 *
 * The folder drop target should copy the contents of the DataObject to the
 * remote end, respond S_OK and any subsequent DragOver calls should be 
 * declined until a new drag-and-drop loop is started with DragEnter().
 *
 * @todo  Actually verify that the file ends up at the target end.
 */
BOOST_AUTO_TEST_CASE( drop )
{
	wpath local = NewFileInSandbox();
	wpath drop_target_directory = Sandbox() / L"drop-target";
	create_directory(drop_target_directory);

	CComPtr<IDataObject> spdo = GetTestDataObject(local);
	CComPtr<IDropTarget> spdt = CDropTarget::Create(
		Provider(), ToRemotePath(drop_target_directory));

	POINTL pt = {0, 0};

	// Do enter with copy which sould be accepted
	DWORD dwEffect = DROPEFFECT_COPY;
	BOOST_REQUIRE_OK(spdt->DragEnter(spdo, MK_LBUTTON, pt, &dwEffect));
	BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_COPY));

	// Continue drag
	BOOST_REQUIRE_OK(spdt->DragOver(MK_LBUTTON, pt, &dwEffect));
	BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_COPY));

	// Drop onto DropTarget
	BOOST_REQUIRE_OK(spdt->Drop(spdo, MK_LBUTTON, pt, &dwEffect));
	BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_COPY));

	// Decline any further queries until next DragEnter()
	BOOST_REQUIRE_OK(spdt->DragOver(MK_LBUTTON, pt, &dwEffect));
	BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_NONE));
}

BOOST_AUTO_TEST_SUITE_END()
