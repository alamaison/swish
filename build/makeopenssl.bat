call "%VS80COMNTOOLS%vsvars32.bat"
cd openssl-1.0.0a

del /Q inc32\*
del /Q tmp32\*
del /Q out32\*

set BUILDPLAT=Win32

mkdir %BUILDPLAT%
del /Q %BUILDPLAT%\*

perl Configure VC-WIN32 no-asm no-shared
call ms\do_ms
nmake -f ms\nt.mak

move inc32 %BUILDPLAT%
move out32\libeay32.lib %BUILDPLAT%
move out32\ssleay32.lib %BUILDPLAT%

call "%VS80COMNTOOLS%..\..\VC\Bin\amd64\vcvarsamd64.bat"

del /Q inc32\*
del /Q tmp32\*
del /Q out32\*

set BUILDPLAT=x64

mkdir %BUILDPLAT%
del /Q %BUILDPLAT%\*

perl Configure VC-WIN64A no-asm no-shared
call ms\do_win64a
nmake -f ms\nt.mak

move inc32 %BUILDPLAT%
move out32\libeay32.lib %BUILDPLAT%
move out32\ssleay32.lib %BUILDPLAT%


pause