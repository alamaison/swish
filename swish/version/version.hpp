/**
    @file

    Swish version information.

    @if license

    Copyright (C) 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_VERSION_HPP
#define SWISH_VERSION_HPP

#include <memory> // auto_ptr
#include <string>

namespace swish {

/**
 * Description of the version control snapshot from which the code was build.
 *
 * The description may be quite rough as there is no good way to describe
 * changes that occur in the working copy. 
 *
 * Currently the description is the output of
 *   `git describe --abbrev=4 --dirty --always`
 * and therefore looks similar to
 *   `swish-0.7.2-1-g5227-dirty`.
 * This format should not be assumed.
 */
std::string snapshot_version();

/**
 * The time of the last build.
 *
 * Technically, the time the compilation unit implementing this function was
 * compiled.
 */
std::string build_time();

/**
 * The date of the last build.
 *
 * Technically, the date on which the compilation unit implementing this 
 * function was compiled.
 */
std::string build_date();


class structured_version_impl
{
public:

    virtual ~structured_version_impl() {}

    virtual int major() const = 0;
    virtual int minor() const = 0;
    virtual int bugfix() const = 0;

    virtual std::string as_string() const = 0;

    virtual std::auto_ptr<structured_version_impl> clone() const = 0;
};

class structured_version
{
public:

    explicit structured_version(const structured_version_impl& impl);

    structured_version(const structured_version& other);
    structured_version& operator=(structured_version other);

    int major() const;
    int minor() const;
    int bugfix() const;

    std::string as_string() const;

    friend void swap(structured_version& l, structured_version& r)
    {
        swap(l.m_pimpl, r.m_pimpl);
    }

private:
    std::auto_ptr<structured_version_impl> m_pimpl;
};

structured_version release_version();

}

#endif
