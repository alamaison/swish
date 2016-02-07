// Copyright 2015, 2016 Alexander Lamaison

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef SSH_FILESYSTEM_PATH_HPP
#define SSH_FILESYSTEM_PATH_HPP

#include <boost/algorithm/string.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/locale/encoding.hpp> // to_utf
#include <boost/locale/generator.hpp>
#include <boost/locale/util.hpp> // get_system_locale
#include <boost/operators.hpp>
#include <boost/throw_exception.hpp>

#include <locale>
#include <ostream>
#include <stdexcept> // logic_error
#include <string>
#include <utility> // swap

namespace ssh
{
namespace filesystem
{

namespace detail
{

/**
 * String tokeniser that separates on /, unless it is leading or trailing.
 *
 * The filesystem concept treats leading and trailing directory separators (/)
 * specially.  A leading separator is the root dirctory and is kept as a token.
 * A trailing separator is a directory path indicator and causes a dot token (.)
 * to be emitted.
 */

// Because the iterators produce paths, std::lexicograpical_compare will not
// terminate. Therefore we have our own that works on the string form of the
// path in each segment
template <typename Iterator>
inline int lexical_compare(Iterator lhs, const Iterator& lhs_end, Iterator rhs,
                           const Iterator& rhs_end)
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

template <typename StringType>
inline typename StringType::size_type
find_next_slash(const StringType& string,
                typename StringType::size_type starting_position)
{
    return string.find('/', starting_position);
}

template <typename StringType>
inline typename StringType::size_type
find_previous_slash(const StringType& string,
                    typename StringType::size_type starting_position)
{
    return string.rfind('/', starting_position);
}

template <typename StringType>
inline typename StringType::size_type
find_next_non_slash(const StringType& string,
                    typename StringType::size_type starting_position)
{
    return string.find_first_not_of('/', starting_position);
}

template <typename StringType>
inline typename StringType::size_type
find_previous_non_slash(const StringType& string,
                        typename StringType::size_type starting_position)
{
    // NOTE: We are implementing what should be rfind_last_not_of. This
    // must already exist somewhere, no?

    // UNOBVIOUS: Because the loop control variable is unsigned and we are
    // looping downwards, the condition has to check for a number larger than
    // the range, not smaller
    for (typename StringType::size_type i = starting_position;
         i < starting_position + 1; --i)
    {
        if (string[i] != '/')
        {
            return i;
        }
    }

    return StringType::npos;
}

inline std::locale utf8_locale()
{
    return boost::locale::generator().generate(
        boost::locale::util::get_system_locale(true));
}

inline std::locale system_locale()
{
    return boost::locale::generator().generate(
        boost::locale::util::get_system_locale(false));
}

inline std::string from_source(const std::string& source)
{
    return source;
}

inline std::string from_source(const std::wstring& source)
{
    return boost::locale::conv::utf_to_utf<char>(source,
                                                 boost::locale::conv::stop);
}

template <typename InputIterator>
inline std::string from_source(const InputIterator& begin,
                               const InputIterator& end)
{
    typedef std::basic_string<
        typename std::iterator_traits<InputIterator>::value_type> string_type;
    string_type source_string(begin, end);
    return from_source(source_string);
}
}

class path : boost::totally_ordered<path>
{
public:
    typedef std::string string_type;
    typedef string_type::value_type value_type;
    class iterator;
    typedef iterator const_iterator;

    path()
    {
    }

    template <typename Source>
    path(const Source& source)
        : m_path(detail::from_source(source))
    {
    }

    template <typename InputIterator>
    path(const InputIterator& begin, const InputIterator& end)
        : m_path(detail::from_source(begin, end))
    {
    }

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

    bool has_parent_path() const
    {
        return !parent_path().empty();
    }

    path parent_path() const;

    bool has_relative_path() const
    {
        return !relative_path().empty();
    }

    path relative_path() const;

    bool has_filename() const
    {
        return !filename().empty();
    }

    path filename() const;

    string_type native() const
    {
        return m_path;
    }

    operator string_type() const
    {
        return native();
    }

    template <typename CharT, typename Traits>
    std::basic_string<CharT, Traits> string() const;

    template <>
    std::string string() const
    {
        return native();
    }

    template <>
    std::wstring string() const
    {
        return boost::locale::conv::utf_to_utf<wchar_t>(m_path);
    }

    std::string u8string() const
    {
        return native();
    }

    std::string string() const
    {
        // Native string is UTF-8 but this method returns string in
        // platform-native encoding
        return from_utf(detail::system_locale());
    }

    std::wstring wstring() const
    {
        return string<std::wstring::value_type, std::wstring::traits_type>();
    }

    int compare(const path& rhs) const;

    iterator begin() const;

    iterator end() const;

