#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

String clean_path( const String &path )
{
    return StringUtil::cleanupPath( path );
}

BOOST_AUTO_TEST_CASE( string_util_parse_bool )
{
    BOOST_CHECK( StringUtil::parseBool( "true" ) == true );
    BOOST_CHECK( StringUtil::parseBool( "false" ) == false );
    BOOST_CHECK( StringUtilW::parseBool( L"true" ) == true );
    BOOST_CHECK( StringUtilW::parseBool( L"false" ) == false );
}

BOOST_AUTO_TEST_CASE( empty_path )
{
    String path = "";
    String expected = "";

    auto cleanPath = clean_path( path );
    BOOST_CHECK_EQUAL( cleanPath, expected );
}

BOOST_AUTO_TEST_CASE( dot_path )
{
    String path = ".";
    String expected = "";
    auto cleanPath = clean_path( path );
    BOOST_CHECK_EQUAL( cleanPath, expected );
}

BOOST_AUTO_TEST_CASE( dot_dot_path )
{
    String path = "..";
    String expected = "..";
    BOOST_CHECK_EQUAL( clean_path( path ), expected );
}

BOOST_AUTO_TEST_CASE( absolute_path )
{
    String path = "/a/b/../c/./d/../e";
    String expected = "/a/c/e";
    BOOST_CHECK_EQUAL( clean_path( path ), expected );
}

BOOST_AUTO_TEST_CASE( relative_path )
{
    String path = "a/b/../c/./d/../e";
    String expected = "a/c/e";
    BOOST_CHECK_EQUAL( clean_path( path ), expected );
}

BOOST_AUTO_TEST_CASE( dot_dot_absolute_path )
{
    String path = "/a/b/../c/./d/../../e";
    String expected = "/a/e";
    auto cleanPath = clean_path( path );
    BOOST_CHECK_EQUAL( cleanPath, expected );
}

BOOST_AUTO_TEST_CASE( dot_dot_relative_path )
{
    String path = "a/b/../c/./d/../../e";
    String expected = "a/e";
    auto cleanPath = clean_path( path );
    BOOST_CHECK_EQUAL( cleanPath, expected );
}

BOOST_AUTO_TEST_CASE( cleanup_path )
{
    // Test with a relative path
    BOOST_CHECK_EQUAL( StringUtility<c8>::cleanupPath( "foo/bar/../baz" ), "foo/baz" );

    // Test with an absolute path
    //BOOST_CHECK_EQUAL( StringUtility<String>::cleanupPath( "/foo/bar/../baz" ), "/foo/baz" );

    // Test with a path containing consecutive slashes
    BOOST_CHECK_EQUAL( StringUtility<c8>::cleanupPath( "foo//bar/baz" ), "foo/bar/baz" );

    // Test with a path containing backslashes
    BOOST_CHECK_EQUAL( StringUtility<c8>::cleanupPath( "foo\\bar\\baz" ), "foo/bar/baz" );

    // Test with a path containing the current directory
    BOOST_CHECK_EQUAL( StringUtility<c8>::cleanupPath( "foo/./bar/baz" ), "foo/bar/baz" );

    // Test with a path containing the parent directory
    BOOST_CHECK_EQUAL( StringUtility<c8>::cleanupPath( "foo/bar/../baz" ), "foo/baz" );

    // Test with a path ending with a directory separator
    BOOST_CHECK_EQUAL( StringUtility<c8>::cleanupPath( "foo/bar/baz/" ), "foo/bar/baz" );

    // Test with a path equal to "./"
    BOOST_CHECK_EQUAL( StringUtility<c8>::cleanupPath( "./" ), "." );

    // Test with a path equal to "/"
    BOOST_CHECK_EQUAL( StringUtility<c8>::cleanupPath( "/" ), "/" );
}
