/**
    @file

    Command message dispatch.

    @if license

    Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    @endif
*/

#ifndef EZEL_COMMAND_DISPATCH_HPP
#define EZEL_COMMAND_DISPATCH_HPP
#pragma once

#include <winapi/gui/commands.hpp> // command

#ifndef BOOST_MPL_LIMIT_VECTOR_SIZE
#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#endif

#ifndef BOOST_MPL_LIMIT_VECTOR_SIZE
#define BOOST_MPL_LIMIT_VECTOR_SIZE 50
#endif

#include <boost/mpl/begin_end.hpp> // begin, end
#include <boost/mpl/copy_if.hpp> // copy_if
#include <boost/mpl/not_equal_to.hpp> // not_equal_to
#include <boost/mpl/vector_c.hpp> // vector_c
#include <boost/type_traits/is_same.hpp> // is_same

#define EZEL_COMMAND_MAP_TEMPLATE \
    int EZC0=-1, int EZC1=-1, int EZC2=-1, int EZC3=-1, int EZC4=-1, \
    int EZC5=-1, int EZC6=-1, int EZC7=-1, int EZC8=-1, int EZC9=-1, \
    int EZC10=-1, int EZC11=-1, int EZC12=-1, int EZC13=-1, int EZC14=-1, \
    int EZC15=-1, int EZC16=-1, int EZC17=-1, int EZC18=-1, int EZC19=-1, \
    int EZC20=-1, int EZC21=-1, int EZC22=-1, int EZC23=-1, int EZC24=-1, \
    int EZC25=-1, int EZC26=-1, int EZC27=-1, int EZC28=-1, int EZC29=-1, \
    int EZC30=-1, int EZC31=-1, int EZC32=-1, int EZC33=-1, int EZC34=-1, \
    int EZC35=-1, int EZC36=-1, int EZC37=-1, int EZC38=-1, int EZC39=-1, \
    int EZC40=-1, int EZC41=-1, int EZC42=-1, int EZC43=-1, int EZC44=-1, \
    int EZC45=-1, int EZC46=-1, int EZC47=-1, int EZC48=-1, int EZC49=-1

#define EZEL_COMMAND_ARG \
    EZC0, EZC1, EZC2, EZC3, EZC4, \
    EZC5, EZC6, EZC7, EZC8, EZC9, \
    EZC10, EZC11, EZC12, EZC13, EZC14, \
    EZC15, EZC16, EZC17, EZC18, EZC19, \
    EZC20, EZC21, EZC22, EZC23, EZC24, \
    EZC25, EZC26, EZC27, EZC28, EZC29, \
    EZC30, EZC31, EZC32, EZC33, EZC34, \
    EZC35, EZC36, EZC37, EZC38, EZC39, \
    EZC40, EZC41, EZC42, EZC43, EZC44, \
    EZC45, EZC46, EZC47, EZC48, EZC49

namespace ezel {
namespace detail {

template<typename T>
inline void dispatch_command(T* obj, WORD id, WPARAM wparam, LPARAM lparam);

template<typename It, typename End, typename T>
inline void dispatch_command_next(
    T* obj, WORD /*id*/, WPARAM wparam, LPARAM lparam, boost::mpl::true_)
{
    obj->on(winapi::gui::command_base(wparam, lparam));
}

template<typename It, typename End, typename T>
inline void dispatch_command_next(
    T* obj, WORD id, WPARAM wparam, LPARAM lparam, boost::mpl::false_)
{
    dispatch_command<T::super>(obj, id, wparam, lparam);
}

/**
 * Dispatch conditionally based on whether we're at end of superclass chain.
 *
 * If we are, this command goes to the default command handler.  Otherwise,
 * we create a dispatch for the superclasses command map and delegate
 * dispatch there.
 */
template<typename It, typename End, typename T>
inline void dispatch_command(
    T* obj, WORD command_id, WPARAM wparam, LPARAM lparam, boost::mpl::true_)
{
    return dispatch_command_next<It, End>(
        obj, command_id, wparam, lparam, boost::is_same<T, T::super>::type());
}

template<typename It, typename End, typename T>
inline void dispatch_command(
    T* obj, WORD command_id, WPARAM wparam, LPARAM lparam, boost::mpl::false_)
{
    typedef boost::mpl::deref<It>::type Front;
    typedef boost::mpl::next<It>::type Next;

    if(command_id == Front::value)
    {
        return obj->on(command<Front::value>(wparam, lparam));
    }
    else
    {
        return dispatch_command<Next, End>(
            obj, command_id, wparam, lparam,
            typename boost::is_same<Next, End>::type());
    }
}

/**
 * Command dispatcher.
 *
 * Messages are dispatched to the superclasses of T one at a time until one is
 * found whose command map contains the current command.  Then its handler
 * for that meassage is invoked.
 * If we reach the end of the chain without finding a matching map entry,
 * the command is delivered to the default command handler.
 */
template<typename T>
inline void dispatch_command(
    T* obj, WORD command_id, WPARAM wparam, LPARAM lparam)
{
    typedef boost::mpl::begin<T::commands::commands>::type Begin;
    typedef boost::mpl::end<T::commands::commands>::type End;

    dispatch_command<Begin, End>(
        obj, command_id, wparam, lparam,
        typename boost::is_same<Begin, End>::type());
}

template<EZEL_COMMAND_MAP_TEMPLATE>
class command_map
{
public:

    // -1 is used to indicate an unused template parameter.  To
    // prevent us having to treat -1 as a special case, we filter it out of
    // the command map immediately here
    typedef typename boost::mpl::copy_if<
        boost::mpl::vector_c<int, EZEL_COMMAND_ARG>,
        boost::mpl::not_equal_to< boost::mpl::_1, boost::mpl::int_<-1> >,
        boost::mpl::back_inserter< boost::mpl::vector_c<WORD> >
    >::type commands;
};

}} // namespace winapi::gui

#endif
