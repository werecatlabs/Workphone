#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/Viewport.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/State/States/ViewportStateData.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, Viewport, IViewport );

    u32 Viewport::m_idExt = 0;

    Viewport::Viewport() = default;

    Viewport::~Viewport() = default;

    void Viewport::load( SmartPtr<ISharedObject> data )
    {
        SharedGraphicsObject<IViewport>::load( data );
    }

    void Viewport::unload( SmartPtr<ISharedObject> data )
    {
        if( auto camera = getCamera() )
        {
            camera->setViewport( nullptr );
        }

        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                state->camera = nullptr;
                state->texture = nullptr;
                state->backgroundTexture = nullptr;
            }
        }

        m_renderTarget = nullptr;
        m_window = nullptr;

        SharedGraphicsObject<IViewport>::unload( data );
    }

    void Viewport::setCamera( SmartPtr<IGraphicsCamera> camera )
    {
        if( auto currentCamera = getCamera() )
        {
            currentCamera->setViewport( nullptr );
        }

        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<ViewportStateData>( getId() ) )
            {
                state->camera = camera;
            }
        }

        if( camera )
        {
            camera->setViewport( this );
        }
    }

    SmartPtr<IGraphicsCamera> Viewport::getCamera() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return state->camera.lock();
            }
        }

        return nullptr;
    }

    hash_type Viewport::getViewportId() const
    {
        return m_viewportId;
    }

    void Viewport::setViewportId( hash_type id )
    {
        m_viewportId = id;
    }

    void Viewport::setOverlaysEnabled( bool enabled )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<ViewportStateData>( getId() ) )
            {
                state->flags = BitUtil::setFlagValue( state->flags, overlaysEnabledFlag, enabled );
            }
        }
    }

    bool Viewport::getOverlaysEnabled() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->flags, overlaysEnabledFlag );
            }
        }

        return false;
    }

    void Viewport::setBackgroundColour( const ColourF &colour )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<ViewportStateData>( getId() ) )
            {
                state->backgroundColour = colour;
            }
        }
    }

    ColourF Viewport::getBackgroundColour() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return state->backgroundColour;
            }
        }

        return ColourF::White;
    }

    void Viewport::setZOrder( s32 zorder )
    {
        //WP_ASSERT(getRenderTarget() && getRenderTarget()->hasViewportWithZOrder( zorder ) == false);

        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<ViewportStateData>( getId() ) )
            {
                state->zorder = zorder;
            }
        }
    }

    s32 Viewport::getZOrder() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return state->zorder;
            }
        }

        return 0;
    }

    void Viewport::setPriority( u8 priority )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<ViewportStateData>( getId() ) )
            {
                state->priority = (u32)priority;
            }
        }
    }

    u8 Viewport::getPriority() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return (u8)state->priority;
            }
        }

        return 0;
    }

    bool Viewport::isActive() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->flags, activeFlag );
            }
        }

        return false;
    }

    void Viewport::setActive( bool active )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<ViewportStateData>( getId() ) )
            {
                state->flags = BitUtil::setFlagValue( state->flags, activeFlag, active );
            }
        }
    }

    void Viewport::setAutoUpdated( bool autoUpdated )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<ViewportStateData>( getId() ) )
            {
                state->flags = BitUtil::setFlagValue( state->flags, autoUpdatedFlag, autoUpdated );
            }
        }
    }

    bool Viewport::isAutoUpdated() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->flags, autoUpdatedFlag );
            }
        }

        return false;
    }

    void Viewport::setSkiesEnabled( bool enabled )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<ViewportStateData>( getId() ) )
            {
                state->flags = BitUtil::setFlagValue( state->flags, skiesEnabledFlag, enabled );
            }
        }
    }

    bool Viewport::getSkiesEnabled() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->flags, skiesEnabledFlag );
            }
        }

        return false;
    }

    void Viewport::setVisibilityMask( u32 mask )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<ViewportStateData>( getId() ) )
            {
                state->visibilityMask = mask;
            }
        }
    }

    u32 Viewport::getVisibilityMask() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return state->visibilityMask;
            }
        }

        return 0;
    }

    void Viewport::setShadowsEnabled( bool enabled )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<ViewportStateData>( getId() ) )
            {
                state->flags = BitUtil::setFlagValue( state->flags, shadowsEnabledFlag, enabled );
            }
        }
    }

    bool Viewport::getShadowsEnabled() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->flags, shadowsEnabledFlag );
            }
        }

        return false;
    }

    Vector2<real_Num> Viewport::getPosition() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return state->position;
            }
        }

        return Vector2<real_Num>::zero();
    }

    Vector2<real_Num> Viewport::getActualPosition() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return state->actualPosition;
            }
        }

        return Vector2<real_Num>::zero();
    }

    void Viewport::setPosition( const Vector2<real_Num> &position )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<ViewportStateData>( getId() ) )
            {
                state->position = position;
            }
        }
    }

    Vector2<real_Num> Viewport::getSize() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return state->size;
            }
        }

        return Vector2<real_Num>::zero();
    }

    Vector2<real_Num> Viewport::getActualSize() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return state->actualSize;
            }
        }

        return Vector2<real_Num>::zero();
    }

    void Viewport::setSize( const Vector2<real_Num> &size )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<ViewportStateData>( getId() ) )
            {
                state->size = size;
            }
        }
    }

    SmartPtr<ITexture> Viewport::getBackgroundTexture() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return state->backgroundTexture.lock();
            }
        }

        return nullptr;
    }

    void Viewport::setBackgroundTexture( SmartPtr<ITexture> texture )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<ViewportStateData>( getId() ) )
            {
                state->backgroundTexture = texture;
            }
        }
    }

    void Viewport::setEnableUI( bool enabled )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<ViewportStateData>( getId() ) )
            {
                state->flags = BitUtil::setFlagValue( state->flags, enableUIFlag, enabled );
            }
        }
    }

    bool Viewport::getEnableUI() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->flags, enableUIFlag );
            }
        }

        return false;
    }

    void Viewport::setEnableSceneRender( bool enabled )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<ViewportStateData>( getId() ) )
            {
                state->flags = BitUtil::setFlagValue( state->flags, enableSceneRenderFlag, enabled );
            }
        }
    }

    bool Viewport::getEnableSceneRender() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->flags, enableSceneRenderFlag );
            }
        }

        return false;
    }

    void Viewport::setMaterialScheme( const String &schemeName )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<ViewportStateData>( getId() ) )
            {
                state->materialScheme = schemeName;
            }
        }
    }

    String Viewport::getMaterialScheme() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return state->materialScheme;
            }
        }

        return {};
    }

    void Viewport::setClearEveryFrame( bool clear, u32 buffers )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<ViewportStateData>( getId() ) )
            {
                state->flags = BitUtil::setFlagValue( state->flags, clearFlag, clear );
                state->buffers = buffers;
            }
        }
    }

    bool Viewport::getClearEveryFrame() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->flags, clearFlag );
            }
        }

        return false;
    }

    u32 Viewport::getClearBuffers() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return state->buffers;
            }
        }

        return 0;
    }

    SmartPtr<IGraphicsWindow> Viewport::getWindow() const
    {
        auto p = m_window.load();
        return p.lock();
    }

    void Viewport::setWindow( SmartPtr<IGraphicsWindow> window )
    {
        m_window = window;
    }

    String Viewport::getBackgroundTextureName() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<ViewportStateData>( getId() ) )
            {
                return state->backgroundTextureName;
            }
        }

        return {};
    }

    void Viewport::setBackgroundTextureName( const String &textureName )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<ViewportStateData>( getId() ) )
            {
                state->backgroundTextureName = textureName;
            }
        }
    }

    SmartPtr<IRenderTarget> Viewport::getRenderTarget() const
    {
        auto p = m_renderTarget.load();
        return p.lock();
    }

    void Viewport::setRenderTarget( SmartPtr<IRenderTarget> renderTarget )
    {
        if( auto rt = getRenderTarget() )
        {
            removeViewportFromRT();
        }

        m_renderTarget = renderTarget;
    }

    void Viewport::removeViewportFromRT()
    {
    }

    bool Viewport::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( message->getSender() == this )
        {
            return true;
        }

        return false;
    }

    bool Viewport::handleStateChanged( SmartPtr<IState> &state )
    {
        if( state->getOwnerPtr() == this )
        {
            return true;
        }

        return false;
    }
}  // namespace workphone::render
