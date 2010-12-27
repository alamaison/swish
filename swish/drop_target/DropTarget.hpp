/**
    @file

    Expose the remote filesystem as an IDropTarget.

    @if license

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

#ifndef SWISH_DROP_TARGET_DROPTARGET_HPP
#define SWISH_DROP_TARGET_DROPTARGET_HPP
#pragma once

#include <winapi/object_with_site.hpp> // object_with_site

#include <boost/filesystem.hpp>  // wpath
#include <boost/shared_ptr.hpp> // shared_ptr to UI callback

#include <comet/ptr.h> // com_ptr
#include <comet/server.h> // simple_object

#include <memory> // auto_ptr
#include <string>

#include <OleIdl.h> // IDropTarget
#include <OCIdl.h> // IObjectWithSite

struct ISftpProvider;
struct ISftpConsumer;

template<> struct comet::comtype<IDropTarget>
{
	static const IID& uuid() throw() { return IID_IDropTarget; }
	typedef IUnknown base;
};

namespace swish {
namespace drop_target {

class Progress
{
public:
	virtual ~Progress() {}
	virtual bool user_cancelled() const = 0;
	virtual void line(DWORD index, const std::wstring& text) = 0;
	virtual void line_path(
		DWORD index, const boost::filesystem::wpath& path) = 0;
	virtual void update(ULONGLONG so_far, ULONGLONG out_of) = 0;
};

class CopyCallback
{
public:
	virtual ~CopyCallback() {}
	virtual void site(comet::com_ptr<IUnknown> ole_site) = 0;
	virtual bool can_overwrite(const boost::filesystem::wpath& target) = 0;
	virtual std::auto_ptr<Progress> progress() = 0;
};

class CDropTarget :
	public comet::simple_object<IDropTarget, winapi::object_with_site>
{
public:

	typedef IDropTarget interface_is;

	CDropTarget(
		comet::com_ptr<ISftpProvider> provider,
		comet::com_ptr<ISftpConsumer> consumer,
		const boost::filesystem::wpath& remote_path,
		boost::shared_ptr<CopyCallback> callback);

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

	virtual void on_set_site(comet::com_ptr<IUnknown> ole_site);

	comet::com_ptr<ISftpProvider> m_provider;
	comet::com_ptr<ISftpConsumer> m_consumer;
	boost::filesystem::wpath m_remote_path;
	comet::com_ptr<IDataObject> m_data_object;
	boost::shared_ptr<CopyCallback> m_callback;
};

void copy_data_to_provider(
	comet::com_ptr<IDataObject> data_object,
	comet::com_ptr<ISftpProvider> provider,
	comet::com_ptr<ISftpConsumer> consumer,
	const boost::filesystem::wpath& remote_path,
	CopyCallback& callback);

}} // namespace swish::drop_target

#endif
