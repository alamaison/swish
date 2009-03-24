/**
 * @file
 *
 * Main test runner implementation.
 */

#include "pch.h"

#include <cppunit/TextTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>

/**
 * Run all tests displaying their names and status.
 *
 * If a test fails, display the error in a compiler compatible form.
 */
int main()
{
	// Create the event manager and test controller
	CPPUNIT_NS::TestResult controller;

	// Add a listener that collects test result
	CPPUNIT_NS::TestResultCollector result;
	controller.addListener(&result);        

	// Add a listener that print dots as test run.
	CPPUNIT_NS::TextTestProgressListener progress;
	controller.addListener(&progress);      

	// Add the top suite to the test runner
	CPPUNIT_NS::TestRunner runner;
	runner.addTest(CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest());
	runner.run(controller);

	// Print test in a compiler compatible format.
	CPPUNIT_NS::CompilerOutputter outputter(&result, CPPUNIT_NS::stdCOut());
	outputter.write(); 

	return result.wasSuccessful() ? 0 : 1;
}