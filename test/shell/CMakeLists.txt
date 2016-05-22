# Copyright 2015, 2016 Alexander Lamaison
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
  shell_test.cpp)

find_package(Boost REQUIRED COMPONENTS filesystem unit_test_framework)

swish_test_suite(
  SUBJECT shell
  SOURCES ${SOURCES}
  LIBRARIES local_sandbox_fixture Boost::unit_test_framework Boost::filesystem Boost::boost
  LABELS unit)
