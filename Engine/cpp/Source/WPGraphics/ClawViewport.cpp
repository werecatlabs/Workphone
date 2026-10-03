#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawViewport.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawViewport, IViewport );

        ClawViewport::ClawViewport()
        {
        }

        ClawViewport::~ClawViewport()
        {
        }

        void ClawViewport::setCamera( SmartPtr<IGraphicsCamera> camera )
        {
            m_camera = camera;
        }

        SmartPtr<IGraphicsCamera> ClawViewport::getCamera() const
        {
            return m_camera;
        }

        hash_type ClawViewport::getViewportId() const
        {
            return m_viewportId;
        }

        void ClawViewport::setViewportId( hash_type id )
        {
            m_viewportId = id;
        }

        s32 ClawViewport::getZOrder() const
        {
            return m_zOrder;
        }

        void ClawViewport::setZOrder( s32 zorder )
        {
            m_zOrder = zorder;
        }

        Vector2<real_Num> ClawViewport::getPosition() const
        {
            return m_position;
        }

        Vector2<real_Num> ClawViewport::getActualPosition() const
        {
            if( m_renderTarget )
            {
                const auto targetSize = m_renderTarget->getSize();
                return Vector2<real_Num>( m_position.x * targetSize.x, m_position.y * targetSize.y );
            }

            return m_actualPosition;
        }

        void ClawViewport::setPosition( const Vector2<real_Num> &position )
        {
            m_position = position;
        }

        Vector2<real_Num> ClawViewport::getSize() const
        {
            return m_size;
        }

        Vector2<real_Num> ClawViewport::getActualSize() const
        {
            if( m_renderTarget )
            {
                const auto targetSize = m_renderTarget->getSize();
                return Vector2<real_Num>( m_size.x * targetSize.x, m_size.y * targetSize.y );
            }

            return m_actualSize;
        }

        void ClawViewport::setSize( const Vector2<real_Num> &size )
        {
            m_size = size;
        }

        void ClawViewport::setBackgroundColour( const ColourF &colour )
        {
            m_backgroundColour = colour;
        }

        ColourF ClawViewport::getBackgroundColour() const
        {
            return m_backgroundColour;
        }

        void ClawViewport::setClearEveryFrame( bool clear, u32 buffers )
        {
            m_clearEveryFrame = clear;
            m_clearBuffers = buffers;
        }

        bool ClawViewport::getClearEveryFrame() const
        {
            return m_clearEveryFrame;
        }

        u32 ClawViewport::getClearBuffers() const
        {
            return m_clearBuffers;
        }

        void ClawViewport::setMaterialScheme( const String &schemeName )
        {
            m_materialScheme = schemeName;
        }

        String ClawViewport::getMaterialScheme() const
        {
            return m_materialScheme;
        }

        void ClawViewport::setEnableSceneRender( bool enabled )
        {
            m_enableSceneRender = enabled;
        }

        bool ClawViewport::getEnableSceneRender() const
        {
            return m_enableSceneRender;
        }

        void ClawViewport::setOverlaysEnabled( bool enabled )
        {
            m_overlaysEnabled = enabled;
        }

        bool ClawViewport::getOverlaysEnabled() const
        {
            return m_overlaysEnabled;
        }

        void ClawViewport::setEnableUI( bool enabled )
        {
            m_enableUI = enabled;
        }

        bool ClawViewport::getEnableUI() const
        {
            return m_enableUI;
        }

        void ClawViewport::setSkiesEnabled( bool enabled )
        {
            m_skiesEnabled = enabled;
        }

        bool ClawViewport::getSkiesEnabled() const
        {
            return m_skiesEnabled;
        }

        void ClawViewport::setShadowsEnabled( bool enabled )
        {
            m_shadowsEnabled = enabled;
        }

        bool ClawViewport::getShadowsEnabled() const
        {
            return m_shadowsEnabled;
        }

        void ClawViewport::setVisibilityMask( u32 mask )
        {
            m_visibilityMask = mask;
        }

        u32 ClawViewport::getVisibilityMask() const
        {
            return m_visibilityMask;
        }

        void ClawViewport::setAutoUpdated( bool autoupdate )
        {
            m_autoUpdated = autoupdate;
        }

        bool ClawViewport::isAutoUpdated() const
        {
            return m_autoUpdated;
        }

        void ClawViewport::_getObject( void **ppObject ) const
        {
            if( ppObject )
                *ppObject = (void *)this;
        }

        SmartPtr<ITexture> ClawViewport::getBackgroundTexture() const
        {
            return m_backgroundTexture;
        }

        void ClawViewport::setBackgroundTexture( SmartPtr<ITexture> texture )
        {
            m_backgroundTexture = texture;
        }

        String ClawViewport::getBackgroundTextureName() const
        {
            return m_backgroundTextureName;
        }

        void ClawViewport::setBackgroundTextureName( const String &textureName )
        {
            m_backgroundTextureName = textureName;
        }

        SmartPtr<IRenderTarget> ClawViewport::getRenderTarget() const
        {
            return m_renderTarget;
        }

        void ClawViewport::setRenderTarget( SmartPtr<IRenderTarget> renderTarget )
        {
            m_renderTarget = renderTarget;
        }

        bool ClawViewport::isActive() const
        {
            return m_active;
        }

        void ClawViewport::setActive( bool active )
        {
            m_active = active;
        }

        bool ClawViewport::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        bool ClawViewport::handleStateChanged( SmartPtr<IState> &state )
        {
            return false;
        }
    }  // namespace render
}  // namespace workphone
