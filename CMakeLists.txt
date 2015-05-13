# Copyright (C) 2015  Alexander Lamaison <swish@lammy.co.uk>
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
    URL "https://github.com/alamaison/hunter/archive/ee83a15960b5d74098d8704bc9cf82ad8ec1734f.tar.gz"
    SHA1 "d02301fee68f00198e7941129da446f80189017e"
    LOCAL
    )

################################################################################

project(swish VERSION 0.8.2)

#include(max_warnings)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/taskdialog98)

# Prevent Winsock errors and gives quicker builds
add_definitions(-DWIN32_LEAN_AND_MEAN)

# Currently required because not all Win32 calls use A or W form explicitly
add_definitions(-DUNICODE -D_UNICODE)

hunter_add_package(Boost
  COMPONENTS filesystem system test date_time regex signals locale)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.40 REQUIRED
  COMPONENTS filesystem date_time system regex signals thread locale)
add_definitions(-DBOOST_FILESYSTEM_VERSION=2)
if(MSVC)
  add_definitions(-DBOOST_ALL_NO_LIB=1)
endif()
include_directories(${Boost_INCLUDE_DIRS})

add_subdirectory(ezel)
add_subdirectory(ssh)
add_subdirectory(swish)

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