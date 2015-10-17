/**
    @file

    SSH SFTP path.

    @if license

    Copyright (C) 2015  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

    In addition, as a special exception, the the copyright holders give you
    permission to combine this program with free software programs or the
    OpenSSL project's "OpenSSL" library (or with modified versions of it,
    with unchanged license). You may copy and distribute such a system
    following the terms of the GNU GPL for this program and the licenses
    of the other code concerned. The GNU General Public License gives
    permission to release a modified version without this exception; this
    exception also makes it possible to release a modified version which
    carries forward this exception.

    @endif
*/

#ifndef SSH_PATH_HPP
#define SSH_PATH_HPP

#include <boost/iterator/transform_iterator.hpp>
#include <boost/locale.hpp> // utf_to_utf
#include <boost/operators.hpp>
#include <boost/token_iterator.hpp> // token_iterator_generator, char_separator

#include <ostream>
#include <string>
#include <utility> // swap

namespace ssh {
namespace filesystem {

namespace detail {

/**
 * String tokeniser that separates on /, unless it is leading or trailing.
 *
 * The filesystem concept treats leading and trailing directory separators (/)
 * specially.  A leading separator is the root dirctory and is kept as a token.
 * A trailing separator is a directory path indicator and causes a dot token (.)
 * to be emitted.
 */
template<typename Value>
class all_but_first_and_last_separator
{
public:
    all_but_first_and_last_separator() :
        m_at_beginning(true), m_normal_separator("/") {}

    void reset()
    {
        all_but_first_and_last_separator<Value> fresh;
        std::swap(*this, fresh);
    }

    template<typename InputIterator, typename Token>
    bool operator()(InputIterator& next, InputIterator& end, Token& token_out)
    {
        if (m_at_beginning)
        {
            m_at_beginning = false;
            if (next != end && *next == '/')
            {
                token_out = "/";
                ++next;
                return true;
            }
            else
            {
                return m_normal_separator(next, end, token_out);
            }
        }
        else
        {
            if (next != end && next + 1 == end && *next == '/')
            {
                token_out = ".";
                ++next;
                return true;
            }
            else
            {
                return m_normal_separator(next, end, token_out);
            }
        }
    }

private:
    boost::char_separator<Value> m_normal_separator;
    bool m_at_beginning;
};


// Because the iterators produce paths, std::lexicograpical_compare will not
// terminate. Therefore we have our own that works on the string form of the
// path in each segment
template<typename Iterator>
inline int lexical_compare(
    Iterator lhs, const Iterator& lhs_end,
    Iterator rhs, const Iterator& rhs_end)
{
    while (lhs != lhs_end && rhs != rhs_end)
    {
        int comparison = lhs->native().compare(rhs->native());
        if (comparison == 0)
        {
            ++lhs;
            ++rhs;
            continue;
        }
        else
        {
            return comparison;
        }
    }

    if (lhs != lhs_end)
    {
        return 1;
    }
    else if (rhs != rhs_end)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

template<typename Destination, typename Source>
inline Destination convert(const Source& source)
{
    return boost::locale::conv::utf_to_utf<Destination::value_type>(
        source, boost::locale::conv::stop);
}

}


class path : boost::totally_ordered<path>
{
public:
    typedef std::string string_type;
    typedef string_type::value_type value_type;

private:
    typedef detail::all_but_first_and_last_separator<value_type> separator_type_;

    static const separator_type_& segment_separator()
    {
        static separator_type_ separator;
        return separator;
    }

    typedef boost::token_iterator_generator<separator_type_>::type token_iterator;

    struct path_factory
    {
        typedef path result_type;
        result_type operator()(const token_iterator::value_type& source) const
        {
            return result_type(source);
        }
    };

    token_iterator begin_tokens() const
    {
        return boost::make_token_iterator<string_type>(
            m_path.begin(), m_path.end(), segment_separator());
    }

    token_iterator end_tokens() const
    {
        return boost::make_token_iterator<string_type>(
            m_path.end(), m_path.end(), segment_separator());
    }

public:
    typedef boost::transform_iterator<path_factory, token_iterator> iterator;

private:

public:

    path() {}
    explicit path(const std::wstring& source)
        : m_path(detail::convert<string_type>(source)) {}
    explicit path(const string_type& source) : m_path(source) {}

    bool is_relative() const
    {
        return !is_absolute();
    }

    bool is_absolute() const
    {
        return !empty() && m_path[0] == '/';
    }

    bool empty() const
    {
        return m_path.empty();
    }

    string_type native() const
    {
        return m_path;
    }

    operator string_type() const
    {
        return native();
    }

    std::string u8string() const
    {
        return native();
    }

    std::wstring wstring() const
    {
        return detail::convert<std::wstring>(m_path);
    }

    int compare(const path& rhs) const
    {
        return detail::lexical_compare(begin(), end(), rhs.begin(), rhs.end());
    }

    iterator begin() const
    {
        return iterator(begin_tokens());
    }

    iterator end() const
    {
        return iterator(end_tokens());
    }

    path& operator/=(const path& rhs)
    {
        if (!empty())
        {
            m_path = m_path + '/' + rhs.m_path;
        }
        else
        {
            m_path = rhs.m_path;
        }
        return *this;
    }

    template<typename Source>
    path& operator/=(const Source& rhs)
    {
        return (*this) /= path(rhs);
    }

private:
    // IMPORTANT: The encoding of this path is UTF8, which is not necessarily
    // the default encoding for strings of string_type
    string_type m_path;
};

std::ostream& operator<<(std::ostream& stream, const path& p)
{
    // TODO: quote string so it can roundtrip
    stream << p.native();
    return stream;
}

bool operator==(const path& lhs, const path& rhs)
{
    return lhs.compare(rhs) == 0;
}

bool operator<(const path& lhs, const path& rhs)
{
    return lhs.compare(rhs) < 0;
}

template<typename Source>
path operator/(const path& lhs, const Source& rhs)
{
    path concatenation(lhs);
    concatenation /= rhs;
    return concatenation;
}

}} // namespace ssh::filesystem

#endif
