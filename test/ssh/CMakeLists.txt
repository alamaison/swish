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

set(INTEGRATION_TESTS
  auth_test.cpp
  filesystem_test.cpp
  host_key_test.cpp
  openssh_fixture.cpp
  sandbox_fixture.cpp
  session_test.cpp
  stream_test.cpp
  openssh_fixture.hpp
  sandbox_fixture.hpp
  session_fixture.hpp
  sftp_fixture.hpp
  sftp_fixture.cpp)

set(UNIT_TESTS
  knownhost_test.cpp
  path_test.cpp)

set(TEST_RUNNER_ARGUMENTS
  --result_code=yes --build_info=yes --log_level=test_suite)

set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

include(CMakeParseArguments)
# ssh_test_suite(SUBJECT test-target [VARIANT suite-variant] SOURCES ...
#                LIBRARIES ... [LABELS ...])
function(SSH_TEST_SUITE)
  set(options)
  set(oneValueArgs SUBJECT VARIANT)
  set(multiValueArgs SOURCES LIBRARIES LABELS)
  cmake_parse_arguments(SSH_TEST_SUITE
    "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(SSH_TEST_SUITE_VARIANT)
    set(_TEST_EXE_NAME
      "test-${SSH_TEST_SUITE_SUBJECT}-${SSH_TEST_SUITE_VARIANT}")
  else()
    set(_TEST_EXE_NAME "test-${SSH_TEST_SUITE_SUBJECT}")
  endif()

  add_executable(${_TEST_EXE_NAME} module.cpp ${SSH_TEST_SUITE_SOURCES})

  target_link_libraries(${_TEST_EXE_NAME}
    PRIVATE
    ${SSH_TEST_SUITE_SUBJECT} ${SSH_TEST_SUITE_LIBRARIES})

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
endfunction()

hunter_add_package(Boost.Process)

ssh_test_suite(
  SUBJECT ssh
  SOURCES ${INTEGRATION_TESTS}
  LIBRARIES ${Boost_LIBRARIES} Boost::Process
  LABELS integration)

ssh_test_suite(
  SUBJECT ssh VARIANT unit
  SOURCES ${UNIT_TESTS}
  LIBRARIES ${Boost_LIBRARIES}
  LABELS unit)
