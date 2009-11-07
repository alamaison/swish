:: Script register Swish COM components manually.
:: 
:: Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>
:: 
:: This program is free software; you can redistribute it and/or modify
:: it under the terms of the GNU General Public License as published by
:: the Free Software Foundation; either version 2 of the License, or
:: (at your option) any later version.
:: 
:: This program is distributed in the hope that it will be useful,
:: but WITHOUT ANY WARRANTY; without even the implied warranty of
:: MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
:: GNU General Public License for more details.
:: 
:: You should have received a copy of the GNU General Public License along
:: with this program; if not, write to the Free Software Foundation, Inc.,
:: 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

@echo off
setlocal
echo.

cd ..\bin

if exist Debug (
echo -=== Registering DEBUG components
cd Debug
((regsvr32 /s shell_folder-com_dll.dll && echo Registered DEBUG shell_folder.) || echo Failed to register DEBUG shell_folder.)
((regsvr32 /s provider-com_dll.dll && echo Registered DEBUG provider.) || echo Failed to register DEBUG provider.)
cd ..
)

if exist Release (
echo -=== Registering RELEASE components
cd Release
((regsvr32 /s shell_folder-com_dll.dll && echo Registered RELEASE shell_folder.) || echo Failed to register RELEASE shell_folder.)
((regsvr32 /s provider-com_dll.dll && echo Registered RELEASE provider.) || echo Failed to register RELEASE provider.)
cd ..
)

pause
exit /B
