/**
    @file

    Executable schedule of operations.

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

#ifndef SWISH_DROP_TARGET_PLAN_HPP
#define SWISH_DROP_TARGET_PLAN_HPP
#pragma once

#include "swish/provider/SftpProvider.h" // sftp_provider, ISftpConsumer
#include "swish/drop_target/Progress.hpp"

#include <boost/shared_ptr.hpp>

namespace swish {
namespace drop_target {

class DropActionCallback;

/**
 * Interface for executable schedule of operations.
 */
class Plan
{
public:
    virtual ~Plan() {}

    virtual void execute_plan(
        DropActionCallback& callback,
        boost::shared_ptr<swish::provider::sftp_provider> provider,
        comet::com_ptr<ISftpConsumer> consumer) const = 0;
};

}}

#endif