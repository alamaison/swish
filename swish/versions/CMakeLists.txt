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

find_package(Git)

set(VERSION_CONFIGURE_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/git_version.h.in")
set(VERSION_CONFIGURE_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/git_version.h")

add_custom_target(git_version
  COMMENT "Regenerating embedded version"
  COMMAND ${CMAKE_COMMAND}
  -DGIT_FOUND=${GIT_FOUND}
  -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
  -DVERSION_CONFIGURE_INPUT=${VERSION_CONFIGURE_INPUT}
  -DVERSION_CONFIGURE_OUTPUT=${VERSION_CONFIGURE_OUTPUT}
  -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
  -P ${CMAKE_CURRENT_SOURCE_DIR}/configure_git_version.cmake
  VERBATIM)

set(SOURCES
  metadata.h
  version.cpp
  version.hpp)

# This target must NOT be called 'version'. That conflicts with the name of the
# Windows library that we also need (via WinSparkle).  If the names collide,
# CMake gets confused which one we mean and chooses the target, not the system
# library.
add_library(versions ${SOURCES})
add_dependencies(versions git_version)

target_include_directories(versions PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
