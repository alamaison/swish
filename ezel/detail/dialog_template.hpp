/**
    @file

    Windows dialog in-memory template helpers

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

#ifndef EZEL_DETAIL_DIALOG_TEMPLATE_HPP
#define EZEL_DETAIL_DIALOG_TEMPLATE_HPP
#pragma once

#include <ezel/detail/window_impl.hpp> // window_impl

#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/static_assert.hpp> // BOOST_STATIC_ASSERT

#include <algorithm> // copy
#include <string>
#include <vector>

namespace ezel {
namespace detail {

#pragma warning(push)
#pragma warning(disable: 4996) // std::copy: Function call that may be unsafe

// the calculations rely on pointer-arithmetic on wchar_t pointers
// skipping 16-bit (WORD-sized) fields
BOOST_STATIC_ASSERT(sizeof(wchar_t) == 2); // 16-bits
BOOST_STATIC_ASSERT(sizeof(wchar_t) == sizeof(WORD));

/**
 * If the pointer is already DWORD-aligned return it unchanged otherwise
 * advance to the next DWORD boundary.
 */
template<typename T>
inline T* next_double_word(T* p)
{
    return reinterpret_cast<T*>((reinterpret_cast<size_t>(p) + 3) & ~3);
}

/**
 * If the pointer is already WORD-aligned return it unchanged otherwise
 * advance to the next WORD boundary.
 */
template<typename T>
inline T* next_word(T* p)
{
    return reinterpret_cast<T*>((reinterpret_cast<size_t>(p) + 1) & ~1);
}

/**
 * Calculate the necessary buffer size for a dialog template.
 *
 * Apparently all the fields are naturally WORD-aligned so we don't need to 
 * force it explicitly
 * (@see http://msdn.microsoft.com/en-us/library/ms644996%28VS.85%29.aspx in
 * the user contrib comments) but we do so anyway just to be on the safe side.
 */
inline size_t calculate_template_size(
    const std::wstring& title, const std::wstring& font)
{
    DLGTEMPLATE* dlg = 0;
                              // template
    wchar_t* pos = reinterpret_cast<wchar_t*>(dlg + 1);

    pos = next_word(pos) + 1; // menu (1)
    pos = next_word(pos) + 1; // window class (1)

                              // title (?) + terminator (1)
    wchar_t* title_array = next_word(pos);
    title_array = title_array + title.size() + 1;

                              // fontsize (1)
    pos = next_word(title_array);
    pos = pos + 1;

                              // font (?) + terminator (1)
    wchar_t* font_array = next_word(pos);
    font_array = next_word(font_array) + font.size() + 1;

                              // padding
    return reinterpret_cast<size_t>(next_double_word(font_array));
}

/**
 * Write the DLGTEMPLATE part of a Dialog template.
 *
 * Assumes the buffer is big enough to fit the data.  Use
 * calculate_template_size to calculate the nescessary size.
 */
inline DLGITEMTEMPLATE* write_template_to_buffer(
    const std::wstring& title, short font_size, const std::wstring& font,
    short left, short top, short width, short height, size_t control_count,
    DLGTEMPLATE* dlg)
{
    // dlg buffer must already be DWORD aligned
    assert((((int)dlg) % sizeof(DWORD)) == 0);
    
    dlg->style = DS_SETFONT | WS_VISIBLE | WS_POPUPWINDOW | DS_MODALFRAME;
    if (!title.empty())
        dlg->style |= WS_CAPTION;

    dlg->cx = width;
    dlg->cy = height;
    dlg->x = left;
    dlg->y = top;

    dlg->cdit = boost::numeric_cast<WORD>(control_count);

    wchar_t* pos = reinterpret_cast<wchar_t*>(dlg + 1);
    pos = next_word(pos);
    *pos++ = 0; // no menu

    pos = next_word(pos);
    *pos++ = 0; // default dialog window class

    // caption
    wchar_t* title_array = next_word(pos);
    title_array = std::copy(title.begin(), title.end(), title_array);
    *title_array++ = 0; // null-terminate

    // font size
    pos = next_word(title_array);
    *pos++ = font_size;

    // font name
    wchar_t* font_array = next_word(pos);
    font_array = std::copy(font.begin(), font.end(), font_array);
    *font_array++ = 0; // null-terminate

    return reinterpret_cast<DLGITEMTEMPLATE*>(next_double_word(font_array));
}

/**
 * Calculate the necessary buffer size for an item template.
 *
 * Apparently all the fields are naturally WORD-aligned so we don't need to 
 * force it explicitly
 * (@see http://msdn.microsoft.com/en-us/library/ms644996%28VS.85%29.aspx in
 * the user contrib comments) but we do so anyway just to be on the safe side.
 *
 * After the custom data field we have to add en extra WORD of buffer *not*
 * including any extra needed for DWORD-alignment.  This does not match the
 * MSDN documentation but is required.
 */
