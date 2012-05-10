/**
    @file

    Directory creation operation.

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

#ifndef SWISH_DROP_TARGET_CREATEDIRECTORYOPERATION_HPP
#define SWISH_DROP_TARGET_CREATEDIRECTORYOPERATION_HPP
#pragma once

#include "swish/drop_target/Operation.hpp"
#include "swish/drop_target/SftpDestination.hpp"

namespace swish {
namespace drop_target {

class CreateDirectoryOperation : public Operation
{
public:

	CreateDirectoryOperation(const SftpDestination& target);

	virtual std::wstring title() const;

	virtual std::wstring description() const;

	virtual void operator()(
		boost::function<void(ULONGLONG, ULONGLONG)> progress,
		comet::com_ptr<ISftpProvider> provider,
		comet::com_ptr<ISftpConsumer> consumer,
		DropActionCallback& callback) const;

private:

	virtual Operation* do_clone() const;

	SftpDestination m_destination;
};

}}

#endif
