/**
    @file

    Expose the remote filesystem as an IDropTarget.

    @if licence

    Copyright (C) 2009, 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "swish/CoFactory.hpp"  // CCoObject factory mixin

#include <boost/filesystem.hpp>  // wpath

#include <comet/ptr.h> // com_ptr

struct ISftpProvider;
struct ISftpConsumer;

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

	static comet::com_ptr<IDropTarget> Create(
		const comet::com_ptr<ISftpProvider>& provider,
		const comet::com_ptr<ISftpConsumer>& consumer,
		const boost::filesystem::wpath& remote_path, bool show_progress=true);
	
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

	comet::com_ptr<ISftpProvider> m_provider;
	comet::com_ptr<ISftpConsumer> m_consumer;
	boost::filesystem::wpath m_remote_path;
	comet::com_ptr<IDataObject> m_data_object;
	bool m_show_progress;
};

void copy_data_to_provider(
	const comet::com_ptr<IDataObject>& data_object,
	const comet::com_ptr<ISftpProvider>& provider,
	const comet::com_ptr<ISftpConsumer>& consumer,
	boost::filesystem::wpath remote_path,
	const comet::com_ptr<IProgressDialog>& progress=NULL);

}} // namespace swish::shell_folder
