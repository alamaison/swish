# Copyright (C) 2016  Alexander Lamaison
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


add_library(openssh_fixture
  openssh_fixture.cpp
  openssh_fixture.hpp)

hunter_add_package(Boost.Process)

target_link_libraries(openssh_fixture
  PUBLIC ${Boost_LIBRARIES}
  PRIVATE Boost::Process)

add_library(provider_fixture
  provider_fixture.cpp
  provider_fixture.hpp)

target_link_libraries(provider_fixture
  PUBLIC openssh_fixture)

add_library(session_fixture
  session_fixture.cpp
  session_fixture.hpp)

target_link_libraries(session_fixture
  PUBLIC openssh_fixture ssh)

add_library(sftp_fixture
  sftp_fixture.cpp
  sftp_fixture.hpp)

target_link_libraries(sftp_fixture
  PUBLIC session_fixture)

add_library(com_stream_fixture
  com_stream_fixture.cpp
  com_stream_fixture.hpp)

target_link_libraries(com_stream_fixture
  PUBLIC sftp_fixture)
