/**
    @file

    FILEDESCRIPTORW clipboard format wrapper.

    @if license

    Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    @endif
*/

#pragma once

#include "GlobalLocker.hpp"  // Manage global memory storing FGD

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION
#include <boost/shared_ptr.hpp> // shared_ptr
#pragma warning(push)
#pragma warning(disable:4244) // conversion from uint64_t to uint32_t
#include <boost/date_time/posix_time/posix_time_types.hpp> // ptime
#include <boost/date_time/gregorian/greg_date.hpp> // date
#include <boost/date_time/posix_time/conversion.hpp> // from_ftime
#pragma warning(pop)
#include <boost/numeric/conversion/cast.hpp>  // numeric_cast
#include <boost/system/system_error.hpp> // system_error
#include <boost/static_assert.hpp> //  BOOST_STATIC_ASSERT
#include <boost/cstdint.hpp> // unint64_t

#include <stdexcept> // out_of_range, length_error, logic_error
#include <string>

#include <shlobj.h> // FILEDESCRIPTOR, FILEGROUPDESCRIPTOR
#include <Windows.h> // HGLOBAL, GetLastError

#pragma warning(push)
#pragma warning(disable:4996) // std::copy ... may be unsafe

namespace swish {
namespace shell_folder {
namespace data_object {

namespace {

    inline boost::uint32_t lo_dword(boost::uint64_t qword)
    {
        return static_cast<boost::uint32_t>(qword & 0xFFFFFFFF);
    }

    inline boost::uint32_t hi_dword(boost::uint64_t qword)
    {
        return static_cast<boost::uint32_t>((qword >> 32) & 0xFFFFFFFF);
    }

    /**
     * Convert a ptime to a Windows FILETIME.
     */
    void ptime_to_filetime(const boost::posix_time::ptime& time, FILETIME& ft)
    {
        boost::posix_time::ptime filetime_epoch(
            boost::gregorian::date(1601,1,1));
        boost::posix_time::time_duration duration = time - filetime_epoch;
        boost::uint64_t hundred_nanosecs = duration.total_nanoseconds();
        hundred_nanosecs /= 100UL;
        ft.dwLowDateTime = lo_dword(hundred_nanosecs);
        ft.dwHighDateTime = hi_dword(hundred_nanosecs);
    }

    /**
     * Convert a Windows FILETIME to a ptime.
     */
    boost::posix_time::ptime filetime_to_ptime(const FILETIME& ft)
    {
        return boost::posix_time::from_ftime<boost::posix_time::ptime>(ft);
    }

}

/**
 * Exception thrown when trying to access a field that has not been set to
 * a value.
 */
class field_error : public std::logic_error
{
public:
    explicit field_error(const std::string message) : std::logic_error(message)
    {}
};

/**
 * C++ interface to the FILEDESCRIPTORW structure.
 */
class Descriptor : private FILEDESCRIPTORW
{
public:

    Descriptor() : FILEDESCRIPTORW() {}
    Descriptor(const FILEDESCRIPTORW& d) : FILEDESCRIPTORW(d) {}
    // default copy, assign and destruct OK

    const FILEDESCRIPTORW& get() const
    {
        return *this;
    }

    /**
     * Return the stored filename or relative path.
     */
    std::wstring path() const
    {
        return cFileName;
    }

    /**
     * Save given path as the descriptor filename/path.
     *
     * FGD paths are relative paths using backslashes as separators.  We allow
     * the path argument to use forward slashes, andthey will be converted
     * accordingly.
     */
    void path(std::wstring path)
    {
        std::replace(path.begin(), path.end(), L'/', L'\\');

        static const size_t BUFFER_SIZE =
            sizeof(cFileName) / sizeof(cFileName[0]);

        if (path.size() >= BUFFER_SIZE)
            BOOST_THROW_EXCEPTION(
                std::length_error("Path greater than MAX_PATH"));

        size_t count = path.copy(cFileName, BUFFER_SIZE - 1);
        cFileName[count] = L'\0';
    }

    /**
     * Get the size of the item described by the descriptor.
     *
     * If the corresponding FILECONTENTS format is stored in an HGLOBAL this
     * is also the size of the allocated memory.
     */
    boost::uint64_t file_size() const
    {
        if (!_valid_field(FD_FILESIZE))
            BOOST_THROW_EXCEPTION(field_error("File size not available."));

        boost::uint64_t s = nFileSizeHigh;
        s = s << 32;
        s += nFileSizeLow;
        return s;
    }

    /**
     * Set the size of the item described by the descriptor.
     *
     * If the corresponding FILECONTENTS format is stored in an HGLOBAL this
     * is also the size of the allocated memory.
     */
    void file_size(boost::uint64_t size)
    {
        nFileSizeLow = lo_dword(size);
        nFileSizeHigh = hi_dword(size);
        _set_field_valid(FD_FILESIZE);
    }

    /**
     * The date and time that the item was created.
     */
    boost::posix_time::ptime creation_time() const
    {
        if (!_valid_field(FD_CREATETIME))
            BOOST_THROW_EXCEPTION(field_error("Creation time not available."));

        return filetime_to_ptime(ftCreationTime);
    }

    /**
     * Set the date and time that the item was created.
     */
    void creation_time(const boost::posix_time::ptime& time)
    {
        ptime_to_filetime(time, ftLastWriteTime);
        _set_field_valid(FD_CREATETIME);
    }

