#include "UnitTests.hpp"
#include <Workphone/Interface/Database/IDatabase.hpp>
#include <Workphone/Interface/Database/IDatabaseQuery.hpp>
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <vector>

using namespace workphone;

namespace
{
    String makeUniqueDatabaseDirectory()
    {
        auto tempDirectory = std::filesystem::temp_directory_path();
        auto folderName = String( "lioncat_database_tests_" ) + StringUtil::getUUID();
        return StringUtil::cleanupPath( String( tempDirectory.string().c_str() ) + "/" + folderName );
    }

    SmartPtr<IDatabase> makeDatabase()
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_REQUIRE( factoryManager );

        return factoryManager->make_object<IDatabase>();
    }

    struct DatabaseTestContext
    {
        DatabaseTestContext()
        {
            database = makeDatabase();
            if( !database )
            {
                BOOST_TEST_MESSAGE( "Skipping database test because no database plugin is available." );
                return;
            }

            databaseDirectory = makeUniqueDatabaseDirectory();
            std::filesystem::create_directories( databaseDirectory.c_str() );

            databasePath = StringUtil::cleanupPath( databaseDirectory + "/database_" +
                                                    StringUtil::getUUID() + ".db" );

            database->loadFromFile( databasePath, "" );
            isAvailable = true;
        }

        ~DatabaseTestContext()
        {
            if( database )
            {
                database->close();
                database->unload( nullptr );
                database = nullptr;
            }

            if( !databaseDirectory.empty() )
            {
                std::error_code errorCode;
                std::filesystem::remove_all( databaseDirectory.c_str(), errorCode );
            }
        }

        SmartPtr<IDatabase> reopen()
        {
            if( database )
            {
                database->close();
                database->unload( nullptr );
                database = nullptr;
            }

            database = makeDatabase();
            BOOST_REQUIRE( database );
            database->loadFromFile( databasePath, "" );
            return database;
        }

        bool isAvailable = false;
        TestGuard guard;
        String databaseDirectory;
        String databasePath;
        SmartPtr<IDatabase> database;
    };

    void createSampleTable( SmartPtr<IDatabase> database )
    {
        BOOST_REQUIRE( database );

        database->queryDML(
            "CREATE TABLE items ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT NOT NULL,"
            "quantity INTEGER NOT NULL,"
            "price REAL NOT NULL,"
            "note TEXT NULL"
            ");" );
    }

    void insertItem( SmartPtr<IDatabase> database, const String &name, s32 quantity, f32 price,
                     const String &noteSql )
    {
        BOOST_REQUIRE( database );

        database->queryDML( String( "INSERT INTO items (name, quantity, price, note) VALUES ('" ) +
                            name + "', " + StringUtil::toString( quantity ) + ", " +
                            StringUtil::toString( price ) + ", " + noteSql + ");" );
    }

    size_t countRows( SmartPtr<IDatabaseQuery> query )
    {
        size_t count = 0;
        while( query && !query->eof() )
        {
            ++count;
            query->nextRow();
        }

        return count;
    }

    std::vector<String> collectFieldValues( SmartPtr<IDatabaseQuery> query, const String &field )
    {
        std::vector<String> values;
        while( query && !query->eof() )
        {
            values.push_back( query->getFieldValue( field ) );
            query->nextRow();
        }

        return values;
    }
}  // namespace

BOOST_AUTO_TEST_SUITE( DatabaseTests )

