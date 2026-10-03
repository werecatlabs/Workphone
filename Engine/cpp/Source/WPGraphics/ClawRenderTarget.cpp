#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/ClawViewport.hpp>
#include <Workphone/Workphone.hpp>
#include "workphone_graphics_renderer.h"
#include <workphone_graphics_renderer_dx11.h>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawRenderTarget, IRenderTarget );

        const String ClawRenderTarget::sizeStr = "size";
        const String ClawRenderTarget::colourDepthStr = "colourDepth";
        const String ClawRenderTarget::priorityStr = "priority";
        const String ClawRenderTarget::activeStr = "active";
        const String ClawRenderTarget::autoUpdatedStr = "autoUpdated";

        ClawRenderTarget::ClawRenderTarget() : m_size( 0, 0 ), m_nativeSize( 0, 0 )
        {
        }

        ClawRenderTarget::~ClawRenderTarget()
        {
            try
            {
                releaseNativeRenderTexture();

                if( !m_viewports.empty() )
                {
                    removeAllViewports();
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void ClawRenderTarget::swapBuffers()
        {
            WP_LOG_WARNING( "ClawRenderTarget::swapBuffers: not yet wired to C renderer API." );
        }

        void ClawRenderTarget::setPriority( u8 priority )
        {
            m_priority = priority;
        }

        u8 ClawRenderTarget::getPriority() const
        {
            return m_priority;
        }

        bool ClawRenderTarget::isActive() const
        {
            return m_active;
        }

        void ClawRenderTarget::setActive( bool state )
        {
            m_active = state;
        }

        void ClawRenderTarget::setAutoUpdated( bool autoupdate )
        {
            m_autoUpdated = autoupdate;
        }

        bool ClawRenderTarget::isAutoUpdated() const
        {
            return m_autoUpdated;
        }

        void ClawRenderTarget::copyContentsToMemory( void *buffer, u32 size, FrameBuffer bufferId )
        {
            if( !buffer )
            {
                WP_LOG_WARNING( "ClawRenderTarget::copyContentsToMemory: null buffer supplied." );
                return;
            }

            if( size == 0 )
            {
                WP_LOG_WARNING( "ClawRenderTarget::copyContentsToMemory: zero size supplied." );
                return;
            }

            WP_LOG_WARNING( "ClawRenderTarget::copyContentsToMemory: not yet wired to C renderer API." );
        }

        Vector2I ClawRenderTarget::getSize() const
        {
            return m_size;
        }

        void ClawRenderTarget::setSize( const Vector2I &size )
        {
            if( size.x <= 0 || size.y <= 0 )
            {
                WP_LOG_WARNING( "ClawRenderTarget::setSize: invalid dimensions, ignored." );
                return;
            }

            m_size = size;
        }

        u32 ClawRenderTarget::getColourDepth() const
        {
            return m_colourDepth;
        }

        void ClawRenderTarget::setColourDepth( u32 colourDepth )
        {
            if( colourDepth == 0 )
            {
                WP_LOG_WARNING( "ClawRenderTarget::setColourDepth: zero depth ignored." );
                return;
            }

            m_colourDepth = colourDepth;
        }

        SmartPtr<IViewport> ClawRenderTarget::addViewport( hash_type id,
                                                           SmartPtr<IGraphicsCamera> camera, s32 ZOrder,
                                                           f32 left, f32 top, f32 width, f32 height )
        {
            try
            {
                if( !camera )
                {
                    WP_LOG_WARNING( "ClawRenderTarget::addViewport: null camera supplied." );
                    return {};
                }

                auto viewport = workphone::make_ptr<ClawViewport>();
                if( !viewport )
                {
                    WP_LOG_ERROR( "ClawRenderTarget::addViewport: failed to create viewport." );
                    return {};
                }

                viewport->setViewportId( id );
                viewport->setCamera( camera );
                viewport->setZOrder( ZOrder < 0 ? static_cast<s32>( m_viewports.size() ) : ZOrder );
                viewport->setPosition( Vector2<real_Num>( left, top ) );
                viewport->setSize( Vector2<real_Num>( width, height ) );
                viewport->setRenderTarget( this );

                m_viewports.push_back( viewport );
                return viewport;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return {};
        }

        u32 ClawRenderTarget::getNumViewports() const
        {
            return static_cast<u32>( m_viewports.size() );
        }

        SmartPtr<IViewport> ClawRenderTarget::getViewport( u32 index )
        {
            if( index >= m_viewports.size() )
            {
                WP_LOG_WARNING( "ClawRenderTarget::getViewport: index out of range." );
                return {};
            }

            return m_viewports[index];
        }

        SmartPtr<IViewport> ClawRenderTarget::getViewportById( hash_type id )
        {
            for( const auto &vp : m_viewports )
            {
                if( vp && vp->getViewportId() == id )
                {
                    return vp;
                }
            }

            WP_LOG_WARNING( "ClawRenderTarget::getViewportById: viewport not found." );
            return {};
        }

        SmartPtr<IViewport> ClawRenderTarget::getViewportByZOrder( s32 zorder ) const
        {
            for( const auto &vp : m_viewports )
            {
                if( vp && vp->getZOrder() == zorder )
                {
                    return vp;
                }
            }

            return {};
        }

        bool ClawRenderTarget::hasViewportWithZOrder( s32 zorder ) const
        {
            for( const auto &vp : m_viewports )
            {
                if( vp && vp->getZOrder() == zorder )
                {
                    return true;
                }
            }

            return false;
        }

        Array<SmartPtr<IViewport>> ClawRenderTarget::getViewports() const
        {
            return m_viewports;
        }

        void ClawRenderTarget::removeViewport( SmartPtr<IViewport> vp )
        {
            if( !vp )
            {
                WP_LOG_WARNING( "ClawRenderTarget::removeViewport: null viewport supplied." );
                return;
            }

            auto it = std::find( m_viewports.begin(), m_viewports.end(), vp );
            if( it != m_viewports.end() )
            {
                m_viewports.erase( it );
            }
            else
            {
                WP_LOG_WARNING( "ClawRenderTarget::removeViewport: viewport not found." );
            }
        }

        void ClawRenderTarget::removeAllViewports()
        {
            m_viewports.clear();
        }

        IRenderTarget::RenderTargetStats ClawRenderTarget::getRenderTargetStats() const
        {
            return m_stats;
        }

        void ClawRenderTarget::_getObject( void **ppObject ) const
        {
            if( ppObject )
            {
                *ppObject = wp_renderer_dx11_get_render_texture_target_view( m_nativeRenderTexture );
            }
        }

        wp_render_texture_dx11 *ClawRenderTarget::getNativeRenderTexture( wp_renderer *renderer )
        {
            auto dx11 = renderer ? wp_renderer_get_dx11( renderer ) : nullptr;
            auto device = dx11 ? wp_renderer_dx11_get_device( dx11 ) : nullptr;
            if( !dx11 || !device || m_size.x <= 0 || m_size.y <= 0 )
            {
                return nullptr;
            }

            if( m_nativeRenderTexture && m_nativeDevice == device && m_nativeSize == m_size )
            {
                return m_nativeRenderTexture;
            }

            releaseNativeRenderTexture();
            m_nativeRenderTexture = wp_renderer_dx11_create_render_texture( dx11, m_size.x, m_size.y );
            if( m_nativeRenderTexture )
            {
                m_nativeDevice = device;
                m_nativeSize = m_size;
            }
            else
            {
                WP_LOG_ERROR( "ClawRenderTarget: failed to allocate the DX11 render texture." );
            }

            return m_nativeRenderTexture;
        }

        void *ClawRenderTarget::getNativeTextureResource() const
        {
            return wp_renderer_dx11_get_render_texture_resource( m_nativeRenderTexture );
        }

        void *ClawRenderTarget::getNativeTextureView() const
        {
            return wp_renderer_dx11_get_render_texture_view( m_nativeRenderTexture );
        }

        void ClawRenderTarget::releaseNativeRenderTexture()
        {
            wp_renderer_dx11_destroy_render_texture( m_nativeRenderTexture );
            m_nativeRenderTexture = nullptr;
            m_nativeDevice = nullptr;
            m_nativeSize = Vector2I( 0, 0 );
        }

        bool ClawRenderTarget::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        bool ClawRenderTarget::handleStateChanged( SmartPtr<IState> &state )
        {
            return false;
        }

        SmartPtr<Properties> ClawRenderTarget::getProperties() const
        {
            try
            {
                auto properties = IRenderTarget::getProperties();
                if( !properties )
                {
                    WP_LOG_WARNING(
                        "ClawRenderTarget::getProperties: base class returned null properties." );
                    return {};
                }

                properties->setProperty( sizeStr, getSize() );
                properties->setProperty( colourDepthStr, getColourDepth() );
                properties->setProperty( priorityStr, static_cast<u32>( getPriority() ) );
                properties->setProperty( activeStr, isActive() );
                properties->setProperty( autoUpdatedStr, isAutoUpdated() );

                return properties;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return {};
        }

        void ClawRenderTarget::setProperties( SmartPtr<Properties> properties )
        {
            try
            {
                if( !properties )
                {
                    WP_LOG_WARNING( "ClawRenderTarget::setProperties: null properties supplied." );
                    return;
                }

                auto size = getSize();
                if( properties->getPropertyValue( sizeStr, size ) )
                {
                    setSize( size );
                }

                auto colourDepth = getColourDepth();
                if( properties->getPropertyValue( colourDepthStr, colourDepth ) )
                {
                    setColourDepth( colourDepth );
                }

                auto priority = static_cast<u32>( getPriority() );
                if( properties->getPropertyValue( priorityStr, priority ) )
                {
                    setPriority( static_cast<u8>( priority ) );
                }

                auto active = isActive();
                if( properties->getPropertyValue( activeStr, active ) )
                {
                    setActive( active );
                }

                auto autoUpdated = isAutoUpdated();
                if( properties->getPropertyValue( autoUpdatedStr, autoUpdated ) )
                {
                    setAutoUpdated( autoUpdated );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
    }  // namespace render
}  // namespace workphone
