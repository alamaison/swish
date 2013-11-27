/**
    @file

    Tests for ssh knownhost interface.

    @if license

    Copyright (C) 2010, 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "ssh/knownhost.hpp"

#include <boost/assign/list_of.hpp> // list_of()
#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

#include <libssh2.h>

using ssh::knownhost::find_result;
using ssh::knownhost::knownhost_collection;
using ssh::knownhost::knownhost;
using ssh::knownhost::knownhost_iterator;
using ssh::knownhost::openssh_knownhost_collection;

using boost::assign::list_of;
using boost::filesystem::path;
using boost::filesystem::ifstream;
using boost::shared_ptr;
using boost::test_tools::predicate_result;

using std::string;
using std::vector;

namespace {
    struct test_datum
    {
        test_datum(
            string name, string ip, string key_algo, string key,
            string fail_key, string comment)
            : name(name), ip(ip), key_algo(key_algo), key(key),
              fail_key(fail_key), comment(comment) {}

        string name;
        string ip;
        string key_algo;
        string key;
        string fail_key;
        string comment;
    };
}

const string KEY_A =
    "AAAAB3NzaC1yc2EAAAABIwAAAQEA9QcrMH117S7SNIzhExJJmbKlCqxcIt2QQ5B4gZni"
    "x8RJci8U/z2P1noALl+oJ59gD9IuJZBXxjDQhxCRHWuvwNPax4BvtZwew0VnXlrs75nC"
    "qtFVwcWPUlSU5ycp958YJ3uKQs9yQffgu+LDU29QJ+r7yQSx/YJPgD+DpVeWG1YNqRbo"
    "dUYQKWktto3OFJi4cO8t7fAteK+u+x26JQdMtplj/xrR8FNNghMyT7Rckh54/KrEdbEl"
    "dwXTbp1bm9zDny9OSK6cwVjAk8zdNHCLx9/uurlSNcDRZXCDx3yRJiv8Q4ne0kmbMm4Q"
    "FeigFf3QY7rGUgBEm/wMgxggdvLUCQ==";

const string KEY_B = 
    "AAAAB3NzaC1yc2EAAAABIwAAAQEAvKS1ply6S6xcb/pxnJQQEB+y123axJUKsYEk2ezs"
    "HRNZP920FNM1KXGMmm+i7KugMk7dz46pkE/p4qJ4qVfoeDKojR4GiP1WleKQniTIdgEY"
    "ho7OmopOUszST1Qo5PK9e2gvVQcsyE6xEJkBdMlBWqfm/2vfyr92IPW1wtR3j3YYCcaM"
    "VMdpo0tHiK4qmVJIGcs4BRYRSeWzSFaFdmkhEM7iRxCgQDLykjQEZcKmF5KUEf+SxfNS"
    "51B0O4D2aoamsYaAC849HBJgMS/I5CxLAah2uMQXnZwJrCIUZcZDUQrC7LnSgd86P+yD"
    "FZYbAkXz8QjhGL/qTywA7Afglyt5/w==";

const string KEY_C = 
    "AAAAB3NzaC1kc3MAAACBAL+sTKUuo0M9zhbDq414IEA8S3FJWliDJNaO3isqDuh3aEEb"
    "2wyDrsTf5b6R73RsrAD6K5b3xfMox7LhjwET3D63OpNmU+SUEJl3oJ/yujPHE87aOkt4"
    "02tB82+yed6V2/Wy4eLcihi4r4VJie9WaBbezvxYbB+hV8YpaoktvI5PAAAAFQCmyKgs"
    "rs/7HtA/WVk2iT4av4dmuQAAAIB4hWeAov90067UdbadIq67v7JM8gFBHRertp33nSYD"
    "UvMwqCguiTEnBiOCvdKqGRy6RnnmXgMFqqqE6mHDOMZRQdVCn6M402CYJQ0+HefsC3WG"
    "I3DLIygHJgAjUswb8qg83ddYhcgLqF4vGqoqUr4Cxsgy3k9zOXEH+NoCylXW9gAAAIAa"
    "kCvnTYROP7rqRx7zAlHElQnbjH7D1/6yBvt2JmkPHxmsxQPhiwrlTJqkkCztunLmvO4Z"
    "+BoB23HQ6utyC4ZBA40dB/Bpq+jbQUq1RLmhlHULqVT/2Z9QLHHcygBddKrUZznsk1/I"
    "QcyLHk77/cxQn6dW+B/7G7AdBc4MYMGM/w==";

const vector<const test_datum> test_data = list_of
    (test_datum(
        "host1.example.com", "192.168.0.1", "ssh-rsa", KEY_A, KEY_B, 
        "test@swish"))
    (test_datum(
        "host2.example.com", "10.0.0.1", "ssh-rsa", KEY_B, KEY_C, ""))
    (test_datum(
        "host3.example.com", "192.168.1.1", "ssh-dss", KEY_C, KEY_A,
        "test@swish"));

const string FAIL_HOST = "i-dontexist-in-the-host-file.example.com";

BOOST_AUTO_TEST_SUITE(knownhost_tests)

/**
 * Create and destroy without leaking.
 */
