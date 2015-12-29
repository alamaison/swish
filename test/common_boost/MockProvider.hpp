/**
    @file

    Mock implementation of swish::provider::sftp_provider.

    @if license

    Copyright (C) 2010, 2012, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef TEST_COMMON_BOOST_MOCK_PROVIDER_HPP
#define TEST_COMMON_BOOST_MOCK_PROVIDER_HPP
#pragma once

#include "test/common_boost/tree.hpp" // tree container for mocking filesystem

#include "swish/provider/sftp_provider.hpp" // sftp_provider

#include <comet/bstr.h> // bstr_t
#include <comet/datetime.h> // datetime_t

#include <boost/bind.hpp>
#include <boost/filesystem.hpp> // path
#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/format.hpp> // wformat
#include <boost/shared_ptr.hpp>
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <functional> // equal_to, less
#include <string>
#include <vector>

#include <Shlwapi.h> // SHCreateMemStream

namespace test {
namespace detail {

    typedef tree<swish::provider::sftp_filesystem_item> Filesystem;
    typedef Filesystem::iterator FilesystemLocation;

    inline bool name_match(
        const std::wstring& name,
        const swish::provider::sftp_filesystem_item& sftp_item)
    {
        return name == sftp_item.filename();
    }

    /**
     * Return an iterator to the node in the mock filesystem indicated by the
     * path given as a string.
     */
    inline FilesystemLocation find_location_from_path(
        const Filesystem& filesystem, const ssh::filesystem::path& path)
    {
        // Start searching in root of 'filesystem'
        FilesystemLocation current_dir = filesystem.begin();

        // Walk down list of tokens finding each item below the previous
        BOOST_FOREACH(ssh::filesystem::path segment, path.relative_path())
        {
            std::wstring name = segment.wstring();

            if (name == L".")
                continue;

            FilesystemLocation dir = find_if(
                filesystem.begin(current_dir), filesystem.end(current_dir),
                boost::bind(&name_match, name, _1));

            if (dir == filesystem.end(current_dir))
            {
                std::string message =
                    str(boost::format("Mock file '%s' not found") % name);
                BOOST_THROW_EXCEPTION(std::exception(message.c_str()));
            }

            current_dir = dir;
        }

        if (current_dir == filesystem.end())
            BOOST_THROW_EXCEPTION(std::exception("Unexpected lookup failure!"));

        return current_dir;
    }

    class mock_filesystem_file :
        public swish::provider::sftp_filesystem_item_interface
    {
    public:
        static swish::provider::sftp_filesystem_item create(
            const std::wstring& name, ULONG permissions, ULONGLONG size,
            comet::datetime_t date)
        {
            return swish::provider::sftp_filesystem_item(
                boost::shared_ptr<swish::provider::sftp_filesystem_item_interface>(
                    new mock_filesystem_file(name, permissions, size, date)));
        }

        BOOST_SCOPED_ENUM(type) type() const
        {
            return type::file;
        }

        ssh::filesystem::path filename() const
        {
            return m_name;
        }

        unsigned long permissions() const
        {
            return m_permissions;
        }

        boost::optional<std::wstring> owner() const
        {
            return L"mockowner";
        }

        unsigned long uid() const
        {
            return 42;
        }

        boost::optional<std::wstring> group() const
        {
            return L"mockgroup";
        }

        unsigned long gid() const
        {
            return 24;
        }

        boost::uint64_t size_in_bytes() const
        {
            return m_size;
        }

        comet::datetime_t last_accessed() const
        {
            return comet::datetime_t();
        }

        comet::datetime_t last_modified() const
        {
            return m_date;
        }

    private:
        mock_filesystem_file(
            const std::wstring& name, ULONG permissions,
            ULONGLONG size, comet::datetime_t date)
            :
        m_name(name), m_permissions(permissions), m_size(size), m_date(date) {}

        std::wstring m_name;
        ULONG m_permissions;
        ULONGLONG m_size;
        comet::datetime_t m_date;
    };

    class mock_filesystem_directory :
        public swish::provider::sftp_filesystem_item_interface
    {
    public:
        static swish::provider::sftp_filesystem_item create(
            const std::wstring& name)
        {
            return swish::provider::sftp_filesystem_item(
                boost::shared_ptr<swish::provider::sftp_filesystem_item_interface>(
                    new mock_filesystem_directory(name)));
        }

        BOOST_SCOPED_ENUM(type) type() const
        {
            return type::directory;
        }

        ssh::filesystem::path filename() const
        {
            return m_name;
        }

        unsigned long permissions() const
        {
            return 040777;
        }

        boost::optional<std::wstring> owner() const
        {
            return L"mockowner";
        }

        unsigned long uid() const
        {
            return 42;
        }

        boost::optional<std::wstring> group() const
        {
            return L"mockgroup";
        }

        unsigned long gid() const
        {
            return 24;
        }

        boost::uint64_t size_in_bytes() const
        {
            return 0U;
        }

        comet::datetime_t last_accessed() const
        {
            return comet::datetime_t();
        }

        comet::datetime_t last_modified() const
        {
            return comet::datetime_t(1601, 10, 5, 13, 54, 22);
        }

    private:
        mock_filesystem_directory(const std::wstring& name) : m_name(name) {}

        std::wstring m_name;
    };

    class mock_filesystem_link :
        public swish::provider::sftp_filesystem_item_interface
    {
    public:
        static swish::provider::sftp_filesystem_item create(
            const std::wstring& name)
        {
            return swish::provider::sftp_filesystem_item(
                boost::shared_ptr<swish::provider::sftp_filesystem_item_interface>(
                    new mock_filesystem_link(name)));
        }

        BOOST_SCOPED_ENUM(type) type() const
        {
            return type::link;
        }

        ssh::filesystem::path filename() const
        {
            return m_name;
        }

        unsigned long permissions() const
        {
            return 040777;
        }

        boost::optional<std::wstring> owner() const
        {
            return L"mockowner";
        }

        unsigned long uid() const
        {
            return 42;
        }

        boost::optional<std::wstring> group() const
        {
            return L"mockgroup";
        }

        unsigned long gid() const
        {
            return 24;
        }

        boost::uint64_t size_in_bytes() const
        {
            return 0U;
        }

        comet::datetime_t last_accessed() const
        {
            return comet::datetime_t();
        }

        comet::datetime_t last_modified() const
        {
            return comet::datetime_t(1601, 10, 5, 13, 54, 22);
        }

    private:
        mock_filesystem_link(const std::wstring& name) : m_name(name) {}

        std::wstring m_name;
    };

    inline std::wstring tag_filename(
        const std::wstring& filename, const ssh::filesystem::path& directory)
    {
        // UNOBVIOUS: converting the path to a string here, rather than passing
        // the path directly to the formatter, because Boost.Filesystem v3,
        // surprisingly, quotes all paths that are sent to an output stream.
        return str(boost::wformat(filename) % directory.filename().wstring());
    }

    inline void make_item_in(
        Filesystem& filesystem, FilesystemLocation loc,
        const swish::provider::sftp_filesystem_item& item)
    {
        filesystem.append_child(loc, item);
    }

    inline void make_item_in(
        Filesystem& filesystem, const ssh::filesystem::path& path,
        const swish::provider::sftp_filesystem_item& item)
    {
        make_item_in(
            filesystem, find_location_from_path(filesystem, path), item);
    }

    /**
     * Generates a listing for the given directory and tags each filename with
     * the name of the parent folder.  This allows us to detect a correct
     * listing later.
     */
    inline void fill_mock_listing(
        Filesystem& filesystem, const ssh::filesystem::path& directory)
    {
        std::vector<std::wstring> filenames;
        filenames.push_back(tag_filename(L"test%sfile", directory));
        filenames.push_back(tag_filename(L"test%sFile", directory));
        filenames.push_back(tag_filename(L"test%sfile.ext", directory));
        filenames.push_back(tag_filename(L"test%sfile.txt", directory));
        filenames.push_back(tag_filename(L"test%sfile with spaces", directory));
        filenames.push_back(
            tag_filename(L"test%sfile with \"quotes\" and spaces", directory));
        filenames.push_back(tag_filename(L"test%sfile.ext.txt", directory));
        filenames.push_back(tag_filename(L"test%sfile..", directory));
        filenames.push_back(tag_filename(L".test%shiddenfile", directory));

        std::vector<comet::datetime_t> dates;
        dates.push_back(comet::datetime_t());
        dates.push_back(comet::datetime_t::now());
        dates.push_back(comet::datetime_t(1899, 7, 13, 17, 59, 12));
        dates.push_back(comet::datetime_t(9999, 12, 31, 23, 59, 59));
        dates.push_back(comet::datetime_t(2000, 2, 29, 12, 47, 1));
        dates.push_back(comet::datetime_t(1978, 3, 3, 3, 00, 00));
        dates.push_back(comet::datetime_t(1601, 1, 1, 0, 00, 00));
        dates.push_back(comet::datetime_t(2007, 2, 28, 0, 0, 0));
        dates.push_back(comet::datetime_t(1752, 9, 03, 7, 27, 8));

        unsigned long cycle = 0;
        unsigned long size = 0;
        while (!filenames.empty())
        {
            // Try to cycle through the permissions on each successive file
            // TODO: I have no idea if this works
            unsigned permissions =
                (cycle % 1) || ((cycle % 2) << 1) || ((cycle % 3) << 2);

            make_item_in(
                filesystem, directory,
                mock_filesystem_file::create(
                    filenames.back(), permissions, size, dates.back()));

            dates.pop_back();
            filenames.pop_back();
            cycle++;
            size = (size + cycle) << 10;
        }

        // Add some dummy folders also
        std::vector<std::wstring> folder_names;
        folder_names.push_back(tag_filename(L"Test%sfolder", directory));
        folder_names.push_back(tag_filename(L"test%sfolder.ext", directory));
        folder_names.push_back(tag_filename(L"test%sfolder.bmp", directory));
        folder_names.push_back(
            tag_filename(L"test%sfolder with spaces", directory));
        folder_names.push_back(tag_filename(L".test%shiddenfolder", directory));

        while (!folder_names.empty())
        {
            make_item_in(
                filesystem, directory,
                mock_filesystem_directory::create(folder_names.back()));
            folder_names.pop_back();
        }

        // Last but not least, links
        std::vector<std::wstring> link_names;
        link_names.push_back(tag_filename(L"link%sfolder", directory));
        link_names.push_back(tag_filename(L"another link%sfolder", directory));
        link_names.push_back(tag_filename(L"p%s", directory));
        link_names.push_back(tag_filename(L".q%s", directory));
        link_names.push_back(tag_filename(L"this_link_is_broken_%s", directory));

        while (!link_names.empty())
        {
            make_item_in(
                filesystem, directory,
                mock_filesystem_link::create(link_names.back()));
            link_names.pop_back();
        }
    }

    struct comparator
    {
        bool operator()(
            const swish::provider::sftp_filesystem_item& left,
            const swish::provider::sftp_filesystem_item& right)
        {
            return left.filename() < right.filename();
        }
    };

}

