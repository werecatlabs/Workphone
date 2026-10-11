#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/WPClawHammer.hpp>
#include <WPGraphics/ClawScene.hpp>
#include <WPGraphics/ClawHammerSystem.hpp>
#include <WPGraphics/ClawMaterialManager.hpp>
#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawMaterialTechnique.hpp>
#include <WPGraphics/ClawMaterialPass.hpp>
#include <WPGraphics/ClawMaterialTexture.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <WPGraphics/ClawTextureManager.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/FoliageImpostorBaker.hpp>
#include <WPGraphics/Resources/FoliageSourceTexture.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawHammer, ISharedObject );

        SmartPtr<ClawHammer> ClawHammer::m_sPlugin;

        ClawHammer::ClawHammer()
        {
        }

        ClawHammer::~ClawHammer()
        {
        }

        void ClawHammer::load( SmartPtr<ISharedObject> data )
        {
            FactoryUtil::addFactory<ClawHammerSystem>();
            FactoryUtil::addFactory<ClawScene>();
            FactoryUtil::addFactory<ClawMaterialManager>();
            FactoryUtil::addFactory<ClawMaterial>();
            FactoryUtil::addFactory<ClawMaterialTechnique>();
            FactoryUtil::addFactory<ClawMaterialPass>();
            FactoryUtil::addFactory<ClawMaterialTexture>();
            FactoryUtil::addFactory<ClawTexture>();
            FactoryUtil::addFactory<ClawTextureManager>();
            setFoliageImpostorTextureResolver([](const String &id,String &error)->std::shared_ptr<const TextureMipLevel>
            {
                auto application=core::IApplicationManager::instancePtr();
                if(id.size()>=7&&id.substr(id.size()-7)==".texres")
                {
#if WP_FOLIAGE_SOURCE_RESOURCES
                    return readFoliageSourceTexture(application?application->getProjectPath():String(),id,error);
#else
                    error="Authored foliage texture descriptors require the resource system.";return {};
#endif
                }
                auto database=application?application->getResourceDatabase():nullptr;
                auto texture=database?database->loadResourceByType<ITexture>(id):nullptr;
                auto claw=dynamic_pointer_cast<ClawTexture>(texture);
                if(!claw){error="Foliage CPU texture resolver requires an available Claw texture: "+id;return {};}
                return claw->getCpuBaseMip(error);
            });
        }

        void ClawHammer::unload( SmartPtr<ISharedObject> data )
        {
            setFoliageImpostorTextureResolver({});
        }

        SmartPtr<ClawHammer> ClawHammer::instance()
        {
            return m_sPlugin;
        }

        void ClawHammer::setInstance( SmartPtr<ClawHammer> plugin )
        {
            m_sPlugin = plugin;
        }
    }  // namespace render
}  // namespace workphone

#ifndef _WP_STATIC_LIB_
extern "C" {
WP_INTERFACE_EXPORT void WP_INTERFACE_API workphone_get_version( int *major, int *minor, int *patch )
{
    *major = WP_VERSION_MAJOR;
    *minor = WP_VERSION_MINOR;
    *patch = WP_VERSION_PATCH;
}

WP_INTERFACE_EXPORT void WP_INTERFACE_API
loadPlugin( workphone::core::ApplicationManager *applicationManager )
{
    using namespace workphone;
    using namespace render;

    auto plugin = workphone::make_ptr<ClawHammer>();
    plugin->load( nullptr );
    ClawHammer::setInstance( plugin );
}

WP_INTERFACE_EXPORT void WP_INTERFACE_API
unloadPlugin( workphone::core::ApplicationManager *applicationManager )
{
    using namespace workphone;
    using namespace render;

    if( auto plugin = ClawHammer::instance() )
    {
        plugin->unload( nullptr );
        ClawHammer::setInstance( nullptr );
    }
}
}
#endif
