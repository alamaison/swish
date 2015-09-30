/* Copyright (C) 2015  Alexander Lamaison <swish@lammy.co.uk>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation, either version 3 of the License, or (at your
   option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SWISH_SHELL_PARENT_AND_ITEM_HPP
#define SWISH_SHELL_PARENT_AND_ITEM_HPP

#include <washer/shell/pidl.hpp>

#include <comet/ptr.h>

#include <shobjidl.h>

namespace comet {

template<> struct comtype<IParentAndItem>
{
    static const IID& uuid() throw()
    { return IID_IParentAndItem; }
    typedef IUnknown base;
};

template<> struct wrap_t<IParentAndItem>
{
    washer::shell::pidl::apidl_t parent_pidl()
    {
        washer::shell::pidl::apidl_t parent;
        HRESULT hr = raw(this)->GetParentAndItem(parent.out(), NULL, NULL);
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error_from_interface(raw(this), hr));
        return parent;
    }

    washer::shell::pidl::cpidl_t item_pidl()
    {
        washer::shell::pidl::cpidl_t item;
        HRESULT hr = raw(this)->GetParentAndItem(NULL, NULL, item.out());
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error_from_interface(raw(this), hr));
        return item;
    }

    washer::shell::pidl::apidl_t absolute_item_pidl()
    {
        washer::shell::pidl::apidl_t parent;
        washer::shell::pidl::cpidl_t item;
        HRESULT hr = raw(this)->GetParentAndItem(parent.out(), NULL, item.out());
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error_from_interface(raw(this), hr));
        return parent + item;
    }

    comet::com_ptr<IShellFolder> parent_folder()
    {
        comet::com_ptr<IShellFolder> folder;
        HRESULT hr = raw(this)->GetParentAndItem(NULL, folder.out(), NULL);
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error_from_interface(raw(this), hr));
        return folder;
    }
};

}

#endif
