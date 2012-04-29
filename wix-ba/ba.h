/**
    @file

    Installer bootstrapper application.

    @if license

    Copyright (C) 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_WIX_BA_BA_HPP
#define SWISH_WIX_BA_BA_HPP
#pragma once

#include <Msi.h>
#include "IBootstrapperEngine.h"
#include "IBootstrapperApplication.h"

extern "C" BOOL WINAPI DllMain(
	HMODULE hmodule, DWORD  reason_for_call, LPVOID reserved);

extern "C" HRESULT WINAPI BootstrapperApplicationCreate(
	IBootstrapperEngine* engine, const BOOTSTRAPPER_COMMAND* command,
	IBootstrapperApplication** application_out);

extern "C" void WINAPI BootstrapperApplicationDestroy();

#endif
