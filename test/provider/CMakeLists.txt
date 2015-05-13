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

set(SOURCES
  provider_test.cpp
  stream_create_test.cpp
  stream_read_test.cpp
  stream_write_test.cpp
  SftpFixture.hpp
  StreamFixture.hpp)

swish_test_suite(
  SUBJECT provider
  SOURCES ${SOURCES}
  LIBRARIES ${Boost_LIBRARIES})

swish_copy_fixture_files(test-provider)