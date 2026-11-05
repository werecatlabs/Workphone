#pragma once

namespace
{
    void measureCatalogScale( const std::filesystem::path &folder, int assets )
    {
        namespace fs = std::filesystem;
        using Clock = std::chrono::steady_clock;
        const auto root = folder / std::to_string( assets );
        fs::create_directory( root );
        const auto databasePath = root / "catalog.db";
        auto catalog = make_ptr<AssetDatabaseManager>();
        require( catalog->setProjectRoot( root.u8string().c_str() ), "Scale fixture source root" );
        catalog->loadFromFile( databasePath.u8string().c_str() );
        const auto database = catalog->getDatabase();
        require( database && database->isLoaded(), "Scale fixture catalog open" );

        // Create the production reference schema through its normal save path.
        auto document = make_ptr<Properties>();
        const auto serialized = DataUtil::toString( document.get(), true );
        const Array<u8> bytes( serialized.begin(), serialized.end() );
        AssetReferenceSave save;
        String error;
        require( beginAssetReferenceSave( catalog, "seed.fbscene", document, bytes, save, error ),
                 error.c_str() );
        std::ofstream file( root / "seed.fbscene", std::ios::binary );
        file.write( serialized.data(), static_cast<std::streamsize>( serialized.size() ) );
        file.close();
        require( file.good() && completeAssetReferenceSave( catalog, save, error ), error.c_str() );

        const auto seedStart = Clock::now();
        const auto execute = [&]( const String &statement ) {
            require( sql( database, statement ) != nullptr, statement.c_str() );
        };
        execute( "BEGIN IMMEDIATE" );
        execute( "CREATE TEMP TABLE scale_digit(value TEXT PRIMARY KEY)" );
        for( int i = 0; i < 10; ++i )
        {
            require( sql( database, "INSERT INTO scale_digit VALUES(?)", { StringUtil::toString( i ) } ) != nullptr,
                     "Seed scale fixture digits" );
        }
        const String keys = "CREATE TEMP TABLE scale_keys AS SELECT a.value||b.value||c.value||d.value||e.value AS suffix "
                            "FROM scale_digit a,scale_digit b,scale_digit c,scale_digit d,scale_digit e";
        execute( assets == 10000 ? keys + " WHERE a.value='0'" : keys );
        execute( u8"INSERT INTO resources(uuid,path,type,kind,path_key) "
                 "SELECT 'f0000000-0000-4000-8000-0000000'||suffix,"
                 "'assets/'||suffix||'/café''s.texres','Texture','file',"
                 "'assets/'||suffix||'/café''s.texres' FROM scale_keys" );
        execute( "INSERT INTO wp_asset_reference_documents(path_key,path,revision,digest,state) "
                 "SELECT 'scenes/'||suffix||'.fbscene','scenes/'||suffix||'.fbscene','scale','scale','ready' "
                 "FROM scale_keys" );
        execute( "INSERT INTO wp_asset_references(source_key,slot,target_uuid,target_catalog,expected_type,required) "
                 "SELECT 'scenes/'||suffix||'.fbscene','material-'||value,"
                 "CASE WHEN value='0' THEN 'f0000000-0000-4000-8000-000000000000' "
                 "ELSE 'f0000000-0000-4000-8000-0000000'||substr(suffix,1,4)||value END,"
                 "'project','Texture',1 FROM scale_keys,scale_digit" );
        execute( "DROP TABLE scale_keys" );
        execute( "DROP TABLE scale_digit" );
        execute( "COMMIT" );
        const auto seedMilliseconds = std::chrono::duration<double,std::milli>( Clock::now() - seedStart ).count();
        require( scalar( database, "SELECT count(*) AS n FROM resources" ) == assets &&
                 scalar( database, "SELECT count(*) AS n FROM wp_asset_references" ) == assets * 10,
                 "Scale fixture must contain the declared catalog and graph sizes" );

        const String suffix = assets == 10000 ? "09999" : "99999";
        const String id = "f0000000-0000-4000-8000-0000000" + suffix;
        const auto exact = [&] {
            AssetDatabaseManager::EntrySnapshot entry;
            require( catalog->tryGetEntry( id, entry ) && entry.uuid == id, "Scale exact UUID lookup" );
        };
        AssetDatabaseManager::EntryQuery filter;
        filter.pageSize = 64;
        filter.type = "Texture";
        filter.includeScenes = false;
        filter.search = suffix + u8"/café's.texres";
        const auto search = [&] {
            const auto page = catalog->queryEntries( filter );
            require( page.succeeded && page.entries.size() == 1 && page.entries.front().uuid == id && !page.hasMore,
                     "Scale rare search must find the final Unicode/quoted basename" );
        };
        auto denseFilter = filter;
        denseFilter.search = u8"café's.texres";
        const auto denseSearch = [&] {
            const auto page = catalog->queryEntries( denseFilter );
            require( page.succeeded && page.entries.size() == 64 && page.hasMore,
                     "Scale common search must retain a full bounded page" );
        };
        const auto whereUsed = [&] {
            const auto page = queryAssetReferences( catalog, id, true, "project", 64 );
            require( page.succeeded && page.edges.size() == 10 && !page.hasMore &&
                     page.edges.front().diagnostic.empty(), "Scale indexed where-used lookup" );
        };
        const String hot = "f0000000-0000-4000-8000-000000000000";
        const auto hotPage = queryAssetReferences( catalog, hot, true, "project", 64 );
        const auto next = queryAssetReferences( catalog, hot, true, "project", 64, &hotPage.next );
        require( hotPage.succeeded && hotPage.edges.size() == 64 && hotPage.hasMore &&
                 next.succeeded && next.edges.size() == 64 && next.hasMore &&
                 hotPage.edges.back().sourcePath != next.edges.front().sourcePath,
                 "Scale high-fanout reverse queries must continue in bounded pages" );

        const auto percentile95 = [&]( const auto &operation ) {
            operation();
            std::vector<double> samples;
            for( int i = 0; i < 40; ++i )
            {
                const auto start = Clock::now();
                operation();
                samples.push_back( std::chrono::duration<double,std::milli>( Clock::now() - start ).count() );
            }
            std::sort( samples.begin(), samples.end() );
            return samples[37];
        };
        std::cout << "CatalogScale assets=" << assets << " edges=" << assets * 10
                  << " seed_ms=" << seedMilliseconds << " exact_p95_ms=" << percentile95( exact )
                  << " rare_search_p95_ms=" << percentile95( search )
                  << " dense_search_p95_ms=" << percentile95( denseSearch )
                  << " where_used_p95_ms=" << percentile95( whereUsed ) << '\n';
        for( const String &statement : {
                 String( "SELECT uuid FROM resources WHERE uuid=?" ),
                 String( "SELECT id FROM wp_asset_references WHERE target_catalog='project' AND target_uuid=? ORDER BY id LIMIT 64" ) } )
        {
            auto plan = sql( database, "EXPLAIN QUERY PLAN " + statement, { id } );
            require( plan && !plan->eof(), "Scale fixture query plan" );
            for( ; !plan->eof(); plan->nextRow() )
                std::cout << "CatalogScalePlan " << plan->getFieldValue( "detail" ) << '\n';
        }
        const auto pragma = [&]( const String &name ) {
            auto row = sql( database, "PRAGMA " + name );
            require( row && !row->eof(), "Scale fixture storage pragma" );
            return row->getFieldValueAsInt( name );
        };
        const int pageSize = pragma( "page_size" );
        for( const String &index : { String( "idx_resources_uuid_unique" ), String( "idx_resources_kind_path" ),
                                    String( "idx_resources_uuid_browse" ),
                                    String( "wp_asset_reference_reverse" ), String( "wp_asset_reference_forward" ) } )
        {
            execute( "BEGIN IMMEDIATE" );
            const int before = pragma( "freelist_count" );
            execute( "DROP INDEX " + index );
            const int pages = pragma( "freelist_count" ) - before;
            execute( "ROLLBACK" );
            require( pages > 0 && pageSize > 0, "Measure index allocation in the disposable catalog" );
            std::cout << "CatalogScaleIndex " << index << " bytes=" << u64( pages ) * pageSize << '\n';
        }
        catalog->unload( nullptr );
        std::cout << "CatalogScaleStorage assets=" << assets << " database_bytes=" << fs::file_size( databasePath ) << '\n';
    }
}
