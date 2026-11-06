#include <Workphone/Workphone.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Interface/Database/IDatabase.hpp>
#include <WPAssimp/CharacterImport.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#ifdef _WIN32
#include <Windows.h>
#elif defined( __linux__ )
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>
#elif defined( __APPLE__ )
#include <stdio.h>
#endif

namespace workphone::resource
{
    namespace
    {
        namespace fs = std::filesystem;
        constexpr std::array<const char *, 4> filenames{
            "character.mesh", "rig.skelres", "character.skinres", "character.fbscene"
        };

        struct ImportStage
        {
            fs::path directory;
            bool published = false;

            ~ImportStage()
            {
                if( !published )
                {
                    std::error_code error;
                    for( const auto filename : filenames )
                    {
                        fs::remove( directory / filename, error );
                    }
                    fs::remove( directory, error );
                }
            }
        };

        void writeSource( const fs::path &path, const char *bytes, size_t size )
        {
            if( !size )
            {
                throw std::runtime_error( "Character import produced an empty source" );
            }
            std::ofstream stream( path, std::ios::binary );
            stream.write( bytes, static_cast<std::streamsize>( size ) );
            stream.close();
            if( !stream )
            {
                throw std::runtime_error( "Cannot write imported source: " + path.u8string() );
            }
        }

        void publishSources( ImportStage &stage, const fs::path &destination )
        {
            // A competing importer can win after the existence preflight. The
            // publish operation itself must refuse to replace that directory.
            bool published = false;
#if defined( _WIN32 )
            published = MoveFileExW( stage.directory.c_str(), destination.c_str(),
                                     MOVEFILE_WRITE_THROUGH ) != 0;
#elif defined( __linux__ ) && defined( SYS_renameat2 )
            published = syscall( SYS_renameat2, AT_FDCWD, stage.directory.c_str(), AT_FDCWD,
                                 destination.c_str(), 1 /* RENAME_NOREPLACE */ ) == 0;
#elif defined( __APPLE__ )
            published = renamex_np( stage.directory.c_str(), destination.c_str(), RENAME_EXCL ) == 0;
#endif
            if( !published )
            {
                throw std::runtime_error( "Cannot publish imported character directory exclusively" );
            }
            stage.published = true;
        }

        String characterScene( const String &skin, const String &rig )
        {
            auto actor = make_ptr<Properties>();
            actor->setName( "actors" );
            actor->setProperty( "uuid", StringUtil::getUUID() );
            actor->setProperty( "name", "ImportedCharacter" );
            auto component = [&]( const char *type ) {
                auto properties = make_ptr<Properties>();
                properties->setName( "component" );
                properties->setProperty( "componentType", String( "workphone::scene::" ) + type );
                properties->setProperty( "uuid", StringUtil::getUUID() );
                actor->addChild( properties );
                return properties;
            };
            auto mesh = component( "Mesh" );
            mesh->setProperty( "Mesh Resource", skin );
            mesh->setProperty( "Skeleton", rig );
            component( "MeshRenderer" );
            auto animator = component( "Animator" );
            animator->setProperty( "animationAssetUuid", rig );
            animator->setProperty( "autoPlay", true );
            auto document = make_ptr<Properties>();
            document->addChild( actor );
            return DataUtil::toString( document.get(), true );
        }
    }

