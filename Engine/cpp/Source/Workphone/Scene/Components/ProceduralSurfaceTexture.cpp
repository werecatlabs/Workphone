#include <Workphone/WorkphonePCH.hpp>
#include "ProceduralProperties.hpp"
#include <Workphone/Scene/Components/ProceduralSurfaceTexture.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <algorithm>
namespace workphone::scene
{
    ProceduralSurfaceTexture::ProceduralSurfaceTexture() = default;
    ProceduralSurfaceTexture::~ProceduralSurfaceTexture() = default;

    WP_CLASS_REGISTER_DERIVED( workphone::scene, ProceduralSurfaceTexture, Component );
    void ProceduralSurfaceTexture::load( SmartPtr<ISharedObject> data )
    {
        Component::load( data );
        m_dirty = true;
        regenerate();
        setLoadingState( LoadingState::Loaded );
    }
    void ProceduralSurfaceTexture::unload( SmartPtr<ISharedObject> data )
    {
        m_result = {};
        m_service = nullptr;
        m_dirty = true;
        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }
    void ProceduralSurfaceTexture::update()
    {
        if( Thread::getCurrentTask() == TaskId::Application && m_dirty && isEnabled() )
            regenerate();
    }
    bool ProceduralSurfaceTexture::regenerate()
    {
        m_dirty = false;
        auto app = core::IApplicationManager::instancePtr();
        if( !m_service && app && app->getFactoryManager() )
            m_service = app->getFactoryManager()->createObjectFromType<procedural::ITextureForge>(
                "ITextureForge" );
        if( !m_service )
        {
            m_error = "Load the WPProcedural plugin to bake surface textures";
            return false;
        }
        try
        {
            m_service->setSeed( m_seed );
            m_result = m_service->bakeSurface( m_surface, m_params );
            m_error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            m_error = e.what();
            return false;
        }
    }
    SmartPtr<Properties> ProceduralSurfaceTexture::getProperties() const
    {
        auto p = Component::getProperties();
        p->setProperty( "Seed", m_seed );
        procedural_properties::setEnum(
            p, "Surface", static_cast<s32>( m_surface ),
            { "Concrete", "Plaster", "Brick", "Wood", "Metal", "Asphalt", "Sand", "Fabric", "Foliage",
              "Glass", "Paint", "Rubber", "Dirt", "Stone" } );
        p->setProperty( "Resolution", m_params.size );
        p->setProperty( "Tile UV", m_params.tileUV );
        p->setProperty( "sRGB Albedo", m_params.sRGB );
        p->setProperty( "Parallax", m_params.parallax );
        p->setProperty( "Detail Scale", m_params.detailScale );
        p->setButtonPressed( "Regenerate" );
        p->setProperty( "Generation Error", m_error );
        p->getPropertyObject( "Generation Error" ).setReadOnly( true );
        return p;
    }
    void ProceduralSurfaceTexture::setProperties( SmartPtr<Properties> p )
    {
        if( !p )
            return;
        Component::setProperties( p );
        p->getPropertyValue( "Seed", m_seed );
        s32 surface = procedural_properties::getEnum(
            p, "Surface", static_cast<s32>( m_surface ),
            { "Concrete", "Plaster", "Brick", "Wood", "Metal", "Asphalt", "Sand", "Fabric", "Foliage",
              "Glass", "Paint", "Rubber", "Dirt", "Stone" } );
        m_surface = static_cast<procedural::SurfaceTag>( std::clamp( surface, 0, 13 ) );
        p->getPropertyValue( "Resolution", m_params.size );
        m_params.size = std::clamp<u32>( m_params.size, 16, 2048 );
        // Use the next lower power of two to keep allocations bounded.
        u32 size = 16;
        while( size * 2 <= m_params.size )
            size *= 2;
        m_params.size = size;
        p->getPropertyValue( "Tile UV", m_params.tileUV );
        p->getPropertyValue( "sRGB Albedo", m_params.sRGB );
        p->getPropertyValue( "Parallax", m_params.parallax );
        p->getPropertyValue( "Detail Scale", m_params.detailScale );
        m_dirty = true;
    }
}  // namespace workphone::scene
