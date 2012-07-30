/**
    @file

    File copy operation.

    @if license

    Copyright (C) 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "CopyFileOperation.hpp"

#include "swish/remote_folder/remote_pidl.hpp" // create_remote_itemid
#include "swish/shell_folder/SftpDirectory.h" // CSftpDirectory

#include <winapi/shell/shell.hpp> // stream_from_pidl
#include <winapi/trace.hpp> // trace

#include <comet/datetime.h> // datetime_t
#include <comet/error.h> // com_error

#include <boost/cstdint.hpp> // int64_t
#include <boost/locale/message.hpp> // translate
#include <boost/locale/format.hpp> // wformat
#include <boost/shared_ptr.hpp>  // shared_ptr
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <exception>
#include <iosfwd> // wstringstream
#include <utility> // pair

using swish::remote_folder::create_remote_itemid;

using winapi::shell::pidl::apidl_t;
using winapi::shell::pidl::cpidl_t;
using winapi::shell::pidl::pidl_t;
using winapi::shell::stream_from_pidl;
using winapi::trace;

using boost::int64_t;
using boost::filesystem::wpath;
using boost::function;
using boost::locale::translate;
using boost::locale::wformat;
using boost::shared_ptr;

using comet::com_error;
using comet::com_ptr;
using comet::datetime_t;

using std::exception;
using std::make_pair;
using std::pair;
using std::wstringstream;

namespace swish {
namespace drop_target {

namespace {

    const size_t COPY_CHUNK_SIZE = 1024 * 32;

    /**
     * Duplicated stat_stream in DropTarget.cpp.
     */
    pair<wpath, int64_t> stat_stream(const com_ptr<IStream>& stream)
    {
        STATSTG statstg;
        HRESULT hr = stream->Stat(&statstg, STATFLAG_DEFAULT);
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error_from_interface(stream, hr));

        shared_ptr<OLECHAR> name(statstg.pwcsName, ::CoTaskMemFree);
        return make_pair(name.get(), statstg.cbSize.QuadPart);
    }

    /**
     * Return size of the streamed object in bytes.
     */
    int64_t size_of_stream(const com_ptr<IStream>& stream)
    {
        return stat_stream(stream).second;
    }

    /**
     * Write a stream to the provider at the given path.
     *
     * If it already exists, we want to ask the user for confirmation.
     * The poor-mans way of checking if the file is already there is to
     * try to get the file read-only first.  If this fails, assume the
     * file noes not already exist.
     *
     * @bug  The get may have failed for a different reason or this
     *       may not work reliably on all SFTP servers.  A safer
     *       solution would be an explicit stat on the file.
     *
     * @bug  Of course, there is a race condition here.  After we check if the
     *       file exists, someone else may have created it.  Unfortunately,
     *       there is nothing we can do about this as SFTP doesn't give us
     *       a way to do this atomically such as locking a file.
     */
    void copy_stream_to_remote_destination(
        com_ptr<IStream> local_stream, com_ptr<ISftpProvider> provider,
        com_ptr<ISftpConsumer> consumer, const resolved_destination& target,
        OperationCallback& callback)
    {
        CSftpDirectory sftp_directory(target.directory(), provider, consumer);

        cpidl_t file = create_remote_itemid(
            target.filename(), false, false, L"", L"", 0, 0, 0, 0,
            datetime_t::now(), datetime_t::now());

        if (sftp_directory.exists(file))
        {
            bool can_overwrite = callback.request_overwrite_permission(
                target.as_absolute_path());

            if (!can_overwrite)
                return;
        }

        com_ptr<IStream> remote_stream;
        
        try
        {
            remote_stream = sftp_directory.GetFile(file, true);
        }
        catch (const com_error& provider_error)
        {
            // TODO: once we decomtaminate the provider, move this to the
            // snitching drop target so it can use the info in the task dialog

            wstringstream new_message;
            new_message <<
                translate("Unable to create file on the server:") << L"\n";
            new_message << provider_error.description();
            new_message << L"\n" << target.as_absolute_path();

            BOOST_THROW_EXCEPTION(
                com_error(
                    new_message.str(), provider_error.hr(),
                    provider_error.source(), provider_error.guid(),
                    provider_error.help_file(), provider_error.help_context()));
        }

        ::SHChangeNotify(
            SHCNE_CREATE, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT,
            (target.directory() + file).get(), NULL);

        // Set both streams back to the start
        LARGE_INTEGER move = {0};
        HRESULT hr = local_stream->Seek(move, SEEK_SET, NULL);
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error_from_interface(local_stream, hr));

        hr = remote_stream->Seek(move, SEEK_SET, NULL);
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error_from_interface(remote_stream, hr));

        // Do the copy in chunks allowing us to cancel the operation
        // and display progress
        ULARGE_INTEGER cb;
        cb.QuadPart = COPY_CHUNK_SIZE;
        int64_t done = 0;
        int64_t total = size_of_stream(local_stream);

        while (true)
        {
            callback.check_if_user_cancelled();

            ULARGE_INTEGER cbRead = {0};
            ULARGE_INTEGER cbWritten = {0};
            // TODO: make our own CopyTo that propagates errors
            hr = local_stream->CopyTo(
                remote_stream.get(), cb, &cbRead, &cbWritten);
            assert(FAILED(hr) || cbRead.QuadPart == cbWritten.QuadPart);
            if (FAILED(hr))
                BOOST_THROW_EXCEPTION(
                    com_error_from_interface(local_stream, hr));

            try
            {
                // We create a different version of the PIDL here whose filesize
                // is the amount copied so far. Otherwise Explorer shows a
                // 0-byte file when the copying is done.
                file = create_remote_itemid(
                    target.filename(), false, false, L"", L"", 0, 0, 0,
                    done, datetime_t::now(), datetime_t::now());

                ::SHChangeNotify(
                    SHCNE_UPDATEITEM, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT,
                    (target.directory() + file).get(), NULL);
            }
            catch(const exception& e)
            {
                // Ignoring error; failing to update the shell doesn't
                // warrant aborting the transfer
                trace("Failed to notify shell of file update %s") % e.what();
            }

            // A failure to update the progress isn't a good enough reason
            // to abort the copy so we swallow the exception.
            try
            {
                done += cbWritten.QuadPart;
                callback.update_progress(done, total);
            }
            catch (const exception& e)
            {
                trace("Progress update threw exception: %s") % e.what();
                assert(false);
            }

            if (cbRead.QuadPart == 0)
                break; // finished
        }
    }

}

CopyFileOperation::CopyFileOperation(
    const RootedSource& source, const SftpDestination& destination) :
m_source(source), m_destination(destination) {}

std::wstring CopyFileOperation::title() const
{
    return (wformat(
        translate(
            "Top line of a transfer progress window saying which "
            "file is being copied. {1} is replaced with the file path "
            "and must be included in your translation.",
            "Copying '{1}'"))
        % m_source.relative_name()).str();
}

std::wstring CopyFileOperation::description() const
{
    return (wformat(
        translate(
            "Second line of a transfer progress window giving the destination "
            "directory. {1} is replaced with the directory path and must be "
            "included in your translation.",
            "To '{1}'"))
        % m_destination.root_name()).str();
}

void CopyFileOperation::operator()(
    OperationCallback& callback,
    com_ptr<ISftpProvider> provider, com_ptr<ISftpConsumer> consumer) const
{
    com_ptr<IStream> stream = stream_from_pidl(m_source.pidl());

    resolved_destination resolved_target(m_destination.resolve_destination());

    copy_stream_to_remote_destination(
        stream, provider, consumer, resolved_target, callback);
}

Operation* CopyFileOperation::do_clone() const
{
    return new CopyFileOperation(*this);
}

}}