class MockProvider : public swish::provider::sftp_provider
{
public:

    /**
    * Possible behaviours of listing returned by mock GetListing() method.
    */
    typedef enum tagListingBehaviour {
        MockListing,     ///< Return a dummy list of files and S_OK.
        EmptyListing,    ///< Return an empty list and S_OK.
        SFalseNoListing, ///< Return a NULL listing and S_FALSE.
        AbortListing,    ///< Return a NULL listing E_ABORT.
        FailListing      ///< Return a NULL listing E_FAIL.
    } ListingBehaviour;

    /**
    * Possible behaviours of mock Rename() method.
    */
    typedef enum tagRenameBehaviour {
        RenameOK,           ///< Return S_OK - rename unconditionally succeeded.
        ConfirmOverwrite,   ///< Call ISftpConsumer's OnConfirmOverwrite and
        ///< return its return value.
        AbortRename,        ///< Return E_ABORT.
        FailRename          ///< Return E_FAIL.
    } RenameBehaviour;

    MockProvider() :
        m_listing_behaviour(MockListing), m_rename_behaviour(RenameOK)
    {
        // Create filesystem root
        detail::FilesystemLocation root = m_filesystem.insert(
            m_filesystem.begin(),
            detail::mock_filesystem_directory::create(L"/"));

        // Create two subdirectories and fill them with an expected set of items
        // whose names are 'tagged' with the directory name
        detail::FilesystemLocation tmp =
            m_filesystem.append_child(
                root, detail::mock_filesystem_directory::create(L"tmp"));
        detail::FilesystemLocation swish =
            m_filesystem.append_child(
                tmp, detail::mock_filesystem_directory::create(L"swish"));
        detail::fill_mock_listing(m_filesystem, L"/tmp");
        detail::fill_mock_listing(m_filesystem, L"/tmp/swish");
    }

