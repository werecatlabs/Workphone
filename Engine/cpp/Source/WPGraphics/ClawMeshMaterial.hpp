#ifndef WP_CLAW_MESH_MATERIAL_PRIVATE_HPP
#define WP_CLAW_MESH_MATERIAL_PRIVATE_HPP

#include <Workphone/Workphone.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/ClawUtil.hpp>

namespace workphone::render
{
    // Publication validation and rendering must resolve the same section asset,
    // including materials named before their resource manager entry was loaded.
    inline SmartPtr<IMaterial> resolveMeshMaterial( ClawMesh *mesh, s32 section )
    {
        if( !mesh ) return nullptr;
        auto material = mesh->getMaterial( section );
        if( !material ) material = mesh->getMaterial();
        if( !material )
        {
            auto name = mesh->getMaterialName( section );
            if( StringUtil::isNullOrEmpty( name ) ) name = mesh->getMaterialName();
            auto app = core::IApplicationManager::instancePtr();
            auto graphics = app ? app->getGraphicsSystem() : nullptr;
            auto manager = graphics ? graphics->getMaterialManager() : nullptr;
            if( manager && !StringUtil::isNullOrEmpty( name ) )
                material = dynamic_pointer_cast<IMaterial>( manager->getByName( name ) );
        }
        return material;
    }

    inline bool isSupportedFoliageMaterial( const SmartPtr<IMaterial> &material )
    {
        return !material || ( !material->isTransparent() &&
            ClawUtil::toCBlendMode( material->getBlendMode() ) == WORKPHONE_BLEND_MODE_NONE );
    }
}

#endif
