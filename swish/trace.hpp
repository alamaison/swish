/**
    @file

    Debug tracing.

    @if license

    Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#pragma once

#ifdef _DEBUG

#include <crtdbg.h> // _CrtDebugReport

#include <string>
#include <vector>
#include <cstdarg> // va_start, va_list, va_end

#include <stdio.h> // _vsnprintf et al

#include <boost/format.hpp> // format
#pragma warning(push)
#pragma warning(disable:4996) // unsafe function wctomb
#include <boost/archive/iterators/mb_from_wchar.hpp> // mb_from_wchar
#pragma warning(pop)

namespace swish {
namespace tracing {

namespace {

    /**
     * Debug tracer.
     */
    class Tracer
    {
    public:
        Tracer()
        {        
            ::_CrtSetReportMode(
                _CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
            ::_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
        }

        /**
         * Output the trace message and break to a new line.
         */
        void trace(const std::string& message) const
        {
            std::string line = message + "\n";
            ::_CrtDbgReport(_CRT_WARN, NULL, 0, NULL, line.c_str());
        }
    };

    /**
     * Helper class to give same usage for boost-style formatting as printf.
     *
     * I.e:
     *     trace("%s %d") % "argument" % 42;
     * behaves indentically to:
     *     trace_f("%s %d", "argument", 42);
     *
     * This works because the temporary TraceFormatter returned by trace() is
     * destroyed only after the final operator% call is made.  On destruction,
     * the formatter outputs the fed values to the tracer.
     *
     * @see swish::tracing::trace()
     */
    class TraceFormatter
    {
    public:
    
        typedef boost::archive::iterators::mb_from_wchar<
            std::wstring::const_iterator>
        converter;

        TraceFormatter(const std::string& format) : m_format(format) {}

        ~TraceFormatter() throw()
        {
            try
            {
                static Tracer tracer;
                tracer.trace(m_format.str());
            }
            catch (...) {}
        }

        /**
         * Feeding operator that narrows wstring values for output.
         */
        TraceFormatter& operator%(const std::wstring& value)
        {
            m_format % std::string(
                converter(value.begin()), converter(value.end()));;
            return *this;
        }

        template<typename T>
        TraceFormatter& operator%(const T& value)
        {
            m_format % value;
            return *this;
        }

    private:
        boost::format m_format;
    };

}

/**
 * Output trace message.
 *
 * Can be, optionally, fed with values in boost-format style:
 *     trace("%s %d") % "argument" % 42;
 * or:
 *     trace("%1% %2%") % "argument" % 42;
 */
inline TraceFormatter trace(const std::string& format)
{
    return TraceFormatter(format);
}

/**
 * Output trace message.
 *
 * Can be, optionally, passed values in printf style:
 *     trace_f("%s %d", "argument", 42);
 */
inline void trace_f(std::string format, ...)
{
    //
    // WARNING: mustn't change format to a reference - this breaks varargs
    //

    std::va_list arglist;
    va_start(arglist, format);

    try
    {
        int cch = ::_vscprintf(format.c_str(), arglist) + 1;
        std::vector<char> buffer(cch);
#pragma warning(push)
#pragma warning(disable:4996) // unsafe function
        ::_vsnprintf(&buffer[0], buffer.size(), format.c_str(), arglist);
#pragma warning(pop)
        trace(&buffer[0]);
    }
    catch (...)
    {
        va_end(arglist);
        throw;
    }
    
    va_end(arglist);
}

}} // namespace swish::trace

#else

namespace swish {
namespace tracing {

namespace {

    class DummyFormatter
    {
    public:
        template<typename T>
        DummyFormatter& operator%(const T&)
        {
            return *this;
        }
    };

}

inline DummyFormatter trace(const std::string&)
{
    return DummyFormatter();
}

inline void trace_f(std::string, ...) {}

}} // namespace swish::trace

#endif
