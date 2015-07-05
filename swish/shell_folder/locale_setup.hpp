/**
    @file

    Static setup for Boost.Locale.

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

#ifndef SWISH_SHELL_FOLDER_LOCALE_SETUP_HPP
#define SWISH_SHELL_FOLDER_LOCALE_SETUP_HPP
#pragma once

#include "swish/atl.hpp"

#include <washer/dynamic_link.hpp> // module_path

#include <boost/filesystem.hpp> // path
#include <boost/locale.hpp> // boost::locale::generator

namespace swish {
namespace shell_folder {

namespace detail {

    /**
     * Initialise Boost.Locale translation mechanism.
     */
    inline std::locale switch_to_boost_locale()
    {
        using boost::filesystem::path;
        using boost::locale::generator;

        try
        {
            generator gen;

            path module_directory = washer::module_path<char>(
                ATL::_AtlBaseModule.GetModuleInstance()).parent_path();
            gen.add_messages_path(module_directory.string());

            gen.add_messages_domain("swish");

            return std::locale::global(gen("")); // default locale
        }
        catch (std::exception)
        {
            // fall-back
            return std::locale::global(std::locale::classic());
        }
    }
}

/**
 * Switch Boost.Locale for the duration of this module's existence.
 *
 * Resets locale to original when the module is unloaded.
 */
class LocaleSetup
{
public:
    LocaleSetup() : m_old_locale(detail::switch_to_boost_locale()) {}
    ~LocaleSetup()
    {
        try
        {
            std::locale::global(m_old_locale);
        }
        catch (std::exception)
        {
            // fall-back
            std::locale::global(std::locale::classic());
        }
    }

private:
    std::locale m_old_locale;
};

}} // namespace swish::shell_folder

#endif
