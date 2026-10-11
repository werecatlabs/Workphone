#include <Workphone/Database/AssetReferenceGraph.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Scene/GameScene.hpp>
#include "AssetDocumentSaveContracts.hpp"
#include "AssetDocumentInventoryContracts.hpp"
#ifdef WP_EDITOR_ASSET_COMMAND_CONTRACTS
#    include <commands/AddResourceCmd.hpp>
#    include <jobs/ProjectCleanJob.hpp>
#endif
namespace
{
    void assetReferenceGraphContracts( const std::filesystem::path &folder )
    {
        namespace fs = std::filesystem;
        const auto root = folder / "reference-graph";
        fs::create_directories( root / "group" );
        auto write = []( const fs::path &file, const Array<u8> &bytes ) {
            std::ofstream stream( file, std::ios::binary | std::ios::trunc );
            stream.write( reinterpret_cast<const char *>( bytes.data() ), bytes.size() );
            require( static_cast<bool>( stream ), "Reference fixture write" );
        };
        auto catalog = make_ptr<AssetDatabaseManager>();
        require( catalog->setProjectRoot( root.u8string().c_str() ), "Reference root" );
        const auto dbPath = ( root / "catalog.db" ).u8string();
        catalog->loadFromFile( dbPath.c_str() );
        auto texture = asset( "texture.bin" ), optional = asset( "optional.bin" ),
             internal = asset( "group/internal.bin" );
        for( auto object : { texture, optional, internal } )
        {
            catalog->addResourceEntry( object );
            write( root / fs::u8path( object->getFilePath().c_str() ), { 1, 2, 3, 0, 255 } );
        }
        const auto uuid = texture->getHandle()->getUUIDAsString();
        const auto optionalId = optional->getHandle()->getUUIDAsString();
        const auto internalId = internal->getHandle()->getUUIDAsString();
        auto document = make_ptr<Properties>();
        auto reference = [&]( const String &name, const String &id, bool required = true,
                              const String &type = "workphone::CatalogTestResource" ) {
            document->setProperty( name, id, Properties::resourceStr );
            auto &p = document->getPropertyObject( name );
            p.setAttribute( "resourceType", type );
            p.setAttribute( "referenceRequired", required ? "true" : "false" );
        };
        reference( "albedo", uuid );
        reference( "optional", optionalId, false );
        reference( "missing", "absent-uuid" );
        auto serialize = []( SmartPtr<Properties> p ) {
            auto text = DataUtil::toString( p.get(), true );
            Array<u8> bytes;
            bytes.resize( text.size() );
            std::copy( text.begin(), text.end(), bytes.begin() );
            return bytes;
        };
        String error;
        AssetReferenceSave save;
        auto bytes = serialize( document );
        require( beginAssetReferenceSave( catalog, "scene.fbscene", document, bytes, save, error ),
                 "Begin durable save" );
        using Op = AssetDatabaseManager::FileOperation;
        auto blocked = catalog->performFileOperation( Op::Delete, "texture.bin" );
        require( !blocked.succeeded && fs::exists( root / "texture.bin" ) &&
                     blocked.error.find( "pending" ) != String::npos,
                 "Pending save must fail closed before staging files" );
        write( root / "scene.fbscene", { 9, 9 } );
        require( !completeAssetReferenceSave( catalog, save, error ),
                 "Reject different saved bytes without publishing graph" );
        write( root / "scene.fbscene", bytes );
        require( completeAssetReferenceSave( catalog, save, error ),
                 "Complete verified reference save" );
        auto users = queryAssetReferences( catalog, uuid );
        require( users.succeeded && users.edges.size() == 1 &&
                     users.edges[0].sourcePath == "scene.fbscene" && users.edges[0].required &&
                     users.edges[0].diagnostic.empty(),
                 "Typed reverse users must resolve without resources" );
        auto forward = queryAssetReferences( catalog, "scene.fbscene", false, "project", 1 );
        require( forward.succeeded && forward.edges.size() == 1 && forward.hasMore,
                 "Bound forward references" );
        auto cursor = forward.next;
        auto second = queryAssetReferences( catalog, "scene.fbscene", false, "project", 1, &cursor );
        require( second.succeeded && second.edges.size() == 1 &&
                     second.edges[0].slot != forward.edges[0].slot,
                 "Reference cursor must visit each edge once" );
        require( !queryAssetReferences( catalog, uuid, true, "project", 0 ).succeeded &&
                     !queryAssetReferences( catalog, uuid, true, "unknown" ).succeeded,
                 "Reject invalid reference queries" );
        auto missing = queryAssetReferences( catalog, "absent-uuid" );
        require( missing.succeeded && missing.edges.size() == 1 &&
                     missing.edges[0].diagnostic == "Missing project asset",
                 "Persist and diagnose missing references" );
        blocked = catalog->performFileOperation( Op::Delete, "texture.bin" );
        require( !blocked.succeeded && blocked.error.find( "scene.fbscene" ) != String::npos &&
                     blocked.error.find( "albedo" ) != String::npos &&
                     fs::exists( root / "texture.bin" ),
                 "Required user must block delete with actionable location" );
        catalog->removeResourceEntry( texture );
        catalog->removeResourceEntryFromPath( "texture.bin" );
        catalog->clearDatabase();
        require( catalog->hasResourceById( uuid ) && catalog->hasResourceById( internalId ),
                 "Legacy removal APIs must respect required references" );
        auto moved = catalog->performFileOperation( Op::Move, "scene.fbscene", "renamed.fbscene" );
        require( moved.succeeded, "Move indexed document" );
        users = queryAssetReferences( catalog, uuid );
        require( users.succeeded && users.edges[0].sourcePath == "renamed.fbscene",
                 "Move must preserve and relocate graph ownership" );
        require( catalog->undoFileOperation( moved.operationId ).succeeded,
                 "Undo indexed document move" );
        auto deletedOptional = catalog->performFileOperation( Op::Delete, "optional.bin" );
        require( deletedOptional.succeeded, "Optional reference allows quarantine" );
        auto optionalUsers = queryAssetReferences( catalog, optionalId );
        require( optionalUsers.succeeded && optionalUsers.edges.size() == 1 &&
                     !optionalUsers.edges[0].required &&
                     optionalUsers.edges[0].diagnostic == "Missing project asset",
                 "Optional missing asset diagnostic" );
        require( catalog->undoFileOperation( deletedOptional.operationId ).succeeded,
                 "Restore optional target" );
        write( root / "scene.fbscene", { 8, 8 } );
        blocked = catalog->performFileOperation( Op::Delete, "optional.bin" );
        require( !blocked.succeeded && fs::exists( root / "optional.bin" ),
                 "Externally changed graph source must block even previously optional removal" );
        auto saveDocument = [&]( const String &path, SmartPtr<Properties> data ) {
            AssetReferenceSave token;
            auto dataBytes = serialize( data );
            require( beginAssetReferenceSave( catalog, path, data, dataBytes, token, error ),
                     "Prepare reindexed document" );
            write( root / fs::u8path( path.c_str() ), dataBytes );
            require( completeAssetReferenceSave( catalog, token, error ),
                     "Complete reindexed document" );
        };
        saveDocument( "scene.fbscene", document );
        require(
            !queryAssetReferences( catalog, "scene.fbscene", false, "project", 1, &cursor ).succeeded,
            "Reference revisions must expire keyset continuations" );
        auto groupData = make_ptr<Properties>();
        groupData->setProperty( "texture", internalId, Properties::resourceStr );
        groupData->getPropertyObject( "texture" )
            .setAttribute( "resourceType", "workphone::CatalogTestResource" );
        saveDocument( "group/ref.fbscene", groupData );
        auto groupDelete = catalog->performFileOperation( Op::Delete, "group" );
        require( groupDelete.succeeded && !catalog->hasResourceById( internalId ),
                 "Internal-only folder users do not block reversible quarantine" );
        require( queryAssetReferences( catalog, internalId ).edges.empty(),
                 "Quarantined document edges must leave active reverse graph" );
        AssetReferenceSave quarantined;
        require( !beginAssetReferenceSave( catalog, "group/ref.fbscene", groupData,
                                           serialize( groupData ), quarantined, error ),
                 "Quarantined graph identity cannot be overwritten before restore" );
        require( catalog->undoFileOperation( groupDelete.operationId ).succeeded &&
                     queryAssetReferences( catalog, internalId ).edges.size() == 1,
                 "Restore must reactivate original typed edges" );
        auto copy = catalog->performFileOperation( Op::Copy, "optional.bin", "copy.bin" );
        require( copy.succeeded, "Copy unreferenced binary" );
        auto copyRow = sql( catalog->getDatabase(), "SELECT uuid FROM resources WHERE path='copy.bin'" );
        require( copyRow && !copyRow->eof(), "Copied target identity" );
        reference( "copyUser", copyRow->getFieldValue( "uuid" ) );
        saveDocument( "scene.fbscene", document );
        require(
            !catalog->undoFileOperation( copy.operationId ).succeeded && fs::exists( root / "copy.bin" ),
            "Copy undo must not remove a newly required asset" );
        document->setProperty( "copyUser", String() );
        saveDocument( "scene.fbscene", document );
        require( catalog->undoFileOperation( copy.operationId ).succeeded,
                 "Copy undo after user repair" );
        AssetReferenceSave first, latest;
        require( beginAssetReferenceSave( catalog, "scene.fbscene", document, bytes, first, error ) &&
                     beginAssetReferenceSave( catalog, "scene.fbscene", document, bytes, latest, error ),
                 "Superseded saves fixture" );
        write( root / "scene.fbscene", bytes );
        require( !completeAssetReferenceSave( catalog, first, error ) &&
                     completeAssetReferenceSave( catalog, latest, error ),
                 "Old completion cannot publish a newer pending save" );
        // A failed graph transaction must preserve the last committed users.
        require( sql( catalog->getDatabase(),
                      "CREATE TRIGGER fail_reference BEFORE INSERT ON wp_asset_references BEGIN SELECT "
                      "RAISE(ABORT,'reference fault'); END" ) != nullptr,
                 "Reference fault injection" );
        require( !beginAssetReferenceSave( catalog, "scene.fbscene", document, bytes, save, error ) &&
                     queryAssetReferences( catalog, uuid ).edges.size() == 1,
                 "Reference write failure must roll back the entire graph update" );
        require( sql( catalog->getDatabase(), "DROP TRIGGER fail_reference" ) != nullptr,
                 "Remove reference fault injection" );
        require( !sql( catalog->getDatabase(), "UPDATE wp_asset_references SET required=2" ),
                 "Reference requirement schema must reject malformed values" );
        AssetReferenceSave futureCompletion;
        require( beginAssetReferenceSave( catalog, "scene.fbscene", document, bytes, futureCompletion,
                                          error ),
                 "Capture pending completion before a schema-version change" );
        require(
            sql( catalog->getDatabase(), "UPDATE wp_asset_reference_schema SET version=2" ) != nullptr,
            "Future reference version fixture" );
        require( !completeAssetReferenceSave( catalog, futureCompletion, error ),
                 "Pending save completion must reject an unsupported graph schema" );
        require( !queryAssetReferences( catalog, uuid ).succeeded &&
                     !catalog->performFileOperation( Op::Delete, "optional.bin" ).succeeded &&
                     !beginAssetReferenceSave( catalog, "scene.fbscene", document, bytes, save, error ),
                 "Future graph schema must fail reads/deletes/saves without migration" );
        require(
            sql( catalog->getDatabase(), "UPDATE wp_asset_reference_schema SET version=1" ) != nullptr,
            "Restore supported reference version" );
        require( completeAssetReferenceSave( catalog, futureCompletion, error ),
                 "Complete the retained pending revision after restoring schema compatibility" );

        // Bundled-media links are owned by the project graph and never write to media.
        fs::create_directories( root / "media" );
        write( root / "media/icon.png", { 1, 2, 3 } );
        const auto mediaRoot = ( root / "media" ).u8string();
        const auto mediaDb = ( root / "media/catalog.db" ).u8string();
        require( buildMediaAssetCatalog( mediaRoot.c_str(), mediaDb.c_str(), error ),
                 "Build reference media fixture" );
        auto media = openMediaAssetCatalog( mediaRoot.c_str(), error, mediaDb.c_str() );
        AssetDatabaseManager::EntryQuery mediaQuery;
        auto mediaRows = media ? media->queryEntries( mediaQuery ) : AssetDatabaseManager::EntryPage{};
        require( mediaRows.succeeded && mediaRows.entries.size() == 1,
                 "Read bundled reference identity" );
        auto mediaData = make_ptr<Properties>();
        mediaData->setProperty( "icon", mediaRows.entries[0].uuid, Properties::resourceStr );
        mediaData->getPropertyObject( "icon" ).setAttribute( "resourceType", "Texture" );
        auto mediaBytes = serialize( mediaData );
        require( beginAssetReferenceSave( catalog, "media-user.fbscene", mediaData, mediaBytes, save,
                                          error, media ),
                 "Capture bundled-media edge" );
        write( root / "media-user.fbscene", mediaBytes );
        require( completeAssetReferenceSave( catalog, save, error ), "Complete bundled-media document" );
        auto mediaUsers = queryAssetReferences( catalog, mediaRows.entries[0].uuid, true, "media" );
        require( mediaUsers.succeeded && mediaUsers.edges.size() == 1 &&
                     queryAssetReferences( catalog, mediaRows.entries[0].uuid ).edges.empty(),
                 "Project and media reference domains must stay distinct" );
        media->unload( nullptr );
        media = nullptr;

        catalog->unload( nullptr );
        catalog->loadFromFile( dbPath.c_str() );
        require( queryAssetReferences( catalog, uuid ).edges.size() == 1,
                 "Typed edges survive catalog reopen" );
        require( !completeAssetReferenceSave( catalog, latest, error ),
                 "Old save tokens expire on reopen" );
        auto readOnly = make_ptr<AssetDatabaseManager>();
        readOnly->setProjectRoot( root.u8string().c_str() );
        require( readOnly->openReadOnly( dbPath.c_str(), error ) &&
                     queryAssetReferences( readOnly, uuid ).edges.size() == 1,
                 "Read-only catalog can inspect graph without migration" );
        require( !beginAssetReferenceSave( readOnly, "scene.fbscene", document, bytes, save, error ),
                 "Reject reference writes through read-only mount" );
        readOnly->unload( nullptr );
        auto untyped = make_ptr<Properties>();
        untyped->setProperty( "asset", uuid, Properties::resourceStr );
        saveDocument( "untyped.fbscene", untyped );
        auto untypedEdges = queryAssetReferences( catalog, "untyped.fbscene", false );
        require( untypedEdges.succeeded && untypedEdges.edges.size() == 1 &&
                     untypedEdges.edges[0].diagnostic.empty(),
                 "Resource properties without family metadata must retain generic typed coverage" );
        // Actual production scene serializer/save path, without a graphics or script VM dependency.
        auto app = core::IApplicationManager::instance();
        auto oldResources = app->getResourceDatabase();
        const auto oldProject = app->getProjectPath();
        auto resources = make_ptr<ResourceDatabase>();
        resources->setDatabaseManager( catalog );
        app->setResourceDatabase( resources );
        app->setProjectPath( root.u8string().c_str() );
        assetDocumentSaveContracts( catalog, root );
        auto scene = make_ptr<scene::GameScene>();
        for( const String extension :
             { String( ".fbscene" ), String( ".fbscenebin" ), String( ".fbscenexml" ) } )
        {
            const auto file = ( root / fs::u8path( ( "saved" + extension ).c_str() ) ).u8string();
            scene->saveScene( file.c_str() );
            auto row = sql( catalog->getDatabase(),
                            "SELECT state FROM wp_asset_reference_documents WHERE path=?",
                            { "saved" + extension } );
            require( row && !row->eof() && row->getFieldValue( "state" ) == "ready",
                     "Real scene save must complete a durable graph for each format" );
        }

        const auto previousScenePath = scene->getFilePath();
        require( sql( catalog->getDatabase(),
                      "CREATE TRIGGER fail_scene_graph BEFORE INSERT ON wp_asset_reference_documents "
                      "BEGIN SELECT RAISE(ABORT,'scene graph fault'); END" ) != nullptr,
                 "Inject production scene graph save failure" );
        bool rejected = false;
        try
        {
            scene->saveScene( ( root / "failed.fbscene" ).u8string().c_str() );
        }
        catch( const std::exception & )
        {
            rejected = true;
        }
        require(
            rejected && !fs::exists( root / "failed.fbscene" ) &&
                scene->getFilePath() == previousScenePath,
            "Production scene save must abort before file publication when graph persistence fails" );
        require( sql( catalog->getDatabase(), "DROP TRIGGER fail_scene_graph" ) != nullptr,
                 "Remove scene save failure hook" );
        scene->saveScene( ( root / "nested/new-scene" ).u8string().c_str() );
        require( fs::exists( root / "nested/new-scene.fbscene" ) &&
                     Path::getFileExtension( scene->getFilePath() ) == ".fbscene",
                 "Scene saves preserve directory creation and extension normalization" );
        const auto oldCache = app->getCachePath();
        app->setCachePath( "reference-cache" );
        scene->saveScene( ( root / "reference-cache/scratch.fbscene" ).u8string().c_str() );
        auto scratch = sql( catalog->getDatabase(),
                            "SELECT state FROM wp_asset_reference_documents WHERE "
                            "path='reference-cache/scratch.fbscene'" );
        require( scratch && scratch->eof(),
                 "Reload scratch files must not become durable project graph owners" );
        app->setCachePath( oldCache );

#ifdef WP_EDITOR_ASSET_COMMAND_CONTRACTS
        auto creation = make_ptr<editor::AddResourceCmd>();
        creation->setFilePath( "created.lua" );
        creation->setResourceType( editor::AddResourceCmd::ResourceType::Script );
        creation->execute();
        require( fs::exists( root / "created.lua" ),
                 "Execute real Editor asset creation without a graphics service" );
        auto created = asset( "created.lua" );
        catalog->addResourceEntry( created );
        const auto createdId = created->getHandle()->getUUIDAsString();
        auto creationUser = make_ptr<Properties>();
        creationUser->setProperty( "asset", createdId, Properties::resourceStr );
        creationUser->getPropertyObject( "asset" ).setAttribute( "resourceType",
                                                                 "workphone::CatalogTestResource" );
        saveDocument( "creation-user.fbscene", creationUser );
        creation->undo();
        require( fs::exists( root / "created.lua" ) && catalog->hasResourceById( createdId ),
                 "Real Editor creation undo must respect required users" );
        creationUser->setProperty( "asset", String() );
        saveDocument( "creation-user.fbscene", creationUser );
        write( root / "created.lua", { 0, 1, 2, 255 } );
        creation->undo();
        require( !fs::exists( root / "created.lua" ) && !catalog->hasResourceById( createdId ),
                 "Creation undo quarantines bytes and catalog identity together" );
        creation->redo();
        require( fs::file_size( root / "created.lua" ) == 4 && catalog->hasResourceById( createdId ),
                 "Creation redo restores retained bytes and UUID rather than generating a new asset" );
        for( const auto *name : { "authored.meta", "authored.lighting", "authored.unity" } )
            write( root / name, { 1, 0, 255, 42 } );
        auto cleaner = make_ptr<editor::ProjectCleanJob>();
        cleaner->execute();
        for( const auto *name : { "authored.meta", "authored.lighting", "authored.unity" } )
            require( fs::file_size( root / name ) == 4,
                     "Project Clean must retain authored metadata/lighting/imported scenes" );
        catalog->unload( nullptr );
        catalog->loadFromFile( dbPath.c_str() );
        creation->undo();
        require( fs::exists( root / "created.lua" ),
                 "Creation command must expire when the project catalog reopens" );
        creation = nullptr;
        cleaner = nullptr;
#endif
        scene = nullptr;
        app->setResourceDatabase( oldResources );
        app->setProjectPath( oldProject );
        resources->setDatabaseManager( nullptr );
        resources = nullptr;
        catalog->unload( nullptr );
        assetDocumentInventoryContracts( folder );
        std::cout << "PASS: durable typed reference graph, saved-byte fences, reverse queries, guarded "
                     "deletion and graph-aware undo\n";
    }
}  // namespace
