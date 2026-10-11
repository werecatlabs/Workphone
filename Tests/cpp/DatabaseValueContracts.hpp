#pragma once
namespace
{
    using namespace workphone;
    void databaseValueContracts( const std::filesystem::path &folder )
    {
        auto check = []( bool ok, const char *message ) {
            if( !ok ) throw std::runtime_error( message );
        };
        auto manager = make_ptr<DatabaseManager>();
        manager->loadFromFile( ( folder / "legacy-values.db" ).u8string().c_str() );
        auto bound = dynamic_cast<IParameterizedDatabase *>( manager->getDatabase().get() );
        check( bound != nullptr, "Legacy value contracts require real SQLite" );
        auto sql = [&]( const String &statement, const Array<String> &values = Array<String>() ) {
            return bound->queryBound( statement, values );
        };
        check( manager->getSetting( "missing", "fallback" ) == "fallback" &&
               manager->getStateValue( "missing" ).empty(), "Missing legacy tables must return defaults without dereferencing null" );
        check( sql( "CREATE TABLE settings(param TEXT,value TEXT); CREATE TABLE sim_states(param TEXT,value TEXT)" ) != nullptr,
               "Create legacy value fixture" );
        const String key = "quoted'_% ; DROP TABLE settings;--";
        const String value = "can't concatenate SQL '; SELECT 99;--";
        manager->setSetting( key, value );
        check( manager->getSetting( key ) == value, "Quoted setting keys and values must round trip as data" );
        manager->setSetting( key, "updated" );
        check( manager->getSetting( key ) == "updated", "Setting updates must preserve exact key identity" );
        manager->setStateValue( key, value );
        check( manager->hasStateValue( key ) && manager->getStateValue( key ) == value,
               "State values must use the same bound exact-key contract" );
        manager->setSettingAsBool( "bool", true );
        manager->setSettingAsInt( "int", -42 );
        manager->setSettingAsFloat( "float", 1.25f );
        check( manager->getSettingAsBool( "bool" ) && manager->getSettingAsInt( "int" ) == -42 &&
               manager->getSettingAsFloat( "float" ) == 1.25f, "Typed settings must retain their public conversion behavior" );
        manager->setStateValueAsBool( "bool", true );
        manager->setStateValueAsInt( "int", -17 );
        manager->setStateValueAsFloat( "float", 0.5f );
        check( manager->getStateValueAsBool( "bool" ) && manager->getStateValueAsInt( "int" ) == -17 &&
               manager->getStateValueAsFloat( "float" ) == 0.5f, "Typed state values must retain conversion behavior" );
        check( sql( "BEGIN IMMEDIATE" ) != nullptr, "Start caller-owned transaction" );
        manager->setSetting( key, "uncommitted" );
        check( manager->getSetting( key ) == "uncommitted", "Nested key/value writes must use the caller's transaction" );
        check( sql( "ROLLBACK" ) != nullptr && manager->getSetting( key ) == "updated",
               "Key/value updates must never commit the caller's transaction" );
        check( sql( "INSERT INTO settings(param,value) VALUES('duplicate','one'); INSERT INTO settings(param,value) VALUES('duplicate','two')" ) != nullptr,
               "Seed duplicate legacy keys" );
        check( manager->getSetting( "duplicate", "ambiguous" ) == "ambiguous", "Ambiguous settings must return the explicit default" );
        manager->setSetting( "duplicate", "overwrite" );
        auto rows = sql( "SELECT count(*) AS n FROM settings WHERE param='duplicate' AND value IN ('one','two')" );
        check( rows && rows->getFieldValueAsInt( "n" ) == 2, "Duplicate-key writes must preserve both authored rows for repair" );
        check( sql( "CREATE TRIGGER reject_state BEFORE UPDATE ON sim_states WHEN NEW.param='blocked' BEGIN SELECT RAISE(ABORT,'fixture rejection'); END" ) != nullptr,
               "Create failed-update fixture" );
        manager->setStateValue( "blocked", "before" );
        manager->setStateValue( "blocked", "after" );
        check( manager->getStateValue( "blocked" ) == "before", "Failed key/value writes must roll back" );
        manager->setStateValue( "after-failure", "works" );
        check( manager->getStateValue( "after-failure" ) == "works", "Failed writes must not strand the connection in a savepoint" );
        std::thread first( [&] { for( int i=0; i<20; ++i ) manager->setSetting( "concurrent", "first" ); } );
        std::thread second( [&] { for( int i=0; i<20; ++i ) manager->setSetting( "concurrent", "second" ); } );
        first.join(); second.join();
        rows = sql( "SELECT count(*) AS n FROM settings WHERE param='concurrent'" );
        check( rows && rows->getFieldValueAsInt( "n" ) == 1, "Concurrent writes through one manager must not create duplicate keys" );
        auto reference = make_ptr<DatabaseManager>();
        const auto referencePath = folder / "reference's.db";
        reference->loadFromFile( referencePath.u8string().c_str() );
        check( reference->executeDML( "CREATE TABLE settings(param TEXT,value TEXT)" ) >= 0, "Create attached settings fixture" );
        reference->setSetting( key, value );
        reference->close();
        manager->attach( referencePath.u8string().c_str() );
        check( manager->getSettingRef( key ) == value, "Attached database paths and setting names must accept apostrophes safely" );
        manager->unload( nullptr );
        reference->unload( nullptr );
        std::cout << "PASS: bound legacy settings/state, defaults, nested transactions, rollback, duplicate repair and concurrent upserts\n";
    }
}
