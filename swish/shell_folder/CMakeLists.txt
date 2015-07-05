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

set(GENERATED_SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/shell_folder.dir/${CMAKE_CFG_INTDIR}/Swish.h
  ${CMAKE_CURRENT_BINARY_DIR}/shell_folder.dir/${CMAKE_CFG_INTDIR}/Swish_i.c)
set_source_files_properties(${GENERATED_SOURCES} PROPERTIES GENERATED TRUE)

set(SOURCES
  DataObject.h
  Folder.h
  IconExtractor.h
  KbdInteractiveDialog.h
  HostFolder.h
  locale_setup.hpp
  Pidl.h
  Registry.h
  RemoteFolder.h
  resource.h
  SftpDataObject.h
  SftpDirectory.h
  shell.hpp
  SnitchingDataObject.hpp
  SwishFolder.hpp
  wtl.hpp
  DataObject.cpp
  HostFolder.cpp
  IconExtractor.cpp
  KbdInteractiveDialog.cpp
  Registry.cpp
  RemoteFolder.cpp
  SftpDataObject.cpp
  SftpDirectory.cpp
  shell.cpp
  data_object/FileGroupDescriptor.hpp
  data_object/GlobalLocker.hpp
  data_object/ShellDataObject.hpp
  data_object/ShellDataObject.cpp
  data_object/StorageMedium.hpp
  Swish.idl
  ${GENERATED_SOURCES}
  shell_folder.rc)

add_library(shell_folder ${SOURCES})

# This exposes the include path of the directory that CMake tells midl
# to output the header file into.  Ideally, it would be
# "${CMAKE_CURRENT_BINARY_DIR}/\$(IntDir)", but that is no good if
# used by another project because $(IntDir) expands to the
# intermediate directory of that project, not this one.
target_include_directories(shell_folder
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/shell_folder.dir/${CMAKE_CFG_INTDIR})

hunter_add_package(Comet)
hunter_add_package(Washer)

find_package(Comet REQUIRED CONFIG)
find_package(Washer REQUIRED CONFIG)

target_link_libraries(shell_folder
  PRIVATE host_folder remote_folder frontend nse drop_target
  urlmon ${Boost_LIBRARIES}
  PUBLIC Washer::washer Comet::comet)