BOOST_AUTO_TEST_CASE( create )
{
    knownhost_collection kh;
}

BOOST_AUTO_TEST_SUITE(openssh_knownhost_tests)

/**
 * Initialise with known_host entries.
 */
BOOST_AUTO_TEST_CASE( init_from_file )
{
    openssh_knownhost_collection kh("test_known_hosts");
}

/**
 * Initialise with hashed known_host entries.
 */
BOOST_AUTO_TEST_CASE( init_from_hashed_file )
{
    openssh_knownhost_collection kh("test_known_hosts_hashed");
}

/**
 * Initialise with a file that doesn't exist.
 */
BOOST_AUTO_TEST_CASE( init_fail )
{
    path bad_path = "i-dont-exist";
    BOOST_REQUIRE(!exists(bad_path));
    BOOST_CHECK_THROW(
        (openssh_knownhost_collection(bad_path)), std::runtime_error);
    BOOST_CHECK(!exists(bad_path));
}

/**
 * Saved file lines should match original except with each entry on its
 * own line.  I.e.:
 *     host3.example.com,192.168.1.1 ssh-dss <key>
 * becomes:
 *     192.168.1.1 ssh-dss <key>
 *     host3.example.com ssh-dss <key>
 */
BOOST_AUTO_TEST_CASE( roundtrip )
{
    openssh_knownhost_collection kh("test_known_hosts");

    vector<string> lines;
    kh.save(kh.begin(), kh.end(), back_inserter(lines));

    ifstream file("test_known_hosts_out");
    BOOST_REQUIRE_EQUAL_COLLECTIONS(
        lines.begin(), lines.end(), 
        std::istream_iterator<ssh::knownhost::detail::line>(file),
        std::istream_iterator<ssh::knownhost::detail::line>());
}

namespace {

    /**
     * Check known_host matches the expected data.
     */
    template<typename T>
    predicate_result entry_matches_impl(
        const T& actual, const test_datum& expected, bool use_name,
        bool hashed_name)
    {
        predicate_result res(false);

        string actual_name = actual.name();
        string expected_name;
        
        if (hashed_name)
            expected_name = "";
        else
            expected_name = (use_name) ? expected.name : expected.ip;

        if (actual_name != expected_name)
            res.message()
                << "Host names or IPs don't match [" << actual_name
                << " != " << expected_name << "]";
        else if (actual.key() != expected.key)
            res.message()
                << "Keys don't match [" << actual.key()
                << " != " << expected.key << "]";
        /*
         TODO: test the comments once the libssh2 API is fixed.

        else if (actual.comment() != expected.comment)
            res.message()
                << "Comments don't match [" << actual.comment()
                << " != " << expected.comment << "]";
        */
        else if (actual.key_algo() != expected.key_algo)
            res.message()
                << "Algorithms don't match [" << actual.key_algo()
                << " != " << expected.key_algo << "]";
        else if (!hashed_name && !actual.is_name_plain())
            res.message() << "Should be plain-text";
        else if (hashed_name && actual.is_name_plain())
            res.message() << "Shouldn't be plain-text";
        else if (!hashed_name && actual.is_name_sha1())
            res.message() << "Should be SHA1-encoded";
        else if (hashed_name && !actual.is_name_sha1())
            res.message() << "Shouldn't be SHA1-encoded";
        else if (actual.is_name_custom())
            res.message() << "Shouldn't be custom encoded";
        else
            return true;

        return res;
    }

