:: Script to fetch Swish build prerequisites
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

cd ..\thirdparty

:: libssh2

echo ===- Dowloading libssh2 ...
cvs -z3 -d:pserver:anonymous:@libssh2.cvs.sourceforge.net:/cvsroot/libssh2 co -r RELEASE_1_0 libssh2 || (
	echo ===- Error while trying to download libssh2 & goto error)

:: zlib

echo.
echo ===- Downloading zlib ...
wget "http://prdownloads.sourceforge.net/libpng/zlib123-dll.zip?download" || (
	echo ===- Error while trying to download zlib. & goto error)
unzip -d zlib zlib123-dll.zip || (
	echo ===- Error while trying to extract zlib. & goto error)
del zlib123-dll.zip

:: OpenSSL

echo.
echo ===- Downloading OpenSSL ...
wget "http://downloads.sourceforge.net/swish/openssl-0.9.8g-swish.zip?download" || (
	echo ===- Error while trying to download OpenSSL. & goto error)
unzip -d openssl openssl-0.9.8g-swish.zip || (
	echo ===- Error while trying to extract OpenSSL. & goto error)
del openssl-0.9.8g-swish.zip

:: WTL

echo.
echo ===- Downloading WTL ...
wget "http://downloads.sourceforge.net/wtl/WTL80_7161_Final.zip?download" || (
	echo ===- Error while trying to download WTL. & goto error)
unzip -d wtl WTL80_7161_Final.zip || (
	echo ===- Error while trying to extract WTL. & goto error)
del WTL80_7161_Final.zip

echo.
echo ===- All build prerequisites successfully created.
echo.
pause
exit /B

:error
echo.
pause
exit /B 1