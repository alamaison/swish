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
  authenticated_session.cpp
  connection_spec.cpp
  running_session.cpp
  session_manager.cpp
  session_pool.cpp
  authenticated_session.hpp
  connection_spec.hpp
  running_session.hpp
  session_manager.hpp
  session_pool.hpp)

add_library(connection ${SOURCES})

hunter_add_package(Comet)
hunter_add_package(Washer)

find_package(Comet REQUIRED CONFIG)
find_package(Washer REQUIRED CONFIG)

target_link_libraries(connection
  PRIVATE Boost::system Boost::filesystem Boost::date_time
  PUBLIC ssh provider Washer::washer Comet::comet Boost::boost Boost::thread)
