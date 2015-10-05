# Copyright (C) 2015  Alexander Lamaison <swish@lammy.co.uk>
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
  session_fixture.hpp)

set(UNIT_TESTS
  knownhost_test.cpp
  path_test.cpp)

set(TEST_RUNNER_ARGUMENTS
  --result_code=yes --build_info=yes --log_level=test_suite)

include(FindCygwin)

if(CYGWIN_INSTALL_PATH)
  set(TEST_RUNNER_ENVIRONMENT
    "PATH=${CYGWIN_INSTALL_PATH}/bin;${CYGWIN_INSTALL_PATH}/usr/sbin")
  string(REPLACE ";" "\\;" TEST_RUNNER_ENVIRONMENT
    "${TEST_RUNNER_ENVIRONMENT}")

  if(NOT EXISTS "${CYGWIN_INSTALL_PATH}/usr/sbin/sshd.exe")
    message(WARNING
      # Running sshd for the tests requires a version of sshd that matches
      # the version of chmod used to set the permissions on the hostkey
      # file.  sshd refuses to run if it thinks the key is world-readable,
      # and different implementations of chmod save those permissions
      # in different ways.  Cygwin uses the 'special' Unix subsystem area
      # of the Windows ACL so the version of sshd must also use that method.
      "Cygwin sshd not found: tests are likely to fail to run")
  endif()
else()
  message(WARNING
    "Cygwin not found: tests are likely to fail to run")
endif()

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
    WORKING_DIRECTORY $<TARGET_FILE_DIR:${_TEST_EXE_NAME}>)

  if(TEST_RUNNER_ENVIRONMENT)
    set_tests_properties(${_TEST_EXE_NAME} PROPERTIES
      ENVIRONMENT "${TEST_RUNNER_ENVIRONMENT}")
  endif()

  if(SSH_TEST_SUITE_LABELS)
    set_tests_properties(
      ${_TEST_EXE_NAME} PROPERTIES LABELS "${SSH_TEST_SUITE_LABELS}")
  endif()
endfunction()

set(_FIXTURE_FILES
  test_known_hosts
  test_known_hosts_hashed
  test_known_hosts_out)
set(_FIXTURE_SSHD_ETC_FILES
  fixture_dsakey fixture_dsakey.pub
  fixture_hostkey fixture_hostkey.pub
  fixture_rsakey fixture_rsakey.pub)
set(_FIXTURE_FILES_DIR ${CMAKE_CURRENT_LIST_DIR})
set(_PERMISSION_SCRIPT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

function(SSH_COPY_FIXTURE_FILES TEST_TARGET)
  foreach(FILE ${_FIXTURE_FILES})
    add_custom_command(
      TARGET ${TEST_TARGET} POST_BUILD
      COMMAND ${CMAKE_COMMAND}
      -E copy_if_different
      ${_FIXTURE_FILES_DIR}/${FILE}
      $<TARGET_FILE_DIR:${TEST_TARGET}>/${FILE}
      VERBATIM)
  endforeach()

  foreach(FILE ${_FIXTURE_SSHD_ETC_FILES})
    add_custom_command(
      TARGET ${TEST_TARGET} POST_BUILD
      COMMAND ${CMAKE_COMMAND}
      -E copy_if_different
      ${_FIXTURE_FILES_DIR}/${FILE}
      $<TARGET_FILE_DIR:${TEST_TARGET}>/sshd-etc/${FILE}
      VERBATIM)
  endforeach()


  if(CYGWIN_INSTALL_PATH)
    add_custom_command(
      TARGET ${TEST_TARGET} POST_BUILD
      COMMAND
      cd $<TARGET_FILE_DIR:${TEST_TARGET}>/sshd-etc/
      VERBATIM)
    add_custom_command(
      TARGET ${TEST_TARGET} POST_BUILD
      COMMAND
      set PATH=${CYGWIN_INSTALL_PATH}/bin$<SEMICOLON>)
    add_custom_command(
      TARGET ${TEST_TARGET} POST_BUILD
      COMMAND
      bash ${_PERMISSION_SCRIPT_DIR}/fix_key_permissions.sh
      VERBATIM)
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

ssh_copy_fixture_files(test-ssh)
