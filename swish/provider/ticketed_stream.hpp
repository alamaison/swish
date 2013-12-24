/**
    @file

    Session-holding IStream.

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

#ifndef SWISH_PROVIDER_TICKETED_STREAM_HPP
#define SWISH_PROVIDER_TICKETED_STREAM_HPP

#include <swish/connection/session_manager.hpp> // session_reservation

#include <comet/server.h> // simple_object

#include <boost/move/move.hpp> // BOOST_RV_REF

namespace swish {
namespace provider {

/**
 * IStream holding a session reservation ticket.
 *
 * The ticket ensures the session remains active for at least as long as the
 * IStream.
 */
class ticketed_stream : public comet::simple_object<IStream>
{
public:

    ticketed_stream(
        com_ptr<IStream> stream,
        BOOST_RV_REF(swish::connection::session_reservation) ticket)
        : m_ticket(ticket), m_inner(stream) {}

    virtual HRESULT STDMETHODCALLTYPE Read( 
        void* buffer, ULONG buffer_size, ULONG* read_count_out)
    {
        return m_inner->Read(buffer, buffer_size, read_count_out);
    }

    virtual HRESULT STDMETHODCALLTYPE Write(
        const void* data, ULONG data_size, ULONG* written_count_out)
    {
        return m_inner->Write(data, data_size, written_count_out);
    }

    virtual HRESULT STDMETHODCALLTYPE Seek(
        LARGE_INTEGER offset, DWORD origin, ULARGE_INTEGER* new_position_out)
    {
        return m_inner->Seek(offset, origin, new_position_out);
    }

    virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER new_size)
    {
        return m_inner->SetSize(new_size);
    }

    virtual HRESULT STDMETHODCALLTYPE CopyTo( 
        IStream* destination, ULARGE_INTEGER amount,
        ULARGE_INTEGER* bytes_read_out, ULARGE_INTEGER* bytes_written_out)
    {
        return m_inner->CopyTo(
            destination, amount, bytes_read_out, bytes_written_out);
    }

    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD commit_flags)
    {
        return m_inner->Commit(commit_flags);
    }

    virtual HRESULT STDMETHODCALLTYPE Revert()
    {
        return m_inner->Revert();
    }

    virtual HRESULT STDMETHODCALLTYPE LockRegion(
        ULARGE_INTEGER offset, ULARGE_INTEGER extent,
        DWORD lock_type)
    {
        return m_inner->LockRegion(offset, extent, lock_type);
    }

    virtual HRESULT STDMETHODCALLTYPE UnlockRegion(
        ULARGE_INTEGER offset, ULARGE_INTEGER extent,
        DWORD lock_type)
    {
        return m_inner->UnlockRegion(offset, extent, lock_type);
    }

    virtual HRESULT STDMETHODCALLTYPE Stat( 
        STATSTG* attributes_out, DWORD stat_flag)
    {
        return m_inner->Stat(attributes_out, stat_flag);
    }

    virtual HRESULT STDMETHODCALLTYPE Clone(IStream** stream_out)
    {
        return m_inner->Clone(stream_out);
    }

private:

    swish::connection::session_reservation m_ticket;

    comet::com_ptr<IStream> m_stream;
};

}} // namespace swish::provider

#endif
