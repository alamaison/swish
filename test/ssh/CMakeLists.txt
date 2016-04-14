# Copyright 2015, 2016 Alexander Lamaison
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <http://www.gnu.org/licenses/>.

hunter_add_package(BoostProcess)
find_package(BoostProcess CONFIG REQUIRED)

find_package(Boost 1.40 CONFIG REQUIRED COMPONENTS unit_test_framework)

# Underscores to distinguish from same-name Swish fixtures.  Can be renamed when
# we split the projects.
add_library(openssh_fixture_
  openssh_fixture.cpp
  openssh_fixture.hpp)
target_link_libraries(openssh_fixture_
  PUBLIC ${Boost_LIBRARIES}
  PRIVATE BoostProcess::boost_process)

add_library(session_fixture_
  session_fixture.cpp
  session_fixture.hpp)
target_link_libraries(session_fixture_
  PUBLIC openssh_fixture_ ssh)

add_library(sftp_fixture_
  sftp_fixture.cpp
  sftp_fixture.hpp)
target_link_libraries(sftp_fixture_
  PUBLIC session_fixture_)

set(INTEGRATION_TESTS
  auth_test
  filesystem_test
  filesystem_construction_test
  host_key_test
  session_test
  input_stream_test
  output_stream_test
  stream_threading_test
  io_stream_test)

set(UNIT_TESTS
  knownhost_test
  path_test)

set(TEST_RUNNER_ARGUMENTS
  --result_code=yes --build_info=yes --log_level=test_suite)

set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

include(CMakeParseArguments)
# ssh_test_suite(SUBJECT test-target TESTS ... LIBRARIES ... [LABELS ...])
function(SSH_TEST_SUITE)
  set(options)
  set(oneValueArgs SUBJECT)
  set(multiValueArgs TESTS LIBRARIES LABELS)
  cmake_parse_arguments(SSH_TEST_SUITE
    "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  foreach(_TEST_NAME ${SSH_TEST_SUITE_TESTS})
    set(_TEST_EXE_NAME "test-${SSH_TEST_SUITE_SUBJECT}-${_TEST_NAME}")
    set(_TEST_SOURCE_FILE "${_TEST_NAME}.cpp")

    add_executable(${_TEST_EXE_NAME} module.cpp ${_TEST_SOURCE_FILE})

    target_link_libraries(${_TEST_EXE_NAME}
      PRIVATE ${SSH_TEST_SUITE_SUBJECT} ${SSH_TEST_SUITE_LIBRARIES})

    add_dependencies(BUILD_ALL_TESTS ${_TEST_EXE_NAME})

    add_test(
      NAME ${_TEST_EXE_NAME}
      COMMAND ${_TEST_EXE_NAME} ${TEST_RUNNER_ARGUMENTS}
      WORKING_DIRECTORY "${TEST_DATA_DIR}")

    if(MEMORY_LEAKS_ARE_FAILURES)
      # Don't hide memory leak detection.  The detector can't change the error
      # code so the test appears successful otherwise.
      set_tests_properties(${_TEST_EXE_NAME} PROPERTIES
	FAIL_REGULAR_EXPRESSION "Detected memory leaks")
    endif()

    if(SSH_TEST_SUITE_LABELS)
      set_tests_properties(
	${_TEST_EXE_NAME} PROPERTIES LABELS "${SSH_TEST_SUITE_LABELS}")
    endif()
  endforeach()
endfunction()

ssh_test_suite(
  SUBJECT ssh
  TESTS ${INTEGRATION_TESTS}
  LIBRARIES Boost::unit_test_framework openssh_fixture_ session_fixture_ sftp_fixture_
  LABELS integration)

ssh_test_suite(
  SUBJECT ssh VARIANT unit
  TESTS ${UNIT_TESTS}
  LIBRARIES ${Boost_LIBRARIES}
  LABELS unit)
