#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UIManagerColibri.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UIButtonColibri.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UIElementColibri.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UIImageColibri.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UILayoutColibri.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UISliderColibri.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UITextColibri.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UIToggleColibri.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreHlms.h>
#include <WPGraphicsOgreNext/ColibriGui/ColibriButton.h>
#include <WPGraphicsOgreNext/ColibriGui/ColibriCheckbox.h>
#include <WPGraphicsOgreNext/ColibriGui/ColibriEditbox.h>
#include <WPGraphicsOgreNext/ColibriGui/ColibriLabel.h>
#include <WPGraphicsOgreNext/ColibriGui/ColibriProgressbar.h>
#include <WPGraphicsOgreNext/ColibriGui/ColibriRadarChart.h>
#include <WPGraphicsOgreNext/ColibriGui/ColibriSlider.h>
#include <WPGraphicsOgreNext/ColibriGui/ColibriSpinner.h>
#include <WPGraphicsOgreNext/ColibriGui/ColibriToggleButton.h>
#include <WPGraphicsOgreNext/ColibriGui/ColibriWindow.h>
#include <WPGraphicsOgreNext/ColibriGui/Text/ColibriShaperManager.h>
#include <WPGraphicsOgreNext/ColibriGui/Layouts/ColibriLayoutLine.h>
#include <WPGraphicsOgreNext/ColibriGui/Layouts/ColibriLayoutMultiline.h>
#include <WPGraphicsOgreNext/ColibriGui/Layouts/ColibriLayoutTableSameSize.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, UIManagerColibri, IUIManager );

    UIManagerColibri::UIManagerColibri()
    {
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto inputListener = factoryManager->make_ptr<EventListener>();
        inputListener->setOwner( this );
        m_inputListener = inputListener;
    }

    UIManagerColibri::~UIManagerColibri()
    {
        auto inputListener = m_inputListener.load();
        if( inputListener )
        {
            inputListener->unload( nullptr );
            m_inputListener = nullptr;
        }
    }

    void UIManagerColibri::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            ScopedLock lock( this );

            auto applicationManager = core::IApplicationManager::instancePtr();
            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            auto fileSystem = applicationManager->getFileSystemPtr();

            auto factoryManager = workphone::make_ptr<FactoryManager>();
            factoryManager->load( nullptr );
            setFactoryManager( factoryManager );

            FactoryUtil::addFactory<UILayoutColibri>( factoryManager );
            FactoryUtil::addFactory<UITextColibri>( factoryManager );
            FactoryUtil::addFactory<UIImageColibri>( factoryManager );
            FactoryUtil::addFactory<UISliderColibri>( factoryManager );
            FactoryUtil::addFactory<UIButtonColibri>( factoryManager );
            FactoryUtil::addFactory<UIToggleColibri>( factoryManager );

            const size_t numLayoutElements = 4;
            const size_t numButtonElements = 4;
            const size_t numImageElements = 32;
            const size_t numTextElements = 32;

            factoryManager->setPoolSizeByType<UILayoutColibri>( numLayoutElements );
            factoryManager->setPoolSizeByType<UIButtonColibri>( numButtonElements );
            factoryManager->setPoolSizeByType<UIImageColibri>( numImageElements );
            factoryManager->setPoolSizeByType<UITextColibri>( numTextElements );

            factoryManager->allocateData();

            struct ShaperSettings
            {
                const char *locale;
                const char *fullpath;
                hb_script_t script;
                Colibri::HorizReadingDir::HorizReadingDir horizReadingDir;
                bool useKerning;
                bool allowsVerticalLayout;

                ShaperSettings( const char *_locale, const char *_fullpath, hb_script_t _script,
                                bool _useKerning = false,
                                Colibri::HorizReadingDir::HorizReadingDir _horizReadingDir =
                                    Colibri::HorizReadingDir::LTR,
                                bool _allowsVerticalLayout = false ) :
                    locale( _locale ),
                    fullpath( _fullpath ),
                    script( _script ),
                    horizReadingDir( _horizReadingDir ),
                    useKerning( _useKerning ),
                    allowsVerticalLayout( _allowsVerticalLayout )
                {
                }
            };

            auto sceneManager = graphicsSystem->getGraphicsScene();
            WP_ASSERT( sceneManager );

            auto dataPath = applicationManager->getRenderMediaPath() + "/";

            m_colibriLogListener = new ColibriLogListener();
            m_colibriListener = new ColibriListener();

            m_colibriManager = new Colibri::ColibriManager( m_colibriLogListener, m_colibriListener );

            ShaperSettings shaperSettings[3] = {
                ShaperSettings( "en", "Fonts/DejaVuSerif.ttf", HB_SCRIPT_LATIN, true ),
                ShaperSettings( "ar", "Fonts/amiri-0.104/amiri-regular.ttf", HB_SCRIPT_ARABIC, false,
                                Colibri::HorizReadingDir::RTL ),
                ShaperSettings( "ch", "Fonts/fireflysung-1.3.0/fireflysung.ttf", HB_SCRIPT_HAN, false,
                                Colibri::HorizReadingDir::LTR, true )
            };

            auto shaperManager = m_colibriManager->getShaperManager();

            auto shaperAdded = false;
            for( auto &shaperSetting : shaperSettings )
            {
                const auto fullPath = dataPath + shaperSetting.fullpath;

                if( fileSystem->isExistingFile( fullPath, true, true ) )
                {
                    auto shaper = shaperManager->addShaper( shaperSetting.script, fullPath.c_str(),
                                                            shaperSetting.locale );
                    if( shaperSetting.useKerning )
                    {
                        shaper->addFeatures( Colibri::Shaper::KerningOn );
                    }

                    shaperAdded = true;
                }
                else
                {
                    WP_LOG_ERROR( "Shaper font file not found: " + fullPath );
                }
            }

            if( shaperAdded )
            {
                auto defaultFont = 0;  //"en"
                shaperManager->setDefaultShaper( defaultFont + 1u,
                                                 shaperSettings[defaultFont].horizReadingDir,
                                                 shaperSettings[defaultFont].allowsVerticalLayout );
            }

            shaperManager->addBmpFont( ( dataPath + "Fonts/ExampleBmpFont.fnt" ).c_str() );
            shaperManager->setDefaultBmpFontForRaster( 0u );

            //mCameraController = new CameraController( mGraphicsSystem, false );

            auto canvasSize = Vector2I( 1280, 720 );

            auto root = Ogre::Root::getSingletonPtr();
            //Ogre::Window *window = mGraphicsSystem->getRenderWindow();

            auto resourcePath = applicationManager->getRenderMediaPath() + "/";

            const auto aspectRatioColibri =
                static_cast<f32>( canvasSize.y ) / static_cast<f32>( canvasSize.x );
            //m_colibriManager->setCanvasSize( m_canvasSize, m_windowResolution );
            m_colibriManager->setCanvasSize( Ogre::Vector2( 1920.0f, 1920.0f * aspectRatioColibri ),
                                             Ogre::Vector2( static_cast<Ogre::Real>( canvasSize.x ),
                                                            static_cast<Ogre::Real>( canvasSize.y ) ) );

            //Ogre::SceneManager *smgr = nullptr;
            //sceneManager->_getObject( (void **)&smgr );

            //m_colibriManager = new Colibri::ColibriManager();
            //m_colibriManager->setOgre( root, root->getRenderSystem()->getVaoManager(), smgr );

            auto skinPath = resourcePath + "Materials/ColibriGui/Skins/DarkGloss/Skins.colibri.json";
            m_colibriManager->loadSkins( skinPath.c_str() );

            auto inputMgr = applicationManager->getInputDeviceManager();
            if( inputMgr )
            {
                inputMgr->addListener( m_inputListener );
            }

            if( sceneManager )
            {
                if( sceneManager->isLoaded() )
                {
                    setupSceneManager( sceneManager );
                    createLayoutWindow();
                }
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void UIManagerColibri::reload( SmartPtr<ISharedObject> data )
    {
        try
        {
            unload( data );
            load( data );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void UIManagerColibri::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                auto graphicsSystem = applicationManager->getGraphicsSystem();

                ScopedLock lock( graphicsSystem );

                m_loadQueue.clear();

                if( !m_unloadQueue.empty() )
                {
                    SmartPtr<ISharedObject> graphicsObject;
                    while( m_unloadQueue.try_pop( graphicsObject ) )
                    {
                        graphicsObject->unload( nullptr );
                    }
                }

                for( auto element : m_elements )
                {
                    if( element )
                    {
                        element->unload( data );
                    }
                }

                m_elements.clear();

                auto inputMgr = applicationManager->getInputDeviceManager();
                if( inputMgr )
                {
                    if( m_inputListener )
                    {
                        inputMgr->removeListener( m_inputListener );
                        m_inputListener = nullptr;
                    }
                }

                if( auto factoryManager = getFactoryManager() )
                {
                    factoryManager->unload( nullptr );
                    setFactoryManager( nullptr );
                }

                if( m_colibriManager )
                {
                    if( auto layoutWindow = getLayoutWindow() )
                    {
                        m_colibriManager->destroyWindow( layoutWindow );
                        setLayoutWindow( nullptr );
                    }

                    delete m_colibriManager;
                    m_colibriManager = nullptr;
                }

                if( m_colibriLogListener )
                {
                    delete m_colibriLogListener;
                    m_colibriLogListener = nullptr;
                }

                if( m_colibriListener )
                {
                    delete m_colibriListener;
                    m_colibriListener = nullptr;
                }

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    bool UIManagerColibri::loadFont( const String &fontPath, const String &type )
    {
        return false;
    }

    void UIManagerColibri::unloadFont( const String &fontPath, const String &type )
    {
    }

    bool UIManagerColibri::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        auto task = Thread::getCurrentTask();
        if( task == TaskId::Render )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto colibriManager = getColibriManager();

            auto eventType = event->getEventType();
            if( eventType == IInputEvent::EventType::Mouse )
            {
                if( auto mouseState = event->getMouseState() )
                {
                    auto absolutePosition = mouseState->getAbsolutePosition();
                    auto x = absolutePosition.X();
                    auto y = absolutePosition.Y();
                    auto z = 0;

                    auto relativePosition = mouseState->getRelativePosition();

                    if( auto uiWindow = applicationManager->getSceneRenderWindow() )
                    {
                        if( auto mainWindow = applicationManager->getWindow() )
                        {
                            auto mainWindowSize = mainWindow->getSize();
                            auto mainWindowSizeF = Vector2F( static_cast<f32>( mainWindowSize.x ),
                                                             static_cast<f32>( mainWindowSize.y ) );

                            auto sceneWindowPosition = uiWindow->getPosition();
                            auto sceneWindowSize = uiWindow->getSize();

                            auto pos = sceneWindowPosition / mainWindowSizeF;
                            auto size = sceneWindowSize / mainWindowSizeF;

                            auto aabb = AABB2F( pos, size, true );
                            if( aabb.isInside( relativePosition ) )
                            {
                                x = ( absolutePosition.X() - sceneWindowPosition.X() ) / size.X();
                                y = ( absolutePosition.Y() - sceneWindowPosition.Y() ) / size.Y();

                                Ogre::Vector2 mousePos( x, y );
                                auto canvasSize = colibriManager->getCanvasSize();
                                auto canvasPoint = mousePos / canvasSize;

                                if( mouseState->getEventType() == IMouseState::Event::Moved )
                                {
                                    colibriManager->setMouseCursorMoved( mousePos );
                                }

                                if( mouseState->getEventType() == IMouseState::Event::LeftPressed )
                                {
                                    colibriManager->setMouseCursorMoved( mousePos );
                                    colibriManager->setMouseCursorPressed( true, false );
                                }

                                if( mouseState->getEventType() == IMouseState::Event::LeftReleased )
                                {
                                    colibriManager->setMouseCursorReleased();
                                }
                            }
                        }
                    }
                    else
                    {
                        if( mouseState->getEventType() == IMouseState::Event::Moved )
                        {
                            Ogre::Vector2 mousePos( x, y );
                            colibriManager->setMouseCursorMoved( mousePos *
                                                                 colibriManager->getCanvasSize() );
                        }

                        if( mouseState->getEventType() == IMouseState::Event::LeftPressed )
                        {
                            Ogre::Vector2 mousePos( x, y );
                            colibriManager->setMouseCursorMoved( mousePos *
                                                                 colibriManager->getCanvasSize() );
                            colibriManager->setMouseCursorPressed( true, false );
                        }

                        if( mouseState->getEventType() == IMouseState::Event::LeftReleased )
                        {
                            colibriManager->setMouseCursorReleased();
                        }
                    }
                }
            }

            auto elements = getElements();
            for( auto &element : elements )
            {
                if( element )
                {
                    element->handleEvent( event );
                }
            }
        }

        return false;
    }

    void UIManagerColibri::_getObject( void **ppObject )
    {
        if( isLoaded() )
        {
            // Check if is in valid state
            auto graphicsScene = getGraphicsScene();
            if( graphicsScene )
            {
                if( m_layoutWindow )
                {
                    *ppObject = m_colibriManager;
                }
            }
        }
    }

    void UIManagerColibri::update()
    {
        try
        {
            if( isLoaded() )
            {
                if( auto graphicsScene = getGraphicsScenePtr() )
                {
                    if( m_colibriManager )
                    {
                        if( m_layoutWindow )
                        {
                            if( !m_unloadQueue.empty() )
                            {
                                SmartPtr<ISharedObject> graphicsObject;
                                while( m_unloadQueue.try_pop( graphicsObject ) )
                                {
                                    if( graphicsObject )
                                    {
                                        graphicsObject->unload( nullptr );
                                    }

                                    m_elements.erase( std::remove( m_elements.begin(), m_elements.end(),
                                                                   graphicsObject ),
                                                      m_elements.end() );
                                }
                            }

                            if( !m_loadQueue.empty() )
                            {
                                SmartPtr<ISharedObject> graphicsObject;
                                while( m_loadQueue.try_pop( graphicsObject ) )
                                {
                                    if( graphicsObject )
                                    {
                                        if( !graphicsObject->isLoaded() )
                                        {
                                            graphicsObject->load( nullptr );
                                        }
                                    }
                                }
                            }

                            ScopedLock lock( this );

                            auto applicationManager = core::IApplicationManager::instancePtr();
                            auto timer = applicationManager->getTimerPtr();

                            auto dt = static_cast<f32>( timer->getDeltaTime() );
                            //m_layoutWindow->updateDerivedTransformFromParent( true );
                            //m_layoutWindow->updateZOrderDirty();
                            m_colibriManager->update( dt );
                        }
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            auto message = e.what();
            WP_LOG_ERROR( message );
        }
    }

    SmartPtr<IUIElement> UIManagerColibri::addElement( hash64 type )
    {
        auto factoryManager = getFactoryManager();
        if( factoryManager )
        {
            auto element = factoryManager->make_object<IUIElement>( (u32)type );
            m_elements.push_back( element );

            loadObject( element );

            Array<Parameter> args;

            auto applicationManager = core::IApplicationManager::instancePtr();
            applicationManager->triggerEvent( EventType::UI, IEvent::addUIElement, args, this,
                                              element, nullptr );
            return element;
        }

        return nullptr;
    }

    void UIManagerColibri::removeElement( SmartPtr<IUIElement> element )
    {
        if( !m_elements.empty() )
        {
            unloadObject( element );

            m_elements.erase( std::remove( m_elements.begin(), m_elements.end(), element ),
                              m_elements.end() );
        }

        Array<Parameter> args;

        auto applicationManager = core::IApplicationManager::instancePtr();
        applicationManager->triggerEvent( EventType::UI, IEvent::removeUIElement, args, this, element,
                                          nullptr );
    }

    void UIManagerColibri::removeElements( const Array<SmartPtr<IUIElement>> &elementsToRemove )
    {
        for( auto &element : elementsToRemove )
        {
            unloadObject( element );

            auto it = std::remove( m_elements.begin(), m_elements.end(), element );
            if( it != m_elements.end() )
            {
                m_elements.erase( it, m_elements.end() );
            }
        }

        Array<Parameter> args;

        auto applicationManager = core::IApplicationManager::instancePtr();
        applicationManager->triggerEvent( EventType::UI, IEvent::removeUIElement, args, this, nullptr,
                                          nullptr );
    }

    void UIManagerColibri::clear()
    {
        m_elements.clear();
    }

    Colibri::ColibriManager *UIManagerColibri::getColibriManager() const
    {
        return m_colibriManager;
    }

    void UIManagerColibri::setColibriManager( Colibri::ColibriManager *colibriManager )
    {
        m_colibriManager = colibriManager;
    }

    Colibri::Window *UIManagerColibri::getLayoutWindow() const
    {
        return m_layoutWindow;
    }

    void UIManagerColibri::setLayoutWindow( Colibri::Window *layoutWindow )
    {
        m_layoutWindow = layoutWindow;
    }

    SmartPtr<IFactoryManager> UIManagerColibri::getFactoryManager() const
    {
        return m_factoryManager;
    }

    void UIManagerColibri::setFactoryManager( SmartPtr<IFactoryManager> factoryManager )
    {
        m_factoryManager = factoryManager;
    }

    Array<SmartPtr<IUIElement>> UIManagerColibri::getElements() const
    {
        return m_elements.snapshot();
    }

    void UIManagerColibri::setElements( Array<SmartPtr<IUIElement>> elements )
    {
        m_elements = ConcurrentArray<SmartPtr<IUIElement>>( elements.begin(), elements.end() );
    }

    SmartPtr<render::IGraphicsScene> UIManagerColibri::getGraphicsScene() const
    {
        auto p = m_graphicsScene.load();
        return p.lock();
    }

    void UIManagerColibri::setGraphicsScene( SmartPtr<render::IGraphicsScene> scene )
    {
        auto graphicsScene = getGraphicsScene();
        if( graphicsScene != scene )
        {
            m_graphicsScene = scene;

            if( scene )
            {
                setupSceneManager( scene );
                createLayoutWindow();
            }
        }
    }

    void UIManagerColibri::lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        graphicsSystem->lock();
    }

    void UIManagerColibri::unlock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        graphicsSystem->unlock();
    }

    bool UIManagerColibri::isValid() const
    {
        if( isLoaded() )
        {
            auto root = Ogre::Root::getSingletonPtr();
            Ogre::HlmsManager *hlmsManager = root->getHlmsManager();
            Ogre::Hlms *hlms = hlmsManager->getHlms( Ogre::HLMS_USER0 );
            if( hlms )
            {
                return true;
            }
        }

        return false;
    }

    void UIManagerColibri::setupSceneManager( SmartPtr<render::IGraphicsScene> sceneManager )
    {
        ScopedLock lock( this );

        m_graphicsScene = sceneManager;

        if( sceneManager )
        {
            auto root = Ogre::Root::getSingletonPtr();

            Ogre::SceneManager *smgr = nullptr;
            sceneManager->_getObject( reinterpret_cast<void **>( &smgr ) );

            if( smgr )
            {
                auto renderSystem = root->getRenderSystem();
                auto vaoManager = renderSystem->getVaoManager();

                Ogre::HlmsManager *hlmsManager = root->getHlmsManager();
                Ogre::Hlms *hlms = hlmsManager->getHlms( Ogre::HLMS_USER0 );
                if( hlms )
                {
                    if( m_colibriManager )
                    {
                        m_colibriManager->setOgre( root, vaoManager, smgr );
                    }
                }
                else
                {
                    WP_LOG_ERROR( "Failed to get Hlms for user 0." );
                }
            }
        }
    }

    void UIManagerColibri::createLayoutWindow()
    {
        ScopedLock lock( this );

        if( !m_layoutWindow )
        {
            if( m_colibriManager )
            {
                m_layoutWindow = m_colibriManager->createWindow( nullptr );
                m_layoutWindow->setTransform( Ogre::Vector2( 0, 0 ), Ogre::Vector2( 1920, 1080 ) );
                m_layoutWindow->setSkin( "EmptyBg" );
                m_layoutWindow->setVisualsEnabled( false );
                //m_layoutWindow->setColour( true, Ogre::ColourValue( 0, 0, 0, 0 ) );
                m_layoutWindow->setHidden( false );
                m_layoutWindow->setKeyboardNavigable( false );
                m_layoutWindow->m_breadthFirst = true;
            }
        }
    }

    void UIManagerColibri::invalidate()
    {
    }

    void UIManagerColibri::unloadObject( SmartPtr<ISharedObject> graphicsObject,
                                         bool forceQueue /*= false */ )
    {
        if( forceQueue )
        {
            graphicsObject->unload( nullptr );
        }
        else
        {
            m_unloadQueue.push( graphicsObject );
        }
    }

    void UIManagerColibri::loadObject( SmartPtr<ISharedObject> graphicsObject,
                                       bool forceQueue /*= false */ )
    {
        if( forceQueue )
        {
            graphicsObject->load( nullptr );
        }
        else
        {
            m_loadQueue.push( graphicsObject );
        }
    }

    void UIManagerColibri::setOverlay( SmartPtr<ISharedObject> overlay )
    {
    }

    SmartPtr<ISharedObject> UIManagerColibri::getOverlay() const
    {
        return nullptr;
    }

    UIManagerColibri::EventListener::EventListener() = default;
    UIManagerColibri::EventListener::~EventListener() = default;

    void UIManagerColibri::EventListener::unload( SmartPtr<ISharedObject> data )
    {
        setOwner( nullptr );
    }

    Parameter UIManagerColibri::EventListener::handleEvent( EventType eventType, hash_type eventValue,
                                                            const Array<Parameter> &arguments,
                                                            SmartPtr<ISharedObject> sender,
                                                            SmartPtr<ISharedObject> object,
                                                            SmartPtr<IEvent> event )
    {
        if( Thread::getTaskFlag( Thread::Render_Flag ) )
        {
            if( auto owner = getOwner() )
            {
                owner->handleEvent( event );
            }
        }

        return {};
    }

    SmartPtr<UIManagerColibri> UIManagerColibri::EventListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void UIManagerColibri::EventListener::setOwner( SmartPtr<UIManagerColibri> owner )
    {
        m_owner = owner;
    }

    void UIManagerColibri::ColibriLogListener::log( const char *text,
                                                    Colibri::LogSeverity::LogSeverity severity )
    {
        Ogre::LogManager::getSingleton().logMessage( text );
    }

    void UIManagerColibri::ColibriListener::freeClipboardText( char *colibri_nullable text )
    {
        free( text );
    }

    bool UIManagerColibri::ColibriListener::getClipboardText(
        char *colibri_nonnull *const colibri_nullable outText )
    {
        //*outText = SDL_GetClipboardText();
        return *outText != nullptr;
    }

    void UIManagerColibri::ColibriListener::setClipboardText( const char *text )
    {
        //SDL_SetClipboardText( text );
    }
}  // namespace workphone::ui
