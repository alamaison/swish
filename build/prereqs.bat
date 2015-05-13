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
