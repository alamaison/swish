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

set(SOURCES
  detail/command_state_conversion.hpp
  Command.cpp
  command_site.hpp
  command_site.cpp
  data_object_util.cpp
  default_context_menu_callback.cpp
  explorer_command.cpp
  UICommand.cpp
  view_callback.cpp
  Command.hpp
  data_object_util.hpp
  default_context_menu_callback.hpp
  explorer_command.hpp
  StaticColumn.hpp
  task_pane.hpp
  UICommand.hpp
  view_callback.hpp)

add_library(nse ${SOURCES})

hunter_add_package(Comet)
hunter_add_package(Washer)
hunter_add_package(Boost)

find_package(Comet REQUIRED CONFIG)
find_package(Washer REQUIRED CONFIG)
find_package(Boost REQUIRED)

target_link_libraries(nse
  PRIVATE shell shlwapi
  PUBLIC Washer::washer Comet::comet Boost::boost)
