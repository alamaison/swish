call "%VS80COMNTOOLS%vsvars32.bat"
cd openssl-1.0.0l

del /Q inc32\*
del /Q tmp32\*
del /Q tmp32dll\*
del /Q out32\*
del /Q out32dll\*

set BUILDPLAT=Win32

mkdir %BUILDPLAT%
del /Q %BUILDPLAT%\*

perl Configure VC-WIN32 no-asm
call ms\do_ms
nmake -f ms\ntdll.mak

move inc32 %BUILDPLAT%
move out32dll\libeay32.lib %BUILDPLAT%
move out32dll\libeay32.dll %BUILDPLAT%
move out32dll\libeay32.dll.manifest %BUILDPLAT%
move out32dll\ssleay32.lib %BUILDPLAT%
move out32dll\ssleay32.dll %BUILDPLAT%
move out32dll\ssleay32.dll.manifest %BUILDPLAT%

call "%VS80COMNTOOLS%..\..\VC\Bin\amd64\vcvarsamd64.bat"

del /Q inc32\*
del /Q tmp32\*
del /Q tmp32dll\*
del /Q out32\*
del /Q out32dll\*

set BUILDPLAT=x64

mkdir %BUILDPLAT%
del /Q %BUILDPLAT%\*

perl Configure VC-WIN64A no-asm
call ms\do_win64a
nmake -f ms\ntdll.mak

move inc32 %BUILDPLAT%
move out32dll\libeay32.lib %BUILDPLAT%
move out32dll\libeay32.dll %BUILDPLAT%
move out32dll\libeay32.dll.manifest %BUILDPLAT%
move out32dll\ssleay32.lib %BUILDPLAT%
move out32dll\ssleay32.dll %BUILDPLAT%
move out32dll\ssleay32.dll.manifest %BUILDPLAT%


pause