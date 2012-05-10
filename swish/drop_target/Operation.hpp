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

#include <boost/function.hpp> // function

#include <comet/ptr.h> // com_ptr

#include <cassert> // assert
#include <string>

struct ISftpProvider;
struct ISftpConsumer;

namespace swish {
namespace drop_target {

class DropActionCallback;

/**
 * Interface of operation functors making up a drop.
 */
class Operation
{
public:

	virtual std::wstring title() const = 0;

	virtual std::wstring description() const = 0;

	virtual void operator()(
		boost::function<void(ULONGLONG, ULONGLONG)> progress,
		comet::com_ptr<ISftpProvider> provider,
		comet::com_ptr<ISftpConsumer> consumer,
		DropActionCallback& callback)
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