BOOST_AUTO_TEST_CASE( database_create_insert_query_and_field_access )
{
    DatabaseTestContext context;
    if( !context.isAvailable )
    {
        return;
    }

    createSampleTable( context.database );
    insertItem( context.database, "hammer", 3, 12.5f, "'steel'" );
    insertItem( context.database, "saw", 7, 8.25f, "'wood'" );

    auto query =
        context.database->query( "SELECT id, name, quantity, price, note FROM items ORDER BY id ASC;" );

    BOOST_REQUIRE( query );
    BOOST_REQUIRE( !query->eof() );
    BOOST_CHECK_EQUAL( query->getNumFields(), 5u );
    BOOST_CHECK_EQUAL( query->getFieldName( 0 ), "id" );
    BOOST_CHECK_EQUAL( query->getFieldName( 1 ), "name" );
    BOOST_CHECK_EQUAL( query->getFieldValue( "name" ), "hammer" );
    BOOST_CHECK_EQUAL( query->getFieldValue( 1 ), "hammer" );
    BOOST_CHECK_EQUAL( query->getFieldValueAsInt( "quantity" ), 3 );
    BOOST_CHECK_CLOSE( query->getFieldValueAsFloat( "price" ), 12.5f, 0.001f );
    BOOST_CHECK( !query->isFieldValueNull( "note" ) );

    query->nextRow();
    BOOST_REQUIRE( !query->eof() );
    BOOST_CHECK_EQUAL( query->getFieldValue( "name" ), "saw" );
    BOOST_CHECK_EQUAL( query->getFieldValueAsInt( "quantity" ), 7 );
    BOOST_CHECK_CLOSE( query->getFieldValueAsFloat( "price" ), 8.25f, 0.001f );

    query->nextRow();
    BOOST_CHECK( query->eof() );
    BOOST_CHECK_EQUAL( query->getNumFields(), 0u );
    BOOST_CHECK_EQUAL( query->getFieldName( 0 ), "" );
    BOOST_CHECK_EQUAL( query->getFieldValue( "name" ), "" );
}

BOOST_AUTO_TEST_CASE( database_data_persists_after_close_and_reopen )
{
    DatabaseTestContext context;
    if( !context.isAvailable )
    {
        return;
    }

    createSampleTable( context.database );
    insertItem( context.database, "persisted", 42, 19.75f, "'survives reopen'" );

    auto reopenedDatabase = context.reopen();
    auto query = reopenedDatabase->query(
        "SELECT name, quantity, price, note FROM items WHERE name = 'persisted';" );

    BOOST_REQUIRE( query );
    BOOST_REQUIRE( !query->eof() );
    BOOST_CHECK_EQUAL( query->getFieldValue( "name" ), "persisted" );
    BOOST_CHECK_EQUAL( query->getFieldValueAsInt( "quantity" ), 42 );
    BOOST_CHECK_CLOSE( query->getFieldValueAsFloat( "price" ), 19.75f, 0.001f );
    BOOST_CHECK_EQUAL( query->getFieldValue( "note" ), "survives reopen" );
    query->nextRow();
    BOOST_CHECK( query->eof() );
}

BOOST_AUTO_TEST_CASE( database_queryDML_supports_transactions_and_rollback )
{
    DatabaseTestContext context;
    if( !context.isAvailable )
    {
        return;
    }

    createSampleTable( context.database );
    context.database->queryDML( "BEGIN TRANSACTION;" );
    insertItem( context.database, "rolled-back", 1, 1.0f, "NULL" );
    context.database->queryDML( "ROLLBACK;" );

    BOOST_CHECK_EQUAL(
        countRows( context.database->query( "SELECT * FROM items WHERE name = 'rolled-back';" ) ), 0u );

    context.database->queryDML( "BEGIN TRANSACTION;" );
    insertItem( context.database, "committed", 2, 2.0f, "NULL" );
    context.database->queryDML( "COMMIT;" );

    BOOST_CHECK_EQUAL(
        countRows( context.database->query( "SELECT * FROM items WHERE name = 'committed';" ) ), 1u );
}

BOOST_AUTO_TEST_CASE( database_empty_results_and_missing_fields_are_safe )
{
    DatabaseTestContext context;
    if( !context.isAvailable )
    {
        return;
    }

    createSampleTable( context.database );

    auto emptyQuery = context.database->query( "SELECT id, name FROM items WHERE id = -1;" );
    BOOST_REQUIRE( emptyQuery );
    BOOST_CHECK( emptyQuery->eof() );
    BOOST_CHECK_EQUAL( emptyQuery->getNumFields(), 0u );
    BOOST_CHECK_EQUAL( emptyQuery->getFieldName( 99 ), "" );
    BOOST_CHECK_EQUAL( emptyQuery->getFieldValue( 99 ), "" );
    BOOST_CHECK_EQUAL( emptyQuery->getFieldValue( "missing" ), "" );
    BOOST_CHECK( emptyQuery->isFieldValueNull( "missing" ) );
}