    void importCharacter( const String &project, const String &catalogPath, const String &model,
                          const String &destinationPath )
    {
        const auto relative = fs::u8path( destinationPath.c_str() );
        if( relative.empty() || relative.has_root_path() || relative.filename().empty() ||
            !fs::is_directory( fs::u8path( project.c_str() ) ) )
        {
            throw std::runtime_error( "Character import requires an existing project and a relative destination directory" );
        }
        if( !fs::is_regular_file( fs::u8path( catalogPath.c_str() ) ) )
        {
            throw std::runtime_error( "Catalog must be an existing regular file" );
        }
        auto catalog = make_ptr<AssetDatabaseManager>();
        if( !catalog->setProjectRoot( project ) )
        {
            throw std::runtime_error( "Invalid project source root" );
        }

        const auto catalogRoot = catalog->getProjectRoot();
        String error;
        AssetCatalogPath directory;
        if( !canonicalAssetCatalogPath( catalogRoot, destinationPath, directory, error ) )
        {
            throw std::runtime_error( error.c_str() );
        }
        const auto destination = fs::u8path( catalogRoot.c_str() ) / fs::u8path( directory.path.c_str() );
        if( fs::exists( fs::symlink_status( destination ) ) || !fs::is_directory( destination.parent_path() ) )
        {
            throw std::runtime_error( "Character import requires a new destination with an existing parent directory" );
        }

        Array<AssetDatabaseManager::FileEntry> files;
        std::array<ResourceID, filenames.size()> ids;
        for( size_t i = 0; i < filenames.size(); ++i )
        {
            const auto path = directory.path + "/" + filenames[i];
            if( !ids[i].set( path, &error ) || ids[i].isSubResource() || ids[i].sourceRelativePath() != path )
            {
                throw std::runtime_error( "Imported source cannot be represented by a canonical ResourceID" );
            }
            files.push_back( { StringUtil::getUUID(), path, ids[i].type().str() } );
        }

        character_import::Sources sources;
        if( !character_import::importSources( model, ids[0], ids[1], sources, error ) )
        {
            throw std::runtime_error( error.c_str() );
        }
        const auto scene = characterScene( files[2].uuid, files[1].uuid );
        catalog->loadFromFile( catalogPath );
        if( !catalog->getDatabase() || !catalog->getDatabase()->isLoaded() || catalog->isReadOnly() )
        {
            throw std::runtime_error( "Character import requires a writable project catalog" );
        }
        for( const auto &file : files )
        {
            if( catalog->getResourceEntryFromPath( file.path ) )
            {
                throw std::runtime_error( ( "Imported source already has a catalog identity: " + file.path ).c_str() );
            }
        }

        auto stagePath = destination.parent_path() /
                         fs::u8path( ( ".wp-character-import-" + StringUtil::getUUID() ).c_str() );
        if( !fs::create_directory( stagePath ) )
        {
            throw std::runtime_error( "Cannot create a private character import staging directory" );
        }
        ImportStage stage{ std::move( stagePath ) };
        writeSource( stage.directory / filenames[0], reinterpret_cast<const char *>( sources.mesh.data() ),
                     sources.mesh.size() );
        writeSource( stage.directory / filenames[1], sources.skeleton.data(), sources.skeleton.size() );
        writeSource( stage.directory / filenames[2], sources.skin.data(), sources.skin.size() );
        writeSource( stage.directory / filenames[3], scene.data(), scene.size() );
        publishSources( stage, destination );

        // The published files are a complete set. Preserve them if catalog
        // commit fails, so an ambiguous commit cannot leave rows with no files.
        if( !catalog->registerFileEntries( files, error ) )
        {
            throw std::runtime_error( ( "Imported files retained in " + directory.path +
                                       ". Catalog registration failed. " + error ).c_str() );
        }

        rapidjson::StringBuffer output;
        rapidjson::Writer<rapidjson::StringBuffer> writer( output );
        writer.StartObject();
        writer.Key( "version" ); writer.Uint( 1 );
        writer.Key( "sceneUuid" ); writer.String( files[3].uuid.c_str() );
        writer.Key( "assets" ); writer.StartArray();
        for( size_t i = 0; i < files.size(); ++i )
        {
            const auto &file = files[i];
            writer.StartObject();
            writer.Key( "uuid" ); writer.String( file.uuid.c_str() );
            writer.Key( "path" ); writer.String( file.path.c_str() );
            writer.Key( "type" ); writer.String( file.type.c_str() );
            writer.Key( "resourceId" ); writer.String( ids[i].str().c_str() );
            writer.EndObject();
        }
        writer.EndArray();
        writer.EndObject();
        std::cout << output.GetString() << '\n';
    }
}
