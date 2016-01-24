# Copyright (C) 2015, 2016  Alexander Lamaison <swish@lammy.co.uk>
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

find_package(Boost 1.40 REQUIRED COMPONENTS unit_test_framework)

add_subdirectory(common_boost)
add_subdirectory(openssh_fixture)

option(MEMORY_LEAKS_ARE_FAILURES
  "Fail the test suite if a memory leak is detected" OFF)

set(TEST_RUNNER_ARGUMENTS
  --result_code=yes --build_info=yes --log_level=test_suite)

include(FindCygwin)

if(CYGWIN_INSTALL_PATH)
  set(TEST_RUNNER_ENVIRONMENT
    "PATH=${CYGWIN_INSTALL_PATH}/bin;${CYGWIN_INSTALL_PATH}/usr/sbin;${PATH}")
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

add_custom_target(BUILD_ALL_TESTS COMMENT "Building all tests")

set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/openssh_fixture")

include(CMakeParseArguments)
# swish_test_suite(SUBJECT test-target [VARIANT suite-variant] SOURCES ...
#                  LIBRARIES ... [LABELS ...])
function(SWISH_TEST_SUITE)
  set(options)
  set(oneValueArgs SUBJECT VARIANT)
  set(multiValueArgs SOURCES LIBRARIES LABELS)
  cmake_parse_arguments(SWISH_TEST_SUITE
    "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(SWISH_TEST_SUITE_VARIANT)
    set(_TEST_EXE_NAME
      "test-${SWISH_TEST_SUITE_SUBJECT}-${SWISH_TEST_SUITE_VARIANT}")
  else()
    set(_TEST_EXE_NAME "test-${SWISH_TEST_SUITE_SUBJECT}")
  endif()

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../module.cpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/module.cpp @ONLY)

  add_executable(${_TEST_EXE_NAME}
    ${SWISH_TEST_SUITE_SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/module.cpp)

  target_link_libraries(${_TEST_EXE_NAME}
    PRIVATE
    ${SWISH_TEST_SUITE_SUBJECT} test-common_boost ${SWISH_TEST_SUITE_LIBRARIES})

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

  # if(TEST_RUNNER_ENVIRONMENT)
  #   set_tests_properties(${_TEST_EXE_NAME} PROPERTIES
  #     ENVIRONMENT "${TEST_RUNNER_ENVIRONMENT}")
  # endif()

  if(SWISH_TEST_SUITE_LABELS)
    set_tests_properties(
      ${_TEST_EXE_NAME} PROPERTIES LABELS "${SWISH_TEST_SUITE_LABELS}")
  endif()
endfunction()

set(_FIXTURE_FILES test_zip_file.zip)
set(_FIXTURE_SSHD_ETC_FILES
  fixture_dsakey fixture_dsakey.pub
  fixture_hostkey fixture_hostkey.pub
  fixture_rsakey fixture_rsakey.pub)
set(_FIXTURE_FILES_DIR ${CMAKE_CURRENT_LIST_DIR}/common_boost)
set(_PERMISSION_SCRIPT_DIR ${CMAKE_CURRENT_LIST_DIR})

function(SWISH_COPY_FIXTURE_FILES TEST_TARGET)
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

add_subdirectory(connection)
add_subdirectory(drop_target)
add_subdirectory(ezel)
add_subdirectory(forms)
add_subdirectory(host_folder)
add_subdirectory(nse)
add_subdirectory(provider)
add_subdirectory(provider-network)
add_subdirectory(remote_folder)
add_subdirectory(shell)
add_subdirectory(shell_folder)
add_subdirectory(shell_folder-dialogue)
add_subdirectory(ssh)
add_subdirectory(versions)

# swish_declare_test_target(COMMAND_NAME [OPTIONAL_FLAGS...])
function(SWISH_DECLARE_TEST_TARGET COMMAND_NAME)
  set(options ALL)
  set(oneValueArgs)
  set(multiValueArgs)
  cmake_parse_arguments(SWISH_DECLARE_TEST_TARGET
    "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(SWISH_DECLARE_TEST_TARGET_ALL)
    set(SWISH_DECLARE_TEST_TARGET_ALL ALL)
  else()
    set(SWISH_DECLARE_TEST_TARGET_ALL)
  endif()

  # From http://stackoverflow.com/a/16163137/67013
  if(CMAKE_CONFIGURATION_TYPES)
    add_custom_target(${COMMAND_NAME} ${SWISH_DECLARE_TEST_TARGET_ALL}
      COMMAND ${CMAKE_CTEST_COMMAND}
      --force-new-ctest-process --output-on-failure
      --build-config "$<CONFIGURATION>"
      ${SWISH_DECLARE_TEST_TARGET_UNPARSED_ARGUMENTS})
  else()
    add_custom_target(${COMMAND_NAME} ${SWISH_DECLARE_TEST_TARGET_ALL}
      COMMAND ${CMAKE_CTEST_COMMAND}
      --force-new-ctest-process --output-on-failure
      ${SWISH_DECLARE_TEST_TARGET_UNPARSED_ARGUMENTS})
  endif()

  add_dependencies(${COMMAND_NAME} BUILD_ALL_TESTS)
endfunction()

swish_declare_test_target(CHECK)
swish_declare_test_target(CHECK_UNIT -L unit ALL)
swish_declare_test_target(CHECK_INTEGRATION -L integration)
swish_declare_test_target(CHECK_GUI -L gui)
swish_declare_test_target(CHECK_NETWORK -L network)
