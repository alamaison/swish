// SimplePlugIn.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#include <cppunit/plugin/TestPlugIn.h>

class CTestProviderModule : public CAtlModuleT<CTestProviderModule> {;};

CTestProviderModule atlModule;

// Implements all the plug-in stuffs, WinMain...
#pragma warning(push)
#pragma warning(disable:4100)
CPPUNIT_PLUGIN_IMPLEMENT();
#pragma warning(pop)
