/**
    @file

    Unit tests for Explorer command implementation classes.

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

#include "swish/shell_folder/explorer_command.hpp" // test subject

#include "swish/shell_folder/commands/Command.hpp" // Command

#include "test/common_boost/helpers.hpp"
#include <boost/test/unit_test.hpp>

#include <comet/ptr.h> // com_ptr
#include <comet/uuid_fwd.h> // uuid_t

#include <boost/shared_ptr.hpp> // shared_ptr

#include <string>

using swish::shell_folder::explorer_command::CExplorerCommandProvider;
using swish::shell_folder::explorer_command::make_explorer_command;
using swish::shell_folder::commands::Command;

using comet::com_ptr;
using comet::uuidof;
using comet::uuid_t;

using boost::shared_ptr;

using std::wstring;

namespace {

	struct TestCommand : public Command
	{
		TestCommand(
			const wstring& title, const uuid_t& guid,
			const wstring& tool_tip=wstring(),
			const wstring& icon_descriptor=wstring())
		: Command(title, guid, tool_tip, icon_descriptor) {}

		bool disabled(const com_ptr<IDataObject>&, bool) const
		{ return false; }

		bool hidden(const com_ptr<IDataObject>&, bool) const
		{ return false; }

		void operator()(const com_ptr<IDataObject>&, const com_ptr<IBindCtx>&)
		const
		{} // noop	
	};

	const uuid_t DUMMY_GUID_1("002F9D5D-DB85-4224-9097-B1D06E681252");
	const uuid_t DUMMY_GUID_2("3BDC0E76-2D94-43c3-AC33-ED629C24AA70");

	struct DummyCommand1 : public TestCommand
	{
		DummyCommand1() : TestCommand(
			L"command_1", DUMMY_GUID_1, L"tool-tip-1") {}
	};

	struct DummyCommand2 : public TestCommand
	{
		DummyCommand2() : TestCommand(
			L"command_2", DUMMY_GUID_2, L"tool-tip-2") {}
	};

	CExplorerCommandProvider::ordered_commands dummy_commands()
	{
		CExplorerCommandProvider::ordered_commands commands;
		commands.push_back(make_explorer_command(DummyCommand1()));
		commands.push_back(make_explorer_command(DummyCommand2()));
		return commands;
	}
}

BOOST_AUTO_TEST_SUITE(explorer_command_tests)

BOOST_AUTO_TEST_CASE( create_empty_provider )
{
	com_ptr<IExplorerCommandProvider> commands = new CExplorerCommandProvider(
		CExplorerCommandProvider::ordered_commands());
	BOOST_REQUIRE(commands);

	// Test GetCommands
	com_ptr<IEnumExplorerCommand> enum_commands;
	BOOST_REQUIRE_OK(
		commands->GetCommands(
			NULL, uuidof(enum_commands.in()),
			reinterpret_cast<void**>(enum_commands.out())));
	
	com_ptr<IExplorerCommand> command;
	BOOST_REQUIRE_EQUAL(enum_commands->Next(1, command.out(), NULL), S_FALSE);

	// Test GetCommand
	BOOST_REQUIRE_EQUAL(
		commands->GetCommand(
			GUID_NULL, uuidof(command.in()),
			reinterpret_cast<void**>(command.out())),
		E_FAIL);
}

BOOST_AUTO_TEST_CASE( commands )
{
	com_ptr<IExplorerCommandProvider> commands = new CExplorerCommandProvider(
		dummy_commands());
	BOOST_REQUIRE(commands);

	// Test GetCommands
	com_ptr<IEnumExplorerCommand> enum_commands;
	BOOST_REQUIRE_OK(
		commands->GetCommands(
			NULL, uuidof(enum_commands.in()),
			reinterpret_cast<void**>(enum_commands.out())));
	
	com_ptr<IExplorerCommand> command;
	uuid_t guid;

	BOOST_REQUIRE_OK(enum_commands->Next(1, command.out(), NULL));
	BOOST_REQUIRE_OK(command->GetCanonicalName(guid.out()));
	BOOST_REQUIRE_EQUAL(guid, DUMMY_GUID_1);

	BOOST_REQUIRE_OK(enum_commands->Next(1, command.out(), NULL));
	BOOST_REQUIRE_OK(command->GetCanonicalName(guid.out()));
	BOOST_REQUIRE_EQUAL(guid, DUMMY_GUID_2);

	BOOST_REQUIRE_EQUAL(enum_commands->Next(1, command.out(), NULL), S_FALSE);

	// Test GetCommand
	BOOST_REQUIRE_OK(
		commands->GetCommand(
			DUMMY_GUID_2, uuidof(command.in()),
			reinterpret_cast<void**>(command.out())));
	BOOST_REQUIRE_OK(command->GetCanonicalName(guid.out()));
	BOOST_REQUIRE_EQUAL(guid, DUMMY_GUID_2);

	BOOST_REQUIRE_OK(
		commands->GetCommand(
			DUMMY_GUID_1, uuidof(command.in()),
			reinterpret_cast<void**>(command.out())));
	BOOST_REQUIRE_OK(command->GetCanonicalName(guid.out()));
	BOOST_REQUIRE_EQUAL(guid, DUMMY_GUID_1);

	BOOST_REQUIRE_EQUAL(
		commands->GetCommand(
			GUID_NULL, uuidof(command.in()),
			reinterpret_cast<void**>(command.out())),
		E_FAIL);
}

namespace {

	const GUID TEST_GUID = 
		{ 0x1621a875, 0x1252, 0x4bde, 
		{ 0xb7, 0x69, 0x70, 0xa9, 0x5f, 0x49, 0x7c, 0x5f } };

	struct HostCommand : public Command
	{
		HostCommand() : Command(L"title", TEST_GUID, L"tool-tip") {}

		bool disabled(const com_ptr<IDataObject>&, bool) const
		{ return false; }

		bool hidden(const com_ptr<IDataObject>&, bool) const
		{ return false; }

		void operator()(const com_ptr<IDataObject>&, const com_ptr<IBindCtx>&)
		const
		{
			throw swish::exception::com_exception(E_ABORT);
		}
	};

	com_ptr<IExplorerCommand> host_command()
	{
		com_ptr<IExplorerCommand> command = make_explorer_command(
			HostCommand());

		BOOST_REQUIRE(command);
		return command;
	}
}


/**
 * GetTitle returns the string given in the constructor.
 */
