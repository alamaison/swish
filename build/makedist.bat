@echo off
rem Script to create a Swish source distribution
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

set DIST=dist
set STAGING=dist.temp

cd ..
set WGET=build\wget\wget.exe -N
set SEVENZ=build\7za\7za.exe

rem libssh2

echo ===- Dowloading latest Swish SVN trunk code ...
%WGET% -O swish.tar.gz "http://swish.svn.sourceforge.net/viewvc/swish/trunk.tar.gz?view=tar" || (
	echo ===- Error while trying to download Swish SVN trunk & goto error)
%SEVENZ% x swish.tar.gz -aoa || (
	echo ===- Error while trying to extract Swish SVN trunk & goto error)
%SEVENZ% x swish.tar -aoa || (
	echo ===- Error while trying to extract Swish SVN trunk & goto error)
if exist %STAGING% rd /S /Q %STAGING%
ren trunk %STAGING% || (
	echo ===- Error while trying to create %STAGING% directory & goto error)
del swish.tar
del swish.tar.gz

rem Dowload all prerequs using just checked out prereq scripts

pushd %STAGING%\build
call prereqs.bat "nopause" || (echo ===- Error downloading prerequs & goto error)
call testingprereqs.bat "nopause" || (
	echo ===- Error downloading testing prerequs & goto error)
popd

rem Package code

if not exist %DIST% md %DIST%

pushd %STAGING%
set VERSION=trunk
set PKG_NAME=swish-%VERSION%-src
if exist ..\%DIST%.\%PKG_NAME%.7z del ..\%DIST%.\%PKG_NAME%.7z
..\%SEVENZ% a -t7z ..\%DIST%.\%PKG_NAME%.7z * || (
	echo ===- Error creating .7z package & goto error)
if exist ..\%DIST%.\%PKG_NAME%.zip del ..\%DIST%.\%PKG_NAME%.zip
..\%SEVENZ% a -tzip ..\%DIST%.\%PKG_NAME%.zip * || (
	echo ===- Error creating .zip package & goto error)
popd

rd /S /Q %STAGING% || (echo ===- Error cleaning staging area & goto error)

echo.
echo ===- Distribution successfully created in dist\.
echo.
if [%1]==[] pause
exit /B 0

:error
echo.
if [%1]==[] pause
exit /B 1