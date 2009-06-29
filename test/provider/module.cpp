/**
 * @file
 *
 * ATL Module required for ATL support.
 */

#include "standard.h"

#include <atlbase.h>   // ATL modules
#include <atlcom.h>    // ATL CComObject et. al.

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