/**
    @file

    Tests for ssh filesystem path.

    @if license

    Copyright (C) 2015 Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "ssh/path.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/locale.hpp>
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <string>

using ssh::filesystem::path;
//using boost::filesystem::path;
using std::string;

namespace std {

    ostream& operator<<(ostream& out, const wstring& wide_in)
    {
        out << boost::locale::conv::from_utf(
            wide_in, boost::locale::util::get_system_locale());
        return out;
    }

    inline ostream& operator<<(ostream& out, const wchar_t* wide_in)
    {
        out << wstring(wide_in);
        return out;
    }
}


BOOST_AUTO_TEST_SUITE(path_tests)

BOOST_AUTO_TEST_CASE( default_path_is_empty )
{
    const path p;
    BOOST_CHECK(p.empty());
}

BOOST_AUTO_TEST_CASE( default_path_is_equal_to_itself )
{
    const path p;
    BOOST_CHECK_EQUAL(p, p);
}

BOOST_AUTO_TEST_CASE( default_path_is_equal_to_another_default_path )
{
    const path p;
    const path q;
    BOOST_CHECK_EQUAL(p, q);
}

BOOST_AUTO_TEST_CASE( default_path_is_equal_to_a_constructed_copy )
{
    const path p;
    const path q(p);
    BOOST_CHECK_EQUAL(p, q);
}

BOOST_AUTO_TEST_CASE( default_path_is_equal_to_an_assigned_copy )
{
    const path p;
    path q;
    q = p;
    BOOST_CHECK_EQUAL(p, q);
}

BOOST_AUTO_TEST_CASE( default_path_is_different_to_a_single_segment_path )
{
    const path p;
    const path q("other path");
    BOOST_CHECK_NE(p, q);
}

BOOST_AUTO_TEST_CASE( default_path_converts_explicity_to_empty_string )
{
    const path p;
    BOOST_CHECK_EQUAL(p.native(), path::string_type());
}

BOOST_AUTO_TEST_CASE( default_path_converts_implicitly_to_empty_string )
{
    const path p;
    const path::string_type s = p;
    BOOST_CHECK_EQUAL(s, path::string_type());
}

BOOST_AUTO_TEST_CASE( default_path_is_at_end_of_iteration )
{
    const path p;
    BOOST_CHECK(p.begin() == p.end());
    BOOST_CHECK_EQUAL(std::distance(p.begin(), p.end()), 0);
}

BOOST_AUTO_TEST_CASE( default_path_is_relative )
{
    const path p;
    BOOST_CHECK(p.is_relative());
}

BOOST_AUTO_TEST_CASE( default_path_is_not_absolute )
{
    const path p;
    BOOST_CHECK(!p.is_absolute());
}

BOOST_AUTO_TEST_CASE( default_path_has_no_parent_path )
{
    const path p;
    BOOST_CHECK(!p.has_parent_path());
}

BOOST_AUTO_TEST_CASE( default_path_parent_path_is_empty )
{
    const path p;
    BOOST_CHECK(p.parent_path().empty());
}

BOOST_AUTO_TEST_CASE( root_path_is_not_empty )
{
    const path p("/");
    BOOST_CHECK(!p.empty());
}

BOOST_AUTO_TEST_CASE( root_path_is_equal_to_itself )
{
    const path p("/");
    BOOST_CHECK_EQUAL(p, p);
}

BOOST_AUTO_TEST_CASE( root_path_is_equal_to_another_root_path )
{
    const path p("/");
    const path q("/");
    BOOST_CHECK_EQUAL(p, q);
}

BOOST_AUTO_TEST_CASE( root_path_is_different_to_a_non_root_relative_path )
{
    const path p("/");
    const path q("foo");
    BOOST_CHECK_NE(p, q);
}

BOOST_AUTO_TEST_CASE( root_path_is_different_to_a_non_root_absolute_path )
{
    const path p("/");
    const path q("/foo");
    BOOST_CHECK_NE(p, q);
}

BOOST_AUTO_TEST_CASE( root_path_is_different_to_a_default_path )
{
    const path p("/");
    const path q;
    BOOST_CHECK_NE(p, q);
}

BOOST_AUTO_TEST_CASE( root_path_is_equal_to_a_constructed_copy )
{
    const path p("/");
    const path q(p);
    BOOST_CHECK_EQUAL(p, q);
}

BOOST_AUTO_TEST_CASE( root_path_is_equal_to_an_assigned_copy )
{
    const path p("/");
    path q;
    q = p;
    BOOST_CHECK_EQUAL(p, q);
}

BOOST_AUTO_TEST_CASE( root_path_converts_explicity_to_original_string )
{
    const path p("/");
    BOOST_CHECK_EQUAL(p.native(), "/");
}

BOOST_AUTO_TEST_CASE( root_path_converts_implicitly_to_original_string )
{
    const path p("/");
    const path::string_type s = p;
    BOOST_CHECK_EQUAL(s, "/");
}

BOOST_AUTO_TEST_CASE( root_path_can_iterate_once )
{
    const path p("/");
    BOOST_CHECK(p.begin() != p.end());
    BOOST_CHECK_EQUAL(std::distance(p.begin(), p.end()), 1);
}

BOOST_AUTO_TEST_CASE( root_path_iterator_produces_original_path )
{
    const path p("/");
    BOOST_REQUIRE(p.begin() != p.end());
    BOOST_CHECK_EQUAL(*(p.begin()), path("/"));
}

BOOST_AUTO_TEST_CASE( root_path_iteration_is_bidirectional )
{
    const path p("/");
    BOOST_REQUIRE(p.begin() != p.end());

    path::iterator it = --p.end();
    BOOST_CHECK_EQUAL(*it, path("/"));
}

BOOST_AUTO_TEST_CASE( root_path_is_not_relative )
{
    const path p("/");
    BOOST_CHECK(!p.is_relative());
}

BOOST_AUTO_TEST_CASE( root_path_is_absolute )
{
    const path p("/");
    BOOST_CHECK(p.is_absolute());
}

BOOST_AUTO_TEST_CASE( root_path_has_no_parent_path )
{
    const path p("/");
    BOOST_CHECK(!p.has_parent_path());
}

BOOST_AUTO_TEST_CASE( root_path_parent_path_is_empty )
{
    const path p("/");
    BOOST_CHECK(p.parent_path().empty());
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_is_not_empty )
{
    const path p("/Test Filename.txt");
    BOOST_CHECK(!p.empty());
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_is_equal_to_itself )
{
    const path p("/Test Filename.txt");
    BOOST_CHECK_EQUAL(p, p);
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_is_equal_to_another_path_from_equal_source )
{
    const path p("/Test Filename.txt");
    const path q("/Test Filename.txt");
    BOOST_CHECK_EQUAL(p, q);
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_is_different_to_another_path_from_different_source )
{
    const path p("/Test Filename.txt");
    const path q("/Test Filename.txp");
    BOOST_CHECK_NE(p, q);
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_is_different_to_similar_relative_path )
{
    const path p("/Test Filename.txt");
    const path q("Test Filename.txt");
    BOOST_CHECK_NE(p, q);
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_equality_is_case_sensitive )
{
    const path p("/Test Filename.txt");
    const path q("/Test filename.txt");
    BOOST_CHECK_NE(p, q);
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_is_equal_to_a_constructed_copy )
{
    const path p("/Test Filename.txt");
    const path q(p);
    BOOST_CHECK_EQUAL(p, q);
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_is_equal_to_an_assigned_copy )
{
    const path p("/Test Filename.txt");
    path q;
    q = p;
    BOOST_CHECK_EQUAL(p, q);
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_is_less_than_lexi_greater_source )
{
    const path p("/Test Filename.txs");
    const path q("/Test Filename.txt");
    BOOST_CHECK_LT(p, q);
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_is_greater_than_lexi_less_source )
{
    const path p("/Test Filename.txt");
    const path q("/Test Filename.txs");
    BOOST_CHECK_GT(p, q);
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_converts_explicity_to_original_string )
{
    const path p("/Test Filename.txt");
    BOOST_CHECK_EQUAL(p.native(), "/Test Filename.txt");
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_converts_implicitly_to_original_string )
{
    const path p("/Test Filename.txt");
    const path::string_type s = p;
    BOOST_CHECK_EQUAL(s, "/Test Filename.txt");
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_can_iterate_twice )
{
    const path p("/Test Filename.txt");
    BOOST_CHECK(p.begin() != p.end());
    BOOST_CHECK_EQUAL(std::distance(p.begin(), p.end()), 2);
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_iterator_produces_root_and_filename_single_segment_paths )
{
    const path p("/Test Filename.txt");
    BOOST_REQUIRE(p.begin() != p.end());

    path::iterator it = p.begin();
    BOOST_CHECK_EQUAL(*it++, path("/"));
    BOOST_CHECK_EQUAL(*it++, path("Test Filename.txt"));
    BOOST_CHECK(it == p.end());
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_iteration_is_bidirectional )
{
    const path p("/Test Filename.txt");
    BOOST_REQUIRE(p.begin() != p.end());

    path::iterator it = --p.end();
    BOOST_CHECK_EQUAL(*it--, path("Test Filename.txt"));
    BOOST_CHECK_EQUAL(*it, path("/"));
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_is_not_relative )
{
    const path p("/Test Filename.txt");
    BOOST_CHECK(!p.is_relative());
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_is_absolute )
{
    const path p("/Test Filename.txt");
    BOOST_CHECK(p.is_absolute());
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_has_parent_path )
{
    const path p("/Test Filename.txt");
    BOOST_CHECK(p.has_parent_path());
}

BOOST_AUTO_TEST_CASE( single_segment_absolute_path_parent_path_is_root_path )
{
    const path p("/Test Filename.txt");
    BOOST_CHECK_EQUAL(p.parent_path(), path("/"));
}

BOOST_AUTO_TEST_CASE( single_segment_relative_path_has_no_parent_path )
{
    const path p("foo");
    BOOST_CHECK(!p.has_parent_path());
}

BOOST_AUTO_TEST_CASE( single_segment_relative_path_parent_path_is_empty )
{
    const path p("foo");
    BOOST_CHECK(p.parent_path().empty());
}

BOOST_AUTO_TEST_CASE( multi_segment_relative_path_is_not_empty )
{
    const path p("Test Dir/Test Filename.txt");
    BOOST_CHECK(!p.empty());
}

BOOST_AUTO_TEST_CASE( multi_segment_relative_path_is_equal_to_itself )
{
    const path p("Test Dir/Test Filename.txt");
    BOOST_CHECK_EQUAL(p, p);
}

BOOST_AUTO_TEST_CASE( multi_segment_relative_path_is_equal_to_another_path_from_equal_source )
{
    const path p("Test Dir/Test Filename.txt");
    const path q("Test Dir/Test Filename.txt");
    BOOST_CHECK_EQUAL(p, q);
}

BOOST_AUTO_TEST_CASE( multi_segment_relative_path_is_different_to_another_path_with_same_dir_different_file )
{
    const path p("Test Dir/Test Filename.txt");
    const path q("Test Dir/Test Filename.txp");
    BOOST_CHECK_NE(p, q);
}

BOOST_AUTO_TEST_CASE( multi_segment_relative_path_is_different_to_another_path_with_different_dir_same_file )
{
    const path p("Test Dir/Test Filename.txt");
    const path q("Test Dir 2/Test Filename.txt");
    BOOST_CHECK_NE(p, q);
}

BOOST_AUTO_TEST_CASE( multi_segment_relative_path_equality_is_case_sensitive )
{
    const path p("Test Dir/Test Filename.txt");
    const path q("Test Dir/Test filename.txt");
    BOOST_CHECK_NE(p, q);
}

BOOST_AUTO_TEST_CASE( multi_segment_relative_path_is_equal_to_a_constructed_copy )
{
    const path p("Test Dir/Test Filename.txt");
    const path q(p);
    BOOST_CHECK_EQUAL(p, q);
}

BOOST_AUTO_TEST_CASE( multi_segment_relative_path_is_equal_to_an_assigned_copy )
{
    const path p("Test Dir/Test Filename.txt");
    path q;
    q = p;
    BOOST_CHECK_EQUAL(p, q);
}

BOOST_AUTO_TEST_CASE( multi_segment_relative_path_compares_less_than_lexi_by_segment )
{
    const path p("a/ad");
    const path q("a+/c");
    BOOST_CHECK_LT(p, q);
}

BOOST_AUTO_TEST_CASE( multi_segment_relative_path_compares_greater_than_lexi_by_segment )
{
    const path p("a+/c");
    const path q("a/ad");
    BOOST_CHECK_GT(p, q);
}

BOOST_AUTO_TEST_CASE( multi_segment_relative_path_converts_explicity_to_original_string )
{
    const path p("Test Dir/Test Filename.txt");
    BOOST_CHECK_EQUAL(p.native(), "Test Dir/Test Filename.txt");
}

BOOST_AUTO_TEST_CASE( multi_segment_relative_path_converts_implicitly_to_original_string )
{
    const path p("Test Dir/Test Filename.txt");
    const path::string_type s = p;
    BOOST_CHECK_EQUAL(s, "Test Dir/Test Filename.txt");
}

BOOST_AUTO_TEST_CASE( multi_segment_relative_path_can_iterate_twice )
{
    const path p("Test Dir/Test Filename.txt");
    BOOST_CHECK(p.begin() != p.end());
    BOOST_CHECK_EQUAL(std::distance(p.begin(), p.end()), 2);
}

BOOST_AUTO_TEST_CASE( multi_segment_relative_path_iterator_produces_dir_and_file_single_segment_paths )
{
    const path p("Test Dir/Test Filename.txt");
    BOOST_REQUIRE(p.begin() != p.end());

    path::iterator it = p.begin();
    BOOST_CHECK_EQUAL(*it++, path("Test Dir"));
    BOOST_CHECK_EQUAL(*it++, path("Test Filename.txt"));
    BOOST_CHECK(it == p.end());
}

BOOST_AUTO_TEST_CASE( multi_segment_relative_path_iteration_is_bidirectional )
{
    const path p("Test Dir/Test Filename.txt");
    BOOST_REQUIRE(p.begin() != p.end());

    path::iterator it = --p.end();
    BOOST_CHECK_EQUAL(*it--, path("Test Filename.txt"));
    BOOST_CHECK_EQUAL(*it, path("Test Dir"));
}

BOOST_AUTO_TEST_CASE( multi_segment_relative_path_is_relative )
{
    const path p("Test Dir/Test Filename.txt");
    BOOST_CHECK(p.is_relative());
}

BOOST_AUTO_TEST_CASE( multi_segment_relative_path_is_not_absolute )
{
    const path p("Test Dir/Test Filename.txt");
    BOOST_CHECK(!p.is_absolute());
}

BOOST_AUTO_TEST_CASE( multi_segment_relative_path_has_parent_path )
{
    const path p("Test Dir/Test Filename.txt");
    BOOST_CHECK(p.has_parent_path());
}

BOOST_AUTO_TEST_CASE(
    multi_segment_relative_path_parent_path_omits_last_segment )
{
    const path p("Test Dir/Test Filename.txt");
    BOOST_CHECK_EQUAL(p.parent_path(), path("Test Dir"));
}

// NOTE: This behaviour seems very odd an anti-STL (non-interchangeable equal
// values) however it seems to be required by the current C++ Standard proposal
// (iteration ignores multiple separators, equality based on iteration).
// Boost.Filesystem in 1.49 did _not_ have this beahviour, but I suspect the
// recent changes to bring it into line with the standard may have introduced
// that behaviour.
//
// TODO: Test what latest Boost.Filesystem behaviour is and, if necessary file a
// bug with Boost and/or the C++ standard.
BOOST_AUTO_TEST_CASE( multiple_adjacent_separators_do_not_affect_path_equality )
{
    const path p("foo//bar");
    BOOST_CHECK_EQUAL(p, path("foo//bar"));
    BOOST_CHECK_EQUAL(p, path("foo/bar"));
    BOOST_CHECK_EQUAL(p, path("foo///bar"));
}

BOOST_AUTO_TEST_CASE( multiple_adjacent_separators_do_not_affect_iteration )
{
    const path p("foo//bar");

    BOOST_REQUIRE(p.begin() != p.end());

    path::iterator it = p.begin();
    BOOST_CHECK_EQUAL(*it++, path("foo"));
    BOOST_CHECK_EQUAL(*it++, path("bar"));
    BOOST_CHECK(it == p.end());
}

BOOST_AUTO_TEST_CASE( directory_path_is_not_empty )
{
    const path p("foo/bar/");
    BOOST_CHECK(!p.empty());
}

BOOST_AUTO_TEST_CASE( directory_path_is_equal_to_itself )
{
    const path p("foo/bar/");
    BOOST_CHECK_EQUAL(p, p);
}

BOOST_AUTO_TEST_CASE( directory_path_is_not_equal_to_similar_file_path )
{
    const path p("foo/bar/");
    const path q("foo/bar");
    BOOST_CHECK_NE(p, q);
}

BOOST_AUTO_TEST_CASE( directory_path_is_equal_to_another_path_from_equal_source )
{
    const path p("foo/bar/");
    const path q("foo/bar/");
    BOOST_CHECK_EQUAL(p, q);
}

BOOST_AUTO_TEST_CASE( directory_path_is_equal_to_a_constructed_copy )
{
    const path p("foo/bar/");
    const path q(p);
    BOOST_CHECK_EQUAL(p, q);
}

BOOST_AUTO_TEST_CASE( directory_path_is_equal_to_an_assigned_copy )
{
    const path p("foo/bar/");
    path q;
    q = p;
    BOOST_CHECK_EQUAL(p, q);
}

BOOST_AUTO_TEST_CASE( directory_path_is_less_than_lexi_greater_source )
{
    const path p("foo/baq/");
    const path q("foo/bar/");
    BOOST_CHECK_LT(p, q);
}

BOOST_AUTO_TEST_CASE( directory_path_is_greater_than_lexi_less_source )
{
    const path p("foo/bar/");
    const path q("foo/baq/");
    BOOST_CHECK_GT(p, q);
}

BOOST_AUTO_TEST_CASE( directory_path_converts_explicity_to_original_string )
{
    const path p("foo/bar/");
    BOOST_CHECK_EQUAL(p.native(), "foo/bar/");
}

BOOST_AUTO_TEST_CASE( directory_path_converts_implicitly_to_original_string )
{
    const path p("foo/bar/");
    const path::string_type s = p;
    BOOST_CHECK_EQUAL(s, "foo/bar/");
}

BOOST_AUTO_TEST_CASE( directory_path_iterates_once_more_than_number_of_names )
{
    const path p("foo/bar/");
    BOOST_CHECK(p.begin() != p.end());
    BOOST_CHECK_EQUAL(std::distance(p.begin(), p.end()), 3);
}

BOOST_AUTO_TEST_CASE(
    directory_path_iterator_produces_filename_single_segments_followed_by_dot )
{
    const path p("foo/bar/");
    BOOST_REQUIRE(p.begin() != p.end());

    path::iterator it = p.begin();
    BOOST_CHECK_EQUAL(*it++, path("foo"));
    BOOST_CHECK_EQUAL(*it++, path("bar"));
    BOOST_CHECK_EQUAL(*it++, path("."));
    BOOST_CHECK(it == p.end());
}

BOOST_AUTO_TEST_CASE( directory_path_iteration_is_bidirectional )
{
    const path p("foo/bar/");
    BOOST_REQUIRE(p.begin() != p.end());

    path::iterator it = --p.end();
    BOOST_CHECK_EQUAL(*it--, path("."));
    BOOST_CHECK_EQUAL(*it--, path("bar"));
    BOOST_CHECK_EQUAL(*it, path("foo"));
}

BOOST_AUTO_TEST_CASE( directory_path_has_parent_path )
{
    const path p("foo/bar/");
    BOOST_CHECK(p.has_parent_path());
}

BOOST_AUTO_TEST_CASE( directory_path_parent_path_omit_trailing_slash )
{
    const path p("foo/bar/");
    BOOST_CHECK_EQUAL(p.parent_path(), path("foo/bar"));
}

BOOST_AUTO_TEST_CASE( dotted_directory_path_has_parent_path )
{
    const path p("foo/bar/.");
    BOOST_CHECK(p.has_parent_path());
}

BOOST_AUTO_TEST_CASE(
    dotted_directory_path_parent_path_omit_trailing_slash_and_dot )
{
    const path p("foo/bar/.");
    BOOST_CHECK_EQUAL(p.parent_path(), path("foo/bar"));
}

BOOST_AUTO_TEST_CASE( relative_directory_path_is_relative )
{
    const path p("foo/bar/");
    BOOST_CHECK(p.is_relative());
}

BOOST_AUTO_TEST_CASE( absolute_directory_path_is_not_relative )
{
    const path p("/foo/bar/");
    BOOST_CHECK(!p.is_relative());
}

BOOST_AUTO_TEST_CASE( relative_directory_path_is_not_absolute )
{
    const path p("foo/bar/");
    BOOST_CHECK(!p.is_absolute());
}

BOOST_AUTO_TEST_CASE( absolute_directory_path_is_absolute )
{
    const path p("/foo/bar/");
    BOOST_CHECK(p.is_absolute());
}

BOOST_AUTO_TEST_CASE( concatenating_relative_paths_returns_concantenation )
{
    const path p("foo/bar");
    const path q("baz/woz");
    BOOST_CHECK_EQUAL(p / q, path("foo/bar/baz/woz"));
}

BOOST_AUTO_TEST_CASE( concatenating_relative_paths_leaves_both_operands_unchanged )
{
    const path p("foo/bar");
    const path q("baz/woz");
    p / q;
    BOOST_CHECK_EQUAL(p, path("foo/bar"));
    BOOST_CHECK_EQUAL(q, path("baz/woz"));
}

BOOST_AUTO_TEST_CASE( appending_relative_path_to_another_returns_concatenation )
{
    path p("foo/bar");
    const path q("baz/woz");
    BOOST_CHECK_EQUAL(p /= q, path("foo/bar/baz/woz"));
}

BOOST_AUTO_TEST_CASE( appending_relative_path_to_another_changes_latter_to_concatenation )
{
    path p("foo/bar");
    const path q("baz/woz");
    p /= q;
    BOOST_CHECK_EQUAL(p, path("foo/bar/baz/woz"));
}

BOOST_AUTO_TEST_CASE(
    appending_relative_path_to_another_leaves_former_unchanged )
{
    path p("foo/bar");
    const path q("baz/woz");
    p /= q;
    BOOST_CHECK_EQUAL(q, path("baz/woz"));
}

BOOST_AUTO_TEST_CASE(
    concatenating_relative_directory_paths_returns_concatenation )
{
    const path p("foo/bar/");
    const path q("baz/woz/");
    BOOST_CHECK_EQUAL(p / q, path("foo/bar/baz/woz/"));
}

BOOST_AUTO_TEST_CASE(
    concatenating_relative_directory_paths_leaves_both_unchanged )
{
    const path p("foo/bar/");
    const path q("baz/woz/");
    p / q;
    BOOST_CHECK_EQUAL(p, path("foo/bar/"));
    BOOST_CHECK_EQUAL(q, path("baz/woz/"));
}

BOOST_AUTO_TEST_CASE(
    appending_relative_directory_path_to_another_returns_concatenation )
{
    path p("foo/bar/");
    const path q("baz/woz/");
    BOOST_CHECK_EQUAL(p /= q, path("foo/bar/baz/woz/"));
}

BOOST_AUTO_TEST_CASE(
    appending_relative_directory_path_to_another_changes_latter_to_concatenation )
{
    path p("foo/bar/");
    const path q("baz/woz/");
    p /= q;
    BOOST_CHECK_EQUAL(p, path("foo/bar/baz/woz/"));
}

BOOST_AUTO_TEST_CASE(
    appending_relative_directory_path_to_another_leaves_former_unchanged )
{
    path p("foo/bar/");
    const path q("baz/woz/");
    p /= q;
    BOOST_CHECK_EQUAL(q, path("baz/woz/"));
}

BOOST_AUTO_TEST_CASE(
    concatenating_relative_and_absolute_returns_concatenation )
{
    const path p("foo/bar");
    const path q("/baz/woz");
    BOOST_CHECK_EQUAL(p / q, path("foo/bar/baz/woz"));
}

BOOST_AUTO_TEST_CASE(
    concatenating_relative_and_absolute_leaves_both_unchanged )
{
    const path p("foo/bar");
    const path q("/baz/woz");
    p / q;
    BOOST_CHECK_EQUAL(p, path("foo/bar"));
    BOOST_CHECK_EQUAL(q, path("/baz/woz"));
}

BOOST_AUTO_TEST_CASE( appending_absolute_to_relative_returns_concatenation )
{
    path p("foo/bar");
    const path q("/baz/woz");
    BOOST_CHECK_EQUAL(p /= q, path("foo/bar/baz/woz"));
}

BOOST_AUTO_TEST_CASE(
    appending_absolute_to_relative_changes_latter_to_concatenation )
{
    path p("foo/bar");
    const path q("/baz/woz");
    p /= q;
    BOOST_CHECK_EQUAL(p, path("foo/bar/baz/woz"));
}

BOOST_AUTO_TEST_CASE(
    appending_absolute_to_relative_another_leaves_former_unchanged )
{
    path p("foo/bar");
    const path q("/baz/woz");
    p /= q;
    BOOST_CHECK_EQUAL(q, path("/baz/woz"));
}

BOOST_AUTO_TEST_CASE(
    concatenating_relative_directory_and_absolute_returns_concatenation )
{
    const path p("foo/bar/");
    const path q("/baz/woz");
    BOOST_CHECK_EQUAL(p / q, path("foo/bar/baz/woz"));
}

BOOST_AUTO_TEST_CASE(
    concatenating_relative_directory_and_absolute_leaves_both_unchanged )
{
    const path p("foo/bar/");
    const path q("/baz/woz");
    p / q;
    BOOST_CHECK_EQUAL(p, path("foo/bar/"));
    BOOST_CHECK_EQUAL(q, path("/baz/woz"));
}

BOOST_AUTO_TEST_CASE(
    appending_absolute_to_relative_directory_returns_concatenation )
{
    path p("foo/bar/");
    const path q("/baz/woz");
    BOOST_CHECK_EQUAL(p /= q, path("foo/bar/baz/woz"));
}

BOOST_AUTO_TEST_CASE(
    appending_absolute_to_relative_directory_changes_latter_to_concatenation )
{
    path p("foo/bar/");
    const path q("/baz/woz");
    p /= q;
    BOOST_CHECK_EQUAL(p, path("foo/bar/baz/woz"));
}

BOOST_AUTO_TEST_CASE(
    appending_absolute_to_relative_directory_leaves_former_unchanged )
{
    path p("foo/bar/");
    const path q("/baz/woz");
    p /= q;
    BOOST_CHECK_EQUAL(q, path("/baz/woz"));
}

BOOST_AUTO_TEST_CASE( concatenating_default_and_relative_returns_the_latter )
{
    const path p;
    const path q("foo/bar");
    BOOST_CHECK_EQUAL(p / q, q);
}

BOOST_AUTO_TEST_CASE( concatenating_default_and_relative_leaves_both_unchanged )
{
    const path p;
    const path q("foo/bar");
    p / q;
    BOOST_CHECK_EQUAL(p, path());
    BOOST_CHECK_EQUAL(q, path("foo/bar"));
}

BOOST_AUTO_TEST_CASE( appending_relative_to_default_returns_the_former )
{
    path p;
    const path q("foo/bar");
    BOOST_CHECK_EQUAL(p /= q, q);
}

BOOST_AUTO_TEST_CASE(
    appending_relative_to_default_changes_latter_to_equal_former )
{
    path p;
    const path q("foo/bar");
    p /= q;
    BOOST_CHECK_EQUAL(p, q);
}

BOOST_AUTO_TEST_CASE( appending_relative_to_default_leaves_former_unchanged )
{
    path p;
    const path q("foo/bar");
    p /= q;
    BOOST_CHECK_EQUAL(q, path("foo/bar"));
}

BOOST_AUTO_TEST_CASE( concatenating_root_and_relative_returns_concatenation )
{
    const path p("/");
    const path q("foo/bar");
    BOOST_CHECK_EQUAL(p / q, path("/foo/bar"));
}

BOOST_AUTO_TEST_CASE(concatenating_root_and_relative_leaves_both_unchanged )
{
    const path p("/");
    const path q("foo/bar");
    p / q;
    BOOST_CHECK_EQUAL(p, path("/"));
    BOOST_CHECK_EQUAL(q, path("foo/bar"));
}

BOOST_AUTO_TEST_CASE( appending_relative_to_root_returns_concatenation )
{
    path p("/");
    const path q("foo/bar");
    BOOST_CHECK_EQUAL(p /= q, path("/foo/bar"));
}

BOOST_AUTO_TEST_CASE(
    appending_relative_to_root_changes_latter_to_concatenation )
{
    path p("/");
    const path q("foo/bar");
    p /= q;
    BOOST_CHECK_EQUAL(p, path("/foo/bar"));
}

BOOST_AUTO_TEST_CASE( appending_relative_to_root_leaves_former_unchanged )
{
    path p("/");
    const path q("foo/bar");
    p /= q;
    BOOST_CHECK_EQUAL(q, path("foo/bar"));
}

BOOST_AUTO_TEST_CASE( concatenating_root_and_root_paths_returns_root_path )
{
    const path p("/");
    const path q("/");
    BOOST_CHECK_EQUAL(p / q, path("/"));
}

BOOST_AUTO_TEST_CASE( concatenating_root_and_root_paths_leaves_both_unchanged )
{
    const path p("/");
    const path q("/");
    p / q;
    BOOST_CHECK_EQUAL(p, path("/"));
    BOOST_CHECK_EQUAL(q, path("/"));
}

BOOST_AUTO_TEST_CASE( appending_root_path_to_root_path_returns_root_path )
{
    path p("/");
    const path q("/");
    BOOST_CHECK_EQUAL(p /= q, path("/"));
}

BOOST_AUTO_TEST_CASE(
    appending_root_path_to_root_path_leaves_both_operands_unchanged )
{
    path p("/");
    const path q("/");
    p /= q;
    BOOST_CHECK_EQUAL(p, path("/"));
    BOOST_CHECK_EQUAL(q, path("/"));
}

namespace {
    const char UTF8_STRING1[] =
        "\xe0\xa4\xae\xe0\xa4\xb9\xe0\xa4\xb8\xe0\xa5\x81\xe0\xa4\xb8";
    const wchar_t WIDE_STRING1[] = L"\x92e\x939\x938\x941\x938";

    const char UTF8_STRING2[] = "\xe4\xb8\xad\xe5\x9c\x8b";
    const wchar_t WIDE_STRING2[] = L"\x4e2d\x570b";

    const char UTF8_CONCATENATION[] =
        "\xe0\xa4\xae\xe0\xa4\xb9\xe0\xa4\xb8\xe0\xa5\x81\xe0\xa4\xb8/"
        "\xe4\xb8\xad\xe5\x9c\x8b";
    const wchar_t WIDE_CONCATENATION[] =
        L"\x92e\x939\x938\x941\x938/\x4e2d\x570b";
}

BOOST_AUTO_TEST_CASE( path_created_from_wide_string_is_equal_to_another )
{
    const path p(WIDE_STRING1);
    const path q(WIDE_STRING1);
    BOOST_CHECK_EQUAL(p, q);
}

BOOST_AUTO_TEST_CASE(
    path_created_from_ascii_wide_string_is_equal_to_narrow_equivalent )
{
    // A non-ASCII narrow path may or may not be interpreted as the same string
    // as the wide path, depending on OS.  On Windows, narrow strings are in the
    // local code page
    const path p(L"hello.txt");
    const path q("hello.txt");
    BOOST_CHECK_EQUAL(p, q);
}

// TODO: Create and test a constructor that takes a std::locale parameter to
// guide how the string is interpreted.  Allows passing UTF8 char strings on
// Windows

BOOST_AUTO_TEST_CASE(
    path_created_from_wide_string_converts_explicitly_to_original_string )
{
    const path p(WIDE_STRING1);
    BOOST_CHECK_EQUAL(p.wstring(), WIDE_STRING1);
}

BOOST_AUTO_TEST_CASE(
    path_created_from_wide_string_converts_explicitly_to_utf8_string )
{
    const path p(WIDE_STRING1);
    BOOST_CHECK_EQUAL(p.u8string(), UTF8_STRING1);
}

BOOST_AUTO_TEST_CASE( native_string_is_utf8 )
{
    const path p(WIDE_STRING1);
    BOOST_CHECK_EQUAL(p.native(), UTF8_STRING1);
}

BOOST_AUTO_TEST_CASE( narrow_string_accessor_is_utf8 )
{
    const path p(WIDE_STRING1);
    string narrow(p.string<string::value_type, string::traits_type>());
    BOOST_CHECK_EQUAL(narrow, UTF8_STRING1);
}

BOOST_AUTO_TEST_CASE( wide_string_accessor_preserves_wide_string )
{
    const path p(WIDE_STRING1);
    wstring wide(p.string<wstring::value_type, wstring::traits_type>());
    BOOST_CHECK_EQUAL(wide, WIDE_STRING1);
}

BOOST_AUTO_TEST_CASE( string_conversion_to_local_codepage_works )
{
    // Can't test non-ASCII conversion because the chars may not be supported in
    // the local codepage
    const path p("hello");
    BOOST_CHECK_EQUAL(p.string(), "hello");
}

BOOST_AUTO_TEST_CASE( implicit_string_conversion_is_utf8 )
{
    const path p(WIDE_STRING1);
    const path::string_type s = p;
    BOOST_CHECK_EQUAL(s, UTF8_STRING1);
}

BOOST_AUTO_TEST_CASE( path_contructs_implicitly_from_narrow_pointer )
{
    const char* s = "hello";
    path p = s;
    BOOST_CHECK_EQUAL(p.string(), "hello");
}

BOOST_AUTO_TEST_CASE( path_contructs_implicitly_from_wide_pointer )
{
    const wchar_t* s = L"hello";
    path p = s;
    BOOST_CHECK_EQUAL(p.string(), "hello");
}

BOOST_AUTO_TEST_CASE( path_contructs_implicitly_from_narrow_string )
{
    const string s = "hello";
    path p = s;
    BOOST_CHECK_EQUAL(p.string(), "hello");
}

BOOST_AUTO_TEST_CASE( path_contructs_implicitly_from_wide_string )
{
    const wstring s = L"hello";
    path p = s;
    BOOST_CHECK_EQUAL(p.string(), "hello");
}

BOOST_AUTO_TEST_CASE( appending_wide_string_to_path_extends_path )
{
    path p(WIDE_STRING1);
    p /= WIDE_STRING2;
    const path q(WIDE_CONCATENATION);
    BOOST_CHECK_EQUAL(p, q);
    BOOST_CHECK_EQUAL(p.u8string(), UTF8_CONCATENATION);
}

BOOST_AUTO_TEST_CASE( concatenating_wide_string_and_path_returns_concatenation )
{
    const path p(WIDE_STRING1);
    const path q(WIDE_CONCATENATION);
    BOOST_CHECK_EQUAL(p / WIDE_STRING2, q);
}

BOOST_AUTO_TEST_SUITE_END();
