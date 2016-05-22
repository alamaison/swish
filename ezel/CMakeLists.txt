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
  detail/command_dispatch.hpp
  detail/dialog_template.hpp
  detail/hooks.hpp
  detail/hwnd_linking.hpp
  detail/message_dispatch.hpp
  detail/window_impl.hpp
  detail/window_link.hpp
  detail/window_proc.hpp
  detail/window_proxy.hpp
  controls/button.hpp
  controls/checkbox.hpp
  controls/edit.hpp
  controls/icon.hpp
  controls/label.hpp
  controls/line.hpp
  controls/spinner.hpp
  control.hpp
  control_parent_impl.hpp
  form.hpp
  window.hpp)

add_custom_target(ezel-src SOURCES ${SOURCES})
add_library(ezel INTERFACE)
target_include_directories(ezel INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
# Boost::serialization is really a dependency of washer (trace.hpp uses
# boost/archive/iterators/mb_from_wchar.hpp), but until the dependency is fixed
# there, we can work around it here.
target_link_libraries(ezel INTERFACE Boost::boost Boost::signals Boost::serialization)
