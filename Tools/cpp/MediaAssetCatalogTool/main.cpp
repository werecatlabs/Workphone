#include <Workphone/Workphone.hpp>
#include <Workphone/Database/MediaAssetCatalog.hpp>
#include <Workphone/Core/LogManagerDefault.hpp>
#include <WPSQLite/WPSQLite.hpp>
#include <iostream>

int main( int argc, char **argv )
{
    using namespace workphone;
    if( argc < 2 || argc > 3 )
    {
        std::cerr << "Usage: WorkphoneMediaCatalog <media-root> [catalog-path]\n";
        return 2;
    }
    TypeManager types;
    types.load();
    TypeManager::setInstance( &types );
    auto app = make_ptr<core::ApplicationManager>();
    core::IApplicationManager::setInstance( app );
    app->setLogManager( make_ptr<LogManagerDefault>() );
    auto factories = make_ptr<FactoryManager>();
    factories->load( nullptr );
    app->setFactoryManager( factories );
    app->setFileSystem( make_ptr<FileSystem>() );
    auto plugin = make_ptr<SQLitePlugin>();
    plugin->load( nullptr );
    String error;
    const auto success = buildMediaAssetCatalog( argv[1], argc == 3 ? String( argv[2] ) : String(), error );
    if( success )
        std::cout << "Built media asset catalog successfully.\n";
    else
        std::cerr << "Media catalog build failed: " << error << '\n';
    plugin->unload( nullptr );
    plugin = nullptr;
    app->setFileSystem( nullptr );
    app->setFactoryManager( nullptr );
    factories->unload( nullptr );
    factories = nullptr;
    core::IApplicationManager::setInstance( nullptr );
    app = nullptr;
    TypeManager::setInstance( nullptr );
    types.unload();
    return success ? 0 : 1;
}
