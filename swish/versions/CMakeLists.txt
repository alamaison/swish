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

include(GetGitRevisionDescription)

git_describe(SWISH_GIT_VERSION)

set(GIT_VERSION_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/git_version.h.in")
set(GIT_VERSION_GENERATED_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/git_version.h")

configure_file(
  ${GIT_VERSION_TEMPLATE}
  ${GIT_VERSION_GENERATED_OUTPUT}
  @ONLY)
set_source_files_properties(
  ${GIT_VERSION_GENERATED_OUTPUT} PROPERTIES GENERATED TRUE)

set(METADATA_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/metadata.h.in")
set(METADATA_GENERATED_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/swish/versions/metadata.h")

configure_file(
  ${METADATA_TEMPLATE}
  ${METADATA_GENERATED_OUTPUT}
  @ONLY)
set_source_files_properties(
  ${METADATA_GENERATED_OUTPUT} PROPERTIES GENERATED TRUE)

set(SOURCES
  ${GIT_VERSION_GENERATED_OUTPUT}
  ${METADATA_GENERATED_OUTPUT}
  version.cpp
  version.hpp)

# This target must NOT be called 'version'. That conflicts with the name of the
# Windows library that we also need (via WinSparkle).  If the names collide,
# CMake gets confused which one we mean and chooses the target, not the system
# library.
add_library(versions ${SOURCES})

target_include_directories(versions PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
