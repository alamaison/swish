/**
    @file

    Message dispatch.

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

#ifndef EZEL_MESSAGE_DISPATCH_HPP
#define EZEL_MESSAGE_DISPATCH_HPP
#pragma once

#include <washer/gui/messages.hpp> // message

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

#define EZEL_MESSAGE_MAP_TEMPLATE \
    UINT EZM0=0, UINT EZM1=0, UINT EZM2=0, UINT EZM3=0, UINT EZM4=0, \
    UINT EZM5=0, UINT EZM6=0, UINT EZM7=0, UINT EZM8=0, UINT EZM9=0, \
    UINT EZM10=0, UINT EZM11=0, UINT EZM12=0, UINT EZM13=0, UINT EZM14=0, \
    UINT EZM15=0, UINT EZM16=0, UINT EZM17=0, UINT EZM18=0, UINT EZM19=0, \
    UINT EZM20=0, UINT EZM21=0, UINT EZM22=0, UINT EZM23=0, UINT EZM24=0, \
    UINT EZM25=0, UINT EZM26=0, UINT EZM27=0, UINT EZM28=0, UINT EZM29=0, \
    UINT EZM30=0, UINT EZM31=0, UINT EZM32=0, UINT EZM33=0, UINT EZM34=0, \
    UINT EZM35=0, UINT EZM36=0, UINT EZM37=0, UINT EZM38=0, UINT EZM39=0, \
    UINT EZM40=0, UINT EZM41=0, UINT EZM42=0, UINT EZM43=0, UINT EZM44=0, \
    UINT EZM45=0, UINT EZM46=0, UINT EZM47=0, UINT EZM48=0, UINT EZM49=0

#define EZEL_MESSAGE_ARG \
    EZM0, EZM1, EZM2, EZM3, EZM4, \
    EZM5, EZM6, EZM7, EZM8, EZM9, \
    EZM10, EZM11, EZM12, EZM13, EZM14, \
    EZM15, EZM16, EZM17, EZM18, EZM19, \
    EZM20, EZM21, EZM22, EZM23, EZM24, \
    EZM25, EZM26, EZM27, EZM28, EZM29, \
    EZM30, EZM31, EZM32, EZM33, EZM34, \
    EZM35, EZM36, EZM37, EZM38, EZM39, \
    EZM40, EZM41, EZM42, EZM43, EZM44, \
    EZM45, EZM46, EZM47, EZM48, EZM49

namespace ezel {
namespace detail {

template<typename T>
inline LRESULT dispatch_message_next(
    T* obj, UINT message_id, WPARAM wparam, LPARAM lparam);

template<typename It, typename End, typename T>
inline LRESULT dispatch_message_next(
    T* obj, UINT message_id, WPARAM wparam, LPARAM lparam, boost::mpl::true_)
{
    return obj->default_message_handler(message_id, wparam, lparam);
}

template<typename It, typename End, typename T>
inline LRESULT dispatch_message_next(
    T* obj, UINT message_id, WPARAM wparam, LPARAM lparam, boost::mpl::false_)
{
    return dispatch_message<T::super>(obj, message_id, wparam, lparam);
}

/**
 * Dispatch conditionally based on whether we're at end of superclass chain.
 *
 * If we are, this message goes to the default message handler.  Otherwise,
 * we create a dispatch for the superclasses message map and delegate
 * dispatch there.
 */
template<typename It, typename End, typename T>
inline LRESULT dispatch_message(
    T* obj, UINT message_id, WPARAM wparam, LPARAM lparam, boost::mpl::true_)
{
    return dispatch_message_next<It, End>(
        obj, message_id, wparam, lparam, boost::is_same<T, T::super>::type());
}

template<typename It, typename End, typename T>
inline LRESULT dispatch_message(
    T* obj, UINT message_id, WPARAM wparam, LPARAM lparam, boost::mpl::false_)
{
    typedef boost::mpl::deref<It>::type Front;
    typedef boost::mpl::next<It>::type Next;

    if(message_id == Front::value)
    {
        return obj->on(message<Front::value>(wparam, lparam));
    }
    else
    {
        return dispatch_message<Next, End>(
            obj, message_id, wparam, lparam,
            typename boost::is_same<Next, End>::type());
    }
}

/**
 * Main message handler.
 *
 * Messages are dispatched to the superclasses of T one at a time until one is
 * found whose message map contains the current message.  Then its handler for
 * that meassage is invoked.  If we reach the end of the chain without finding
 * a matching map entry, the message is delivered to the default message
 * handler.
 */
template<typename T>
inline LRESULT dispatch_message(
    T* obj, UINT message_id, WPARAM wparam, LPARAM lparam)
{
    typedef boost::mpl::begin<T::messages::messages>::type Begin;
    typedef boost::mpl::end<T::messages::messages>::type End;

    return dispatch_message<Begin, End>(
        obj, message_id, wparam, lparam,
        typename boost::is_same<Begin, End>::type());
}

template<EZEL_MESSAGE_MAP_TEMPLATE>
class message_map
{
public:

    // Zero (0) is used to indicate an unused template parameter.  To
    // prevent us having to treat 0 as a special case, we filter it out of
    // the message map immediately here
    typedef typename boost::mpl::copy_if<
        boost::mpl::vector_c<UINT, EZEL_MESSAGE_ARG>,
        boost::mpl::not_equal_to< boost::mpl::_1, boost::mpl::int_<0> >,
        boost::mpl::back_inserter< boost::mpl::vector_c<UINT> >
    >::type messages;
};

}} // namespace washer::gui

#endif
