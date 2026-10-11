#pragma once
#include <Workphone/Database/MediaAssetCatalog.hpp>
#include <Workphone/Script/ScriptAsset.hpp>
#include <fstream>
#include <map>

namespace
{
    using namespace workphone;
    void mediaAssetCatalogContracts( const std::filesystem::path &folder )
    {
        namespace fs = std::filesystem;
        auto check = []( bool ok, const char *message ) {
            if( !ok ) throw std::runtime_error( message );
        };
        auto query = [&]( SmartPtr<AssetDatabaseManager> catalog, const String &statement ) {
            auto bound = dynamic_cast<IParameterizedDatabase *>( catalog->getDatabase().get() );
            check( bound != nullptr, "Media contracts require real parameterized SQLite" );
            return bound->queryBound( statement, {} );
        };
        auto write = []( const fs::path &path, const std::string &bytes ) {
            fs::create_directories( path.parent_path() );
            std::ofstream file( path, std::ios::binary );
            file.write( bytes.data(), bytes.size() );
            if( !file ) throw std::runtime_error( "Cannot write media fixture" );
        };
        auto identities = [&]( SmartPtr<AssetDatabaseManager> catalog ) {
            std::map<String, String> result;
            AssetDatabaseManager::EntryQuery request;
            auto page = catalog->queryEntries( request );
            check( page.succeeded && !page.hasMore, "Small media fixture must fit a valid page" );
            for( const auto &entry : page.entries ) result[entry.path] = entry.uuid;
            return result;
        };
        const auto root = folder / "bundled-media";
        write( root / "Fonts" / "default.ttf", std::string( "font\0bytes", 10 ) );
        write( root / "checker.png", "image" );
        write( root / "Editor" / "tool.lua", "return {}" );
        write( root / "Runtime" / "shared.png", "shared" );
        write( root / "ignored.db", "never index database files" );
        write( root / ".workphone" / "ignored.lua", "never index private state" );
        String error;
        auto catalog = openMediaAssetCatalog( root.u8string().c_str(), error );
        check( catalog && catalog->isReadOnly(), "Opening media must create then mount it read-only" );
        const auto initial = identities( catalog );
        check( initial.size() == 4, "Media inventory must classify supported files and skip private state" );
        auto projectMount = make_ptr<AssetDatabaseManager>();
        projectMount->setProjectRoot( folder.u8string().c_str() );
        projectMount->loadFromFile( ( folder / "media-mount-project.db" ).u8string().c_str() );
        auto resources = make_ptr<ResourceDatabase>();
        resources->setDatabaseManager( projectMount );
        check( !resources->setMediaCatalog( projectMount ) && resources->setMediaCatalog( catalog ),
               "Resource resolution must mount only an owned read-only media catalog" );
        const auto scriptId = initial.at( "Editor/tool.lua" );
        auto script = dynamic_pointer_cast<ScriptAsset>( resources->getObject( StringUtil::parseUUID( scriptId ) ) );
        check( script && script->getHandle()->getUUIDAsString() == scriptId &&
               script->getFilePath() == catalog->getProjectRoot() + "/Editor/tool.lua",
               "Media UUID resolution must use its own source root without importing project rows" );
        check( resources->loadResourceById( StringUtil::parseUUID( scriptId ) ) != nullptr,
               "UUID loading must support the same mounted media identity" );
        check( !resources->loadResource( String( "Editor/tool.lua" ) ),
               "Project-relative paths must not silently resolve against bundled media" );
        auto projectBound = dynamic_cast<IParameterizedDatabase *>( projectMount->getDatabase().get() );
        check( projectBound->queryBound( "INSERT INTO resources(uuid,path,path_key,type,kind) VALUES(?,'collision.lua','collision.lua','script','file')", {scriptId} ) != nullptr,
               "Catalog UUID collision fixture must seed" );
        check( !resources->getObject( StringUtil::parseUUID( scriptId ) ) &&
               !resources->loadResourceById( StringUtil::parseUUID( scriptId ) ),
               "Cross-catalog UUID collisions must reject resolution instead of shadowing assets" );
        check( projectBound->queryBound( "DELETE FROM resources", {} ) != nullptr,
               "Clear project collision fixture" );
        auto emptyProject = query( projectMount, "SELECT count(*) AS n FROM resources" );
        check( emptyProject && emptyProject->getFieldValueAsInt( "n" ) == 0,
               "Media resolution must never adopt media rows into the project database" );
        resources->setMediaCatalog( nullptr );
        check( !resources->getObject( StringUtil::parseUUID( scriptId ) ),
               "Unmounting media must stop new media UUID resolution" );
        resources->setDatabaseManager( nullptr );
        projectMount->unload( nullptr );
        auto runtime = query( catalog, "SELECT uuid FROM wp_media_entries WHERE usage='runtime'" );
        check( runtime && !runtime->eof() && runtime->getFieldValue( "uuid" ) == initial.at( "Runtime/shared.png" ),
               "Runtime eligibility must be explicit and tied to the media UUID" );
        runtime->nextRow();
        check( runtime->eof(), "Other bundled media must default to Editor-only" );
        auto before = catalog->getCatalogGeneration();
        check( !query( catalog, "DELETE FROM resources" ), "Read-only media SQLite connection must reject raw SQL writes" );
        check( catalog->getCatalogGeneration() == before, "Rejected writes must not change catalog revision" );
        for( auto operation : { AssetDatabaseManager::FileOperation::Move,
                               AssetDatabaseManager::FileOperation::Copy,
                               AssetDatabaseManager::FileOperation::Delete } )
            check( !catalog->performFileOperation( operation, "checker.png", "changed.png" ).succeeded,
                   "Read-only media must reject filesystem mutations before touching bytes" );
        check( fs::exists( root / "checker.png" ) && !fs::exists( root / "changed.png" ),
               "Rejected media operations must preserve source and destination" );
        check( !catalog->undoFileOperation( "unknown" ).succeeded,
               "Media mounts must reject file-operation undo" );
        auto source = std::ifstream( root / "Fonts" / "default.ttf", std::ios::binary );
        std::string bytes( ( std::istreambuf_iterator<char>( source ) ), {} );
        check( bytes == std::string( "font\0bytes", 10 ), "Indexing must never modify media payloads" );
        source.close();
        const auto destination = root / ".workphone" / "media-assets.db";
        catalog->unload( nullptr );
        catalog = nullptr;
        write( root / "checker.png", "new source bytes" );
        fs::remove( root / "Editor" / "tool.lua" );
        write( root / "new.otf", "new font" );
        check( buildMediaAssetCatalog( root.u8string().c_str(), {}, error ), "Explicit media rebuild must succeed" );
        catalog = openMediaAssetCatalog( root.u8string().c_str(), error );
        auto updated = identities( catalog );
        check( updated.size() == 4 && updated.at( "checker.png" ) == initial.at( "checker.png" ) &&
               updated.at( "Fonts/default.ttf" ) == initial.at( "Fonts/default.ttf" ) &&
               updated.count( "Editor/tool.lua" ) == 0 && updated.count( "new.otf" ) == 1,
               "Media rebuild must retain path identities and remove/add inventory rows" );
        auto snapshot = catalog->queryEntries( {} ).entries.front();
        catalog->close();
        check( catalog->openReadOnly( destination.u8string().c_str(), error ) &&
               !catalog->isEntryCurrent( snapshot ), "Close/reopen must invalidate detached media selections" );
        catalog->unload( nullptr );
        catalog = nullptr;
        const auto relocated = folder / "relocated-media";
        fs::copy( root, relocated, fs::copy_options::recursive );
        catalog = openMediaAssetCatalog( relocated.u8string().c_str(), error );
        check( catalog && identities( catalog ) == updated, "Relocated media catalogs must retain UUIDs" );
        catalog->unload( nullptr );
        catalog = nullptr;

        auto missing = make_ptr<AssetDatabaseManager>();
        missing->setProjectRoot( root.u8string().c_str() );
        auto absent = root / "missing.db";
        check( !missing->openReadOnly( absent.u8string().c_str(), error ) && !fs::exists( absent ),
               "A failed read-only open must never create a database" );
        missing->unload( nullptr );
        auto invalidDestination = root / "not-a-database.lua";
        check( !buildMediaAssetCatalog( root.u8string().c_str(), invalidDestination.u8string().c_str(), error ) &&
               !fs::exists( invalidDestination ), "A catalog must not create a database disguised as a source asset" );
        auto foreignPath = folder / "project-catalog.db";
        auto project = make_ptr<AssetDatabaseManager>();
        project->setProjectRoot( root.u8string().c_str() );
        project->loadFromFile( foreignPath.u8string().c_str() );
        auto bound = dynamic_cast<IParameterizedDatabase *>( project->getDatabase().get() );
        check( bound->queryBound( "INSERT INTO resources(uuid,path,path_key,type,kind) VALUES('project-only','owned.png','owned.png','Texture','file')", {} ) != nullptr,
               "Project isolation fixture must seed" );
        project->unload( nullptr );
        check( !buildMediaAssetCatalog( root.u8string().c_str(), foreignPath.u8string().c_str(), error ),
               "Media builder must refuse to overwrite a project catalog" );
        project->loadFromFile( foreignPath.u8string().c_str() );
        check( project->hasResourceById( "project-only" ), "Ownership rejection must preserve project data" );
        project->unload( nullptr );
        auto foreign = folder / "AssetDatabase.db";
        write( foreign, "domain database bytes" );
        check( !buildMediaAssetCatalog( root.u8string().c_str(), foreign.u8string().c_str(), error ),
               "Media builder must refuse unrelated/domain database files" );
        check( fs::file_size( foreign ) == 21, "Refusing a foreign database must not modify it" );
        std::cout << "PASS: media creation, stable UUIDs, rebuild/relocation, read-only mounts, UUID resolution, collision rejection and project/domain isolation\n";
    }
}
