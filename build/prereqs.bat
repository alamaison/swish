:: Script to fetch Swish build prerequisites
::
:: Copyright (C) 2010, 2011, 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>
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

set WGET="%~dp0\wget\wget.exe" -N
echo Using wget at : %WGET%
set SEVENZ="%~dp0\7za\7za.exe"
echo using 7zip at %SEVENZ%

cd ..

:: Git submodules: libssh2, winapi, comet

echo ===- Initialising GIT submodules ...
call git submodule init || (
	echo ===- Error while Initialising GIT submodules & goto error)

echo ===- Cloning GIT submodules ...
call git submodule update || (
	echo ===- Error while Initialising GIT submodules & goto error)

cd thirdparty

:: OpenSSL

echo.
echo ===- Downloading OpenSSL ...
%WGET% "http://sourceforge.net/projects/swish/files/openssl-swish/openssl-1.0.0l-swish/openssl-1.0.0l-swish.7z/download" || (
	echo ===- Error while trying to download OpenSSL. & goto error)
%SEVENZ% x openssl-1.0.0l-swish.7z -oopenssl -aoa || (
	echo ===- Error while trying to extract OpenSSL. & goto error)
del openssl-1.0.0l-swish.7z

:: WTL

echo.
echo ===- Downloading WTL ...

%WGET% "http://downloads.sourceforge.net/project/wtl/WTL%%208.1/WTL%%208.1.9127/WTL81_9127.zip" || (
	echo ===- Error while trying to download WTL. & goto error)
%SEVENZ% x WTL81_9127.zip -owtl -aoa || (
	echo ===- Error while trying to extract WTL. & goto error)
del WTL81_9127.zip

:: Winsparkle

echo.
echo ===- Downloading Winsparkle ...
%WGET% "http://sourceforge.net/projects/swish/files/winsparkle-swish/winsparkle-4477633.7z/download" || (
	echo ===- Error while trying to download Winsparkle. & goto error)
%SEVENZ% x winsparkle-4477633.7z -owinsparkle -aoa || (
	echo ===- Error while trying to extract Winsparkle. & goto error)
del winsparkle-4477633.7z

:: Pageant

echo.
echo ===- Downloading Pageant ...
md putty
cd putty
%WGET% "http://the.earth.li/~sgtatham/putty/latest/x86/pageant.exe" || (
	echo ===- Error while trying to download Pageant. & goto error)
cd ..

echo.
echo ===- All build prerequisites successfully created.
echo.
if [%1]==[] pause
exit /B 0

:error
echo.
if [%1]==[] pause
exit /B 1