    /**
     * The date and time that the item was last accessed.
     */
    boost::posix_time::ptime last_access_time() const
    {
        if (!_valid_field(FD_ACCESSTIME))
            BOOST_THROW_EXCEPTION(
                field_error("Last access time not available."));

        return filetime_to_ptime(ftLastAccessTime);
    }

    /**
     * Set the date and time that the item was last accessed.
     */
    void last_access_time(const boost::posix_time::ptime& time)
    {
        ptime_to_filetime(time, ftLastAccessTime);
        _set_field_valid(FD_ACCESSTIME);
    }

    /**
     * The date and time that the item was last modified.
     */
    boost::posix_time::ptime last_write_time() const
    {
        if (!_valid_field(FD_WRITESTIME))
            BOOST_THROW_EXCEPTION(
                field_error("Last write time not available."));

        return filetime_to_ptime(ftLastWriteTime);
    }

    /**
     * Set the date and time that the item was last modified.
     */
    void last_write_time(const boost::posix_time::ptime& time)
    {
        ptime_to_filetime(time, ftLastWriteTime);
        _set_field_valid(FD_WRITESTIME);
    }

    /**
     * Should shell show progress UI when copying items?
     */
    bool want_progress() const
    {
        return _valid_field(FD_PROGRESSUI);
    }

    /**
     * Set whether the shell should show progress UI when copying items.
     */
    void want_progress(bool show)
    {
        if (show)
            _set_field_valid(FD_PROGRESSUI);
        else
            _unset_field_valid(FD_PROGRESSUI);
    }

    /**
     * FILE_ATTRIBUTE* bit values of item.
     */
    DWORD attributes() const
    {
        if (!_valid_field(FD_ATTRIBUTES))
            BOOST_THROW_EXCEPTION(field_error("Attributes not available."));
        return dwFileAttributes;
    }

    /**
     * Set FILE_ATTRIBUTE* bit values for item.
     */
    void attributes(DWORD attrs)
    {
        dwFileAttributes = attrs;
        _set_field_valid(FD_ATTRIBUTES);
    }

private:

    /**
     * Is the field with the given field flag valid?
     */
    bool _valid_field(DWORD field) const
    {
        return !!(dwFlags & field);
    }

    /**
     * Set the validity of the given field.
     */
    void _set_field_valid(DWORD field)
    {
        dwFlags = dwFlags | field;
    }

    /**
     * Unset the validity of the given field.
     */
    void _unset_field_valid(DWORD field)
    {
        dwFlags = (dwFlags & (~field));
    }
};

BOOST_STATIC_ASSERT(sizeof(Descriptor) == sizeof(FILEDESCRIPTORW));

/**
 * Wrapper around the FILEGROUPDESCRIPTORW structure.
 *
 * This wrapper adds construction as well as access to the FILEDESCRIPTORS
 * contained within it.
 */
class FileGroupDescriptor
{
public:

    /**
     * Create wrapper around an existing FILEGROUPDESCRIPTORW in global memory.
     */
    FileGroupDescriptor(HGLOBAL hglobal) : m_lock(hglobal) {}

    // default copy, assign and destruct OK

    /**
     * Number of FILEDESCRIPTORS in the FILEGROUPDESCRIPTORW.
     */
    size_t size() const
    {
        return m_lock.get()->cItems;
    }

    /**
     * Return a reference to the ith FILEDESCRIPTORW as a Descriptor.
     */
    Descriptor& operator[](size_t i) const
    {
        if (i >= size())
            BOOST_THROW_EXCEPTION(
                std::out_of_range(
                    "Attempt to access FILEDESCRIPTORW out of range"));

        return *static_cast<Descriptor*>(&m_lock.get()->fgd[i]);
    }

private:
    GlobalLocker<FILEGROUPDESCRIPTORW> m_lock;
};

/**
 * Allocate a FILEGROUPDESCRIPTORW in global memory holding the given
 * descriptors.
 *
 * The descriptor are give as a [first, last) range who element type is
 * copyable to FILEDESCRIPTORW.
 *
 * @returns  HGLOBAL handle to the allocated global memory.  Caller must free.
 */
template<typename It>
HGLOBAL group_descriptor_from_range(It first, It last)
{
    size_t count = std::distance(first, last);
    if (count < 1)
        BOOST_THROW_EXCEPTION(
            std::length_error("Range must have at least one descriptor."));

    size_t bytes =
        sizeof(FILEGROUPDESCRIPTORW ) + (count * sizeof(FILEDESCRIPTORW));

    HGLOBAL hglobal = ::GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!hglobal)
        BOOST_THROW_EXCEPTION(
            boost::system::system_error(
                ::GetLastError(), boost::system::get_system_category()));

    try
    {
        GlobalLocker<FILEGROUPDESCRIPTORW> lock(hglobal);
        lock.get()->cItems = boost::numeric_cast<UINT>(count);
        std::copy(first, last, &lock.get()->fgd[0]);
        // last arg above: decay array to stop false +ive by checked iterator
    }
    catch (...)
    {
        ::GlobalFree(hglobal);
        throw;
    }

    return hglobal;
}

}}} // namespace swish::shell_folder::data_object

#pragma warning(pop)
