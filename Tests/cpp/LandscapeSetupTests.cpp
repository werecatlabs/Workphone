#include <Workphone/Workphone.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/System/StateManager.hpp>
#include <Workphone/System/TimerMT.hpp>
#include <Workphone/System/CommandManagerMT.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/Scene/GameScene.hpp>
#include <Workphone/Scene/Components/Terrain/LandscapeFoliage.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainLandscapeMaterials.hpp>
#include <WPProcedural/CTextureForge.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <stdexcept>
#include "LandscapeOceanContracts.hpp"

using namespace workphone;

namespace
{
    void requireLandscape( bool condition, const String &message )
    {
        if( !condition ) throw std::runtime_error( message.c_str() );
    }

    String readText( const std::filesystem::path &path )
    {
        std::ifstream file( path, std::ios::binary );
        requireLandscape( bool( file ), "Cannot read landscape fixture." );
        std::ostringstream bytes;
        bytes << file.rdbuf();
        return bytes.str().c_str();
    }

    void writeText( const std::filesystem::path &path, const String &text )
    {
        std::ofstream file( path, std::ios::binary );
        file.write( text.data(), std::streamsize( text.size() ) );
        file.close();
        requireLandscape( bool( file ), "Cannot write landscape output." );
    }

    void materialContracts( const std::filesystem::path &project )
    {
        auto terrain = make_ptr<scene::TerrainSystem>();
        String error;
        const auto commands = core::IApplicationManager::instance()->getCommandManager();
        requireLandscape( scene::createLandscapeMaterials( terrain.get(), commands, error ), error );
        auto generated = terrain->getTerrainSnapshot();
        for( const auto &layer : generated->layers )
        {
            for( const auto &id : { layer.albedo, layer.normal, layer.orm } )
            {
                requireLandscape( id.find( "data://" ) == 0, "Generated textures need durable source IDs." );
                requireLandscape( std::filesystem::exists( project / id.substr( 7 ).c_str() ),
                                  "Generated texture descriptor must exist." );
            }
            requireLandscape( layer.metallic == 0 && layer.uvScale == Vector2F( .25f, .25f ),
                              "Landscape ground must use tiled nonmetallic materials." );
        }
        const auto serialized = terrain->exportTerrainData();
        requireLandscape( scene::createLandscapeMaterials( terrain.get(), commands, error ), error );
        requireLandscape( serialized == terrain->exportTerrainData(),
                          "Repeated landscape texture baking must reuse stable content IDs." );
        auto restored = make_ptr<scene::TerrainSystem>();
        requireLandscape( restored->importTerrainData( serialized, error ), error );
        requireLandscape( restored->getTerrainSnapshot()->layers == generated->layers,
                          "Landscape texture references must survive save/reload." );
        std::puts( "Landscape material generation and durable sources: PASS" );
    }

    void exportIsland( const std::filesystem::path &input, const std::filesystem::path &output )
    {
        auto terrain = make_ptr<scene::TerrainSystem>();
        String error;
        requireLandscape( terrain->importTerrainData( readText( input ), error ), error );
        auto data = *terrain->getTerrainSnapshot();
        data.autoMaterial.enabled = true;
        data.appearance.projection = render::TerrainProjection::Triplanar;
        data.appearance.macroStrength = .12f;
        requireLandscape( terrain->applyTerrainData( data, error ), error );
        requireLandscape( scene::createLandscapeMaterials( terrain.get(),
            core::IApplicationManager::instance()->getCommandManager(), error ), error );
        scene::LandscapeFoliageSettings settings;
        requireLandscape( scene::configureLandscapeFoliage( terrain.get(), settings, error ), error );
        std::filesystem::create_directories( output );
        writeText( output / "island_landscape.terrain.json", terrain->exportTerrainData() );
        writeText( output / "island_landscape.foliage.json", terrain->exportFoliageData() );
        const auto foliage = terrain->getFoliageAuthoring()->getSnapshot();
        std::printf( "Island exported: %zu foliage instances, %zu species.\n",
                     foliage->instances.size(), foliage->species.size() );
    }
}

int main( int argc, char **argv )
{
    TypeManager types;
    types.load();
    TypeManager::setInstance( &types );
    auto app = make_ptr<core::ApplicationManager>();
    core::IApplicationManager::setInstance( app );
    auto factory = make_ptr<FactoryManager>();
    app->setFactoryManager( factory );
    factory->load( nullptr );
    auto states = make_ptr<StateManager>();
    app->setStateManager( states );
    states->load( nullptr );
    auto timer = make_ptr<TimerMT>();
    app->setTimer( timer );
    timer->load( nullptr );
    app->setCommandManager( make_ptr<CommandManagerMT>() );
    FactoryUtil::addFactory<scene::Transform>();
    FactoryUtil::addFactoryByName<procedural::CTextureForge>( "ITextureForge" );
    auto game = make_ptr<scene::GameManager>();
    app->setGameManager( game );
    game->load( nullptr );
    // Actor parenting transitions child state through the manager's current scene.
    auto currentScene = make_ptr<scene::GameScene>();
    currentScene->load( nullptr );
    game->setCurrentScene( currentScene );
    int result = 0;
    try
    {
        const bool exporting = argc == 5 && String( argv[1] ) == "--island";
        const auto project = exporting ? std::filesystem::u8path( argv[2] ) :
            std::filesystem::temp_directory_path() /
                ( std::string( "workphone_landscape_" ) + StringUtil::getUUID().c_str() );
        std::filesystem::create_directories( project );
        app->setProjectPath( project.u8string().c_str() );
        if( exporting )
            exportIsland( std::filesystem::u8path( argv[3] ), std::filesystem::u8path( argv[4] ) );
        else
        {
            materialContracts( project );
            runLandscapeOceanContracts();
        }
    }
    catch( const std::exception &error )
    {
        std::fprintf( stderr, "Landscape setup failed: %s\n", error.what() );
        result = 1;
    }
    game->unload( nullptr );
    currentScene = nullptr; // GameManager unloads and releases its current scene.
    app->setCommandManager( nullptr );
    app->setGameManager( nullptr );
    game = nullptr;
    factory->unload( nullptr );
    states->unload( nullptr );
    timer->unload( nullptr );
    app->setStateManager( nullptr );
    app->setFactoryManager( nullptr );
    app->setTimer( nullptr );
    core::IApplicationManager::setInstance( nullptr );
    app = nullptr;
    TypeManager::setInstance( nullptr );
    types.unload();
    return result;
}
