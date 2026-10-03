#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Skybox.hpp>
#include <Workphone/Scene/Components/Material.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Graphics/ISkybox.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Skybox, Component );
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Skybox::MaterialSharedListener, IEventListener );

    const String Skybox::frontPropertyKey = "Front";
    const String Skybox::backPropertyKey = "Back";
    const String Skybox::upPropertyKey = "Up";
    const String Skybox::downPropertyKey = "Down";
    const String Skybox::rightPropertyKey = "Right";
    const String Skybox::leftPropertyKey = "Left";
    const String Skybox::swapLeftRightPropertyKey = "swapLeftRight";
    const String Skybox::swapUpDownPropertyKey = "swapUpDown";
    const String Skybox::swapFrontBackPropertyKey = "swapFrontBack";

    static const String defaultSkyboxTextureName = "checker.png";

    Skybox::Skybox()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto factoryManager = applicationManager->getFactoryManager();

        m_materialSharedListener = factoryManager->make_ptr<MaterialSharedListener>();
        m_materialSharedListener->setOwner( this );
    }

    Skybox::~Skybox() = default;

    void Skybox::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( getLoadingState() == LoadingState::Loaded )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );

            if( !m_materialSharedListener )
            {
                m_materialSharedListener = workphone::make_ptr<MaterialSharedListener>();
                m_materialSharedListener->setOwner( this );
            }

            Component::load( data );

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                WP_LOG_ERROR( "ApplicationManager is null during Skybox::load" );
                setLoadingState( LoadingState::Loaded );
                return;
            }

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( graphicsSystem )
            {
                auto smgr = graphicsSystem->getGraphicsScenePtr();
                if( !smgr )
                {
                    WP_LOG_ERROR( "GraphicsScene is null during Skybox::load" );
                }
                else
                {
                    setupMaterial();

                    if( m_skybox = smgr->addGraphicsObjectByType<render::ISkybox>() )
                    {
                        syncRenderSkybox( this );

                        if( auto actor = getActorPtr() )
                        {
                            auto visible = isEnabled() && actor->isEnabledInScene();
                            m_skybox->setVisible( visible );
                        }
                    }
                    else
                    {
                        WP_LOG_ERROR( "Failed to create ISkybox object in graphics scene" );
                    }
                }
            }
            else
            {
                WP_LOG_ERROR( "GraphicsSystem is null during Skybox::load" );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Loaded );
        }
    }

    void Skybox::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto loadingState = getLoadingState();
            if( loadingState == LoadingState::Unloaded )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( applicationManager )
            {
                auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
                if( graphicsSystem )
                {
                    if( m_skybox )
                    {
                        m_skybox->setVisible( false );

                        auto smgr = graphicsSystem->getGraphicsScenePtr();
                        if( smgr )
                        {
                            smgr->removeGraphicsObject( m_skybox );
                        }
                        else
                        {
                            WP_LOG_ERROR( "GraphicsScene is null during Skybox::unload" );
                        }

                        m_skybox = nullptr;
                    }
                }
            }

            if( m_materialSharedListener )
            {
                if( m_material )
                {
                    m_material->removeObjectListener( m_materialSharedListener.get() );
                }
                m_materialSharedListener = nullptr;
            }

            for( auto &t : m_textures )
            {
                t = nullptr;
            }

            m_material = nullptr;

            Component::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Unloaded );
        }
    }

    void Skybox::updateFlags( u32 flags, u32 oldFlags )
    {
        auto actor = getActorPtr();
        if( !actor )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            WP_LOG_ERROR( "ApplicationManager is null during Skybox::updateFlags" );
            return;
        }

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        if( !graphicsSystem )
        {
            return;
        }

        auto smgr = graphicsSystem->getGraphicsScenePtr();
        if( !smgr )
        {
            WP_LOG_ERROR( "GraphicsScene is null during Skybox::updateFlags" );
            return;
        }

        bool needsMaterialUpdate = false;
        bool needsVisibilityUpdate = false;

        if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagInScene ) !=
            BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagInScene ) )
        {
            needsVisibilityUpdate = true;
        }

        if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagEnabled ) !=
            BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagEnabled ) )
        {
            needsMaterialUpdate = true;
            needsVisibilityUpdate = true;
        }

        if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagDirty ) !=
            BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagDirty ) )
        {
            needsMaterialUpdate = true;
            needsVisibilityUpdate = true;
        }

        if( !needsMaterialUpdate && !needsVisibilityUpdate )
        {
            return;
        }

        switch( auto state = actor->getState() )
        {
        case IGameActor::State::Play:
        case IGameActor::State::Edit:
        {
            if( needsMaterialUpdate )
            {
                setupMaterial();
                syncRenderSkybox( this );
            }

            if( needsVisibilityUpdate && m_skybox )
            {
                auto visible = isEnabled() && actor->isEnabledInScene();
                m_skybox->setVisible( visible );
            }
        }
        break;
        default:
            break;
        }
    }

    auto Skybox::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = Component::getProperties();

        properties->setProperty( frontPropertyKey,
                                 m_textures[static_cast<u32>( SkyboxTextureTypes::Front )] );
        properties->setProperty( backPropertyKey,
                                 m_textures[static_cast<u32>( SkyboxTextureTypes::Back )] );
        properties->setProperty( upPropertyKey, m_textures[static_cast<u32>( SkyboxTextureTypes::Up )] );
        properties->setProperty( downPropertyKey,
                                 m_textures[static_cast<u32>( SkyboxTextureTypes::Down )] );
        properties->setProperty( rightPropertyKey,
                                 m_textures[static_cast<u32>( SkyboxTextureTypes::Right )] );
        properties->setProperty( leftPropertyKey,
                                 m_textures[static_cast<u32>( SkyboxTextureTypes::Left )] );

        properties->setProperty( swapLeftRightPropertyKey, m_swapLeftRight );
        properties->setProperty( swapUpDownPropertyKey, m_swapUpDown );
        properties->setProperty( swapFrontBackPropertyKey, getSwapFrontBack() );

        return properties;
    }

    void Skybox::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        auto actor = getActorPtr();
        if( !actor )
        {
            return;
        }

        auto wasEnabled = isEnabled() && actor->isEnabledInScene();

        Component::setProperties( properties );

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            WP_LOG_ERROR( "ApplicationManager is null during Skybox::setProperties" );
            return;
        }

        Array<SmartPtr<render::ITexture>> textures;
        textures.resize( 6 );

        properties->getPropertyValue( frontPropertyKey,
                                      textures[static_cast<u32>( SkyboxTextureTypes::Front )] );

        SmartPtr<render::ITexture> backTexture;
        properties->getPropertyValue( backPropertyKey,
                                      textures[static_cast<u32>( SkyboxTextureTypes::Back )] );
        properties->getPropertyValue( upPropertyKey,
                                      textures[static_cast<u32>( SkyboxTextureTypes::Up )] );
        properties->getPropertyValue( downPropertyKey,
                                      textures[static_cast<u32>( SkyboxTextureTypes::Down )] );
        properties->getPropertyValue( rightPropertyKey,
                                      textures[static_cast<u32>( SkyboxTextureTypes::Right )] );
        properties->getPropertyValue( leftPropertyKey,
                                      textures[static_cast<u32>( SkyboxTextureTypes::Left )] );

        auto swapLeftRight = getSwapLeftRight();
        auto swapUpDown = getSwapUpDown();
        auto swapFrontBack = getSwapFrontBack();

        properties->getPropertyValue( swapLeftRightPropertyKey, swapLeftRight );
        properties->getPropertyValue( swapUpDownPropertyKey, swapUpDown );
        properties->getPropertyValue( swapFrontBackPropertyKey, swapFrontBack );

        auto dirty = false;

        if( swapLeftRight != getSwapLeftRight() )
        {
            dirty = true;
            setSwapLeftRight( swapLeftRight );
        }

        if( swapUpDown != getSwapUpDown() )
        {
            dirty = true;
            setSwapUpDown( swapUpDown );
        }

        if( swapFrontBack != getSwapFrontBack() )
        {
            dirty = true;
            setSwapFrontBack( swapFrontBack );
        }

        for( size_t i = 0; i < textures.size(); ++i )
        {
            if( m_textures[i] != textures[i] )
            {
                m_textures[i] = textures[i];
                dirty = true;
            }
        }

        auto enabled = isEnabled() && actor->isEnabledInScene();
        if( !dirty )
        {
            dirty = enabled != wasEnabled;
        }

        if( dirty )
        {
            setupMaterial();
            syncRenderSkybox( this );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            if( graphicsSystem )
            {
                auto smgr = graphicsSystem->getGraphicsScene();
                if( smgr )
                {
                    auto distance = getDistance();
                    if( auto skybox = getSkyboxPtr() )
                    {
                        skybox->setDistance( distance );
                    }
                }
                else
                {
                    WP_LOG_ERROR( "GraphicsScene is null during Skybox::setProperties" );
                }
            }
            else
            {
                WP_LOG_ERROR( "GraphicsSystem is null during Skybox::setProperties" );
            }
        }

        if( auto skybox = getSkyboxPtr() )
        {
            skybox->setVisible( enabled );
        }
    }

    auto Skybox::getMaterial() const -> SmartPtr<render::IMaterial>
    {
        return m_material;
    }

    void Skybox::updateMaterials()
    {
        try
        {
            if( auto actor = getActor() )
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                if( graphicsSystem )
                {
                    auto smgr = graphicsSystem->getGraphicsScene();
                    WP_ASSERT( smgr );

                    auto visible = actor->isVisible();
                    auto skyboxMaterial = getMaterial();
                    auto distance = getDistance();

                    if( auto skybox = getSkyboxPtr() )
                    {
                        skybox->setMaterial( skyboxMaterial );
                        skybox->setVisible( visible );
                        skybox->setDistance( distance );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto Skybox::getSwapLeftRight() const -> bool
    {
        return m_swapLeftRight;
    }

    void Skybox::setSwapLeftRight( bool swapLeftRight )
    {
        m_swapLeftRight = swapLeftRight;
    }

    auto Skybox::getDistance() const -> f32
    {
        return m_distance;
    }

    void Skybox::setDistance( f32 distance )
    {
        m_distance = distance;
    }

    auto Skybox::getTextures() const -> Array<SmartPtr<render::ITexture>>
    {
        return { m_textures.begin(), m_textures.end() };
    }

    void Skybox::setTextures( const Array<SmartPtr<render::ITexture>> &textures )
    {
        for( size_t i = 0; i < textures.size(); ++i )
        {
            if( i < m_textures.size() )
            {
                m_textures[i] = textures[i];
            }
        }

        setupMaterial();
        syncRenderSkybox( this );
    }

    auto Skybox::getTexture( u8 index ) const -> SmartPtr<render::ITexture>
    {
        if( index < m_textures.size() )
        {
            return m_textures[index];
        }

        return nullptr;
    }

    void Skybox::setTexture( SmartPtr<render::ITexture> texture, u8 index )
    {
        if( texture )
        {
            if( index < m_textures.size() )
            {
                m_textures[index] = texture;
            }
        }

        setupMaterial();
        syncRenderSkybox( this );
    }

    void Skybox::setTextureByName( const String &textureName, u8 index )
    {
        if( index < m_textures.size() )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto resourceDatabase = applicationManager->getResourceDatabasePtr();

            auto texture = resourceDatabase->loadResourceByType<render::ITexture>( textureName );
            if( texture )
            {
                m_textures[index] = texture;
            }

            setupMaterial();
            syncRenderSkybox( this );
        }
    }

    auto Skybox::handleComponentEvent( u32 state, FSMEvent eventType ) -> FSMReturnType
    {
        switch( eventType )
        {
        case FSMEvent::Change:
        {
        }
        break;
        case FSMEvent::Enter:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
                if( auto actor = getActorPtr() )
                {
                    setupMaterial();
                    syncRenderSkybox( this );

                    auto applicationManager = core::IApplicationManager::instancePtr();
                    WP_ASSERT( applicationManager );

                    auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
                    if( graphicsSystem )
                    {
                        auto smgr = graphicsSystem->getGraphicsScenePtr();
                        WP_ASSERT( smgr );

                        auto visible = actor->isEnabledInScene();
                        auto distance = getDistance();

                        if( auto skybox = getSkybox() )
                        {
                            skybox->setVisible( visible );
                            skybox->setDistance( distance );
                        }
                    }
                }
            }
            break;
            case State::Destroyed:
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                if( graphicsSystem )
                {
                    if( auto skybox = getSkybox() )
                    {
                        skybox->setVisible( false );
                    }
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
            case State::Edit:
            case State::Play:
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                if( graphicsSystem )
                {
                    if( auto skybox = getSkybox() )
                    {
                        skybox->setVisible( false );
                    }
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
        {
        }
        break;
        case FSMEvent::Complete:
        {
        }
        break;
        case FSMEvent::NewState:
        {
        }
        break;
        case FSMEvent::WaitForChange:
        {
        }
        break;
        default:
        {
        }
        break;
        }

        return FSMReturnType::Ok;
    }

    void Skybox::setupMaterial()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                WP_LOG_ERROR( "ApplicationManager is null during Skybox::setupMaterial" );
                return;
            }

            auto resourceDatabase = applicationManager->getResourceDatabasePtr();
            if( !resourceDatabase )
            {
                WP_LOG_ERROR( "Resource database is null during Skybox::setupMaterial" );
                return;
            }

            if( auto handle = getHandle() )
            {
                auto material = getMaterial();
                if( !material )
                {
                    if( auto actor = getActorPtr() )
                    {
                        if( auto materialComponent = actor->getComponent<Material>() )
                        {
                            material = materialComponent->getMaterial();
                        }
                    }
                }

                if( !material )
                {
                    auto uuid = handle->getUUIDAsString();
                    auto materialResult =
                        resourceDatabase->createOrRetrieveByType<render::IMaterial>( uuid );
                    if( materialResult.first )
                    {
                        material = materialResult.first;
                    }

                    if( materialResult.second && material )
                    {
                        if( !material->hasObjectListener( m_materialSharedListener ) )
                        {
                            material->addObjectListener( m_materialSharedListener );
                        }
                    }
                    else if( materialResult.second )
                    {
                        WP_LOG_ERROR( "Failed to create/retrieve material for UUID: " + uuid );
                    }

                    if( auto actor = getActorPtr() )
                    {
                        if( auto materialComponent = actor->getComponent<Material>() )
                        {
                            materialComponent->setMaterial( material );
                        }
                    }
                }

                if( material )
                {
                    material->setMaterialType( MaterialType::SkyboxCubemap );
                    material->setLightingEnabled( false );

                    auto textures = getTextures();
                    for( auto &texture : textures )
                    {
                        if( !texture )
                        {
                            texture = resourceDatabase->loadResourceByType<render::ITexture>(
                                defaultSkyboxTextureName );
                        }
                    }

                    if( ApplicationUtil::hasAnyTexture( textures ) )
                    {
                        if( getSwapLeftRight() )
                        {
                            std::swap( textures[static_cast<u32>( SkyboxTextureTypes::Left )],
                                       textures[static_cast<u32>( SkyboxTextureTypes::Right )] );
                        }

                        if( getSwapUpDown() )
                        {
                            std::swap( textures[static_cast<u32>( SkyboxTextureTypes::Up )],
                                       textures[static_cast<u32>( SkyboxTextureTypes::Down )] );
                        }

                        if( getSwapFrontBack() )
                        {
                            std::swap( textures[static_cast<u32>( SkyboxTextureTypes::Front )],
                                       textures[static_cast<u32>( SkyboxTextureTypes::Back )] );
                        }

                        material->setCubicTextures( textures );
                    }
                }

                m_material = material;
            }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Skybox::setSkybox( SmartPtr<render::ISkybox> skybox )
    {
        m_skybox = skybox;
    }

    SmartPtr<render::ISkybox> Skybox::getSkybox() const
    {
        return m_skybox;
    }

    void Skybox::updateVisibility()
    {
        if( auto actor = getActorPtr() )
        {
            auto enabled = isEnabled() && actor->isEnabledInScene();

            if( auto skybox = getSkyboxPtr() )
            {
                skybox->setVisible( enabled );
            }
        }
    }

    void Skybox::syncRenderSkybox( Skybox *owner )
    {
        if( !owner )
        {
            return;
        }

        if( auto skybox = owner->getSkyboxPtr() )
        {
            skybox->setMaterial( owner->getMaterial() );
            skybox->setDistance( owner->getDistance() );
        }
    }

    void Skybox::setSwapFrontBack( bool swapFrontBack )
    {
        m_swapFrontBack = swapFrontBack;
    }

    bool Skybox::getSwapFrontBack() const
    {
        return m_swapFrontBack;
    }

    void Skybox::setSwapUpDown( bool swapUpDown )
    {
        m_swapUpDown = swapUpDown;
    }

    bool Skybox::getSwapUpDown() const
    {
        return m_swapUpDown;
    }

    auto Skybox::MaterialSharedListener::handleEvent( EventType eventType, hash_type eventValue,
                                                      const Array<Parameter> &arguments,
                                                      SmartPtr<ISharedObject> sender,
                                                      SmartPtr<ISharedObject> object,
                                                      SmartPtr<IEvent> event ) -> Parameter
    {
        if( eventValue == IEvent::loadingStateChanged )
        {
            loadingStateChanged( sender.get(), (LoadingState)arguments[0].getS32(),
                                 (LoadingState)arguments[1].getS32() );
        }

        return {};
    }

    void Skybox::MaterialSharedListener::loadingStateChanged( ISharedObject *sharedObject,
                                                              LoadingState oldState,
                                                              LoadingState newState )
    {
        if( newState == LoadingState::Loaded )
        {
            if( sharedObject->isDerived<render::ITexture>() )
            {
                if( auto owner = getOwnerPtr() )
                {
                    if( auto actor = owner->getActorPtr() )
                    {
                        actor->setDirty( true );

                        auto applicationManager = core::IApplicationManager::instancePtr();
                        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
                        if( graphicsSystem )
                        {
                            auto smgr = graphicsSystem->getGraphicsScenePtr();
                            WP_ASSERT( smgr );

                            auto visible = actor->isEnabledInScene();
                            auto distance = owner->getDistance();

                            if( auto skybox = owner->getSkyboxPtr() )
                            {
                                owner->setupMaterial();
                                skybox->setMaterial( owner->getMaterial() );
                                skybox->setDistance( distance );
                                skybox->setVisible( visible );
                            }
                        }
                    }
                }
            }
        }
    }

    auto Skybox::MaterialSharedListener::destroy( void *ptr ) -> bool
    {
        return false;
    }

    auto Skybox::MaterialSharedListener::getOwner() const -> SmartPtr<Skybox>
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void Skybox::MaterialSharedListener::setOwner( SmartPtr<Skybox> owner )
    {
        m_owner = owner;
    }

    Skybox::MaterialSharedListener::MaterialSharedListener() = default;

    Skybox::MaterialSharedListener::~MaterialSharedListener() = default;

}  // namespace workphone::scene
