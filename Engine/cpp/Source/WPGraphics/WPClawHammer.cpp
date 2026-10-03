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
        }

        void ClawHammer::unload( SmartPtr<ISharedObject> data )
        {
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
