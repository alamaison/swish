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

set(SOURCES
  openssh_fixture.cpp
  openssh_fixture.hpp)

set(TEST_RUNNER_ARGUMENTS
  --result_code=yes --build_info=yes --log_level=test_suite)

add_library(openssh_fixture ${SOURCES})

hunter_add_package(Boost.Process)

target_link_libraries(openssh_fixture
  PUBLIC ${Boost_LIBRARIES}
  PRIVATE Boost::Process)
