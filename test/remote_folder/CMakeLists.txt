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

set(UNIT_TESTS
  columns_test.cpp
  properties_test.cpp
  remote_pidl_test.cpp
  swish_pidl_test.cpp)

set(INTEGRATION_TESTS
  remote_commands_test.cpp)

swish_test_suite(
  SUBJECT remote_folder VARIANT unit
  SOURCES ${UNIT_TESTS}
  LIBRARIES ${Boost_LIBRARIES}
  LABELS unit)

swish_test_suite(
  SUBJECT remote_folder VARIANT integration
  SOURCES ${INTEGRATION_TESTS}
  LIBRARIES ${Boost_LIBRARIES} provider_fixture
  LABELS integration)