BOOST_AUTO_TEST_CASE( database_null_values_are_reported_without_crashing )
{
    DatabaseTestContext context;
    if( !context.isAvailable )
    {
        return;
    }

    createSampleTable( context.database );
    insertItem( context.database, "nullable", 5, 3.5f, "NULL" );

    auto query = context.database->query( "SELECT name, note FROM items WHERE name = 'nullable';" );
    BOOST_REQUIRE( query );
    BOOST_REQUIRE( !query->eof() );
    BOOST_CHECK_EQUAL( query->getFieldValue( "name" ), "nullable" );
    BOOST_CHECK_EQUAL( query->getFieldValue( "note" ), "" );
    BOOST_CHECK( query->isFieldValueNull( "note" ) );
}

BOOST_AUTO_TEST_CASE( database_invalid_or_empty_sql_returns_null_result )
{
    DatabaseTestContext context;
    if( !context.isAvailable )
    {
        return;
    }

    BOOST_CHECK( !context.database->query( "" ) );
    BOOST_CHECK( !context.database->query( "SELECT * FROM table_that_does_not_exist;" ) );

    context.database->queryDML( "CREATE TABLE valid_table (id INTEGER PRIMARY KEY);" );
    context.database->queryDML( "INSERT INTO valid_table (id) VALUES (1);" );
    context.database->queryDML( "INSERT INTO missing_table (id) VALUES (2);" );

    auto query = context.database->query( "SELECT id FROM valid_table;" );
    BOOST_REQUIRE( query );
    BOOST_REQUIRE( !query->eof() );
    BOOST_CHECK_EQUAL( query->getFieldValueAsInt( "id" ), 1 );
}

BOOST_AUTO_TEST_CASE( database_wide_string_overloads_match_narrow_query_behavior )
{
    DatabaseTestContext context;
    if( !context.isAvailable )
    {
        return;
    }

    context.database->queryDML(
        StringW( L"CREATE TABLE wide_items (id INTEGER PRIMARY KEY, name TEXT, quantity INTEGER);" ) );
    context.database->queryDML(
        StringW( L"INSERT INTO wide_items (id, name, quantity) VALUES (1, 'wide', 9);" ) );

    auto query =
        context.database->query( StringW( L"SELECT id, name, quantity FROM wide_items WHERE id = 1;" ) );

    BOOST_REQUIRE( query );
    BOOST_REQUIRE( !query->eof() );
    BOOST_CHECK_EQUAL( query->getFieldValueAsInt( "id" ), 1 );
    BOOST_CHECK_EQUAL( query->getFieldValue( "name" ), "wide" );
    BOOST_CHECK_EQUAL( query->getFieldValueAsInt( "quantity" ), 9 );
}

BOOST_AUTO_TEST_CASE( database_multiple_rows_preserve_requested_order )
{
    DatabaseTestContext context;
    if( !context.isAvailable )
    {
        return;
    }

    createSampleTable( context.database );
    insertItem( context.database, "third", 30, 3.0f, "NULL" );
    insertItem( context.database, "first", 10, 1.0f, "NULL" );
    insertItem( context.database, "second", 20, 2.0f, "NULL" );

    auto names = collectFieldValues(
        context.database->query( "SELECT name FROM items ORDER BY quantity ASC;" ), "name" );

    BOOST_REQUIRE_EQUAL( names.size(), 3u );
    BOOST_CHECK_EQUAL( names[0], "first" );
    BOOST_CHECK_EQUAL( names[1], "second" );
    BOOST_CHECK_EQUAL( names[2], "third" );
}

BOOST_AUTO_TEST_SUITE_END()
