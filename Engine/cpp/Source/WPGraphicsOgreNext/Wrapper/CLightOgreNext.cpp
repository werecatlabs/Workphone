#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CLightOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSceneOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/GraphicsObjectListenerOgreNext.hpp>
#include <WPGraphicsOgreNext/WPGraphicsOgreNextTypes.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreLight.h>
#include <Ogre.h>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CLightOgreNext,
                               CGraphicsObjectOgreNext<GraphicsLight> );

    u32 CLightOgreNext::m_nameExt = 0;

    CLightOgreNext::CLightOgreNext()
    {
        auto name = lightStr + StringUtil::toString( m_nameExt++ );
        setName( name );

        auto id = StringUtil::getHash( name );
        setId( id );

        setupStateContext();
    }

    CLightOgreNext::CLightOgreNext( SmartPtr<IGraphicsScene> creator )
    {
        setCreator( creator );

        auto name = lightStr + StringUtil::toString( m_nameExt++ );
        setName( name );

        auto id = StringUtil::getHash( name );
        setId( id );

        setupStateContext();
    }

    CLightOgreNext::~CLightOgreNext()
    {
        unload( nullptr );
    }

    void CLightOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            Ogre::SceneManager *smgr = nullptr;

            if( auto creator = getCreator() )
            {
                creator->_getObject( reinterpret_cast<void **>( &smgr ) );
            }

            if( !smgr )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            if( smgr )
            {
                ScopedLock lock( this );

                auto light = smgr->createLight();
                light->setCastShadows( false );
                light->setPowerScale( 1.0f );
                light->setType( Ogre::Light::LT_DIRECTIONAL );
                m_light = light;

                auto lightNode = smgr->getRootSceneNode()->createChildSceneNode();
                lightNode->attachObject( light );
                setLightNode( lightNode );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CLightOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            ScopedLock lock( this );

            Ogre::SceneManager *smgr = nullptr;
            if( auto creator = getCreator() )
            {
                creator->_getObject( reinterpret_cast<void **>( &smgr ) );
            }

            if( auto owner = getOwner() )
            {
                owner->detachObject( this );
                setOwner( nullptr );
            }

            auto lightNode = getLightNode();
            if( lightNode )
            {
                lightNode->detachObject( m_light );

                auto lightNodeParent = lightNode->getParentSceneNode();
                if( lightNodeParent )
                {
                    lightNodeParent->removeAndDestroyChild( lightNode );
                }
                else if( smgr )
                {
                    smgr->destroySceneNode( lightNode );
                }

                setLightNode( nullptr );
            }

            if( m_light )
            {
                if( smgr )
                {
                    smgr->destroyLight( m_light );
                }

                m_light = nullptr;
            }

            GraphicsLight::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<IGraphicsObject> CLightOgreNext::clone( const String &name ) const
    {
        auto light = workphone::make_ptr<CLightOgreNext>();

        auto properties = getProperties();
        light->setProperties( properties );

        return light;
    }

    void CLightOgreNext::_getObject( void **ppObject ) const
    {
        *ppObject = m_light;
    }

    Ogre::Light *CLightOgreNext::getLight() const
    {
        return m_light;
    }

    SmartPtr<Properties> CLightOgreNext::getProperties() const
    {
        auto properties = GraphicsLight::getProperties();
        return properties;
    }

    void CLightOgreNext::setProperties( SmartPtr<Properties> properties )
    {
        GraphicsLight::setProperties( properties );
    }

    bool CLightOgreNext::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( CGraphicsObjectOgreNext<GraphicsLight>::handleStateMessage( message ) )
        {
            return true;
        }

        if( message->getSender() == this )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto renderTask = graphicsSystem->getRenderTask();
            auto currentTaskId = Thread::getCurrentTask();
            WP_ASSERT( currentTaskId == renderTask );

            if( currentTaskId == renderTask )
            {
                if( message->isExactly<StateMessageVisible>() )
                {
                    auto visibleMessage = workphone::static_pointer_cast<StateMessageVisible>( message );
                    WP_ASSERT( visibleMessage );

                    auto visible = visibleMessage->isVisible();
                    this->setVisible( visible );

                    return true;
                }
                else if( message->isExactly<StateMessageUIntValue>() )
                {
                    auto valueMessage = workphone::static_pointer_cast<StateMessageUIntValue>( message );
                    auto type = valueMessage->getType();
                    auto value = valueMessage->getValue();

                    if( type == VISIBILITY_MASK_HASH )
                    {
                        this->setVisibilityFlags( value );
                        return true;
                    }
                }
                else if( message->isExactly<StateMessageIntValue>() )
                {
                    auto valueMessage = workphone::static_pointer_cast<StateMessageIntValue>( message );
                    auto type = valueMessage->getType();
                    auto value = valueMessage->getValue();

                    if( type == LIGHT_TYPE_HASH )
                    {
                        this->setType( static_cast<LightTypes>( value ) );
                        return true;
                    }
                }
                else if( message->isExactly<StateMessageVector3>() )
                {
                    auto valueMessage = workphone::static_pointer_cast<StateMessageVector3>( message );
                    auto type = valueMessage->getType();
                    auto value = valueMessage->getValue();

                    if( type == GraphicsOgreNextTypes::STATE_MESSAGE_DIRECTION )
                    {
                        this->setDirection( value );
                        return true;
                    }
                }
                else if( message->isExactly<StateMessageVector4>() )
                {
                    auto valueMessage = workphone::static_pointer_cast<StateMessageVector4>( message );
                    auto type = valueMessage->getType();
                    auto value = valueMessage->getValue();

                    if( type == DIFFUSE_COLOUR_HASH )
                    {
                        this->setDiffuseColour( ColourF( value.X(), value.Y(), value.Z(), value.W() ) );
                        return true;
                    }
                }
            }
        }

        return false;
    }

    bool CLightOgreNext::handleStateChanged( SmartPtr<IState> &state )
    {
        if( isLoaded() )
        {
            if( CGraphicsObjectOgreNext<GraphicsLight>::handleStateChanged( state ) )
            {
                return true;
            }

            if( state->getOwnerPtr() == this )
            {
                if( auto stateData = state->getData() )
                {
                    if( stateData->isDerived<GraphicsObjectData>() )
                    {
                        auto graphicsObjectData = state->getDataByType<GraphicsObjectData>();

                        if( auto light = this->getLight() )
                        {
                            auto visibilityMask = graphicsObjectData->visibilityMask;

                            if( light->isAttached() )
                            {
                                auto visible = ( graphicsObjectData->flags & visibleFlag ) != 0;
                                light->setVisible( visible );
                            }

                            auto castShadows = ( graphicsObjectData->flags & castShadowsFlag ) != 0;
                            light->setCastShadows( castShadows );

                            return true;
                        }
                    }
                    else if( stateData->isDerived<LightStateData>() )
                    {
                        auto lightData = state->getDataByType<LightStateData>();

                        auto diffuse = lightData->diffuseColour;
                        auto specular = lightData->specularColour;
                        auto direction = lightData->direction;

                        if( auto light = this->getLight() )
                        {
                            if( light->isAttached() )
                            {
                                auto lightType =
                                    static_cast<Ogre::Light::LightTypes>( lightData->lightType );
                                light->setType( lightType );

                                //auto visibilityMask = lightData->visibilityMask;
                                //visibilityMask = light->getVisibilityFlags();
                                //light->setVisibilityFlags( lightData->visibilityMask );

                                //light->setAttenuation( lightData->range, lightData->constant, lightData->linear,
                                //                       lightData->quadratic );

                                //light->setRenderQueueGroup( lightData->renderQueueGroup );

                                light->setDiffuseColour(
                                    Ogre::ColourValue( diffuse.r, diffuse.g, diffuse.b, diffuse.a ) );
                                light->setSpecularColour( Ogre::ColourValue( specular.r, specular.g,
                                                                             specular.b, specular.a ) );
                                light->setDirection(
                                    Ogre::Vector3( direction.X(), direction.Y(), direction.Z() ) );

                                return true;
                            }
                        }
                    }
                    else if( stateData->isDerived<LightAttenuationStateData>() )
                    {
                        auto lightAttenuationData = state->getDataByType<LightAttenuationStateData>();
                        if( auto light = this->getLight() )
                        {
                            if( light->isAttached() )
                            {
                                light->setAttenuation(
                                    lightAttenuationData->range, lightAttenuationData->constant,
                                    lightAttenuationData->linear, lightAttenuationData->quadratic );

                                light->setPowerScale( lightAttenuationData->powerScale );

                                return true;
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    Ogre::SceneNode *CLightOgreNext::getLightNode() const
    {
        return m_lightNode.load();
    }

    void CLightOgreNext::setLightNode( Ogre::SceneNode *node )
    {
        m_lightNode = node;
    }

    Vector3<real_Num> CLightOgreNext::getDerivedDirection() const
    {
        ScopedLock lock( this );

        if( auto light = getLight() )
        {
            if( light->isAttached() )
            {
                auto derivedDirection = light->getDerivedDirection();
                return Vector3<real_Num>( derivedDirection.x, derivedDirection.y, derivedDirection.z );
            }
        }

        return Vector3<real_Num>::zero();
    }

    void CLightOgreNext::setupStateContext()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManagerPtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        auto factoryManager = applicationManager->getFactoryManagerPtr();

        auto sender = getCreator();

        auto stateContext = sender->getGraphicsObjectContextPtr( getTypeInfo() );
        WP_ASSERT( stateContext );
        setStateContext( stateContext );

        auto state = factoryManager->make_ptr<State>();
        state->setId( getId() );
        state->setOwner( this );
        stateContext->addState( state );

        auto lightData = factoryManager->make_ptr<LightStateData>();
        state->setData( lightData );

        auto lightAttenuationState = factoryManager->make_ptr<State>();
        lightAttenuationState->setId( getId() );
        lightAttenuationState->setOwner( this );
        stateContext->addState( lightAttenuationState );

        auto lightAttenuationData = factoryManager->make_ptr<LightAttenuationStateData>();
        lightAttenuationState->setData( lightAttenuationData );

        auto graphicsObjectState = factoryManager->make_ptr<State>();
        graphicsObjectState->setId( getId() );
        graphicsObjectState->setOwner( this );
        stateContext->addState( graphicsObjectState );

        auto graphicsObjectStateData = factoryManager->make_ptr<GraphicsObjectData>();
        graphicsObjectState->setData( graphicsObjectStateData );
    }

}  // namespace workphone::render
