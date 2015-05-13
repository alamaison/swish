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

set(COMMAND_SOURCES
  commands/commands.cpp
  commands/commands.hpp
  commands/delete.cpp
  commands/delete.hpp
  commands/NewFolder.cpp
  commands/NewFolder.hpp)

set(SOURCES
  columns.cpp
  context_menu_callback.cpp
  filemode.c
  Mode.cpp
  pidl_connection.cpp
  properties.cpp
  ViewCallback.cpp
  columns.hpp
  context_menu_callback.hpp
  filemode.h
  Mode.h
  pidl_connection.hpp
  properties.hpp
  remote_pidl.hpp
  swish_pidl.hpp
  ViewCallback.hpp
  ${COMMAND_SOURCES})

add_library(remote_folder ${SOURCES})

target_compile_definitions(remote_folder
  PRIVATE ISOLATION_AWARE_ENABLED=1)

hunter_add_package(Comet)
hunter_add_package(Washer)

find_package(Comet REQUIRED CONFIG)
find_package(Washer REQUIRED CONFIG)

target_link_libraries(remote_folder
  PRIVATE connection provider nse frontend shell_folder
  PUBLIC Washer::washer Comet::comet ${Boost_LIBRARIES})