/**
    @file

    Standard drop operation plan implementation.

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

#ifndef SWISH_DROP_TARGET_SEQUENTIALPLAN_HPP
#define SWISH_DROP_TARGET_SEQUENTIALPLAN_HPP
#pragma once

#include "swish/drop_target/Plan.hpp"

#include <boost/ptr_container/ptr_vector.hpp>

namespace swish {
namespace drop_target {

class Operation;

/**
 * Standard plan implementation made from list of Operation objects.
 *
 * Each object is executed in the order they are added and progress
 * displayed accordingly.
 */
class SequentialPlan : public Plan
{
public: // Plan

	virtual void execute_plan(
		Progress& progress, comet::com_ptr<ISftpProvider> provider,
		comet::com_ptr<ISftpConsumer> consumer, DropActionCallback& callback)
		const;

public:

	void add_stage(const Operation& entry);

private:

	boost::ptr_vector<Operation> m_copy_list;
};

}}

#endif