    ~MockProvider()
    {
        m_filesystem.clear();
    }

    void set_listing_behaviour(ListingBehaviour behaviour)
    {
        m_listing_behaviour = behaviour;
    }

    void set_rename_behaviour(RenameBehaviour behaviour)
    {
        m_rename_behaviour = behaviour;
    }

    virtual swish::provider::directory_listing listing(
        const ssh::filesystem::path& directory)
    {
        std::vector<swish::provider::sftp_filesystem_item> files;

        switch (m_listing_behaviour)
        {
        case EmptyListing:
            break;

        case MockListing:
            {
                detail::FilesystemLocation dir =
                    detail::find_location_from_path(m_filesystem, directory);

                // Copy directory out of tree and sort alphabetically
                files.insert(
                    files.begin(), m_filesystem.begin(dir),
                    m_filesystem.end(dir));

                std::sort(files.begin(), files.end(), detail::comparator());
            }
            break;

        case SFalseNoListing:
            BOOST_THROW_EXCEPTION(comet::com_error(S_FALSE));

        case AbortListing:
            BOOST_THROW_EXCEPTION(comet::com_error(E_ABORT));

        case FailListing:
            BOOST_THROW_EXCEPTION(comet::com_error(E_FAIL));

        default:
            BOOST_THROW_EXCEPTION(comet::com_error(
                "Unreachable: Unrecognised mock behaviour", E_UNEXPECTED));
        }

        return files;
    }

