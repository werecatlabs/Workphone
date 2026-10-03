#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

BOOST_AUTO_TEST_CASE( query_test )
{
    /*
    sqlite3 *db;
    int rc = sqlite3_open( ":memory:", &db );
    BOOST_REQUIRE_EQUAL( rc, SQLITE_OK );

    const char *create_table_sql = "CREATE TABLE test (id INTEGER, name TEXT);";
    char *error_msg = nullptr;
    rc = sqlite3_exec( db, create_table_sql, nullptr, nullptr, &error_msg );
    BOOST_REQUIRE_EQUAL( rc, SQLITE_OK );

    const char *insert_sql = "INSERT INTO test (id, name) VALUES (1, 'foo');";
    rc = sqlite3_exec( db, insert_sql, nullptr, nullptr, &error_msg );
    BOOST_REQUIRE_EQUAL( rc, SQLITE_OK );

    const char *select_sql = "SELECT id, name FROM test WHERE id = 1;";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2( db, select_sql, -1, &stmt, nullptr );
    BOOST_REQUIRE_EQUAL( rc, SQLITE_OK );

    rc = sqlite3_step( stmt );
    BOOST_REQUIRE_EQUAL( rc, SQLITE_ROW );

    int id = sqlite3_column_int( stmt, 0 );
    BOOST_REQUIRE_EQUAL( id, 1 );

    const unsigned char *name = sqlite3_column_text( stmt, 1 );
    BOOST_REQUIRE_EQUAL( std::string( reinterpret_cast<const char *>( name ) ), "foo" );

    sqlite3_finalize( stmt );
    sqlite3_close( db );
    */
}
