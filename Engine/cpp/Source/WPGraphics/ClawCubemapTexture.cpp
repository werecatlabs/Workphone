#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawCubemapTexture.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <stdexcept>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawCubemapTexture, Texture );

    ClawCubemapTexture::ClawCubemapTexture()
    {
        setTextureType( TextureType::TEX_TYPE_CUBE_MAP );
    }
    void ClawCubemapTexture::setFaces( const Array<SmartPtr<ITexture>> &faces )
    {
        if( faces.size() != 6 )
            throw std::invalid_argument( "A cubemap requires six faces." );
        for( const auto &face : faces )
            if( !face || face.get() == this )
                throw std::invalid_argument( "Cubemap faces must be valid 2D textures." );
        m_faces = faces;
    }
    Array<SmartPtr<ITexture>> ClawCubemapTexture::getFaces() const
    {
        return m_faces;
    }
    Array<SmartPtr<ITexture>> ClawCubemapTexture::getCubemapFaces() const
    {
        return getFaces();
    }
    Vector2I ClawCubemapTexture::getSize() const
    {
        return Vector2I( 128, 128 );
    }
    Vector2I ClawCubemapTexture::getActualSize() const
    {
        return getSize();
    }
    void ClawCubemapTexture::getTextureGPU( void **texture ) const
    {
        getTextureFinal( texture );
    }
    void ClawCubemapTexture::_getObject( void **texture ) const
    {
        getTextureFinal( texture );
    }
    void ClawCubemapTexture::load( SmartPtr<ISharedObject> data )
    {
        Texture::load( data );
        setLoadingState( LoadingState::Loaded );
    }
    void ClawCubemapTexture::unload( SmartPtr<ISharedObject> data )
    {
        m_cube.reset();
        m_renderer = nullptr;
        Texture::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }
    void ClawCubemapTexture::getTextureFinal( void **texture ) const
    {
        if( !texture )
            return;
        *texture = nullptr;
        auto app = core::IApplicationManager::instancePtr();
        auto graphics = app ? app->getGraphicsSystemPtr() : nullptr;
        auto renderer =
            graphics ? dynamic_pointer_cast<ClawRendererDX11>( graphics->getRenderer() ) : nullptr;
        auto dx11 = renderer ? wp_renderer_get_dx11( renderer->getNativeRenderer() ) : nullptr;
        if( !dx11 || m_faces.size() != 6 )
            return;
        std::array<ID3D11ShaderResourceView *, 6> views{};
        for( size_t i = 0; i < 6; ++i )
        {
            void *view = nullptr;
            m_faces[i]->getTextureFinal( &view );
            if( !view )
                return;
            views[i] = static_cast<ID3D11ShaderResourceView *>( view );
        }
        if( m_renderer != dx11 )
        {
            m_cube.reset();
            m_renderer = dx11;
        }
        if( m_cube.update( static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( dx11 ) ),
                           static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( dx11 ) ),
                           views ) )
            *texture = m_cube.getView();
    }
}  // namespace workphone::render
