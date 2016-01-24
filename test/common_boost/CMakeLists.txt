# Copyright (C) 2015, 2016 Alexander Lamaison <swish@lammy.co.uk>
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
  data_object_utils.cpp
  fixtures.cpp
  PidlFixture.cpp
  ProviderFixture.cpp
  stream_utils.cpp
  ConsumerStub.hpp
  data_object_utils.hpp
  fixtures.hpp
  helpers.hpp
  MockConsumer.hpp
  MockProvider.hpp
  PidlFixture.hpp
  ProviderFixture.hpp
  stream_utils.hpp
  SwishPidlFixture.hpp
  tree.hh
  tree.hpp)

add_library(test-common_boost ${SOURCES})

hunter_add_package(Comet)
hunter_add_package(Washer)
hunter_add_package(Boost.Process)

find_package(Comet REQUIRED CONFIG)
find_package(Washer REQUIRED CONFIG)

target_link_libraries(test-common_boost
  PRIVATE shell shell_folder
  PUBLIC Comet::comet Washer::washer ${Boost_LIBRARIES} Boost::Process)
