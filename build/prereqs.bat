:: Script to fetch Swish build prerequisites
:: 
:: Copyright (C) 2010, 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>
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
%WGET% -O libssh2.tar.gz "http://git.stuge.se/?p=libssh2.git;a=snapshot;h=c87a48ae4c21e999444cdcfa09c80ed643235f67;sf=tgz" || (
	echo ===- Error while trying to download libssh2 & goto error)
%SEVENZ% x libssh2.tar.gz -aoa || (
	echo ===- Error while trying to extract libssh2 & goto error)
%SEVENZ% x libssh2.tar -aoa || (
	echo ===- Error while trying to extract libssh2 & goto error)
xcopy /E /Q /Y libssh2-c87a48a libssh2 || (
	echo ===- Error while trying to copy libssh2 files & goto error)
rd /S /Q libssh2-c87a48a || (
	echo ===- Error while trying to clean up libssh2 files & goto error)
del pax_global_header
del libssh2.tar
del libssh2.tar.gz

:: zlib

echo.
echo ===- Downloading zlib ...
%WGET% "http://prdownloads.sourceforge.net/libpng/zlib125.zip?download" || (
	echo ===- Error while trying to download zlib. & goto error)
%SEVENZ% x zlib125.zip -ozlib -aoa || (
	echo ===- Error while trying to extract zlib. & goto error)
xcopy /E /Q /Y zlib\zlib-1.2.5 zlib || (
	echo ===- Error while trying to copy zlib files & goto error)
rd /S /Q zlib\zlib-1.2.5 || (
	echo ===- Error while trying to clean up zlib files & goto error)
del zlib125.zip

:: OpenSSL

echo.
echo ===- Downloading OpenSSL ...
%WGET% "http://sourceforge.net/projects/swish/files/openssl-swish/openssl-1.0.0a-swish/openssl-1.0.0a-swish.zip/download" || (
	echo ===- Error while trying to download OpenSSL. & goto error)
%SEVENZ% x openssl-1.0.0a-swish.zip -oopenssl -aoa || (
	echo ===- Error while trying to extract OpenSSL. & goto error)
del openssl-1.0.0a-swish.zip

:: WTL

echo.
echo ===- Downloading WTL ...

%WGET% "http://downloads.sourceforge.net/project/wtl/WTL%%208.1/WTL%%208.1.9127/WTL81_9127.zip" || (
	echo ===- Error while trying to download WTL. & goto error)
%SEVENZ% x WTL81_9127.zip -owtl -aoa || (
	echo ===- Error while trying to extract WTL. & goto error)
del WTL81_9127.zip

:: comet

echo.
echo ===- Downloading comet ...
%WGET% --no-check-certificate "http://bitbucket.org/alamaison/swish_comet/get/bd51035cab59.zip" || (
	echo ===- Error while trying to download comet. & goto error)
%SEVENZ% x bd51035cab59.zip -aoa || (
	echo ===- Error while trying to extract comet. & goto error)
xcopy /E /Q /Y alamaison-swish_comet-bd51035cab59 comet || (
	echo ===- Error while trying to copy comet files & goto error)
rd /S /Q alamaison-swish_comet-bd51035cab59 || (
	echo ===- Error while trying to clean up comet files & goto error)
del bd51035cab59.zip

:: Boost.Locale

echo.
echo ===- Dowloading Boost.Locale ...
%WGET% -O boost_locale.tar.bz2 "http://downloads.sourceforge.net/project/cppcms/boost_locale/boost_locale_v2.92.tar.bz2" || (
	echo ===- Error while trying to download Boost.Locale & goto error)
%SEVENZ% x boost_locale.tar.bz2 -aoa || (
	echo ===- Error while trying to extract Boost.Locale & goto error)
%SEVENZ% x boost_locale.tar -aoa || (
	echo ===- Error while trying to extract Boost.Locale & goto error)
xcopy /E /Q /Y boost_locale_v2.92 boost.locale || (
	echo ===- Error while trying to copy Boost.Locale files & goto error)
rd /S /Q boost_locale_v2.92 || (
	echo ===- Error while trying to clean up Boost.Locale files & goto error)
del boost_locale.tar.bz2
del boost_locale.tar

:: Winsparkle

echo.
echo ===- Downloading Winsparkle ...
%WGET% "http://sourceforge.net/projects/swish/files/winsparkle-swish/winsparkle.zip/download" || (
	echo ===- Error while trying to download Winsparkle. & goto error)
%SEVENZ% x winsparkle.zip -owinsparkle -aoa || (
	echo ===- Error while trying to extract Winsparkle. & goto error)
del winsparkle.zip

echo.
echo ===- All build prerequisites successfully created.
echo.
if [%1]==[] pause
exit /B 0

:error
echo.
if [%1]==[] pause
exit /B 1