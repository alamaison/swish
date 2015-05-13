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
  commands/About.cpp
  commands/About.hpp)

set(SOURCES
  announce_error.hpp
  bind_best_taskdialog.hpp
  UserInteraction.hpp
  winsparkle_shower.hpp
  announce_error.cpp
  bind_best_taskdialog.cpp
  UserInteraction.cpp
  winsparkle_shower.cpp
  ${COMMAND_SOURCES})

add_library(frontend ${SOURCES})

hunter_add_package(Comet)
hunter_add_package(Washer)
hunter_add_package(WinSparkle)
hunter_add_package(WTL)

find_package(Comet REQUIRED CONFIG)
find_package(Washer REQUIRED CONFIG)
find_package(WinSparkle REQUIRED CONFIG)

target_link_libraries(frontend
  PRIVATE nse versions WinSparkle::WinSparkle
  PUBLIC WTL::wtl Washer::washer Comet::comet)