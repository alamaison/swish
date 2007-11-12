// Swish.cpp : Implementation of DLL Exports.
//

#include "stdafx.h"

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlw.h>
#include <atlctrlx.h>
#include <atlsplit.h>

#include "resource.h"

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

/*
   The module attribute causes 
     DllMain
     DllRegisterServer
     DllUnregisterServer
     DllCanUnloadNow
     DllGetClassObject. 
   to be automatically implemented
 */
[ module(dll,
		 uuid = "{b816a838-5022-11dc-9153-0090f5284f85}", 
		 name = "Swish",
		 version = "0.2",
		 helpstring = "Swish 0.2 Type Library",
		 resource_name = "IDR_SWISH")
]
class CSwish
{
public:
// Override CAtlDllModuleT members here
};
		 
