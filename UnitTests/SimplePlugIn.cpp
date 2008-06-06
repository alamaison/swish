// SimplePlugIn.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

[ module( name="UnitTests" ) ];
//class CUnitTests
//{};

#include <cppunit/plugin/TestPlugIn.h>

// Implements all the plug-in stuffs, WinMain...
#pragma warning(push)
#pragma warning(disable:4100)
CPPUNIT_PLUGIN_EXPORTED_FUNCTION_IMPL( CPPUNIT_NS::TestPlugInDefaultImpl );
#pragma warning(pop)
