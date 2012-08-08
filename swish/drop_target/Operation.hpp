/**
    @file

    Interface to drop target operations.

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

#ifndef SWISH_DROP_TARGET_OPERATION_HPP
#define SWISH_DROP_TARGET_OPERATION_HPP
#pragma once

#include "swish/provider/SftpProvider.h" // sftp_provider, ISftpConsumer

#include <boost/cstdint.hpp> // uintmax_t
#include <boost/function.hpp> // function
#include <boost/shared_ptr.hpp>

#include <comet/ptr.h> // com_ptr

#include <cassert> // assert
#include <string>

namespace swish {
namespace drop_target {

class DropActionCallback;

/**
 * Interface through which individual drop operations interact with user.
 *
 * Purpose: to abstract the interaction so that an operation can pretend it
 * is the only operation happening.  The operation doesn't need to think
 * about the lifetime of the progress display and just updates it as it
 * wishes till so_far == out_of.
 */
class OperationCallback
{
public:

    /**
     * Throw com_error(E_ABORT) if user cancelled.
     *
     * It throws rather than returning a boolean in order to force the operation
     * to abort with an exception.  This behaviour is expected by drag-and-drop.
     */
    virtual void check_if_user_cancelled() const = 0;

    virtual bool request_overwrite_permission(
        const boost::filesystem::wpath& target) const = 0;

    virtual void update_progress(
        boost::uintmax_t so_far, boost::uintmax_t out_of) = 0;

    virtual ~OperationCallback() {}
};

/**
 * Interface of operation functors making up a drop.
 */
class Operation
{
public:

    virtual std::wstring title() const = 0;

    virtual std::wstring description() const = 0;

    virtual void operator()(
        OperationCallback& callback,
        boost::shared_ptr<swish::provider::sftp_provider> provider,
        comet::com_ptr<ISftpConsumer> consumer)
        const = 0;

    Operation* clone() const
    {
        Operation* item = do_clone();
        assert(typeid(*this) == typeid(*item) &&
            "do_clone() sliced object!");
        return item;
    }

private:
    virtual Operation* do_clone() const = 0;
};

inline Operation* new_clone(const Operation& item)
{
    return item.clone();
}

}} // namespace swish::drop_target

#endif
