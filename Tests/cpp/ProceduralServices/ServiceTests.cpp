#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <WPProcedural/WPProcedural.hpp>
#include <iostream>
#include <stdexcept>
void checkClientContracts( workphone::SmartPtr<workphone::IFactoryManager> factories );
#if WP_PROCEDURAL_TEST_CLAW
void checkClawContracts();
#endif
int main()
{
    workphone::TypeManager types;
    types.load();
    int result = 0;
    {
        auto app = workphone::make_ptr<workphone::core::ApplicationManager>();
        workphone::core::IApplicationManager::setInstance( app );
        auto factories = workphone::make_ptr<workphone::FactoryManager>();
        factories->load( nullptr );
        app->setFactoryManager( factories );
        auto plugin = workphone::make_ptr<workphone::procedural::WPProcedural>();
        try
        {
            plugin->load( nullptr );
            checkClientContracts( factories );
#if WP_PROCEDURAL_TEST_CLAW
            checkClawContracts();
#endif
            plugin->unload( nullptr );
            if( factories->hasFactoryByName( "IVehicleGenerator" ) )
                throw std::runtime_error( "Plugin unload left a procedural factory installed" );
            std::cout << "Procedural service and editor property contracts passed\n";
        }
        catch( const std::exception &e )
        {
            std::cerr << e.what() << '\n';
            result = 1;
        }
        plugin = nullptr;
        app->setFactoryManager( nullptr );
        factories->unload( nullptr );
        factories = nullptr;
        workphone::core::IApplicationManager::setInstance( nullptr );
        app = nullptr;
    }
    types.unload();
    return result;
}
