#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UIImageColibri.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UIManagerColibri.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTextureOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialOgreNext.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UICustomShapeColibri.hpp>
#include <WPGraphicsOgreNext/OgreUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <ColibriGui/ColibriCustomShape.h>
#include <ColibriGui/ColibriWindow.h>
#include <ColibriGui/ColibriLabel.h>
#include <ColibriGui/ColibriManager.h>
#include <ColibriGui/Ogre/OgreHlmsColibri.h>
#include <ColibriGui/Ogre/OgreHlmsColibriDatablock.h>
#include <OgreHlmsManager.h>
#include <OgreHlms.h>
#include <OgreRoot.h>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UIImageColibri, UIElementColibri<IUIImage> );

        UIImageColibri::UIImageColibri()
        {
            createStateContext();
        }

        UIImageColibri::~UIImageColibri()
        {
            unload( nullptr );
            destroyStateContext();
        }

        void UIImageColibri::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                ScopedLock lock( this );

                auto ui = applicationManager->getRenderUI();
                auto renderUI = workphone::static_pointer_cast<UIManagerColibri>( ui );

                auto window = renderUI->getLayoutWindow();
                auto colibriManager = renderUI->getColibriManager();
                //m_renderable = colibriManager->createWindow( window );
                m_renderable = colibriManager->createWidget<UICustomShapeColibri>( window );

                updateTiling();

                m_renderable->setTransform( Ogre::Vector2( 0, 0 ), Ogre::Vector2( 1920, 1080 ) );
                m_renderable->setSkin( "EmptyBg" );

                m_renderable->setKeyboardNavigable( false );

                createDatablock();
                updateMaterial();
                m_renderable->setDatablock( m_datablock );

                setWidget( m_renderable );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->setDirty( true );
                }

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UIImageColibri::updateTiling()
        {
            if( isLoaded() )
            {
                ScopedLock lock( this );

                auto quadTopLeft = Ogre::Vector2::UNIT_SCALE * -1.0f;
                auto quadSize = Ogre::Vector2::UNIT_SCALE * 2.0f;

                auto quadBottomRight = quadTopLeft + quadSize;
                auto quadSizeHalf = quadSize * 0.5f;
                auto quadTop = quadTopLeft.y;

                auto colour = Ogre::ColourValue::White;
                auto baseColour = Ogre::ColourValue::White * 0.1f;

                colour.a = 1.0f;
                baseColour.a = 1.0f;

                auto useTiling = getUseTiling();
                if( !useTiling )
                {
                    m_renderable->setNumTriangles( 2 );
                    m_renderable->setQuad( 0, quadTopLeft, quadSize, colour );
                }
                else
                {
                    auto numTiles = 9;
                    m_renderable->setNumTriangles( 2 * numTiles );

                    //auto totalWidth = maxScale.x - quadTopLeft.x;
                    //auto totalHeight = maxScale.y - quadTopLeft.y;

                    auto sliceWidth = quadSize.x / 3;
                    auto sliceHeight = quadSize.y / 3;

                    auto spriteSize = getSpriteSize();

                    auto borderLeft = getBorderLeft() / spriteSize.x;
                    auto borderRight = getBorderRight() / spriteSize.x;
                    auto borderTop = getBorderTop() / spriteSize.y;
                    auto borderBottom = getBorderBottom() / spriteSize.y;

                    auto min = quadTopLeft;
                    auto max = min;

                    auto uvWidth = 1.0f / 3.0f;
                    auto uvHeight = 1.0f / 3.0f;

                    auto uvStart = Ogre::Vector2::ZERO;
                    auto uvEnd =
                        uvStart + ( sliceWidth / spriteSize.x );  // Adjust UV based on slice width

                    for( int y = 0; y < 3; ++y )
                    {
                        for( int x = 0; x < 3; ++x )
                        {
                            auto i = x + ( y * 3 );

                            if( x == 0 )
                            {
                                min.x = quadTopLeft.x;
                            }

                            if( y == 0 )
                            {
                                min.y = quadTopLeft.y;
                            }

                            if( x == 0 && y == 0 )
                            {
                                max.x = quadTopLeft.x + borderLeft;
                                max.y = quadTopLeft.y + borderTop;
                            }
                            else if( x == 0 && y == 1 )
                            {
                                max.x = quadTopLeft.x + borderLeft;
                                max.y = quadBottomRight.y - borderBottom;
                            }
                            else if( x == 0 && y == 2 )
                            {
                                max.x = quadTopLeft.x + borderLeft;
                                max.y = quadBottomRight.y;
                            }
                            else if( x == 1 && y == 0 )
                            {
                                max.x = quadBottomRight.x - borderRight;
                                max.y = quadTopLeft.y + borderTop;
                            }
                            else if( x == 1 && y == 1 )
                            {
                                max.x = quadBottomRight.x - borderRight;
                                max.y = quadBottomRight.y - borderBottom;
                            }
                            else if( x == 1 && y == 2 )
                            {
                                max.x = quadBottomRight.x - borderRight;
                                max.y = quadBottomRight.y;
                            }
                            else if( x == 2 && y == 0 )
                            {
                                max.x = quadBottomRight.x;
                                max.y = quadTopLeft.y + borderTop;
                            }
                            else if( x == 2 && y == 1 )
                            {
                                max.x = quadBottomRight.x;
                                max.y = quadBottomRight.y - borderBottom;
                            }
                            else if( x == 2 && y == 2 )
                            {
                                max.x = quadBottomRight.x;
                                max.y = quadBottomRight.y;
                            }

                            auto size = max - min;

                            auto uvStart = Ogre::Vector2( x / 3.0f, y / 3.0f );
                            auto uvEnd = uvStart + ( 1.0f / 3.0f );
                            auto uvSize = uvEnd - uvStart;

                            //m_renderable->setQuad( ( i * 3 ) * 2, min, size, colour );
                            m_renderable->setQuad( ( i * 3 ) * 2, min, size, colour, uvStart, uvSize );

                            min.x = max.x;
                        }

                        min.y = max.y;
                    }
                }
            }
        }

        void UIImageColibri::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                if( m_renderable )
                {
                    try
                    {
                        auto ui = workphone::static_pointer_cast<UIManagerColibri>(
                            applicationManager->getRenderUI() );
                        auto colibriManager = ui->getColibriManager();
                        if( colibriManager )
                        {
                            colibriManager->destroyWidget( m_renderable );
                        }

                        m_renderable = nullptr;
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }

                    setWidget( nullptr );
                }

                UIElementColibri<UIImage>::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UIImageColibri::createDatablock()
        {
            ScopedLock lock( this );

            if( !m_datablock )
            {
                auto root = Ogre::Root::getSingletonPtr();

                auto hlmsManager = root->getHlmsManager();
                auto hlms = hlmsManager->getHlms( Ogre::HLMS_USER0 );
                COLIBRI_ASSERT_HIGH( dynamic_cast<Ogre::HlmsColibri *>( hlms ) );
                auto hlmsColibri = dynamic_cast<Ogre::HlmsColibri *>( hlms );

                Ogre::HlmsMacroblock macroblock;
                Ogre::HlmsBlendblock blendblock;

                macroblock.mDepthCheck = false;
                macroblock.mDepthWrite = false;
                //macroblock.mDepthFunc = Ogre::CompareFunction::CMPF_ALWAYS_PASS;

                blendblock.setBlendType( Ogre::SBT_TRANSPARENT_ALPHA );
                blendblock.mIsTransparent = 3;

                m_datablockName = StringUtil::getUUID();
                auto datablock =
                    hlmsColibri->createDatablock( m_datablockName.c_str(), m_datablockName.c_str(),
                                                  macroblock, blendblock, Ogre::HlmsParamVec() );

                m_datablock = static_cast<Ogre::HlmsColibriDatablock *>( datablock );

                m_datablock->setUseColour( true );
                //m_datablock->setColour( Ogre::ColourValue::White );
                m_datablock->setColour( Ogre::ColourValue::Red );
            }
        }

        bool UIImageColibri::handleStateChanged( SmartPtr<IState> &state )
        {
            if( isLoaded() )
            {
                ScopedLock lock( this );

                auto stateData = state->getData();
                if( stateData->isDerived<UIImageStateData>() )
                {
                    auto imageStateData = workphone::static_pointer_cast<UIImageStateData>( stateData );

                    if( auto widget = getWidget() )
                    {
                        //widget->setTransform( Ogre::Vector2( 0, 0 ), Ogre::Vector2( 50, 50 ) );
                    }

                    auto applicationManager = core::IApplicationManager::instance();
                    auto resourceDatabase = applicationManager->getResourceDatabase();

                    if( auto texture = getTexture() )
                    {
                        auto director = resourceDatabase->loadDirector( texture );
                        auto textureDirector =
                            dynamic_pointer_cast<scene::TextureResourceDirector>( director );
                        if( textureDirector )
                        {
                            auto useTiling = (f32)textureDirector->getUseTiling();
                            auto borderLeft = (f32)textureDirector->getBorderLeft();
                            auto borderRight = (f32)textureDirector->getBorderRight();
                            auto borderTop = (f32)textureDirector->getBorderTop();
                            auto borderBottom = (f32)textureDirector->getBorderBottom();
                        }
                    }

                    if( auto texture = getTexture() )
                    {
                        auto size = texture->getSize();
                        setSpriteSize( size );
                    }

                    updateTiling();
                    createDatablock();
                    updateMaterial();

                    if( m_renderable )
                    {
                        auto datablock = m_renderable->getDatablock();
                        if( datablock != m_datablock )
                        {
                            m_renderable->setDatablock( m_datablock );
                        }
                    }

                    if( auto widget = getWidget() )
                    {
                        widget->updateDerivedTransformFromParent( false );
                    }

                    return true;
                }

                return UIElementColibri<UIImage>::handleStateChanged( state );
            }

            return false;
        }

        SmartPtr<Properties> UIImageColibri::getProperties() const
        {
            auto texture = getTexture();

            auto spriteSize = getSpriteSize();
            auto useTiling = getUseTiling();
            auto borderLeft = getBorderLeft();
            auto borderRight = getBorderRight();
            auto borderTop = getBorderTop();
            auto borderBottom = getBorderBottom();

            auto properties = UIElementColibri<UIImage>::getProperties();
            properties->setProperty( borderLeftStr, borderLeft );
            properties->setProperty( borderRightStr, borderRight );
            properties->setProperty( borderTopStr, borderTop );
            properties->setProperty( borderBottomStr, borderBottom );
            properties->setProperty( useTilingStr, useTiling );
            properties->setProperty( spriteSizeStr, spriteSize );
            //properties->setProperty( materialStr, m_material );
            properties->setProperty( textureStr, texture );

            return properties;
        }

        void UIImageColibri::setProperties( SmartPtr<Properties> properties )
        {
            auto spriteSize = getSpriteSize();
            auto useTiling = getUseTiling();
            auto borderLeft = getBorderLeft();
            auto borderRight = getBorderRight();
            auto borderTop = getBorderTop();
            auto borderBottom = getBorderBottom();

            UIElementColibri<UIImage>::setProperties( properties );

            auto texture = SmartPtr<render::ITexture>();

            properties->getPropertyValue( borderLeftStr, borderLeft );
            properties->getPropertyValue( borderRightStr, borderRight );
            properties->getPropertyValue( borderTopStr, borderTop );
            properties->getPropertyValue( borderBottomStr, borderBottom );
            properties->getPropertyValue( useTilingStr, useTiling );
            properties->getPropertyValue( spriteSizeStr, spriteSize );
            //properties->getPropertyValue( materialStr, m_material );
            properties->getPropertyValue( textureStr, texture );

            setUseTiling( useTiling );

            updateMaterial();

            if( m_renderable )
            {
                if( m_renderable->getDatablock() != m_datablock )
                {
                    m_renderable->setDatablock( m_datablock );
                }
            }
        }

        Ogre::HlmsColibriDatablock *UIImageColibri::getDatablock() const
        {
            return m_datablock;
        }

        void UIImageColibri::setDatablock( Ogre::HlmsColibriDatablock *datablock )
        {
            m_datablock = datablock;
        }

        String UIImageColibri::getDatablockName() const
        {
            return m_datablockName;
        }

        void UIImageColibri::setDatablockName( const String &datablockName )
        {
            m_datablockName = datablockName;
        }

        void UIImageColibri::createStateContext()
        {
            WP_ASSERT( getStateContext() == nullptr );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();
            WP_ASSERT( stateManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto stateTask = graphicsSystem->getStateTask();

            auto stateContext = stateManager->addStateContext();
            stateContext->setOwner( this );
            setStateContext( stateContext );
            stateContext->setTaskId( stateTask );

            auto listener = factoryManager->make_ptr<ElementStateListener>();
            listener->setOwner( this );
            stateContext->addStateListener( listener );
            setStateListener( listener );

            auto state = factoryManager->make_ptr<State>();
            state->setId( getId() );
            state->setOwner( this );
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<UIElementStateData>();
            state->setData( stateData );

            auto transfromState = factoryManager->make_ptr<State>();
            transfromState->setId( getId() );
            transfromState->setOwner( this );
            stateContext->addState( transfromState );

            auto transformStateData = factoryManager->make_ptr<UITransformStateData>();
            transfromState->setData( transformStateData );

            auto imageState = factoryManager->make_ptr<State>();
            imageState->setId( getId() );
            imageState->setOwner( this );
            stateContext->addState( imageState );

            auto imageStateData = factoryManager->make_ptr<UIImageStateData>();
            imageState->setData( imageStateData );
        }

        void UIImageColibri::updateMaterial()
        {
            ScopedLock lock( this );

            if( m_datablock )
            {
                auto colour = getColour();
                auto colourValue = render::OgreUtil::convertToOgre( colour );
                m_datablock->setColour( colourValue );

                auto tex = getTexture();
                auto ogreTexture = workphone::static_pointer_cast<render::CTextureOgreNext>( tex );
                if( ogreTexture )
                {
                    Ogre::HlmsSamplerblock hlmsSamplerblock;
                    hlmsSamplerblock.mU = Ogre::TAM_WRAP;
                    hlmsSamplerblock.mV = Ogre::TAM_WRAP;
                    hlmsSamplerblock.mU = Ogre::TAM_WRAP;

                    if( !ogreTexture->isLoaded() )
                    {
                        ogreTexture->load( nullptr );
                    }

                    auto pOgreTexture = ogreTexture->getTexture();
                    m_datablock->setTexture( 0, pOgreTexture, &hlmsSamplerblock );
                }
            }
        }

    }  // namespace ui
}  // namespace workphone