    /**
     * Check that a known_host matches the expected data.
     *
     * The host name is expected to be the IP address..
     */
    template<typename T>
    predicate_result entry_matches_ip(
        const T& entry, const test_datum& expected)
    {
        return entry_matches_impl(entry, expected, false, false);
    }

    /**
     * Check that a known_host matches the expected data.
     *
     * The host name is expected to be the IP address..
     */
    template<typename T>
    predicate_result entry_matches(
        const T& entry, const test_datum& expected)
    {
        return entry_matches_impl(entry, expected, true, false);
    }

    /**
     * Check that a hashed known_host matches the expected data.
     */
    template<typename T>
    predicate_result hashed_entry_matches(
        const T& entry, const test_datum& expected)
    {
        return entry_matches_impl(entry, expected, false, true);
    }
}

/**
 * Initialise with known_host entries and test retrieval.
 *
 * The iterator should keep working after the collection is destroyed
 * (this isn't strictly needed but as its easy for is to implement, its
 * a nice feature to enforce).
 */
BOOST_AUTO_TEST_CASE( iterate_entries )
{
    knownhost_iterator it;
    knownhost_iterator end;
    {
        openssh_knownhost_collection kh("test_known_hosts");
        it = kh.begin();
        end = kh.end();
    }

    // There should be one entry for IP and one for hostname
    BOOST_FOREACH(const test_datum& datum, test_data)
    {
        BOOST_REQUIRE(it != end);
        BOOST_CHECK(entry_matches_ip(*it++, datum));

        BOOST_REQUIRE(it != end);
        BOOST_CHECK(entry_matches(*it++, datum));
    }
    
    BOOST_REQUIRE(it == end);
}

/**
 * Initialise with *hashed* known_host entries and test retrieval.
 */
BOOST_AUTO_TEST_CASE( iterate_hashed_entries )
{
    knownhost_iterator it;
    knownhost_iterator end;
    {
        openssh_knownhost_collection kh("test_known_hosts_hashed");
        it = kh.begin();
        end = kh.end();
    }

    // Two entries per host even though we cannot see which is IP and
    // which hostname
    BOOST_FOREACH(const test_datum& datum, test_data)
    {
        BOOST_REQUIRE(it != end);
        BOOST_CHECK(hashed_entry_matches(*it++, datum));

        BOOST_REQUIRE(it != end);
        BOOST_CHECK(hashed_entry_matches(*it++, datum));
    }

    BOOST_REQUIRE(it == end);
}

/**
 * Iterators should no affect each other.
 */
BOOST_AUTO_TEST_CASE( iterator_independence )
{
    openssh_knownhost_collection kh("test_known_hosts");

    knownhost_iterator it1 = kh.begin();

    BOOST_CHECK(entry_matches_ip(*it1++, test_data[0]));

        knownhost_iterator it2 = kh.begin();

    BOOST_CHECK(entry_matches(*it1++, test_data[0]));

        BOOST_CHECK(entry_matches_ip(*it2++, test_data[0]));
        BOOST_CHECK(entry_matches(*it2++, test_data[0]));
        BOOST_CHECK(entry_matches_ip(*it2++, test_data[1]));
        BOOST_CHECK(entry_matches(*it2++, test_data[1]));
    
    BOOST_CHECK(entry_matches_ip(*it1++, test_data[1]));
    BOOST_CHECK(entry_matches(*it1++, test_data[1]));
    BOOST_CHECK(entry_matches_ip(*it1++, test_data[2]));
    BOOST_CHECK(entry_matches(*it1++, test_data[2]));

    BOOST_REQUIRE(it1 == kh.end());

        BOOST_CHECK(entry_matches_ip(*it2++, test_data[2]));
        BOOST_CHECK(entry_matches(*it2++, test_data[2]));

        BOOST_REQUIRE(it2 == kh.end());
}