BOOST_AUTO_TEST_CASE( title )
{
	com_ptr<IExplorerCommand> command = host_command();

	wchar_t* ret_val;
	BOOST_REQUIRE_OK(command->GetTitle(NULL, &ret_val));

	shared_ptr<wchar_t> title(ret_val, ::CoTaskMemFree);
	BOOST_REQUIRE_EQUAL(title.get(), L"title");
}

/**
 * GetIcon returns the expected empty string as it wasn't set in the
 * constructor.
 */
BOOST_AUTO_TEST_CASE( icon )
{
	com_ptr<IExplorerCommand> command = host_command();

	wchar_t* ret_val;
	BOOST_REQUIRE_OK(command->GetIcon(NULL, &ret_val));
	
	shared_ptr<wchar_t> icon(ret_val, ::CoTaskMemFree);
	BOOST_REQUIRE_EQUAL(icon.get(), L"");
}

/**
 * GetToolTip returns the string given in the constructor.
 */
BOOST_AUTO_TEST_CASE( tool_tip )
{
	com_ptr<IExplorerCommand> command = host_command();

	wchar_t* ret_val;
	BOOST_REQUIRE_OK(command->GetToolTip(NULL, &ret_val));

	shared_ptr<wchar_t> tip(ret_val, ::CoTaskMemFree);
	BOOST_REQUIRE_EQUAL(tip.get(), L"tool-tip");
}

/**
 * GetCanonicalName returns the test GUID given in the constructor.
 */
BOOST_AUTO_TEST_CASE( guid )
{
	com_ptr<IExplorerCommand> command = host_command();

	GUID guid;
	BOOST_REQUIRE_OK(command->GetCanonicalName(&guid));
	BOOST_REQUIRE_EQUAL(uuid_t(guid), uuid_t(TEST_GUID));
}

/**
 * GetFlags returns ECF_DEFAULT (0).
 */
BOOST_AUTO_TEST_CASE( flags )
{
	com_ptr<IExplorerCommand> command = host_command();

	EXPCMDFLAGS flags;
	BOOST_REQUIRE_OK(command->GetFlags(&flags));
	BOOST_REQUIRE_EQUAL(flags, 0U);
}

/**
 * GetState returns ECS_ENABLED (0).
 */
BOOST_AUTO_TEST_CASE( state )
{
	com_ptr<IExplorerCommand> command = host_command();

	EXPCMDSTATE flags;
	BOOST_REQUIRE_OK(command->GetState(NULL, false, &flags));
	BOOST_REQUIRE_EQUAL(flags, 0U);
}

/**
 * Invoke returns error that matches exception thrown by throwing_function 
 * passed to constructor.
 */
BOOST_AUTO_TEST_CASE( invoke )
{
	com_ptr<IExplorerCommand> command = host_command();

	BOOST_REQUIRE_EQUAL(command->Invoke(NULL, NULL), E_ABORT);
}

BOOST_AUTO_TEST_SUITE_END();
