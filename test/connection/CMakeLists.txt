# Copyright (C) 2015, 2016 Alexander Lamaison
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

set(UNIT_TESTS
  connection_spec_test.cpp)

set(INTEGRATION_TESTS
  authenticated_session_test.cpp
  connection_spec_create_session_test.cpp
  running_session_test.cpp
  session_manager_test.cpp
  session_pool_test.cpp)

swish_test_suite(
  SUBJECT connection VARIANT unit
  SOURCES ${UNIT_TESTS}
  LIBRARIES ${Boost_LIBRARIES}
  LABELS unit)

swish_test_suite(
  SUBJECT connection VARIANT integration
  SOURCES ${INTEGRATION_TESTS}
  LIBRARIES ${Boost_LIBRARIES} openssh_fixture
  LABELS integration)