namespace {

    /**
     * Return first knownhost in file.
     */
    knownhost get_host_but_destroy_collection_and_iterator()
    {
        openssh_knownhost_collection kh("test_known_hosts");

        return *(kh.begin());
    }
}

/**
 * knownhosts should outlive their iterator and collection.
 *
 * They do this by keeping the raw libssh2 collection alive inside them.  Ooo,
 * spooky!
 */
BOOST_AUTO_TEST_CASE( knownhost_lifetime )
{
    knownhost host = get_host_but_destroy_collection_and_iterator();

    BOOST_CHECK(entry_matches_ip(host, test_data[0]));
}

void do_find_match_test(
    const boost::filesystem::path& file, bool is_hashed)
{
    openssh_knownhost_collection kh(file);

    // Find each datum twice, once by IP once by name
    BOOST_FOREACH(const test_datum& datum, test_data)
    {
        find_result result = kh.find(datum.name, datum.key, true);
        
        BOOST_CHECK(result.match());
        BOOST_CHECK(!result.mismatch());
        BOOST_CHECK(!result.not_found());

        BOOST_REQUIRE(result.host() != kh.end());
        if (!is_hashed)
            BOOST_CHECK_EQUAL(result.host()->name(), datum.name);
        BOOST_CHECK_EQUAL(result.host()->key(), datum.key);

        result = kh.find(datum.ip, datum.key, true);
        
        BOOST_CHECK(result.match());
        BOOST_CHECK(!result.mismatch());
        BOOST_CHECK(!result.not_found());

        BOOST_REQUIRE(result.host() != kh.end());
        if (!is_hashed)
            BOOST_CHECK_EQUAL(result.host()->name(), datum.ip);
        BOOST_CHECK_EQUAL(result.host()->key(), datum.key);
    }
}

/**
 * Search for all the test entries.  Each one should result in a match.
 */
BOOST_AUTO_TEST_CASE( find_match )
{
    do_find_match_test("test_known_hosts", false);
}

/**
 * Search for all the test entries in hashed collection.
 * Each one should result in a match.
 */
BOOST_AUTO_TEST_CASE( find_match_hashed )
{
    do_find_match_test("test_known_hosts_hashed", true);
}

void do_find_mismatch_test(
    const boost::filesystem::path& file, bool is_hashed)
{
    openssh_knownhost_collection kh(file);

    // Find each datum twice, once by IP once by name
    BOOST_FOREACH(const test_datum& datum, test_data)
    {
        find_result result = kh.find(datum.name, datum.fail_key, true);
        
        BOOST_CHECK(!result.match());
        BOOST_CHECK(result.mismatch());
        BOOST_CHECK(!result.not_found());

        BOOST_REQUIRE(result.host() != kh.end());
        if (!is_hashed)
            BOOST_CHECK_EQUAL(result.host()->name(), datum.name);
        BOOST_CHECK_EQUAL(result.host()->key(), datum.key);

        result = kh.find(datum.ip, datum.fail_key, true);
        
        BOOST_CHECK(!result.match());
        BOOST_CHECK(result.mismatch());
        BOOST_CHECK(!result.not_found());

        BOOST_REQUIRE(result.host() != kh.end());
        if (!is_hashed)
            BOOST_CHECK_EQUAL(result.host()->name(), datum.ip);
        BOOST_CHECK_EQUAL(result.host()->key(), datum.key);
    }
}

/**
 * Search for each test host with a key that doesn't match.
 */
BOOST_AUTO_TEST_CASE( find_mismatch )
{
    do_find_mismatch_test("test_known_hosts", false);
}

/**
 * Search for each test host with a key that doesn't match.
 */
