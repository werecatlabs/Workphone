#include <Workphone/Scene/SceneWorld.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace workphone;
using namespace workphone::scene;

int main( int argc, char **argv )
{
    TypeManager types;
    types.load(); TypeManager::setInstance( &types );
    int status = 0;
    try
    {
        if( argc < 3 )
            throw std::runtime_error( "Usage: WorkphoneSceneWorld INPUT_SCENE_OR_PREFAB OUTPUT_DIRECTORY [--cell-size N] [--max-actors N] [--asset-root PATH] [--render-only-static]" );
        WorldCookOptions options;
        options.assetRoot = std::filesystem::absolute( argv[1] ).parent_path().generic_string().c_str();
        for( int i = 3; i < argc; ++i )
        {
            const std::string option = argv[i];
            if( option == "--render-only-static" ) options.convertRenderOnlyLeaves = true;
            else if( option == "--cell-size" && i + 1 < argc ) options.cellSize = std::stof( argv[++i] );
            else if( option == "--max-actors" && i + 1 < argc ) options.maximumActorsPerCell = std::stoul( argv[++i] );
            else if( option == "--asset-root" && i + 1 < argc ) options.assetRoot = argv[++i];
            else throw std::runtime_error( "Unknown or incomplete world cook option" );
        }
        std::ifstream input( std::filesystem::path( argv[1] ), std::ios::binary | std::ios::ate );
        if( !input || input.tellg() > std::streamoff( 1ull << 30 ) ) throw std::runtime_error( "Invalid or oversized source scene" );
        std::string bytes( static_cast<size_t>( input.tellg() ), '\0' );
        input.seekg( 0 );
        if( !input.read( bytes.data(), bytes.size() ) ) throw std::runtime_error( "Could not read scene" );
        auto source = make_ptr<Properties>();
        String error;
        if( PropertiesBinarySerializer::hasBinaryHeader( reinterpret_cast<const u8 *>( bytes.data() ), bytes.size() ) )
        {
            if( !PropertiesBinarySerializer::deserialize( reinterpret_cast<const u8 *>( bytes.data() ), bytes.size(), *source, &error ) )
                throw std::runtime_error( error.c_str() );
        }
        else
        {
            const auto first = bytes.find_first_not_of( " \t\r\n" );
            if( first == std::string::npos ) throw std::runtime_error( "Empty source scene" );
            const auto format = bytes[first] == '<' ? DataFormat::XML : DataFormat::JSON;
            DataUtil::parse( String( bytes.c_str() ), source.get(), format );
        }
        String manifest;
        if( !SceneWorld::cook( *source, argv[2], options, manifest, error ) ) throw std::runtime_error( error.c_str() );
        WorldManifest cooked;
        if( !SceneWorld::readManifest( manifest, cooked, error ) ) throw std::runtime_error( error.c_str() );
        auto bootstrap = make_ptr<Properties>();
        auto actor = make_ptr<Properties>();
        actor->setName( ApplicationUtil::actorsStr );
        actor->setProperty( "label", "Streamed city" );
        actor->setProperty( "uuid", StringUtil::getUUID() );
        auto component = make_ptr<Properties>();
        component->setName( "component" );
        component->setProperty( "uuid", StringUtil::getUUID() );
        component->setProperty( "componentType", "workphone::scene::WorldStreaming" );
        auto manifestLocation = std::filesystem::absolute( manifest.c_str() );
        const auto relativeManifest = manifestLocation.lexically_relative(
            std::filesystem::absolute( options.assetRoot.c_str() ) );
        if( !relativeManifest.empty() && *relativeManifest.begin() != ".." ) manifestLocation = relativeManifest;
        component->setProperty( "World Manifest", manifestLocation.generic_string().c_str() );
        component->setProperty( "Preview Editor Camera", true );
        actor->addChild( component ); bootstrap->addChild( actor );
        auto text = DataUtil::toString( bootstrap.get(), true, DataFormat::JSON );
        std::ofstream preview( std::filesystem::path( argv[2] ) / "world-preview.fbscene", std::ios::binary );
        preview.write( text.c_str(), text.size() ); preview.close();
        if( !preview ) throw std::runtime_error( "Could not write world bootstrap scene" );
        u64 actors = 0;
        for( const auto &cell : cooked.cells ) actors += cell.actors;
        std::cout << "manifest=" << manifest.c_str() << " cells=" << cooked.cells.size() << " actors=" << actors << '\n';
    }
    catch( const std::exception &e ) { std::cerr << e.what() << '\n'; status = 1; }
    TypeManager::setInstance( nullptr );
    return status;
}
