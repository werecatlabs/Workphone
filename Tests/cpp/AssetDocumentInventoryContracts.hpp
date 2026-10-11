#include <Workphone/Database/AssetDocumentInventory.hpp>
#if defined WP_PLATFORM_WIN32
#    include <winioctl.h>
#endif
namespace
{
    void assetDocumentInventoryContracts( const std::filesystem::path &folder )
    {
        namespace fs = std::filesystem;
        const auto root = folder / "document-inventory";
        fs::create_directories( root / ".workphone" );
        auto catalog = make_ptr<AssetDatabaseManager>();
        require( catalog->setProjectRoot( root.u8string().c_str() ), "Inventory project root" );
        const auto dbPath = ( root / ".workphone/catalog.db" ).u8string();
        catalog->loadFromFile( dbPath.c_str() );
        auto app = core::IApplicationManager::instance();
        auto oldResources = app->getResourceDatabase();
        const auto oldProject = app->getProjectPath(), oldCache = app->getCachePath();
        auto resources = make_ptr<ResourceDatabase>();
        resources->setDatabaseManager( catalog );
        app->setResourceDatabase( resources );
        app->setProjectPath( root.u8string().c_str() );
        app->setCachePath( "scan-cache" );
        auto write = [&]( const String &path, const String &text ) {
            const auto file = root / fs::u8path( path.c_str() );
            fs::create_directories( file.parent_path() );
            std::ofstream output( file, std::ios::binary | std::ios::trunc );
            output.write( text.data(), text.size() );
            require( static_cast<bool>( output ), "Inventory fixture write" );
        };
        auto data = make_ptr<Properties>();
        data->setProperty( "asset", "missing-inventory-target", Properties::resourceStr );
        data->getPropertyObject( "asset" ).setAttribute( "resourceType", "Texture" );
        const auto json = DataUtil::toString( data.get(), true );
        write( "a.mat", json );
        write( "nested/b.fbscenexml", DataUtil::toString( data.get(), true, DataFormat::XML ) );
        const auto binary = PropertiesBinarySerializer::serialize( *data );
        String error;
        require( !binary.empty(), "Inventory binary fixture" );
        write( "c.fbscenebin",
               String( reinterpret_cast<const char *>( binary.data() ), binary.size() ) );
        write( "legacy.prefab", "{\"texture\":\"untyped-legacy-id\"}" );
        write( "malformed.material", "{\"children\":[],\"children\":[]}" );
        for( const String path :
             { ".workphone/private.fbscene", ".git/hidden.prefab", "scan-cache/derived.mat" } )
            write( path, "invalid excluded fixture" );
        write( "raw.png", "raw imported source" );
        auto scan = scanAssetDocuments( catalog );
        require( scan.succeeded && scan.ready == 3 && scan.blocked == 2 && scan.excluded == 3,
                 "Discovery must find unindexed JSON/XML/binary, diagnose legacy/malformed and exclude "
                 "private/cache/VCS" );
        auto tables = sql( catalog->getDatabase(),
                           "SELECT name FROM sqlite_master WHERE name='wp_asset_reference_documents'" );
        require( tables && tables->eof(), "Preview must not create or mutate the reference schema" );
        require( saveAssetDocumentText( "existing.custom", data, json, error ),
                 "Index nonstandard extension" );
        scan = scanAssetDocuments( catalog );
        require( scan.succeeded && scan.ready == 4 && scan.blocked == 2,
                 "Discovery must include existing graph owners regardless of extension" );
        auto owner =
            sql( catalog->getDatabase(),
                 "SELECT revision FROM wp_asset_reference_documents WHERE path='existing.custom'" );
        const auto revision = owner->getFieldValue( "revision" );
        require( sql( catalog->getDatabase(),
                      "CREATE TRIGGER fail_inventory_batch BEFORE UPDATE OF state ON "
                      "wp_asset_reference_documents "
                      "WHEN NEW.path='nested/b.fbscenexml' AND NEW.state='ready' "
                      "BEGIN SELECT RAISE(ABORT,'batch fault'); END" ) != nullptr,
                 "Inject batch completion fault" );
        u32 count = 77;
        require( !reindexAssetDocuments( catalog, scan, count, error ) && count == 0,
                 "Batch failure must report no published graphs" );
        auto untouched =
            sql( catalog->getDatabase(),
                 "SELECT revision FROM wp_asset_reference_documents WHERE path='existing.custom'" );
        auto absent = sql( catalog->getDatabase(),
                           "SELECT path FROM wp_asset_reference_documents WHERE path='a.mat'" );
        require(
            untouched && untouched->getFieldValue( "revision" ) == revision && absent && absent->eof(),
            "Later batch failure must roll back earlier replacements and new graph owners" );
        require( sql( catalog->getDatabase(), "DROP TRIGGER fail_inventory_batch" ) != nullptr,
                 "Remove batch completion fault" );
        scan = scanAssetDocuments( catalog );
        const auto modified = fs::last_write_time( root / "a.mat" );
        // Public display data is not authority for apply.
        scan.documents.clear();
        scan.ready = 0;
        scan.blocked = 0;
        scan.succeeded = false;
        require( reindexAssetDocuments( catalog, scan, count, error ) && count == 4 &&
                     fs::last_write_time( root / "a.mat" ) == modified,
                 "Batch uses private preview proof and never rewrites authored bytes" );
        auto edges = queryAssetReferences( catalog, "a.mat", false );
        require( edges.succeeded && edges.edges.size() == 1 && edges.edges[0].sourceState == "ready" &&
                     edges.edges[0].diagnostic == "Missing project asset",
                 "Batch preserves typed missing-target diagnostics" );
        require( !reindexAssetDocuments( catalog, scan, count, error ) && count == 0,
                 "Published batch expires its original catalog revision" );
        auto oldBinary =
            sql( catalog->getDatabase(),
                 "SELECT revision FROM wp_asset_reference_documents WHERE path='c.fbscenebin'" );
        const auto oldBinaryRevision = oldBinary->getFieldValue( "revision" );
        write( "c.fbscenebin", "{\"asset\":\"legacy-with-unknown-type\"}" );
        auto blockedScan = scanAssetDocuments( catalog );
        require( blockedScan.succeeded && blockedScan.ready == 3 && blockedScan.blocked == 3 &&
                     reindexAssetDocuments( catalog, blockedScan, count, error ) && count == 3,
                 "Verified documents may rebuild while legacy owners remain explicitly blocked" );
        auto retainedBinary =
            sql( catalog->getDatabase(),
                 "SELECT revision FROM wp_asset_reference_documents WHERE path='c.fbscenebin'" );
        require( retainedBinary && retainedBinary->getFieldValue( "revision" ) == oldBinaryRevision,
                 "Blocked legacy owner must retain its previous graph revision" );
        write( "c.fbscenebin",
               String( reinterpret_cast<const char *>( binary.data() ), binary.size() ) );

        scan = scanAssetDocuments( catalog );
        write( "new.resource", json );
        require( !reindexAssetDocuments( catalog, scan, count, error ) && count == 0,
                 "New document membership expires preview before publication" );
        fs::remove( root / "new.resource" );
        scan = scanAssetDocuments( catalog );
        write( "a.mat", DataUtil::toString( make_ptr<Properties>().get(), true ) );
        require( !reindexAssetDocuments( catalog, scan, count, error ) && count == 0 &&
                     queryAssetReferences( catalog, "a.mat", false ).edges.size() == 1,
                 "External byte changes expire preview and retain old edges" );
        write( "a.mat", json );
        scan = scanAssetDocuments( catalog );
        app->setCachePath( "different-cache" );
        require( !reindexAssetDocuments( catalog, scan, count, error ),
                 "Changed cache policy expires preview" );
        app->setCachePath( "scan-cache" );
        fs::remove( root / "existing.custom" );
        scan = scanAssetDocuments( catalog );
        require( scan.succeeded && scan.ready == 3 && scan.blocked == 3,
                 "Missing indexed documents remain explicit blocked owners" );
        write( "existing.custom", json );
        AssetDocumentScanOptions limits;
        limits.maxDocuments = 2;
        auto truncated = scanAssetDocuments( catalog, limits );
        require( !truncated.succeeded && truncated.documents.empty() &&
                     !reindexAssetDocuments( catalog, truncated, count, error ),
                 "Document limit must not expose an applicable partial inventory" );
        limits = {};
        limits.maxBytes = 1;
        require( !scanAssetDocuments( catalog, limits ).succeeded, "Global byte budget fails closed" );
        limits = {};
        limits.maxEntries = 1;
        require( !scanAssetDocuments( catalog, limits ).succeeded, "Traversal budget fails closed" );
        limits = {};
        limits.maxDepth = 1;
        require( !scanAssetDocuments( catalog, limits ).succeeded,
                 "Directory depth budget fails closed" );
        // Mounted bundled media is excluded even when physically under the project.
        write( "media/icon.png", "media fixture" );
        write( "media/hidden.prefab", "legacy media must not become a project owner" );
        require(
            buildMediaAssetCatalog( ( root / "media" ).u8string().c_str(),
                                    ( root / "media/.workphone/media.db" ).u8string().c_str(), error ),
            "Build inventory media fixture" );
        auto media = openMediaAssetCatalog( ( root / "media" ).u8string().c_str(), error,
                                            ( root / "media/.workphone/media.db" ).u8string().c_str() );
        require( media && resources->setMediaCatalog( media ), "Mount inventory media" );
        auto mediaEntries = media->queryEntries( {} );
        String mediaId;
        for( const auto &entry : mediaEntries.entries )
            if( entry.path == "icon.png" )
                mediaId = entry.uuid;
        require( !mediaId.empty(), "Media texture identity" );
        auto mediaData = make_ptr<Properties>();
        mediaData->setProperty( "icon", mediaId, Properties::resourceStr );
        mediaData->getPropertyObject( "icon" ).setAttribute( "resourceType", "Texture" );
        write( "media-user.mat", DataUtil::toString( mediaData.get(), true ) );
        scan = scanAssetDocuments( catalog );
        require(
            scan.succeeded && scan.ready == 5 && scan.blocked == 2 && scan.excluded == 4 &&
                reindexAssetDocuments( catalog, scan, count, error ) && count == 5 &&
                queryAssetReferences( catalog, mediaId, true, "media" ).edges.size() == 1,
            "Project batch must exclude media documents and retain media-domain reference ownership" );
        require(
            sql( catalog->getDatabase(), "UPDATE wp_asset_reference_schema SET version=2" ) != nullptr,
            "Inject future graph schema for inventory" );
        require( !scanAssetDocuments( catalog ).succeeded,
                 "Project inventory refuses unknown graph schemas" );
        require(
            sql( catalog->getDatabase(), "UPDATE wp_asset_reference_schema SET version=1" ) != nullptr,
            "Restore graph schema after inventory check" );
        scan = scanAssetDocuments( catalog );
        resources->setMediaCatalog( nullptr );
        require( !reindexAssetDocuments( catalog, scan, count, error ),
                 "Unmounting media expires preview" );
        media->unload( nullptr );
        // A linked directory must be surfaced without recursive traversal.
        std::error_code linkError;
        fs::create_directory_symlink( root / "nested", root / "linked", linkError );
#if defined WP_PLATFORM_WIN32
        if( linkError )
        {
            // Directory junctions exercise the same reparse-point path without
            // requiring SeCreateSymbolicLinkPrivilege or Developer Mode.
            struct JunctionData
            {
                DWORD tag = IO_REPARSE_TAG_MOUNT_POINT;
                WORD length = 0, reserved = 0;
                WORD substituteOffset = 0, substituteLength = 0, printOffset = 0, printLength = 0;
                WCHAR buffer[4096] = {};
            } junction;
            const auto target = ( root / "nested" ).wstring();
            const std::wstring ntPrefix = LR"(\??\)";
            const auto substitute = target.compare( 0, 2, LR"(\\)" ) == 0
                                        ? ntPrefix + LR"(UNC\)" + target.substr( 2 )
                                        : ntPrefix + target;
            const auto characters = substitute.size() + target.size() + 2;
            require( characters < 4096, "Junction fixture path budget" );
            junction.substituteLength = static_cast<WORD>( substitute.size() * sizeof( WCHAR ) );
            junction.printOffset = static_cast<WORD>( ( substitute.size() + 1 ) * sizeof( WCHAR ) );
            junction.printLength = static_cast<WORD>( target.size() * sizeof( WCHAR ) );
            junction.length = static_cast<WORD>( 8 + characters * sizeof( WCHAR ) );
            std::copy( substitute.begin(), substitute.end(), junction.buffer );
            std::copy( target.begin(), target.end(), junction.buffer + substitute.size() + 1 );
            const auto link = root / "linked";
            require( CreateDirectoryW( link.c_str(), nullptr ) != 0,
                     "Create junction fixture directory" );
            auto handle =
                CreateFileW( link.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                             FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr );
            require( handle != INVALID_HANDLE_VALUE, "Open junction fixture for reparse metadata" );
            DWORD returned = 0;
            const auto installed =
                DeviceIoControl( handle, FSCTL_SET_REPARSE_POINT, &junction, junction.length + 8,
                                 nullptr, 0, &returned, nullptr );
            const auto failure = GetLastError();
            CloseHandle( handle );
            require( installed != 0,
                     ( "Install junction fixture, error " + std::to_string( failure ) ).c_str() );
            linkError.clear();
            std::cout << "PASS: directory junction fallback exercises link traversal rejection\n";
        }
#endif
        if( !linkError )
        {
            scan = scanAssetDocuments( catalog );
            bool linked = false;
            for( const auto &entry : scan.documents )
                if( entry.path == "linked" )
                    linked = !entry.canReindex;
            require( scan.succeeded && linked,
                     "Links must be diagnosed, never traversed as duplicate owners" );
            fs::remove( root / "linked" );
        }
        else
            std::cout << "SKIP: directory symlink fixture unavailable: " << linkError.message() << '\n';
        scan = scanAssetDocuments( catalog );
        catalog->addResourceEntry( asset( "new-identity.bin" ) );
        require( !reindexAssetDocuments( catalog, scan, count, error ),
                 "Catalog identity changes expire preview" );
        scan = scanAssetDocuments( catalog );
        app->setProjectPath( folder.u8string().c_str() );
        require( !reindexAssetDocuments( catalog, scan, count, error ),
                 "Project activation before catalog rebinding expires preview" );
        app->setProjectPath( root.u8string().c_str() );
        app->setCachePath( "." );
        require( !scanAssetDocuments( catalog ).succeeded,
                 "Cache cannot silently exclude the entire project root" );
        app->setCachePath( "scan-cache" );
        auto reader = make_ptr<AssetDatabaseManager>();
        require( reader->setProjectRoot( root.u8string().c_str() ) &&
                     reader->openReadOnly( dbPath.c_str(), error ),
                 "Open independent read-only project inspection" );
        auto inspection = scanAssetDocuments( reader );
        require( inspection.succeeded && !reindexAssetDocuments( reader, inspection, count, error ),
                 "Read-only mounts may inspect documents but cannot apply a batch" );
        reader->unload( nullptr );
        scan = scanAssetDocuments( catalog );
        catalog->unload( nullptr );
        catalog->loadFromFile( dbPath.c_str() );
        require( !reindexAssetDocuments( catalog, scan, count, error ),
                 "Reopened catalog expires inventory session" );
        resources->setDatabaseManager( nullptr );
        app->setResourceDatabase( oldResources );
        app->setProjectPath( oldProject );
        app->setCachePath( oldCache );
        catalog->unload( nullptr );
        std::cout << "PASS: bounded project discovery, transactional batch rollback, stale previews, "
                     "media exclusion and legacy diagnostics\n";
    }
}  // namespace