BOOST_AUTO_TEST_CASE( find_mismatch_hashed )
{
    do_find_mismatch_test("test_known_hosts_hashed", true);
}

/**
 * Search for a non-existent hostname in the collection.
 * This should fail to find a match.
 */
BOOST_AUTO_TEST_CASE( find_fail )
{
    openssh_knownhost_collection kh("test_known_hosts");
    find_result result = kh.find(FAIL_HOST, KEY_A, true);

    BOOST_CHECK(!result.match());
    BOOST_CHECK(!result.mismatch());
    BOOST_CHECK(result.not_found());
    BOOST_CHECK(result.host() == kh.end());
}

/**
 * Search for a non-existent hostname in the collection.
 * This should fail to find a match.
 */
BOOST_AUTO_TEST_CASE( find_fail_hashed )
{
    openssh_knownhost_collection kh("test_known_hosts_hashed");
    find_result result = kh.find(FAIL_HOST, KEY_A, true);

    BOOST_CHECK(!result.match());
    BOOST_CHECK(!result.mismatch());
    BOOST_CHECK(result.not_found());
    BOOST_CHECK(result.host() == kh.end());
}

void do_erase_test(
    const openssh_knownhost_collection& kh, const test_datum& datum,
    bool is_hashed)
{
    std::string expected_ip = (is_hashed) ? "" : datum.ip;
    std::string expected_name = (is_hashed) ? "" : datum.name;

    // find target entry by IP address
    find_result ip_result = kh.find(datum.ip, datum.key, true);
    BOOST_CHECK_EQUAL(ip_result.host()->name(), expected_ip);
    BOOST_CHECK_EQUAL(ip_result.host()->key(), datum.key);

    // erase it which should give us pointer to next entry
    // (the hostname version of the entry)
    knownhost_iterator next = ip_result.host();
    ++next;
    BOOST_CHECK(erase(ip_result.host()) == next);
    BOOST_CHECK_EQUAL(next->name(), expected_name);
    BOOST_CHECK_EQUAL(next->key(), datum.key);

    // searching for this host entry should also work and give an
    // equal iterator
    find_result host_result = kh.find(datum.name, datum.key, true);
    BOOST_CHECK(host_result.match());
    BOOST_CHECK_EQUAL(host_result.host()->name(), expected_name);
    BOOST_CHECK_EQUAL(host_result.host()->key(), datum.key);
    BOOST_CHECK(next == host_result.host());
     
    // but searching for the IP entry we just deleted should fail to
    // find anything
    ip_result = kh.find(datum.ip, datum.key, true);
    BOOST_CHECK(ip_result.not_found());

    // erase host entry as well
    erase(host_result.host());

    // searching for it again should fail this time
    host_result = kh.find(datum.name, datum.key, true);
    BOOST_CHECK(host_result.not_found());
}

void do_erase_test_loop(const boost::filesystem::path& file, bool is_hashed)
{
    BOOST_FOREACH(const test_datum& datum, test_data)
    {
        openssh_knownhost_collection kh(file);

        do_erase_test(kh, datum, is_hashed);
    }
}

/**
 * Erase one item from a collection.
 *
 * We test this for all entries with a fresh collection each time.
 *
 * The item in question should be gone but the other items should still
 * exist.

 * @warning  Strictly speaking we erase two items at a time due to the
 * relationship between host and IP entries.  This may be fragile.
 */
BOOST_AUTO_TEST_CASE( erase_plain )
{
    do_erase_test_loop("test_known_hosts", false);
}

/**
 * Erase one item from a collection of hashed entries.
 */
BOOST_AUTO_TEST_CASE( erase_hashed )
{
    do_erase_test_loop("test_known_hosts_hashed", true);
}

/**
 * Erase all items from a collection.
 *
 * The item in question should be gone but the other items should still
 * exist.

 * @warning  Strictly speaking we erase two items at a time due to the
 * relationship between host and IP entries.  This may be fragile.
 */
