/**
 * @file
 *
 * ATL Module required for ATL support.
 */

#include "pch.h"

#include "common/atl.hpp"  // Common ATL setup

using namespace ATL;

/**
 * ATL module needed to use ATL-based objects.
 */
class CModule : public CAtlModule
{
public :
	
	virtual HRESULT AddCommonRGSReplacements(IRegistrarBase*) throw()
	{
		return S_OK;
	}
};

CModule _AtlModule; ///< Global module instance