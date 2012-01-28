@echo off
rem Script to build Boost in 32 and 64 bit variants as needed by Swish
rem 
rem Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>
rem 
rem This program is free software; you can redistribute it and/or modify
rem it under the terms of the GNU General Public License as published by
rem the Free Software Foundation; either version 2 of the License, or
rem (at your option) any later version.
rem 
rem This program is distributed in the hope that it will be useful,
rem but WITHOUT ANY WARRANTY; without even the implied warranty of
rem MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
rem GNU General Public License for more details.
rem 
rem You should have received a copy of the GNU General Public License along
rem with this program; if not, write to the Free Software Foundation, Inc.,
rem 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

setlocal
echo.

cd ..\thirdparty\boost

call "%VS80COMNTOOLS%\vsvars32.bat"

call bootstrap

set WITH_LIBRARIES=--with-date_time --with-filesystem --with-regex --with-signals --with-system --with-test
set COMMON_ARGS=--toolset=msvc-8.0 --link=static %WITH_LIBRARIES%

bjam %COMMON_ARGS% address-model=32 --stagedir=lib\Win32 stage
bjam %COMMON_ARGS% address-model=64 --stagedir=lib\x64 stage

pause