    virtual comet::com_ptr<IStream> get_file(
        const ssh::filesystem::path& file_path, std::ios_base::openmode /*mode*/)
    {
        detail::find_location_from_path(
            m_filesystem, file_path); // test existence

        // Create IStream instance whose data is the file path
        return ::SHCreateMemStream(
            reinterpret_cast<const BYTE*>(file_path.wstring().c_str()),
            static_cast<UINT>((file_path.wstring().size() + 1) * sizeof(wchar_t)));
    }

    virtual VARIANT_BOOL rename(
        ISftpConsumer* consumer, const ssh::filesystem::path& from_path,
        const ssh::filesystem::path& to_path)
    {
        detail::find_location_from_path(
            m_filesystem, from_path); // test existence

        switch (m_rename_behaviour)
        {
        case RenameOK:
            return VARIANT_FALSE;

        case ConfirmOverwrite:
            {
                HRESULT hr = consumer->OnConfirmOverwrite(
                        comet::bstr_t(from_path).in(),
                        comet::bstr_t(to_path).in());
                if (SUCCEEDED(hr))
                    return VARIANT_TRUE;
                BOOST_THROW_EXCEPTION(
                    comet::com_error_from_interface<ISftpConsumer>(
                        consumer, hr));
            }

        case AbortRename:
            BOOST_THROW_EXCEPTION(comet::com_error(E_ABORT));

        case FailRename:
            BOOST_THROW_EXCEPTION(comet::com_error(E_FAIL));

        default:
            BOOST_THROW_EXCEPTION(comet::com_error(
                "Unreachable: Unrecognised mock behaviour", E_UNEXPECTED));
        }
    }

    virtual void remove_all(const ssh::filesystem::path& /*path*/)
    {};

    virtual void create_new_directory(const ssh::filesystem::path& /*path*/)
    {};

    virtual ssh::filesystem::path resolve_link(
        const ssh::filesystem::path& path)
    {
        std::wstring p(path.wstring());

        // link names with 'broken' in their name we pretend to resolve to
        // a target that doesn't exist
        if (p.find(L"broken") != std::wstring::npos)
            return ssh::filesystem::path(L"/tmp/broken_link_target");

        // link names with 'folder' in their name we pretend target a directory
        // (/tmp/testtmpfolder) and the others we target at a file
        // (/tmp/testfile)
        else if (p.find(L"folder") != std::wstring::npos)
            return ssh::filesystem::path(L"/tmp/Testtmpfolder");
        else
            return ssh::filesystem::path(L"/tmp/testtmpfile");
    };

    virtual swish::provider::sftp_filesystem_item stat(
        const ssh::filesystem::path& path, bool follow_links)
    {
        ssh::filesystem::path target;
        if (follow_links)
        {
            target = resolve_link(path);
        }
        else
        {
            target = path;
        }

        detail::FilesystemLocation dir =
            detail::find_location_from_path(m_filesystem, target);

        return *dir;
    }

private:

    detail::Filesystem m_filesystem;
    ListingBehaviour m_listing_behaviour;
    RenameBehaviour m_rename_behaviour;
};

} // namespace test

#endif