template<typename Customdata>
inline size_t calculate_control_template_size(
    const std::wstring& window_class, const std::wstring& title,
    size_t current_buffer_size)
{
                              // item template
    DLGITEMTEMPLATE* item =
        reinterpret_cast<DLGITEMTEMPLATE*>(current_buffer_size);
    item = next_double_word(item) + 1;

                              // class (?) + terminator (1)
    wchar_t* class_array = reinterpret_cast<wchar_t*>(item);
    class_array = next_word(class_array) + window_class.size() + 1;

                              // title (?) + terminator (1)
    wchar_t* title_array = class_array + 1;
    title_array = next_word(title_array) + title.size() + 1;

                     // custom data size (1) + custom data (?) + extra WORD (1)
    wchar_t* custom_data_size = title_array + 1;
    custom_data_size = next_word(custom_data_size);
    Customdata* data = reinterpret_cast<Customdata*>(custom_data_size + 1);
    data++;
    wchar_t* mystery_extra = reinterpret_cast<wchar_t*>(data);
    mystery_extra++;

    return reinterpret_cast<size_t>(next_double_word(mystery_extra));
}

/**
 * Write a DLGITEMTEMPLATE entry for a dialog control.
 *
 * Assumes the buffer is big enough to fit the data.  Use
 * calculate_item_template_size to calculate the nescessary size.
 */
template<typename CustomData>
inline DLGITEMTEMPLATE* write_control_to_buffer(
    const std::wstring& window_class, const std::wstring& title,
    unsigned short id, DWORD style,
    short width, short height, short left, short top, 
    const CustomData& custom_data, DLGITEMTEMPLATE* buffer)
{
    DLGITEMTEMPLATE* item = next_double_word(buffer);

    item->style = style;

    item->id = id;

    item->cx = width;
    item->cy = height;
    item->x = left;
    item->y = top;

    item = item + 1; // skip over template

    // control window class name
    wchar_t* class_array = reinterpret_cast<wchar_t*>(next_word(item));
    class_array = std::copy(
        window_class.begin(), window_class.end(), class_array);
    *class_array++ = 0; // null-terminate

    // title
    wchar_t* title_array = next_word(class_array);
    title_array = std::copy(title.begin(), title.end(), title_array);
    *title_array++ = 0; // null-terminate

    // custom data size must include its own memory in the size value
    wchar_t* custom_data_size = next_word(title_array);
    *custom_data_size++ = sizeof(wchar_t) + sizeof(CustomData);

    CustomData* data = reinterpret_cast<CustomData*>(custom_data_size);
    *data++ = custom_data;

    wchar_t* mystery_extra = reinterpret_cast<wchar_t*>(data);
    mystery_extra++;

    return reinterpret_cast<DLGITEMTEMPLATE*>(next_double_word(mystery_extra));
}

/**
 * The required buffer size to store the window as a control in a
 * dialog template.
 */
inline size_t increment_required_buffer_size(
    const window_impl* w, size_t current_buffer_size)
{
    return calculate_control_template_size<window_impl*>(
        w->window_class(), w->text(), current_buffer_size);
}

/**
 * Write the window to a byte buffer as a control in a dialog template.
 */
inline DLGITEMTEMPLATE* to_buffer(
    window_impl* w, unsigned short id, DLGITEMTEMPLATE* buffer)
{
    return write_control_to_buffer(
        w->window_class(), w->text(), id, w->style(),
        w->width(), w->height(), w->left(), w->top(),
        w, buffer);
}

const unsigned short BUTTON_ID_OFFSET = 100;

/**
 * Build a dialog resource template in memory.
 */
inline std::vector<unsigned char> build_dialog_template_in_memory(
    const std::wstring& font, short font_size, const std::wstring& title,
    short width, short height, short left, short top,
    const std::vector<boost::shared_ptr<window_impl> >& controls)
{
    size_t buffer_len = calculate_template_size(title, font);
    BOOST_FOREACH(boost::shared_ptr<window_impl> w, controls)
    {
        buffer_len = increment_required_buffer_size(w.get(), buffer_len);
    }

    std::vector<unsigned char> buffer(buffer_len, 0);

    DLGITEMTEMPLATE* pos = write_template_to_buffer(
        title, font_size, font, width, height, left, top,
        controls.size(), reinterpret_cast<DLGTEMPLATE*>(&buffer[0]));

    for (size_t i = 0; i < controls.size(); ++i)
    {
        window_impl* w = controls[i].get();

        // offset ID to avoid collision with dialog manager's 
        // 'special' button IDs
        unsigned short id = boost::numeric_cast<unsigned short>(
            i + BUTTON_ID_OFFSET);

        pos = to_buffer(
            w, id, reinterpret_cast<DLGITEMTEMPLATE*>(pos));
    }

    return buffer;
}

#pragma warning(pop)

}} // namespace ezel::detail

#endif
