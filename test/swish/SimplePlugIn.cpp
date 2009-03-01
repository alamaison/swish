/**
 * @file Entry point for the DLL plugin.  The CPPUNIT test runner uses this.
 */

#include "stdafx.h"

#include <atlbase.h>   // ATL modules
#include <atlcom.h>    // ATL CComObject et. al.

#include <cppunit/plugin/TestPlugIn.h>

using namespace ATL;

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

CModule _AtlModule; ///< Global module instance

#pragma warning(push)
#pragma warning(disable:4100)
CPPUNIT_PLUGIN_IMPLEMENT();
#pragma warning(pop)
