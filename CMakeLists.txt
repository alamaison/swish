# Copyright (C) 2015, 2016  Alexander Lamaison <swish@lammy.co.uk>
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
# Street, Fifth Floor, Boston, MA 02110-1301 USA.

cmake_minimum_required(VERSION 3.1)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Package management ###########################################################

include(HunterGate)
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.14.23.tar.gz"
    SHA1 "999f07d004649f5f549953f74f21e46b48d53931"
)

################################################################################

project(swish VERSION 0.8.3)

set(SWISH_FRIENDLY_NAME "Swish")
set(SWISH_VENDOR "swish-sftp.org")
set(SWISH_DESCRIPTION "Easy SFTP for Windows Explorer")
set(SWISH_COPYRIGHT "Copyright (C) 2006-2015  Alexander Lamaison and contributors")

#include(max_warnings)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/taskdialog98)

# Prevent Winsock errors and gives quicker builds
add_definitions(-DWIN32_LEAN_AND_MEAN)

# Silence bogus MSVC warnings
add_definitions(-D_SCL_SECURE_NO_WARNINGS)

# Currently required because not all Win32 calls use A or W form explicitly
add_definitions(-DUNICODE -D_UNICODE)

hunter_add_package(Boost
  COMPONENTS filesystem system test date_time regex signals locale thread)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.49 REQUIRED
  COMPONENTS filesystem date_time system regex signals thread locale)
if(MSVC)
  add_definitions(-DBOOST_ALL_NO_LIB=1)
endif()
include_directories(${Boost_INCLUDE_DIRS})

add_subdirectory(ezel)
add_subdirectory(ssh)
add_subdirectory(swish)
add_subdirectory(po)

option(BUILD_TESTING "Build test suite" ON)
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()

set(INSTALLER_URL
  "https://sourceforge.net/projects/swish/files/swish/swish-${swish_VERSION}/swish-${swish_VERSION}.exe")
string(TIMESTAMP PUBLICATION_DATE UTC)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/appcast.xml.in
  ${CMAKE_CURRENT_BINARY_DIR}/appcast.xml
  @ONLY)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/setup_conf.xml.in
  ${CMAKE_CURRENT_BINARY_DIR}/setup_conf.xml
  @ONLY)

file(DOWNLOAD "http://the.earth.li/~sgtatham/putty/0.64/x86/pageant.exe"
  "${CMAKE_CURRENT_BINARY_DIR}/pageant.exe"
  SHOW_PROGRESS
  EXPECTED_HASH SHA1=4f7ec7e53b7dd557603c2447fd177d85f14006ad)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/pageant.exe" NEWS LICENSE.txt README.md
  DESTINATION .)

include(InstallRequiredSystemLibraries)

set(CPACK_GENERATOR WIX)
set(CPACK_PACKAGE_NAME "${SWISH_FRIENDLY_NAME}")
set(CPACK_PACKAGE_VENDOR "${SWISH_VENDOR}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${SWISH_DESCRIPTION}")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_WIX_UPGRADE_GUID "97CF376F-FFDE-472A-946B-E3F5D45229DA")
set(CPACK_WIX_PATCH_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cpack_wix_patch.xml")

set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${SWISH_FRIENDLY_NAME}")
set(CMAKE_WIX_PROPERTY_ARPCONTACT swish@lammy.co.uk)
set(CMAKE_WIX_PROPERTY_ARPHELPLINK http://www.swish-sftp.org)
set(CMAKE_WIX_PROPERTY_ARPURLINFOABOUT http://www.swish-sftp.org)
set(CMAKE_WIX_PROPERTY_ARPURLUPDATEINFO http://sourceforge.net/projects/swish/)

#set(CPACK_WIX_UI_REF WixUI_Minimal)

include(CPack)
