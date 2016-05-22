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
  libssh2_sftp_filesystem_item.cpp
  Provider.cpp
  libssh2_sftp_filesystem_item.hpp
  Provider.hpp
  sftp_filesystem_item.hpp
  sftp_provider.hpp
  ticketed_stream.hpp)

add_library(provider ${SOURCES})

hunter_add_package(Comet)
hunter_add_package(Washer)
hunter_add_package(Boost COMPONENTS filesystem regex system)

find_package(Comet REQUIRED CONFIG)
find_package(Washer REQUIRED CONFIG)
find_package(Boost REQUIRED COMPONENTS filesystem regex system)

target_link_libraries(provider
  PUBLIC Comet::comet connection Boost::boost Boost::filesystem
  PRIVATE ssh Washer::washer Boost::regex Boost::system)