    path& operator/=(const path& rhs)
    {
        if (!empty())
        {
            string_type lhs_string = boost::algorithm::trim_right_copy_if(
                m_path, boost::algorithm::is_any_of("/"));
            string_type rhs_string = boost::algorithm::trim_left_copy_if(
                rhs.m_path, boost::algorithm::is_any_of("/"));
            m_path = lhs_string + '/' + rhs_string;
        }
        else
        {
            m_path = rhs.m_path;
        }
        return *this;
    }

    template <typename Source>
    path& operator/=(const Source& rhs)
    {
        return (*this) /= path(rhs);
    }

private:
    template <typename InputIterator>
    static path path_from_range(const InputIterator& begin,
                                const InputIterator& end)
    {
        path p;
        for (InputIterator position = begin; position != end; ++position)
        {
            p /= *position;
        }
        return p;
    }

    std::string from_utf(const std::locale& locale) const
    {
        return boost::locale::conv::from_utf<char>(m_path, locale,
                                                   boost::locale::conv::stop);
    }

    // IMPORTANT: The encoding of this path is UTF8, which is not necessarily
    // the default encoding for strings of string_type
    string_type m_path;
};

class path::iterator
    : public boost::iterator_facade<path::iterator, const path,
                                    boost::bidirectional_traversal_tag>
{
private:
    friend class path;

    explicit iterator(const path* source_path, string_type::size_type position)
        : m_source(source_path),
          m_current_positions(std::make_pair(
              string_type::size_type(position),
              find_element_last_position(m_source->m_path, position))),
          m_current_segment(path(
              element_from_positions(m_source->m_path, m_current_positions)))
    {
    }

    // NOT the end iterator - behaviour undefined
    iterator()
    {
    }

    friend class boost::iterator_core_access;

    bool equal(const iterator& other) const
    {
        return m_source == other.m_source &&
               m_current_positions == other.m_current_positions;
    }

    const path& dereference() const
    {
        if (m_current_positions.first == m_source->m_path.size())
        {
            BOOST_THROW_EXCEPTION(
                std::logic_error("dereferencing past the end of the path"));
        }
        else
        {
            return m_current_segment;
        }
    }

    void increment()
    {
        m_current_positions =
            next_element_positions(m_source->m_path, m_current_positions);
        if (m_current_positions.first != m_source->m_path.size())
        {
            m_current_segment = path(
                element_from_positions(m_source->m_path, m_current_positions));
        }
    }

    void decrement()
    {
        m_current_positions =
            previous_element_positions(m_source->m_path, m_current_positions);
        m_current_segment =
            path(element_from_positions(m_source->m_path, m_current_positions));
    }

    static std::pair<string_type::size_type, string_type::size_type>
    next_element_positions(const string_type& source,
                           std::pair<string_type::size_type,
                                     string_type::size_type> current_positions)
    {
        string_type::size_type new_first =
            find_next_element_first_position(source, current_positions.first);
        string_type::size_type new_last =
            find_element_last_position(source, new_first);
        return std::make_pair(new_first, new_last);
    }

    static std::pair<string_type::size_type, string_type::size_type>
    previous_element_positions(
        const string_type& source,
        std::pair<string_type::size_type, string_type::size_type>
            current_positions)
    {
        string_type::size_type new_first = find_previous_element_first_position(
            source, current_positions.first);
        string_type::size_type new_last =
            find_element_last_position(source, new_first);
        return std::make_pair(new_first, new_last);
    }

    static string_type::size_type
    find_next_element_first_position(const string_type& source,
                                     string_type::size_type current_position)
    {
        if (current_position == source.size())
        {
            BOOST_THROW_EXCEPTION(std::logic_error("already at end of path"));
        }
        else if (current_position == source.size() - 1)
        {
            return source.size();
        }
        else if (source[current_position] == '/')
        {
            if (current_position == 0)
            {
                string_type::size_type next_non_slash_position =
                    detail::find_next_non_slash(source, current_position);
                if (next_non_slash_position == string_type::npos)
                {
                    // Path only contains slashes so we have consumed
                    // everything.
                    // Move off the end to indicate this
                    return source.size();
                }
                else
                {
                    // next segment starts after slash(es)
                    return next_non_slash_position;
                }
            }
            else
            {
                assert(!"position is at slash that isn't leading or trailing");
                BOOST_THROW_EXCEPTION(std::logic_error("unreachable"));
            }
        }
        else
        {
            string_type::size_type next_slash_position =
                detail::find_next_slash(source, current_position);
            if (next_slash_position == string_type::npos)
            {
                // Path ends - no new position
                return source.size();
            }
            else
            {
                string_type::size_type next_non_slash_position =
                    detail::find_next_non_slash(source, next_slash_position);
                if (next_non_slash_position == string_type::npos)
                {
                    // Path ends with trailing slash(es) - slash is new position
                    return next_slash_position;
                }
                else
                {
                    // Normal case - slash found - next segment starts after
                    // slash
                    return next_non_slash_position;
                }
            }
        }
    }

    static string_type::size_type find_previous_element_first_position(
        const string_type& source, string_type::size_type current_position)
    {
        if (current_position == 0)
        {
            BOOST_THROW_EXCEPTION(std::logic_error("already at start of path"));
        }
        else if (current_position == source.size())
        {
            // Because iterators are half-open, we may be at a position past the
            // last character so we have to treat this as a special case
            string_type::size_type previous_slash_position =
                detail::find_previous_slash(source, current_position - 1);
            if (previous_slash_position == string_type::npos)
            {
                // we ran off the beginning of the path so we must be off the
                // end of a single segment relative path
                return 0;
            }
            else
            {
                if (previous_slash_position == source.size() - 1)
                {
                    return previous_slash_position;
                }
                else
                {
                    return previous_slash_position + 1;
                }
            }
        }
        else if (source[current_position] == '/')
        {
            if (current_position == source.size() - 1)
            {
                string_type::size_type previous_slash_position =
                    detail::find_previous_slash(source, current_position - 1);
                if (previous_slash_position == string_type::npos)
                {
                    // we ran off the beginning of the path so we must be at the
                    // slash following the first segment of a relative path
                    return 0;
                }
                else
                {
                    return previous_slash_position + 1;
                }
            }
            else
            {
                assert(!"position is at slash that isn't leading or trailing");
                BOOST_THROW_EXCEPTION(std::logic_error("unreachable"));
            }
        }
        else
        {
            assert(source[current_position - 1] == '/');

            string_type::size_type previous_non_slash_position =
                detail::find_previous_non_slash(source, current_position - 1);
            if (previous_non_slash_position == string_type::npos)
            {
                // We're at the first segment of an absolute path.
                // The leading slash is the next position
                return 0;
            }
            else
            {
                string_type::size_type previous_slash_position =
                    detail::find_previous_slash(source,
                                                previous_non_slash_position);

                if (previous_slash_position == string_type::npos)
                {
                    // we ran off the beginning of the path so we must be at the
                    // start of the second segment of a relative path
                    return 0;
                }
                else
                {
                    return previous_slash_position + 1;
                }
            }
        }
    }

    static string_type::size_type
    find_element_last_position(const string_type& source,
                               string_type::size_type first_position)
    {
        if (first_position == source.size())
        {
            return source.size();
        }
        else if (source[first_position] == '/')
        {
            if (first_position == source.size() - 1)
            {
                return first_position;
            }
            else if (first_position == 0)
            {
                return first_position;
            }
            else
            {
                assert(!"position is at slash that isn't leading or trailing");
                BOOST_THROW_EXCEPTION(std::logic_error("unreachable"));
            }
        }
        else
        {
            string_type::size_type next_slash_position =
                source.find_first_of('/', first_position);
            if (next_slash_position == string_type::npos)
            {
                // Path ends - segment ends at last char in string
                return source.size() - 1;
            }
            else
            {
                // Normal case - slash found - next segment ends just before it
                return next_slash_position - 1;
            }
        }
    }

    static string_type element_from_positions(
        const string_type& source,
        std::pair<string_type::size_type, string_type::size_type> positions)
    {
        if (positions.first == source.size())
        {
            return "";
        }
        else if (source[positions.first] == '/')
        {
            // leading or trailing /
            if (positions.first == 0)
            {
                return "/";
            }
            else if (positions.first == source.size() - 1)
            {
                return ".";
            }
            else
            {
                assert(!"position is at slash that isn't leading or trailing");
                BOOST_THROW_EXCEPTION(std::logic_error("unreachable"));
            }
        }
        else
        {
            return string_type(source.begin() + positions.first,
                               source.begin() + positions.second + 1);
        }
    }

    const path* m_source;
    std::pair<string_type::size_type, string_type::size_type>
        m_current_positions;
    path m_current_segment;
};

inline path path::parent_path() const
{
    if (empty())
    {
        return path();
    }
    else
    {
        return path_from_range(begin(), --end());
    }
}

inline path path::relative_path() const
{
    if (is_relative())
    {
        return *this;
    }
    else
    {
        return path_from_range(++begin(), end());
    }
}

inline path path::filename() const
{
    if (empty())
    {
        return path();
    }
    else
    {
        return path_from_range(--end(), end());
    }
}

inline int path::compare(const path& rhs) const
{
    return detail::lexical_compare(begin(), end(), rhs.begin(), rhs.end());
}

inline path::iterator path::begin() const
{
    return iterator(this, 0);
}

inline path::iterator path::end() const
{
    return iterator(this, m_path.size());
}

template <typename CharT, typename Traits>
inline std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& stream, const path& p)
{
    // TODO: quote string so it can roundtrip
    stream << p.string<CharT, Traits>();
    return stream;
}

inline bool operator==(const path& lhs, const path& rhs)
{
    return lhs.compare(rhs) == 0;
}

inline bool operator<(const path& lhs, const path& rhs)
{
    return lhs.compare(rhs) < 0;
}

template <typename Source>
inline path operator/(const path& lhs, const Source& rhs)
{
    path concatenation(lhs);
    concatenation /= rhs;
    return concatenation;
}
}
} // namespace ssh::filesystem

#endif
