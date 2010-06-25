/**
    @file

    Test suite runner.

    @if licence

    Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

//////////////////////////////////////////////////////////////////////////
// REMOVE THIS WHEN STRIPPING DROP_TARGET ATL
//////////////////////////////////////////////////////////////////////////

#include "swish/atl.hpp"

namespace test {
namespace swish {
namespace drop_target {

using ATL::CAtlModule;

/**
 * ATL module needed to use ATL-based objects, e.g. CMockSftpConsumer.
 */
class CModule : public CAtlModule
{
public :
	
	virtual HRESULT AddCommonRGSReplacements(IRegistrarBase*) throw()
	{
		return S_OK;
	}
};

}}}

test::swish::drop_target::CModule _AtlModule; ///< Global module instance

//////////////////////////////////////////////////////////////////////////

#define BOOST_TEST_MODULE swish::drop_target tests
#include <boost/test/unit_test.hpp>
