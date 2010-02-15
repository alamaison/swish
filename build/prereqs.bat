:: Script to fetch Swish build prerequisites
:: 
:: Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>
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
set WGET=..\build\wget\wget.exe -N
set SEVENZ=..\build\7za\7za.exe

:: libssh2

echo ===- Dowloading libssh2 ...
%WGET% "http://www.libssh2.org/download/libssh2-1.2.4.tar.gz" || (
	echo ===- Error while trying to download libssh2 & goto error)
%SEVENZ% x libssh2-1.2.4.tar.gz -aoa || (
	echo ===- Error while trying to extract libssh2 & goto error)
%SEVENZ% x libssh2-1.2.4.tar -aoa || (
	echo ===- Error while trying to extract libssh2 & goto error)
xcopy /E /Q /Y libssh2-1.2.4 libssh2 || (
	echo ===- Error while trying to copy libssh2 files & goto error)
rd /S /Q libssh2-1.2.4 || (
	echo ===- Error while trying to clean up libssh2 files & goto error)
del libssh2-1.2.4.tar
del libssh2-1.2.4.tar.gz

:: zlib

echo.
echo ===- Downloading zlib ...
%WGET% "http://prdownloads.sourceforge.net/libpng/zlib123-dll.zip?download" || (
	echo ===- Error while trying to download zlib. & goto error)
%SEVENZ% x zlib123-dll.zip -ozlib -aoa || (
	echo ===- Error while trying to extract zlib. & goto error)
del zlib123-dll.zip

:: OpenSSL

echo.
echo ===- Downloading OpenSSL ...
%WGET% "http://downloads.sourceforge.net/swish/openssl-0.9.8g-swish.zip?download" || (
	echo ===- Error while trying to download OpenSSL. & goto error)
%SEVENZ% x openssl-0.9.8g-swish.zip -oopenssl -aoa || (
	echo ===- Error while trying to extract OpenSSL. & goto error)
del openssl-0.9.8g-swish.zip

:: WTL

echo.
echo ===- Downloading WTL ...
%WGET% "http://downloads.sourceforge.net/wtl/WTL80_7161_Final.zip?download" || (
	echo ===- Error while trying to download WTL. & goto error)
%SEVENZ% x WTL80_7161_Final.zip -owtl -aoa || (
	echo ===- Error while trying to extract WTL. & goto error)
del WTL80_7161_Final.zip

:: comet

echo.
echo ===- Downloading comet ...
%WGET% "http://bitbucket.org/alamaison/swish_comet/get/98baa6c53f89.zip" || (
	echo ===- Error while trying to download comet. & goto error)
%SEVENZ% x 98baa6c53f89.zip -aoa || (
	echo ===- Error while trying to extract comet. & goto error)
xcopy /E /Q /Y swish_comet comet || (
	echo ===- Error while trying to copy comet files & goto error)
rd /S /Q swish_comet || (
	echo ===- Error while trying to clean up comet files & goto error)
del 98baa6c53f89.zip

echo.
echo ===- All build prerequisites successfully created.
echo.
if [%1]==[] pause
pause
exit /B 0

:error
echo.
if [%1]==[] pause
exit /B 1