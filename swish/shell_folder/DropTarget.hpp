/**
    @file

    Expose the remote filesystem as an IDropTarget.

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

#pragma once

#include "swish/shell_folder/SftpProvider.h"  // ISftpProvider

#include "swish/CoFactory.hpp"  // CCoObject factory mixin
#include "swish/atl.hpp"  // Common ATL setup

#include <boost/filesystem.hpp>  // wpath

namespace swish {
namespace shell_folder {

class CDropTarget :
	public ATL::CComObjectRoot,
	public IDropTarget,
	public swish::CCoFactory<CDropTarget>
{
public:

	BEGIN_COM_MAP(CDropTarget)
		COM_INTERFACE_ENTRY(IDropTarget)
	END_COM_MAP()

	static ATL::CComPtr<IDropTarget> Create(
		__in ISftpProvider* pProvider,
		const boost::filesystem::wpath& remote_path);
	
	CDropTarget();
	~CDropTarget();

	/** @name IDropTarget methods */
	// @{

	IFACEMETHODIMP DragEnter( 
		__in_opt IDataObject* pDataObj,
		__in DWORD grfKeyState,
		__in POINTL pt,
		__inout DWORD* pdwEffect);

	IFACEMETHODIMP DragOver( 
		__in DWORD grfKeyState,
		__in POINTL pt,
		__inout DWORD* pdwEffect);

	IFACEMETHODIMP DragLeave();

	IFACEMETHODIMP Drop( 
		__in_opt IDataObject* pDataObj,
		__in DWORD grfKeyState,
		__in POINTL pt,
		__inout DWORD* pdwEffect);

	// @}

private:

	ATL::CComPtr<ISftpProvider> m_spProvider;
	boost::filesystem::wpath m_remote_path;
	ATL::CComPtr<IDataObject> m_spDataObject;
};

void copy_data_to_provider(
	IDataObject* pdo, ISftpProvider* pProvider, 
	boost::filesystem::wpath remote_path);

}} // namespace swish::shell_folder
