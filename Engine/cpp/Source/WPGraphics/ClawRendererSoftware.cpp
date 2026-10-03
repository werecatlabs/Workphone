#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawRendererSoftware.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IRenderTarget.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include "workphone_graphics_renderer.h"
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>

#include <algorithm>
#include <cstring>

#if defined( WP_PLATFORM_WIN32 )
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#endif

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawRendererSoftware, IRenderer );

    namespace
    {
        constexpr u16 k_quadIndices[] = { 0, 1, 2, 0, 2, 3 };
    }

    ClawRendererSoftware::ClawRendererSoftware() : m_bufferSize( 0, 0 )
    {
        for( auto &transform : m_transforms )
        {
            transform = Matrix4F::identity();
        }
    }

    ClawRendererSoftware::~ClawRendererSoftware()
    {
        destroyRenderer();
    }

    void ClawRendererSoftware::load( SmartPtr<ISharedObject> data )
    {
        if( m_renderer )
        {
            return;
        }

        setLoadingState( LoadingState::Loading );

        m_bufferSize = Vector2I( 1280, 720 );
        if( auto renderTarget = dynamic_pointer_cast<IRenderTarget>( data ) )
        {
            const auto size = renderTarget->getSize();
            if( size.x > 0 && size.y > 0 )
            {
                m_bufferSize = size;
            }
            else
                WP_LOG_WARNING( "WPGraphics/Software: render target has invalid dimensions; using 1280x720." );
            m_renderTarget = renderTarget;
        }

        WP_LOG_INFO( "WPGraphics/Software: allocating BGRA8 framebuffer and depth buffer at " + std::to_string( m_bufferSize.x ) + "x" + std::to_string( m_bufferSize.y ) );
        m_renderer =
            wp_renderer_create_software( m_bufferSize.x, m_bufferSize.y, WORKPHONE_PIXEL_FORMAT_BGRA8 );
        if( m_renderer )
        {
            wp_renderer_set_blend_mode( m_renderer, WORKPHONE_BLEND_MODE_ALPHA );
            wp_renderer_set_fill_mode( m_renderer, WORKPHONE_FILL_MODE_SOLID );
            wp_renderer_set_cull_mode( m_renderer, WORKPHONE_CULL_MODE_NONE );
            wp_renderer_set_depth_test_enabled( m_renderer, 1 );
            wp_renderer_set_depth_write_enabled( m_renderer, 1 );
            wp_renderer_set_depth_func( m_renderer, WORKPHONE_DEPTH_FUNC_LESS );
            apply3DTransforms();
        }

        setLoadingState( m_renderer ? LoadingState::Loaded : LoadingState::Unloaded );
        if( m_renderer )
        {
            WP_LOG_INFO( "WPGraphics/Software: renderer ready; render state and transforms configured." );
        }
        else
        {
            WP_LOG_ERROR( "WPGraphics/Software: framebuffer allocation failed at " + std::to_string( m_bufferSize.x ) + "x" + std::to_string( m_bufferSize.y ) );
        }
    }

    void ClawRendererSoftware::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        destroyRenderer();
        m_textures.clear();
        m_currentMaterial = nullptr;
        m_renderTarget = nullptr;
        m_viewport = nullptr;
        m_bufferSize = Vector2I::zero();
        m_inFrame = false;

        setLoadingState( LoadingState::Unloaded );
    }

    void ClawRendererSoftware::destroyRenderer()
    {
        if( m_renderer )
        {
            wp_renderer_destroy( m_renderer );
            m_renderer = nullptr;
        }
    }

    void ClawRendererSoftware::beginRender()
    {
        if( !m_renderer || m_inFrame )
        {
            return;
        }

        m_primitiveCount = 0;
        m_inFrame = true;
        wp_renderer_begin_frame( m_renderer );
    }

    void ClawRendererSoftware::endRender()
    {
        if( !m_renderer || !m_inFrame )
        {
            return;
        }

        wp_renderer_end_frame( m_renderer );
        m_inFrame = false;
        ++m_frameCount;

        constexpr f32 tickSeconds = 1.0f / 60.0f;
        m_frameTimer += tickSeconds;
        if( m_frameTimer >= 1.0f )
        {
            m_fps = static_cast<s32>( m_frameCount );
            m_frameCount = 0;
            m_frameTimer -= 1.0f;
        }

#if defined( WP_PLATFORM_WIN32 )
        // Blit the CPU framebuffer to the window's client area.
        auto window = dynamic_pointer_cast<IGraphicsWindow>( m_renderTarget );
        if( window )
        {
            void *windowHandle = nullptr;
            window->getWindowHandle( &windowHandle );
            auto hwnd = static_cast<HWND>( windowHandle );
            if( hwnd )
            {
                const auto fbWidth = wp_renderer_get_width( m_renderer );
                const auto fbHeight = wp_renderer_get_height( m_renderer );
                const auto *framebuffer =
                    static_cast<const unsigned char *>( wp_renderer_get_framebuffer( m_renderer ) );
                if( framebuffer && fbWidth > 0 && fbHeight > 0 )
                {
                    HDC hdc = GetDC( hwnd );
                    if( hdc )
                    {
                        BITMAPINFO bi = {};
                        bi.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
                        bi.bmiHeader.biWidth = fbWidth;
                        bi.bmiHeader.biHeight = -fbHeight;  // top-down
                        bi.bmiHeader.biPlanes = 1;
                        bi.bmiHeader.biBitCount = 32;
                        bi.bmiHeader.biCompression = BI_RGB;

                        RECT rc;
                        GetClientRect( hwnd, &rc );
                        const int destWidth = rc.right - rc.left;
                        const int destHeight = rc.bottom - rc.top;

                        StretchDIBits( hdc, 0, 0, destWidth, destHeight, 0, 0, fbWidth, fbHeight,
                                       framebuffer, &bi, DIB_RGB_COLORS, SRCCOPY );
                        ReleaseDC( hwnd, hdc );
                    }
                }
            }
        }
#endif
    }

    void ClawRendererSoftware::flush()
    {
        // The C89 software renderer executes draw calls immediately.
    }

    void ClawRendererSoftware::clear( const ColourF &colour )
    {
        if( !m_renderer )
        {
            return;
        }

        wp_renderer_set_clear_color( m_renderer, colour.r, colour.g, colour.b, colour.a );
        wp_renderer_set_clear_depth( m_renderer, 1.0f );
        wp_renderer_clear( m_renderer, WORKPHONE_CLEAR_FLAG_ALL );
    }

    void ClawRendererSoftware::setRenderTarget( SmartPtr<IRenderTarget> renderTarget )
    {
        m_renderTarget = renderTarget;
        if( !m_renderer || !m_renderTarget )
        {
            return;
        }

        const auto size = m_renderTarget->getSize();
        if( size.x > 0 && size.y > 0 && size != m_bufferSize &&
            wp_renderer_resize( m_renderer, size.x, size.y ) )
        {
            m_bufferSize = size;
        }
    }

    SmartPtr<IRenderTarget> ClawRendererSoftware::getRenderTarget() const
    {
        return m_renderTarget;
    }

    void ClawRendererSoftware::setViewport( SmartPtr<IViewport> viewport )
    {
        m_viewport = viewport;
        if( !m_renderer )
        {
            return;
        }

        wp_viewport_i nativeViewport = { 0, 0, m_bufferSize.x, m_bufferSize.y };
        if( m_viewport )
        {
            const auto position = m_viewport->getActualPosition();
            const auto size = m_viewport->getActualSize();
            nativeViewport.x = static_cast<wp_s32>( position.X() );
            nativeViewport.y = static_cast<wp_s32>( position.Y() );
            if( size.X() > 0 && size.Y() > 0 )
            {
                nativeViewport.width = static_cast<wp_s32>( size.X() );
                nativeViewport.height = static_cast<wp_s32>( size.Y() );
            }
        }

        wp_renderer_set_viewport( m_renderer, nativeViewport );
    }

    SmartPtr<IViewport> ClawRendererSoftware::getViewport() const
    {
        return m_viewport;
    }

    void ClawRendererSoftware::render( const SmartPtr<ISharedObject> &renderData,
                                       const SmartPtr<ITexture> &texture, const Matrix4F &transform,
                                       const ColourF &colour )
    {
        if( !m_renderer )
        {
            return;
        }

        setTransform( TransformState::World, transform );

        // ITexture does not expose a CPU readback contract. An unbound C texture
        // uses the backend's white texel, preserving the former tint-only fallback.
        wp_renderer_set_texture( m_renderer, nullptr, 0, 0, WORKPHONE_PIXEL_FORMAT_RGBA8 );

        const auto packedColour = packColour( colour );
        const wp_vertex_ptc vertices[] = {
            { { -0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f }, packedColour },
            { { 0.5f, 0.5f, 0.0f }, { 1.0f, 0.0f }, packedColour },
            { { 0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f }, packedColour },
            { { -0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f }, packedColour },
        };

        wp_renderer_draw_indexed_triangles_ptc( m_renderer, vertices, 4, k_quadIndices, 6 );
        m_primitiveCount += 2;
    }

    void ClawRendererSoftware::render( const SmartPtr<ISharedObject> &renderData,
                                       const SmartPtr<IMaterial> &material, const Matrix4F &transform,
                                       const ColourF &colour )
    {
        if( !m_renderer )
        {
            return;
        }

        setMaterial( material );
        setTransform( TransformState::World, transform );

        const auto packedColour = packColour( colour );
        const wp_vertex_pc vertices[] = {
            { { -0.5f, 0.5f, 0.0f }, packedColour },
            { { 0.5f, 0.5f, 0.0f }, packedColour },
            { { 0.5f, -0.5f, 0.0f }, packedColour },
            { { -0.5f, -0.5f, 0.0f }, packedColour },
        };

        wp_renderer_draw_indexed_triangles_pc( m_renderer, vertices, 4, k_quadIndices, 6 );
        m_primitiveCount += 2;
    }

    void ClawRendererSoftware::_getObject( void **ppObject )
    {
        if( ppObject )
        {
            *ppObject =
                m_renderer ? const_cast<void *>( wp_renderer_get_framebuffer( m_renderer ) ) : nullptr;
        }
    }

    void ClawRendererSoftware::setCamera( SmartPtr<IGraphicsCamera> camera )
    {
        m_camera = camera;
        m_transforms[static_cast<u32>( TransformState::View )] =
            camera ? Matrix4F( camera->getViewMatrix().ptr() ) : Matrix4F::identity();
        m_transforms[static_cast<u32>( TransformState::Projection )] =
            camera ? Matrix4F( camera->getProjectionMatrix().ptr() ) : Matrix4F::identity();
        applyTransform( TransformState::View );
        applyTransform( TransformState::Projection );
    }

    SmartPtr<IGraphicsCamera> ClawRendererSoftware::getCamera() const
    {
        return m_camera;
    }

    void ClawRendererSoftware::drawLine( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                                         const ColourF &colour )
    {
        draw3DLine( start, end, colour );
    }

    void ClawRendererSoftware::setTransform( TransformState state, const Matrix4F &matrix )
    {
        const auto index = static_cast<u32>( state );
        if( index >= static_cast<u32>( TransformState::Count ) )
        {
            return;
        }

        m_transforms[index] = matrix;
        applyTransform( state );
    }

    Matrix4F ClawRendererSoftware::getTransform( TransformState state ) const
    {
        const auto index = static_cast<u32>( state );
        return index < static_cast<u32>( TransformState::Count ) ? m_transforms[index]
                                                                 : Matrix4F::identity();
    }

    void ClawRendererSoftware::applyTransform( TransformState state )
    {
        if( !m_renderer )
        {
            return;
        }

        const auto nativeMatrix = toCMatrix( m_transforms[static_cast<u32>( state )] );
        switch( state )
        {
        case TransformState::World:
            wp_renderer_set_world_matrix( m_renderer, &nativeMatrix );
            break;
        case TransformState::View:
            wp_renderer_set_view_matrix( m_renderer, &nativeMatrix );
            break;
        case TransformState::Projection:
            wp_renderer_set_projection_matrix( m_renderer, &nativeMatrix );
            break;
        default:
            break;
        }
    }

    void ClawRendererSoftware::apply3DTransforms()
    {
        applyTransform( TransformState::World );
        applyTransform( TransformState::View );
        applyTransform( TransformState::Projection );
    }

    void ClawRendererSoftware::setMaterial( SmartPtr<IMaterial> material )
    {
        m_currentMaterial = material;
    }

    SmartPtr<IMaterial> ClawRendererSoftware::getMaterial() const
    {
        return m_currentMaterial;
    }

    SmartPtr<ITexture> ClawRendererSoftware::getTexture( const String &filename )
    {
        for( auto &texture : m_textures )
        {
            if( texture && texture->getName() == filename )
            {
                return texture;
            }
        }
        return nullptr;
    }

    SmartPtr<ITexture> ClawRendererSoftware::createTexture( const Vector2I &size, const String &name,
                                                            PixelFormat format )
    {
        // ITexture has no concrete CPU texture factory contract.
        return nullptr;
    }

    void ClawRendererSoftware::removeTexture( SmartPtr<ITexture> texture )
    {
        const auto it = std::find( m_textures.begin(), m_textures.end(), texture );
        if( it != m_textures.end() )
        {
            m_textures.erase( it );
        }
    }

    void ClawRendererSoftware::removeAllTextures()
    {
        m_textures.clear();
    }

    Vector2I ClawRendererSoftware::getScreenSize() const
    {
        if( m_renderer )
        {
            return Vector2I( wp_renderer_get_width( m_renderer ), wp_renderer_get_height( m_renderer ) );
        }
        return m_bufferSize;
    }

    void ClawRendererSoftware::setScissorRect( const AABB2<real_Num> &rect )
    {
        if( !m_renderer )
        {
            return;
        }

        const auto minimum = rect.getMin();
        const auto maximum = rect.getMax();
        const wp_viewport_i scissor = {
            static_cast<wp_s32>( minimum.X() ), static_cast<wp_s32>( minimum.Y() ),
            std::max<wp_s32>( 0, static_cast<wp_s32>( maximum.X() - minimum.X() + 1 ) ),
            std::max<wp_s32>( 0, static_cast<wp_s32>( maximum.Y() - minimum.Y() + 1 ) )
        };
        wp_renderer_set_scissor_rect( m_renderer, scissor );
        wp_renderer_set_scissor_enabled( m_renderer, 1 );
    }

    void ClawRendererSoftware::clearScissorRect()
    {
        if( m_renderer )
        {
            wp_renderer_set_scissor_enabled( m_renderer, 0 );
        }
    }

    void ClawRendererSoftware::draw3DLine( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                                           const ColourF &colour )
    {
        if( !m_renderer )
        {
            return;
        }

        apply3DTransforms();
        const auto packedColour = packColour( colour );
        const wp_vertex_pc vertices[] = {
            { { ( start.X() ), ( start.Y() ), ( start.Z() ) }, packedColour },
            { { ( end.X() ), ( end.Y() ), ( end.Z() ) }, packedColour },
        };
        wp_renderer_draw_lines_pc( m_renderer, vertices, 2 );
        ++m_primitiveCount;
    }

    void ClawRendererSoftware::draw3DBox( const AABB3<real_Num> &box, const ColourF &colour )
    {
        const auto minimum = box.getMinimum();
        const auto maximum = box.getMaximum();
        const Vector3<real_Num> corners[] = {
            { minimum.X(), minimum.Y(), minimum.Z() }, { maximum.X(), minimum.Y(), minimum.Z() },
            { maximum.X(), maximum.Y(), minimum.Z() }, { minimum.X(), maximum.Y(), minimum.Z() },
            { minimum.X(), minimum.Y(), maximum.Z() }, { maximum.X(), minimum.Y(), maximum.Z() },
            { maximum.X(), maximum.Y(), maximum.Z() }, { minimum.X(), maximum.Y(), maximum.Z() },
        };
        constexpr u32 edges[][2] = {
            { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, { 4, 5 }, { 5, 6 },
            { 6, 7 }, { 7, 4 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },
        };

        for( const auto &edge : edges )
        {
            draw3DLine( corners[edge[0]], corners[edge[1]], colour );
        }
    }

    void ClawRendererSoftware::draw3DTriangle( const Vector3<real_Num> &a, const Vector3<real_Num> &b,
                                               const Vector3<real_Num> &c, const ColourF &colour )
    {
        draw3DLine( a, b, colour );
        draw3DLine( b, c, colour );
        draw3DLine( c, a, colour );
    }

    void ClawRendererSoftware::beginScreenSpace()
    {
        m_screenDepthTest = wp_renderer_get_depth_test_enabled( m_renderer ) != 0;
        wp_renderer_set_depth_test_enabled( m_renderer, 0 );

        const auto identity = toCMatrix( Matrix4F::identity() );
        wp_renderer_set_world_matrix( m_renderer, &identity );
        wp_renderer_set_view_matrix( m_renderer, &identity );
        wp_renderer_set_projection_matrix( m_renderer, &identity );
    }

    void ClawRendererSoftware::endScreenSpace()
    {
        apply3DTransforms();
        wp_renderer_set_depth_test_enabled( m_renderer, m_screenDepthTest ? 1 : 0 );
    }

    void ClawRendererSoftware::draw2DLine( const Vector2<real_Num> &start, const Vector2<real_Num> &end,
                                           const ColourF &colour )
    {
        if( !m_renderer || m_bufferSize.x <= 0 || m_bufferSize.y <= 0 )
        {
            return;
        }

        const auto toNdc = [this]( const Vector2<real_Num> &point ) {
            return wp_vec3f{ static_cast<wp_f32>( 2.0 * point.X() / m_bufferSize.x - 1.0 ),
                             static_cast<wp_f32>( 1.0 - 2.0 * point.Y() / m_bufferSize.y ), 0.0f };
        };

        beginScreenSpace();
        const auto packedColour = packColour( colour );
        const wp_vertex_pc vertices[] = {
            { toNdc( start ), packedColour },
            { toNdc( end ), packedColour },
        };
        wp_renderer_draw_lines_pc( m_renderer, vertices, 2 );
        endScreenSpace();
        ++m_primitiveCount;
    }

    void ClawRendererSoftware::draw2DRect( const AABB2<real_Num> &rect, const ColourF &colour )
    {
        const auto minimum = rect.getMin();
        const auto maximum = rect.getMax();
        draw2DLine( minimum, Vector2<real_Num>( maximum.X(), minimum.Y() ), colour );
        draw2DLine( Vector2<real_Num>( maximum.X(), minimum.Y() ), maximum, colour );
        draw2DLine( maximum, Vector2<real_Num>( minimum.X(), maximum.Y() ), colour );
        draw2DLine( Vector2<real_Num>( minimum.X(), maximum.Y() ), minimum, colour );
    }

    void ClawRendererSoftware::draw2DFilledRect( const AABB2<real_Num> &rect, const ColourF &colour )
    {
        const AABB2<real_Num> uv( static_cast<real_Num>( 0 ), static_cast<real_Num>( 0 ),
                                  static_cast<real_Num>( 1 ), static_cast<real_Num>( 1 ) );
        draw2DImage( nullptr, rect, uv, colour );
    }

    void ClawRendererSoftware::draw2DImage( const SmartPtr<ITexture> &texture,
                                            const AABB2<real_Num> &destRect,
                                            const AABB2<real_Num> &srcRect, const ColourF &colour )
    {
        if( !m_renderer || m_bufferSize.x <= 0 || m_bufferSize.y <= 0 )
        {
            return;
        }

        const auto toNdc = [this]( const Vector2<real_Num> &point ) {
            return wp_vec3f{ static_cast<wp_f32>( 2.0 * point.X() / m_bufferSize.x - 1.0 ),
                             static_cast<wp_f32>( 1.0 - 2.0 * point.Y() / m_bufferSize.y ), 0.0f };
        };

        const auto minimum = destRect.getMin();
        const auto maximum = destRect.getMax();
        const auto uvMinimum = srcRect.getMin();
        const auto uvMaximum = srcRect.getMax();
        const auto packedColour = packColour( colour );

        const wp_vertex_ptc vertices[] = {
            { toNdc( minimum ), { ( uvMinimum.X() ), ( uvMinimum.Y() ) }, packedColour },
            { toNdc( Vector2<real_Num>( maximum.X(), minimum.Y() ) ),
              { ( uvMaximum.X() ), ( uvMinimum.Y() ) },
              packedColour },
            { toNdc( maximum ), { ( uvMaximum.X() ), ( uvMaximum.Y() ) }, packedColour },
            { toNdc( Vector2<real_Num>( minimum.X(), maximum.Y() ) ),
              { ( uvMinimum.X() ), ( uvMaximum.Y() ) },
              packedColour },
        };

        beginScreenSpace();
        wp_renderer_set_texture( m_renderer, nullptr, 0, 0, WORKPHONE_PIXEL_FORMAT_RGBA8 );
        wp_renderer_draw_indexed_triangles_ptc( m_renderer, vertices, 4, k_quadIndices, 6 );
        endScreenSpace();
        m_primitiveCount += 2;
    }

    s32 ClawRendererSoftware::getFPS() const
    {
        return m_fps;
    }

    u32 ClawRendererSoftware::getPrimitiveCountDrawn() const
    {
        return m_primitiveCount;
    }

    wp_renderer *ClawRendererSoftware::getNativeRenderer() const
    {
        return m_renderer;
    }

    wp_mat4f ClawRendererSoftware::toCMatrix( const Matrix4F &matrix )
    {
        wp_mat4f result;
        std::memcpy( result.m, matrix.ptr(), sizeof( result.m ) );
        return result;
    }

    u32 ClawRendererSoftware::packColour( const ColourF &colour )
    {
        const auto toByte = []( f32 value ) {
            return static_cast<u32>( std::clamp( value, 0.0f, 1.0f ) * 255.0f + 0.5f );
        };

        return ( toByte( colour.r ) << 24 ) | ( toByte( colour.g ) << 16 ) |
               ( toByte( colour.b ) << 8 ) | toByte( colour.a );
    }
}  // namespace workphone::render
