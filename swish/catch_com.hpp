/**
    @file

    COM Exception handler.

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

    In addition, as a special exception, the the copyright holders give you
    permission to combine this program with free software programs or the 
    OpenSSL project's "OpenSSL" library (or with modified versions of it, 
    with unchanged license). You may copy and distribute such a system 
    following the terms of the GNU GPL for this program and the licenses 
    of the other code concerned. The GNU General Public License gives 
    permission to release a modified version without this exception; this 
    exception also makes it possible to release a modified version which 
    carries forward this exception.

    @endif
*/

#pragma once

#include "atl.hpp"        // For CAtlException, com_exception

#include <ComDef.h>       // For _com_error

#include <exception>      // For std::exception and std::bad_alloc

#ifdef _DEBUG

#pragma warning(push)
#pragma warning(disable:4996) // strerror unsafe
#include <boost/exception/all.hpp>
#pragma warning(pop)

#include <string>

template<typename T>
inline void trace_diagnostic_info(const T& e)
{
	std::string info = boost::diagnostic_information(e);
	if (!info.empty())
		ATLTRACE("%s", info.c_str());
}

#define catchCom()                  \
catch (const _com_error& e)         \
{                                   \
	ATLTRACE("Caught _com_error: %ws\n", e.ErrorMessage()); \
	return e.Error();               \
}                                   \
catch (const swish::exception::com_exception& e) \
{                                   \
	ATLTRACE("Caught com_exception %s\n", e.what()); \
	trace_diagnostic_info(e);       \
	return e;                       \
}                                   \
catch (const std::bad_alloc& e)       \
{                                   \
	ATLTRACE("Caught std::bad_alloc\n"); \
	trace_diagnostic_info(e);       \
	return E_OUTOFMEMORY;           \
}                                   \
catch (const std::exception& e)     \
{                                   \
	ATLTRACE("Caught std::exception: %hs\n", e.what()); \
	trace_diagnostic_info(e);       \
	return E_UNEXPECTED;            \
}                                   \
catch (const ATL::CAtlException& e) \
{                                   \
	ATLTRACE("Caught CAtlException: %ws\n", _com_error(e).ErrorMessage()); \
	return e;                       \
}
#else
#define catchCom()                  \
catch (const _com_error& e)         \
{                                   \
	ATLTRACE("Caught _com_error: %ws\n", e.ErrorMessage()); \
	return e.Error();               \
}                                   \
catch (const swish::exception::com_exception& e) \
{                                   \
	return e;                       \
}                                   \
catch (const std::bad_alloc&)       \
{                                   \
	return E_OUTOFMEMORY;           \
}                                   \
catch (const std::exception&)       \
{                                   \
	return E_UNEXPECTED;            \
}                                   \
catch (const ATL::CAtlException& e) \
{                                   \
	return e;                       \
}                                   \
catch (...)                         \
{                                   \
	ATLASSERT(!"Caught unknown exception at COM boundary"); \
	return E_UNEXPECTED;            \
}
#endif // _DEBUG
