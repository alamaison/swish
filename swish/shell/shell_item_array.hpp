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

#ifndef SWISH_SHELL_SHELL_ITEM_ARRAY_HPP
#define SWISH_SHELL_SHELL_ITEM_ARRAY_HPP

#include <washer/shell/pidl_array.hpp>

#include <comet/enum_iterator.h>
#include <comet/error.h>
#include <comet/ptr.h>

#include <boost/numeric/conversion/cast.hpp> // numeric_cast

#include <shobjidl.h>

namespace comet {

template<> struct comtype<IShellItemArray>
{
    static const IID& uuid() throw() { return IID_IShellItemArray; }
    typedef IUnknown base;
};

template<> struct comtype<IShellItem>
{
    static const IID& uuid() throw() { return IID_IShellItem; }
    typedef IUnknown base;
};

template<> struct comet::impl::type_policy<IShellItem*>
{
    static void init(IShellItem*& destination, IShellItem* source)
    {
        destination = source;
        destination->AddRef();
    }

    static void clear(IShellItem*& p)
    {
        p->Release();
    }
};


template<> struct enumerated_type_of<IEnumShellItems>
{
    typedef IShellItem* is;
};

template<> struct wrap_t<IShellItemArray>
{
    typedef comet::enum_iterator<IEnumShellItems> iterator_type;

    size_t size()
    {
        DWORD array_size = 0;
        HRESULT hr = raw(this)->GetCount(&array_size);
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error_from_interface(raw(this), hr));

        return array_size;
    }

    comet::com_ptr<IShellItem> at(size_t index)
    {
        comet::com_ptr<IShellItem> item;
        HRESULT hr = raw(this)->GetItemAt(
            boost::numeric_cast<DWORD>(index), item.out());
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error_from_interface(raw(this), hr));

        return item;
    }

    comet::com_ptr<IShellItem> operator[](size_t index)
    {
        return at(index);
    }

    iterator_type begin()
    {
        com_ptr<IEnumShellItems> enumerator;
        HRESULT hr = raw(this)->EnumItems(enumerator.out());
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error_from_interface(raw(this), hr));
        return iterator_type(enumerator);
    }

    iterator_type end()
    {
        return iterator_type();
    }
};

}

namespace swish {
namespace shell {

inline comet::com_ptr<IShellItemArray> shell_item_array_from_folder_items(
    comet::com_ptr<IShellFolder> parent_folder,
    UINT pidl_count, PCUITEMID_CHILD_ARRAY item_pidls)
{
    comet::com_ptr<IShellItemArray> item_array;
    // Not passing the folder PIDL, so this relies on the folder
    // implementing IPersistFolder2
    HRESULT hr = ::SHCreateShellItemArray(
        NULL, parent_folder.in(), pidl_count, item_pidls,
        item_array.out());
    if (FAILED(hr))
        BOOST_THROW_EXCEPTION(comet::com_error(hr));

    return item_array;
}

inline comet::com_ptr<IShellItemArray> shell_item_array_from_folder_pidl_and_items(
    PCIDLIST_ABSOLUTE parent_folder_pidl,
    UINT pidl_count, PCUITEMID_CHILD_ARRAY item_pidls)
{
    comet::com_ptr<IShellItemArray> item_array;
    HRESULT hr = ::SHCreateShellItemArray(
        parent_folder_pidl, NULL, pidl_count, item_pidls,
        item_array.out());
    if (FAILED(hr))
        BOOST_THROW_EXCEPTION(comet::com_error(hr));

    return item_array;
}

inline comet::com_ptr<IShellItemArray> shell_item_array_from_pidls(
    UINT cidl, PCIDLIST_ABSOLUTE_ARRAY rgpidl)
{
    comet::com_ptr<IShellItemArray> item_array;
    HRESULT hr = ::SHCreateShellItemArrayFromIDLists(
        cidl, rgpidl, item_array.out());
    if (FAILED(hr))
        BOOST_THROW_EXCEPTION(comet::com_error(hr));

    return item_array;
}

inline comet::com_ptr<IShellItemArray> shell_item_array_from_data_object(
    comet::com_ptr<IDataObject> data_object)
{
    comet::com_ptr<IShellItemArray> item_array;
    HRESULT hr = ::SHCreateShellItemArrayFromDataObject(
        data_object.in(), item_array.iid(),
        reinterpret_cast<void**>(item_array.out()));
    if (FAILED(hr))
        BOOST_THROW_EXCEPTION(comet::com_error(hr));

    return item_array;
}

}}

#endif
