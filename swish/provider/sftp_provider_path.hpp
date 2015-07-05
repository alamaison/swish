/**
    @file

    SFTP backend paths.

    @if license

    Copyright (C) 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    If you modify this Program, or any covered work, by linking or
    combining it with the OpenSSL project's OpenSSL library (or a
    modified version of that library), containing parts covered by the
    terms of the OpenSSL or SSLeay licenses, the licensors of this
    Program grant you additional permission to convey the resulting work.

    @endif
*/

#ifndef SWISH_PROVIDER_SFTP_PROVIDER_PATH_HPP
#define SWISH_PROVIDER_SFTP_PROVIDER_PATH_HPP

#include <boost/filesystem/path.hpp> // path

namespace swish {
namespace provider {

// Currently we just borrow the Boost.Filesystem path, unadulterated.
// Eventually we should adapt it to handle locales and maybe even SFTP
// operations.
typedef boost::filesystem::path sftp_provider_path;

}}

#endif
