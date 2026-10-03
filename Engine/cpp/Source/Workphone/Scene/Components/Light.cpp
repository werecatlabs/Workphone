#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Light.hpp>
#include <Workphone/Scene/Transform.hpp>
#include <Workphone/Interface/Graphics/IDebug.hpp>
#include <Workphone/Interface/Graphics/IDebugLine.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Light, Component );
    const hash_type Light::lightHash = StringUtil::getHash( "Light" );

    const String Light::diffuseColourStr = "diffuseColour";
    const String Light::specularColourStr = "specularColour";
    const String Light::rangeStr = "range";
    const String Light::constantStr = "constant";
    const String Light::linearStr = "linear";
    const String Light::quadraticStr = "quadratic";
    const String Light::intensityStr = "intensity";
    const String Light::lightTypeStr = "lightType";
    const String Light::debugColourStr = "debugColour";

    const Array<String> Light::lightTypeEnum =
        Array<String>( { "Directional", "Point", "Spot", "VPL", "Area Approx", "Area LTC" } );

    u32 Light::m_nameExt = 0;

    Light::Light() = default;

    Light::~Light()
    {
        unload( nullptr );
    }

    void Light::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            Component::load( data );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto sceneManager = applicationManager->getGameManager();

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            if( graphicsSystem )
            {
                auto smgr = graphicsSystem->getGraphicsScene();
                WP_ASSERT( smgr );

                auto name = String( "Light" ) + StringUtil::toString( m_nameExt++ );

                if( auto light = smgr->addGraphicsObjectByType<render::IGraphicsLight>() )
                {
                    light->setName( name );

                    light->setType( m_lightType );
                    light->setDiffuseColour( m_diffuseColour );
                    light->setSpecularColour( m_specularColour );
                    light->setPowerScale( m_intensity );
                    light->setAttenuation( m_range, m_constant, m_linear, m_quadratic );

                    setLight( light );

                    auto rootSceneNode = smgr->getRootSceneNode();
                    WP_ASSERT( rootSceneNode );

                    auto sceneNode = rootSceneNode->addChildSceneNode( name );
                    WP_ASSERT( sceneNode );

                    sceneNode->load( nullptr );

                    sceneNode->attachObject( light );
                    setSceneNode( sceneNode );
                }
            }

            sceneManager->registerComponentUpdate( TaskId::Render, Thread::UpdateState::Transform,
                                                   this );
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Light::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                if( graphicsSystem )
                {
                    auto smgr = graphicsSystem->getGraphicsScene();
                    WP_ASSERT( smgr );

                    if( auto light = getLight() )
                    {
                        if( auto sceneNode = light->getOwner() )
                        {
                            sceneNode->detachObject( light );
                            light->setOwner( nullptr );
                        }

                        smgr->removeGraphicsObject( light );
                        setLight( nullptr );
                    }

                    if( auto sceneNode = getSceneNode() )
                    {
                        smgr->removeSceneNode( sceneNode );
                        setSceneNode( nullptr );
                    }
                }

                Component::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Light::updateDebugDraw()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto debug = graphicsSystem->getDebug();

        if( auto actor = getActor() )
        {
            auto pos = actor->getPosition();
            debug->drawLine( lightHash, pos, pos + Vector3<real_Num>::unitX(), m_debugColour );
        }
    }

    void Light::updateFlags( u32 flags, u32 oldFlags )
    {
        if( auto actor = getActor() )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
            {
                auto smgr = graphicsSystem->getGraphicsScene();
                WP_ASSERT( smgr );

                auto rootNode = smgr->getRootSceneNode();

                if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagInScene ) !=
                    BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagInScene ) )
                {
                    auto visible = isEnabled() && actor->isEnabledInScene();

                    if( auto light = getLight() )
                    {
                        light->setVisible( visible );
                    }

                    if( auto sceneNode = getSceneNode() )
                    {
                        sceneNode->makeDirty();
                    }
                }
                else if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagEnabled ) !=
                         BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagEnabled ) )
                {
                    auto visible = isEnabled() && actor->isEnabledInScene();

                    if( auto light = getLight() )
                    {
                        light->setVisible( visible );
                    }

                    if( auto sceneNode = getSceneNode() )
                    {
                        sceneNode->makeDirty();
                    }
                }
            }
        }
    }

    Array<SmartPtr<ISharedObject>> Light::getChildObjects() const
    {
        auto objects = Component::getChildObjects();
        objects.push_back( getLight() );
        return objects;
    }

    auto Light::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = Component::getProperties();

        properties->setProperty( diffuseColourStr, m_diffuseColour );
        properties->setProperty( specularColourStr, m_specularColour );
        properties->setProperty( intensityStr, m_intensity );
        properties->setProperty( rangeStr, m_range );
        properties->setProperty( constantStr, m_constant );
        properties->setProperty( linearStr, m_linear );
        properties->setProperty( quadraticStr, m_quadratic );

        auto iLightType = static_cast<s32>( m_lightType );
        properties->setPropertyAsEnum( lightTypeStr, iLightType, lightTypeEnum );

        properties->setProperty( debugColourStr, m_debugColour );

        return properties;
    }

    void Light::setProperties( SmartPtr<Properties> properties )
    {
        Component::setProperties( properties );

        properties->getPropertyValue( diffuseColourStr, m_diffuseColour );
        properties->getPropertyValue( specularColourStr, m_specularColour );
        properties->getPropertyValue( intensityStr, m_intensity );
        properties->getPropertyValue( rangeStr, m_range );
        properties->getPropertyValue( constantStr, m_constant );
        properties->getPropertyValue( linearStr, m_linear );
        properties->getPropertyValue( quadraticStr, m_quadratic );

        u32 iLightType = static_cast<u32>( m_lightType );
        properties->getPropertyValue( lightTypeStr, iLightType );
        if( iLightType < static_cast<u32>( LightTypes::Count ) )
        {
            m_lightType = static_cast<LightTypes>( iLightType );
        }

        properties->getPropertyValue( debugColourStr, m_debugColour );

        if( auto light = getLight() )
        {
            auto actor = getActor();
            auto visible = isEnabled() && actor && actor->isEnabledInScene();

            light->setVisible( visible );
            light->setDiffuseColour( m_diffuseColour );
            light->setSpecularColour( m_specularColour );
            light->setPowerScale( m_intensity );
            light->setAttenuation( m_range, m_constant, m_linear, m_quadratic );
            light->setType( m_lightType );
        }

        if( auto sceneNode = getSceneNode() )
        {
            sceneNode->makeDirty();
        }
    }

    void Light::updateTransform()
    {
        if( auto actor = getActor() )
        {
            if( auto actorTransform = actor->getTransform() )
            {
                auto t = actorTransform->getWorldTransform();

                if( auto sceneNode = getSceneNode() )
                {
                    sceneNode->setWorldTransform( t );
                }

                auto direction = t.getOrientation() * -Vector3<real_Num>::unitY();
                auto light = getLight();
                if( light )
                {
                    light->setDirection( direction );
                }

                if( auto sceneNode = getSceneNode() )
                {
                    sceneNode->makeDirty();
                }
            }

            if( auto sceneNode = getSceneNode() )
            {
                sceneNode->makeDirty();
            }
        }
    }

    void Light::updateTransform( const Transform3<real_Num> &transform )
    {
        if( auto &sceneNode = getSceneNode() )
        {
            auto &p = transform.getPosition();
            auto &r = transform.getOrientation();
            auto &s = transform.getScale();

            sceneNode->setPosition( p );
            sceneNode->setOrientation( r );
            sceneNode->setScale( s );

            auto direction = r * -Vector3<real_Num>::unitY();
            auto light = getLight();
            if( light )
            {
                light->setDirection( direction );
            }
        }

        if( auto sceneNode = getSceneNode() )
        {
            sceneNode->makeDirty();
        }
    }

    auto Light::getLightType() const -> LightTypes
    {
        return m_lightType;
    }

    void Light::setLightType( LightTypes lightType )
    {
        m_lightType = lightType;

        if( m_light )
        {
            m_light->setType( m_lightType );
        }
    }

    void Light::setAttenuation( f32 range, f32 constant, f32 linear, f32 quadratic )
    {
        m_range = range;
        m_constant = constant;
        m_linear = linear;
        m_quadratic = quadratic;

        if( auto light = getLight() )
        {
            light->setAttenuation( m_range, m_constant, m_linear, m_quadratic );
        }
    }

    auto Light::getDiffuseColour() const -> ColourF
    {
        return m_diffuseColour;
    }

    void Light::setDiffuseColour( const ColourF &diffuseColour )
    {
        m_diffuseColour = diffuseColour;

        if( auto light = getLight() )
        {
            light->setDiffuseColour( m_diffuseColour );
        }
    }

    auto Light::getSpecularColour() const -> ColourF
    {
        return m_specularColour;
    }

    void Light::setSpecularColour( const ColourF &specularColour )
    {
        m_specularColour = specularColour;

        if( auto light = getLight() )
        {
            light->setSpecularColour( m_specularColour );
        }
    }

    auto Light::getLight() const -> SmartPtr<render::IGraphicsLight>
    {
        return m_light;
    }

    void Light::setLight( SmartPtr<render::IGraphicsLight> light )
    {
        m_light = light;
    }

    SmartPtr<render::IGraphicsSceneNode> &Light::getSceneNode()
    {
        return m_sceneNode;
    }

    const SmartPtr<render::IGraphicsSceneNode> &Light::getSceneNode() const
    {
        return m_sceneNode;
    }

    void Light::setSceneNode( SmartPtr<render::IGraphicsSceneNode> sceneNode )
    {
        m_sceneNode = sceneNode;
    }

    auto Light::handleComponentEvent( u32 state, FSMEvent eventType ) -> FSMReturnType
    {
        Component::handleComponentEvent( state, eventType );

        switch( eventType )
        {
        case FSMEvent::Change:
            break;
        case FSMEvent::Enter:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Destroyed:
            {
            }
            break;
            case State::Edit:
            case State::Play:
            {
                if( auto sceneNode = getSceneNode() )
                {
                    sceneNode->makeDirty();
                }
            }
            break;
            default:
            {
            }
            }
        }
        break;
        case FSMEvent::Leave:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Destroyed:
            {
            }
            break;
            case State::Edit:
            case State::Play:
            {
                if( auto sceneNode = getSceneNode() )
                {
                    sceneNode->makeDirty();
                }
            }
            break;
            default:
            {
            }
            }
        }
        break;
        case FSMEvent::Pending:
            break;
        case FSMEvent::Complete:
            break;
        case FSMEvent::NewState:
            break;
        case FSMEvent::WaitForChange:
            break;
        default:
        {
        }
        break;
        }

        return FSMReturnType::Ok;
    }

    auto Light::handleEvent( EventType eventType, hash_type eventValue,
                             const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                             SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) -> Parameter
    {
        Component::handleEvent( eventType, eventValue, arguments, sender, object, event );

        if( eventValue == IEvent::loadScene || eventValue == IEvent::cameraManagerReset ||
            eventValue == IEvent::sceneChanged )
        {
            if( auto sceneNode = getSceneNode() )
            {
                sceneNode->makeDirty();
            }
        }

        return {};
    }

    void Light::setIntensity( f32 intensity )
    {
        m_intensity = intensity;

        if( auto light = getLight() )
        {
            light->setPowerScale( m_intensity );
        }
    }

    workphone::f32 Light::getIntensity() const
    {
        return m_intensity;
    }

    void Light::setAttenuationQuadratic( real_Num quadratic )
    {
        m_quadratic = quadratic;

        if( auto light = getLight() )
        {
            light->setAttenuation( m_range, m_constant, m_linear, m_quadratic );
        }
    }

    void Light::setAttenuationLinear( real_Num linear )
    {
        m_linear = linear;

        if( auto light = getLight() )
        {
            light->setAttenuation( m_range, m_constant, m_linear, m_quadratic );
        }
    }

    void Light::setAttenuationConstant( real_Num constant )
    {
        m_constant = constant;

        if( auto light = getLight() )
        {
            light->setAttenuation( m_range, m_constant, m_linear, m_quadratic );
        }
    }

    void Light::setAttenuationRange( real_Num range )
    {
        m_range = range;

        if( auto light = getLight() )
        {
            light->setAttenuation( m_range, m_constant, m_linear, m_quadratic );
        }
    }

    workphone::real_Num Light::getAttenuationQuadratic() const
    {
        return m_quadratic;
    }

    workphone::real_Num Light::getAttenuationLinear() const
    {
        return m_linear;
    }

    workphone::real_Num Light::getAttenuationConstant() const
    {
        return m_constant;
    }

    workphone::real_Num Light::getAttenuationRange() const
    {
        return m_range;
    }

    bool Light::isVisible() const
    {
        if( m_light )
        {
            return m_light->isVisible();
        }

        return false;
    }

    void Light::setVisible( bool visible )
    {
        if( m_light )
        {
            m_light->setVisible( visible );
        }
    }

    u32 Light::getDebugColour() const
    {
        return m_debugColour;
    }

    void Light::setDebugColour( u32 colour )
    {
        m_debugColour = colour;
    }

}  // namespace workphone::scene
