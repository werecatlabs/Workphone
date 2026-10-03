#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/GraphicsWindow.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindowListener.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindowEvent.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/State/States/State.hpp>
#include <Workphone/State/States/WindowStateData.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, GraphicsWindow, RenderTarget<IGraphicsWindow> );

    GraphicsWindow::GraphicsWindow() = default;

    GraphicsWindow::~GraphicsWindow() = default;

    void GraphicsWindow::load( SmartPtr<ISharedObject> data )
    {
        SharedGraphicsObject<IGraphicsWindow>::load( data );
    }

    void GraphicsWindow::unload( SmartPtr<ISharedObject> data )
    {
        SharedGraphicsObject<IGraphicsWindow>::unload( data );
    }

    void GraphicsWindow::handleEvent( SmartPtr<IGraphicsWindowEvent> event )
    {
    }

    String GraphicsWindow::getTitle() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateData<WindowStateData>() )
            {
                return stateData->title;
            }
        }

        return {};
    }

    void GraphicsWindow::setTitle( const String &title )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateData<WindowStateData>() )
            {
                stateData->title = title;
            }
        }
    }

    void GraphicsWindow::setFullscreen( bool fullScreen, u32 width, u32 height )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateData<WindowStateData>() )
            {
                if( fullScreen )
                {
                    stateData->flags |= WINDOW_FLAG_FULLSCREEN;
                }
                else
                {
                    stateData->flags &= ~WINDOW_FLAG_FULLSCREEN;
                }
            }

            if( auto stateData = stateContext->getStateData<RenderTargetStateData>() )
            {
                stateData->size = Vector2I( width, height );
            }
        }
    }

    void GraphicsWindow::setFullscreen( bool fullscreen )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateData<WindowStateData>() )
            {
                if( fullscreen )
                {
                    stateData->flags |= WINDOW_FLAG_FULLSCREEN;
                }
                else
                {
                    stateData->flags &= ~WINDOW_FLAG_FULLSCREEN;
                }
            }
        }
    }

    void GraphicsWindow::destroy()
    {
    }

    void GraphicsWindow::resize( u32 width, u32 height )
    {
    }

    void GraphicsWindow::windowMovedOrResized()
    {
    }

    void GraphicsWindow::reposition( s32 left, s32 top )
    {
    }

    void GraphicsWindow::maximize()
    {
    }

    bool GraphicsWindow::isVisible() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateData<WindowStateData>() )
            {
                return ( stateData->flags & WINDOW_FLAG_VISIBLE ) != 0;
            }
        }

        return false;
    }

    void GraphicsWindow::setVisible( bool visible )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateData<WindowStateData>() )
            {
                if( visible )
                {
                    stateData->flags |= WINDOW_FLAG_VISIBLE;
                }
                else
                {
                    stateData->flags &= ~WINDOW_FLAG_VISIBLE;
                }
            }
        }
    }

    bool GraphicsWindow::isClosed() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateData<WindowStateData>() )
            {
                return ( stateData->flags & WINDOW_FLAG_IS_CLOSED ) != 0;
            }
        }

        return false;
    }

    bool GraphicsWindow::isPrimary() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateData<WindowStateData>() )
            {
                return ( stateData->flags & WINDOW_FLAG_IS_PRIMARY ) != 0;
            }
        }

        return false;
    }

    bool GraphicsWindow::isFullScreen() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateData<WindowStateData>() )
            {
                return ( stateData->flags & WINDOW_FLAG_FULLSCREEN ) != 0;
            }
        }

        return false;
    }

    bool GraphicsWindow::isDeactivatedOnFocusChange() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateData<WindowStateData>() )
            {
                return ( stateData->flags & WINDOW_FLAG_DEACTIVATE_ON_FOCUS_CHANGE ) != 0;
            }
        }

        return false;
    }

    void GraphicsWindow::setDeactivateOnFocusChange( bool deactivate )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateData<WindowStateData>() )
            {
                if( deactivate )
                {
                    stateData->flags |= WINDOW_FLAG_DEACTIVATE_ON_FOCUS_CHANGE;
                }
                else
                {
                    stateData->flags &= ~WINDOW_FLAG_DEACTIVATE_ON_FOCUS_CHANGE;
                }
            }
        }
    }

    void GraphicsWindow::getCustomAttribute( const String &name, void *pData )
    {
    }

    void GraphicsWindow::getWindowHandle( void *pData )
    {
    }

    void GraphicsWindow::getDeviceHandle( void *pData )
    {
    }

    String GraphicsWindow::getWindowHandleAsString() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateData<WindowStateData>() )
            {
                return stateData->windowHandle;
            }
        }

        return {};
    }

    void GraphicsWindow::setWindowHandleAsString( const String &handle )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateData<WindowStateData>() )
            {
                stateData->windowHandle = handle;
            }
        }
    }

    void GraphicsWindow::addListener( SmartPtr<IGraphicsWindowListener> listener )
    {
        m_listeners.push_back( listener );
    }

    void GraphicsWindow::removeListener( SmartPtr<IGraphicsWindowListener> listener )
    {
        ScopedLock lock( &m_listeners );

        auto it = std::find( m_listeners.begin(), m_listeners.end(), listener );
        if( it != m_listeners.end() )
        {
            m_listeners.erase( it );
        }
    }

    Array<SmartPtr<IGraphicsWindowListener>> GraphicsWindow::getListeners() const
    {
        return m_listeners.snapshot();
    }

    SmartPtr<ITexture> GraphicsWindow::getTexture() const
    {
        return nullptr;
    }

    void GraphicsWindow::setPosition( const Vector2I &position )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateData<WindowStateData>() )
            {
                stateData->position = position;
            }
        }
    }

    Vector2I GraphicsWindow::getPosition() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateData<WindowStateData>() )
            {
                return stateData->position;
            }
        }

        return {};
    }

    void GraphicsWindow::setupStateObject()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManager();
        WP_ASSERT( stateManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto stateContext = stateManager->addStateContext();
        stateContext->setOwner( this );
        setStateContext( stateContext );

        //auto stateListener = factoryManager->make_ptr<WindowStateListener>();
        //stateListener->setOwner( this );
        //m_stateListener = stateListener;
        //stateContext->addStateListener( stateListener );

        auto state = factoryManager->make_ptr<State>();
        stateContext->addState( state );

        auto stateData = factoryManager->make_ptr<WindowStateData>();
        state->setData( stateData );

        auto renderTask = applicationManager->hasTasks() ? TaskId::Render : TaskId::Primary;
        stateContext->setTaskId( renderTask );
    }
}  // namespace workphone::render