BOOST_AUTO_TEST_CASE( erase_all )
{
    openssh_knownhost_collection kh("test_known_hosts");

    BOOST_FOREACH(const test_datum& datum, test_data)
    {
        do_erase_test(kh, datum, false);
    }

    BOOST_CHECK(kh.begin() == kh.end());
}

/**
 * Erase the last item in the collection.
 */
BOOST_AUTO_TEST_CASE( erase_last )
{
    openssh_knownhost_collection kh("test_known_hosts");
    find_result result = kh.find(test_data[2].name, test_data[2].key, true);

    BOOST_REQUIRE(result.host() != kh.end());

    knownhost_iterator next = erase(result.host());
    BOOST_REQUIRE(next == kh.end());

    result = kh.find(test_data[2].name, test_data[2].key, true);
    BOOST_CHECK(result.not_found());
}

/**
 * Add an item to the collection.
 */
BOOST_AUTO_TEST_CASE( add )
{
    openssh_knownhost_collection kh("test_known_hosts");

    kh.add("new.example.com", KEY_B, ssh::host_key::ssh_dss, true);

    find_result result = kh.find("new.example.com", KEY_B, true);

    BOOST_CHECK(result.match());
    BOOST_REQUIRE(result.host() != kh.end());
    BOOST_CHECK_EQUAL(result.host()->name(), "new.example.com");
    BOOST_CHECK_EQUAL(result.host()->key(), KEY_B);
}

/**
 * Lines must be written back exactly as they are read with exception of:
 *  - comma-separated host names being split into separate lines
 *  - newlines stripped
 *  - tabs replaced with spaces
 */
BOOST_AUTO_TEST_CASE( load_save )
{
    const vector<string> lines = list_of
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA==")
        ("host.example.com,192.0.32.10 ssh-rsa AAAAB3NzaC1yc2EAA==")
        ("hostalias1,hostalias2 ssh-rsa AAAAB3NzaC1yc2EAA==")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA== ")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA==\t")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA==\n")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA== \n")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA== test@swish")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA== test swish")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA==\ttest swish")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA== test swish\n")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA== test swish ")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA== test swish \n")
        ("|1|wWleTRHpe2S17RMX0bNldkfB/6Y=|8KTu5EjSLKwlkr0JoNo2QA3uhJs= "
         "ssh-rsa AAAAB3NzaC1yc2EAA==")
        // this one will fail with libssh2 < 1.2.8
        ("host1,host2,host3,192.168.1.1 ssh-rsa AAAAB3NzaC1yc2EAA==");
    const vector<string> expected_output = list_of
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA==")
        ("192.0.32.10 ssh-rsa AAAAB3NzaC1yc2EAA==")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA==")
        ("hostalias2 ssh-rsa AAAAB3NzaC1yc2EAA==")
        ("hostalias1 ssh-rsa AAAAB3NzaC1yc2EAA==")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA== ")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA== ")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA==")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA== ")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA== test@swish")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA== test swish")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA== test swish")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA== test swish")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA== test swish ")
        ("host.example.com ssh-rsa AAAAB3NzaC1yc2EAA== test swish ")
        ("|1|wWleTRHpe2S17RMX0bNldkfB/6Y=|8KTu5EjSLKwlkr0JoNo2QA3uhJs= "
         "ssh-rsa AAAAB3NzaC1yc2EAA==")
        ("192.168.1.1 ssh-rsa AAAAB3NzaC1yc2EAA==")
        ("host3 ssh-rsa AAAAB3NzaC1yc2EAA==")
        ("host2 ssh-rsa AAAAB3NzaC1yc2EAA==")
        ("host1 ssh-rsa AAAAB3NzaC1yc2EAA==");

    openssh_knownhost_collection kh(lines.begin(), lines.end());

    vector<string> output;
    kh.save(kh.begin(), kh.end(), back_inserter(output));

    for (size_t i = 0; i < output.size(); ++i)
    {
        string o = output[i];
        string e = expected_output[i];
        BOOST_CHECK_EQUAL(o, e);
        BOOST_CHECK_EQUAL_COLLECTIONS(
            o.begin(), o.end(), e.begin(), e.end());
    }
}


BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE_END();
