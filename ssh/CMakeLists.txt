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
  agent.hpp
  detail/agent_state.hpp
  detail/file_handle_state.hpp
  detail/libssh2/agent.hpp
  detail/libssh2/knownhost.hpp
  detail/libssh2/libssh2.hpp
  detail/libssh2/session.hpp
  detail/libssh2/sftp.hpp
  detail/libssh2/userauth.hpp
  detail/session_state.hpp
  detail/sftp_channel_state.hpp
  filesystem.hpp
  filesystem/path.hpp
  host_key.hpp
  knownhost.hpp
  session.hpp
  sftp_error.hpp
  ssh_error.hpp
  stream.hpp)

add_custom_target(ssh-src SOURCES ${SOURCES})
add_library(ssh INTERFACE)
target_include_directories(ssh INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

hunter_add_package(Libssh2)
find_package(Libssh2 REQUIRED CONFIG)

target_link_libraries(ssh INTERFACE Libssh2::libssh2 ${Boost_LIBRARIES})
