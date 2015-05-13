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
  commands/Add.cpp
  commands/Add.hpp
  commands/CloseSession.cpp
  commands/CloseSession.hpp
  commands/commands.cpp
  commands/commands.hpp
  commands/LaunchAgent.cpp
  commands/LaunchAgent.hpp
  commands/Remove.cpp
  commands/Remove.hpp)

set(SOURCES
  columns.cpp
  host_itemid_connection.cpp
  host_management.cpp
  menu_command_manager.cpp
  properties.cpp
  ViewCallback.cpp
  columns.hpp
  extract_icon.hpp
  host_itemid_connection.hpp
  host_management.hpp
  host_pidl.hpp
  menu_command_manager.hpp
  overlay_icon.hpp
  properties.hpp
  ViewCallback.hpp
  ${COMMAND_SOURCES})

add_library(host_folder ${SOURCES})

hunter_add_package(Comet)
hunter_add_package(Washer)

find_package(Comet REQUIRED CONFIG)
find_package(Washer REQUIRED CONFIG)

target_link_libraries(host_folder
  PRIVATE connection nse version forms frontend shell_folder
  PUBLIC Washer::washer Comet::comet ${Boost_LIBRARIES})
