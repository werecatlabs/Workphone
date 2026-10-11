#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Database/MediaAssetCatalog.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <Workphone/Interface/Database/IParameterizedDatabase.hpp>
#include <boost/uuid/name_generator_sha1.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <filesystem>
#include <set>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#endif

namespace workphone
{
    namespace
    {
        namespace fs = std::filesystem;
        const String mediaNamespace = "workphone.builtin.media.v1";
        SmartPtr<IDatabaseQuery> mediaQuery( SmartPtr<AssetDatabaseManager> catalog,
                                            const String &sql, const Array<String> &values = {} )
        {
            auto db = catalog->getDatabase();
            auto bound = db ? dynamic_cast<IParameterizedDatabase *>( db.get() ) : nullptr;
            return bound ? bound->queryBound( sql, values ) : nullptr;
        }
        bool ownedMediaCatalog( SmartPtr<AssetDatabaseManager> catalog )
        {
            auto marker = mediaQuery( catalog,
                "SELECT namespace,version FROM wp_media_catalog WHERE id=1" );
            return marker && !marker->eof() && marker->getFieldValue( "namespace" ) == mediaNamespace &&
                   marker->getFieldValue( "version" ) == "1";
        }
        String mediaType( const fs::path &path )
        {
            auto ext = path.extension().u8string();
            for( auto &c : ext ) if( c >= 'A' && c <= 'Z' ) c += 'a' - 'A';
            if( ext == ".ttf" || ext == ".otf" || ext == ".font" ) return "Font";
            if( ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" ||
                ext == ".tga" || ext == ".gif" || ext == ".dds" || ext == ".ktx" || ext == ".ktx2" ) return "Texture";
            if( ext == ".mat" || ext == ".material" ) return "Material";
            if( ext == ".lua" ) return "script";
            if( ext == ".wav" || ext == ".ogg" || ext == ".mp3" || ext == ".flac" ) return "Sound";
            if( ext == ".fbmeshbin" ) return "Mesh";
            return {};
        }
    }
    bool buildMediaAssetCatalog( const String &mediaRoot, const String &requestedPath, String &error )
    {
        error.clear();
        fs::path temporary;
        SmartPtr<AssetDatabaseManager> catalog;
        bool transaction = false;
        try
        {
            String root;
            if( !canonicalAssetCatalogRoot( mediaRoot, root, error ) ||
                !fs::is_directory( fs::u8path( root.c_str() ) ) )
                throw std::runtime_error( "Bundled media root is not an existing directory." );
            auto destination = fs::absolute( fs::u8path( requestedPath.empty() ?
                ( root + "/.workphone/media-assets.db" ).c_str() : requestedPath.c_str() ) ).lexically_normal();
            if( !mediaType( destination ).empty() )
                throw std::runtime_error( "A media catalog destination cannot use a source-asset extension." );
            const bool exists = fs::exists( destination );
            // Validate ownership read-only before a builder opens an existing DB for writes.
            if( exists )
            {
                auto old = make_ptr<AssetDatabaseManager>();
                if( !old->setProjectRoot( root ) || !old->openReadOnly( destination.u8string().c_str(), error ) ||
                    !ownedMediaCatalog( old ) )
                    throw std::runtime_error( "Destination is not an owned built-in media catalog; it was left unchanged." );
                old->unload( nullptr );
            }
            struct Row { String uuid, path, key, type, usage; };
            Array<Row> entries;
            std::set<std::string> keys, ids;
            const auto namespaceId = StringUtil::parseUUID( "8daa280e-45e1-4a75-9b22-84a3cc2e5991" );
            boost::uuids::uuid namespaceUuid;
            std::copy( namespaceId.data(), namespaceId.data() + namespaceId.size(), namespaceUuid.begin() );
            boost::uuids::name_generator_sha1 uuid( namespaceUuid );
            size_t visited = 0;
            for( fs::recursive_directory_iterator it( fs::u8path( root.c_str() ) ), end; it != end; ++it )
            {
                if( ++visited > 500000 || it.depth() > 64 )
                    throw std::runtime_error( "Media inventory exceeds its traversal budget." );
                auto status = it->symlink_status();
                if( fs::is_symlink( status ) ) { it.disable_recursion_pending(); continue; }
#ifdef _WIN32
                // Directory junctions are reparse points too. Never traverse a
                // link outside the installation or into a recursive tree.
                const auto attributes = GetFileAttributesW( it->path().c_str() );
                if( attributes == INVALID_FILE_ATTRIBUTES )
                    throw std::runtime_error( "Cannot inspect a media path." );
                if( attributes & FILE_ATTRIBUTE_REPARSE_POINT )
                { it.disable_recursion_pending(); continue; }
#endif
                if( fs::is_directory( status ) )
                {
                    auto name = it->path().filename().u8string();
#ifdef _WIN32
                    for( auto &c : name ) if( c >= 'A' && c <= 'Z' ) c += 'a' - 'A';
#endif
                    if( name == ".workphone" || name == ".git" ) it.disable_recursion_pending();
                    continue;
                }
                if( !fs::is_regular_file( status ) ) continue;
                auto type = mediaType( it->path() );
                if( type.empty() ) continue;
                if( entries.size() >= 100000 ) throw std::runtime_error( "Media catalog exceeds the 100000-asset budget." );
                AssetCatalogPath path;
                if( !canonicalAssetCatalogPath( root, it->path().u8string().c_str(), path, error ) )
                    throw std::runtime_error( error.c_str() );
                auto id = boost::uuids::to_string( uuid( std::string( ( mediaNamespace + ":" + path.key ).c_str() ) ) );
                if( !keys.insert( path.key.c_str() ).second || !ids.insert( id.c_str() ).second )
                    throw std::runtime_error( "Bundled media has colliding canonical paths or identities." );
                // Runtime eligibility is opt-in by a dedicated bundled Runtime tree.
                const bool runtime = path.key.find( "Runtime/" ) == 0 || path.key.find( "runtime/" ) == 0;
                entries.push_back( { id, path.path, path.key, type, runtime ? "runtime" : "editor-only" } );
            }
            std::sort( entries.begin(), entries.end(),
                []( const Row &a, const Row &b ) { return a.key < b.key; } );
            fs::create_directories( destination.parent_path() );
            auto working = destination;
            if( !exists )
            {
                temporary = destination;
                temporary += ".building." + std::string( StringUtil::getUUID().c_str() );
                working = temporary;
            }
            catalog = make_ptr<AssetDatabaseManager>();
            if( !catalog->setProjectRoot( root ) ) throw std::runtime_error( "Cannot bind media source root." );
            catalog->loadFromFile( working.u8string().c_str() );
            auto db = catalog->getDatabase();
            DatabaseConnectionLock guard( db ? dynamic_cast<ISerializedDatabase *>( db.get() ) : nullptr );
            if( !db || !db->isLoaded() || !mediaQuery( catalog, "BEGIN IMMEDIATE" ) )
                throw std::runtime_error( "Cannot start media catalog publication." );
            transaction = true;
            auto run = [&]( const String &sql, const Array<String> &values = Array<String>() ) {
                if( !mediaQuery( catalog, sql, values ) ) throw std::runtime_error( "Media catalog publication failed." );
            };
            run( "CREATE TABLE IF NOT EXISTS wp_media_catalog(id INTEGER PRIMARY KEY CHECK(id=1),namespace TEXT NOT NULL,version INTEGER NOT NULL)" );
            run( "CREATE TABLE IF NOT EXISTS wp_media_entries(uuid TEXT PRIMARY KEY REFERENCES resources(uuid) ON DELETE CASCADE,usage TEXT NOT NULL CHECK(usage IN ('runtime','editor-only')))" );
            run( "INSERT OR REPLACE INTO wp_media_catalog(id,namespace,version) VALUES(1,?,1)", { mediaNamespace } );
            run( "DELETE FROM wp_media_entries" );
            run( "DELETE FROM resources" );
            for( const auto &entry : entries )
            {
                run( "INSERT INTO resources(uuid,path,path_key,type,kind) VALUES(?,?,?,?,'file')",
                     { entry.uuid, entry.path, entry.key, entry.type } );
                run( "INSERT INTO wp_media_entries(uuid,usage) VALUES(?,?)", { entry.uuid, entry.usage } );
            }
            run( "COMMIT" );
            transaction = false;
            catalog->unload( nullptr );
            if( !exists )
            {
                // Same-directory exclusive publication; never replace a racing builder's DB.
                fs::create_hard_link( temporary, destination );
                fs::remove( temporary );
                temporary.clear();
            }
            return true;
        }
        catch( const std::exception &failure )
        {
            error = failure.what();
            if( catalog )
            {
                if( transaction ) mediaQuery( catalog, "ROLLBACK" );
                catalog->unload( nullptr );
            }
            if( !temporary.empty() ) { std::error_code ignored; fs::remove( temporary, ignored ); }
            return false;
        }
    }
    SmartPtr<AssetDatabaseManager> openMediaAssetCatalog( const String &mediaRoot, String &error,
                                                       const String &catalogPath )
    {
        error.clear();
        String root;
        if( !canonicalAssetCatalogRoot( mediaRoot, root, error ) ) return nullptr;
        const auto path = catalogPath.empty() ? root + "/.workphone/media-assets.db" : catalogPath;
        std::error_code ec;
        const bool exists = fs::exists( fs::u8path( path.c_str() ), ec );
        if( ec ) { error = "Cannot inspect the bundled media catalog path."; return nullptr; }
        if( !exists && !buildMediaAssetCatalog( root, path, error ) ) return nullptr;
        auto result = make_ptr<AssetDatabaseManager>();
        if( !result->setProjectRoot( root ) || !result->openReadOnly( path, error ) || !ownedMediaCatalog( result ) )
        {
            if( error.empty() ) error = "Not a supported built-in media catalog.";
            return nullptr;
        }
        return result;
    }
}
