Swish
=====

What is Swish?
--------------

Swish is a plugin for Microsoft Windows Explorer that adds support for SFTP.

If you've used Explorer's built-in FTP support, Swish is that but for SFTP
over SSH.

Supported Operating Systems
---------------------------
  * Windows 8     (most tested)
  * Windows 7     (occasionally tested)
  * Windows Vista (rarely tested)

Swish may also work on Windows XP, but we haven't tested that in a while.

Binaries
--------
~~Binary installers are [on our website](http://www.swish-sftp.org).~~ (Currently down.)

Getting involved
----------------
We welcome patches to help improve Swish.

Swish fetches almost all its dependencies when you configure it with [CMake].
That magic happens thanks to the [Hunter package manager].  However, you will
need Perl installed. [Strawberry Perl] is good, and available on [Chocolatey].

You'll also need a compiler (obviously), a recent version of the Windows SDK
and CMake.

[CMake]:                  https://cmake.org/
[Hunter package manager]: https://github.com/ruslo/hunter/
[Strawberry Perl]:        http://strawberryperl.com/
[Chocolatey]:             https://chocolatey.org/packages/StrawberryPerl

Licensing
---------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

If you modify this Program, or any covered work, by linking or
combining it with the OpenSSL project's OpenSSL library (or a modified
version of that library), containing parts covered by the terms of the
OpenSSL or SSLeay licenses, the licensors of this Program grant you
additional permission to convey the resulting work.

### Why have an exception for OpenSSL?

The [OpenSSL] library is incompatible with the GPL license because it
contains an advertising clause.  However lots of useful, open source
software (including our own projects) need to use it and currently the
alternatives aren't quite up to scratch.  As we want these projects to
be able to reuse Washer, we have added this exception to the GPL - a
common technique used by other projects such as [wget].

If [GnuTLS] improves to the point where OpenSSL is no longer
necessary, we may remove this exception.

[OpenSSL]: http://www.openssl.org/
[wget]:    http://www.gnu.org/software/wget/
[GnuTLS]:  http://www.gnu.org/software/gnutls/
