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

set(RESOURCES
  com_dll.rc
  HostFolder.rgs
  RemoteFolder.rgs
  Swish.rgs)

set(SOURCES
  resource.h
  com_dll.def
  SwishCoClasses.cpp
  SwishModule.cpp
  ${RESOURCES})

add_library(shell_folder-com_dll SHARED ${SOURCES})

hunter_add_package(WTL)
find_package(WTL CONFIG REQUIRED)

target_link_libraries(shell_folder-com_dll
  PRIVATE shell_folder versions WTL::WTL) # WTL needed for atlres.h

install(TARGETS shell_folder-com_dll RUNTIME DESTINATION .)

# This step is a dev-time convenience and not guaranteed to produce the same
# result as the installer
add_custom_target(RegisterDllFromBuildTree
  COMMAND regsvr32 $<TARGET_FILE:shell_folder-com_dll>
  COMMENT "Registering Swish DLL from build tree" VERBATIM)
