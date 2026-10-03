#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Material.hpp>
#include <Workphone/Scene/Components/Skybox.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Scene/Components/UI/Image.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITaskLock.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/State/Messages/StateMessageLoad.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/FileInfo.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Material, Component );
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Material::MaterialStateObjectListener, IEventListener );
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Material::MaterialStateListener, IStateListener );

    Material::Material()
    {
        m_index = 0;

        m_mainTextureStr = String( "Main Texture" );
        m_materialStr = String( "Material" );
        m_materialPathStr = String( "Material Path" );
        m_indexStr = String( "index" );

        auto materialListener = workphone::make_ptr<MaterialStateListener>();
        materialListener->setOwner( this );
        m_materialListener = materialListener;
    }

    Material::~Material()
    {
        unload( nullptr );
    }

    void Material::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            Component::load( data );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto materialObjectListener = factoryManager->make_ptr<MaterialStateObjectListener>();
            materialObjectListener->setOwner( this );
            setMaterialObjectListener( materialObjectListener );

            updateMaterial();
            updateImageComponent();
            updateDependentComponents();
            updateFlags();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Material::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                if( m_material )
                {
                    WP_ASSERT( getMaterial() && getMaterial()->isValid() );

                    if( auto materialObjectListener = getMaterialObjectListener() )
                    {
                        m_material->removeObjectListener( materialObjectListener );
                    }

                    if( auto materialStateObject = m_material->getStateContext() )
                    {
                        materialStateObject->removeStateListener( m_materialListener );
                    }
                }

                if( auto materialObjectListener = getMaterialObjectListener() )
                {
                    if( m_material )
                    {
                        m_material->removeObjectListener( materialObjectListener );
                    }

                    materialObjectListener->unload( data );
                    setMaterialObjectListener( nullptr );
                }

                m_material = nullptr;

                Component::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Material::updateFlags( u32 flags, u32 oldFlags )
    {
        if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagInScene ) !=
            BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagInScene ) )
        {
            updateMaterial();
            updateImageComponent();
            updateDependentComponents();
            updateFlags();
        }
        else if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagEnabled ) !=
                 BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagEnabled ) )
        {
            updateMaterial();
            updateImageComponent();
            updateDependentComponents();
            updateFlags();
        }
    }

    auto Material::handleComponentEvent( u32 state, FSMEvent eventType ) -> FSMReturnType
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
            case State::Edit:
            case State::Play:
            {
                updateMaterial();
                updateImageComponent();
                updateDependentComponents();
                updateFlags();
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
            case State::Play:
            {
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

    auto Material::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = Component::getProperties();
        WP_ASSERT( properties );

        auto materialPath = getMaterialPath();

        properties->setProperty( getMaterialStr(), m_material );
        properties->setProperty( getMaterialPathStr(), materialPath );
        properties->setProperty( getIndexStr(), m_index );

        return properties;
    }

    void Material::setProperties( SmartPtr<Properties> properties )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

            auto taskManager = applicationManager->getTaskManagerPtr();
            WP_ASSERT( taskManager );

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            auto resourceDatabase = applicationManager->getResourceDatabasePtr();
            WP_ASSERT( resourceDatabase );

            SmartPtr<render::IMaterial> material;
            auto materialPath = getMaterialPath();
            auto index = 0u;

            properties->getPropertyValue( getMaterialStr(), material );
            properties->getPropertyValue( getMaterialPathStr(), materialPath );
            properties->getPropertyValue( getIndexStr(), index );

            setMaterialPath( materialPath );

            auto dirty = false;

            if( m_material != material )
            {
                m_material = material;

                if( m_material )
                {
                    m_materialPath = m_material->getFilePath();
                }

                dirty = true;
            }

            if( !m_material )
            {
                m_material = resourceDatabase->loadResourceByType<render::IMaterial>( m_materialPath );
            }

            if( m_material )
            {
                if( !m_material->isLoaded() )
                {
                    if( graphicsSystem )
                    {
                        graphicsSystem->loadObject( m_material, false );
                    }
                    else
                    {
                        m_material->load( nullptr );
                    }

                    dirty = true;
                }
            }

            if( auto material = m_material )
            {
                auto count = 0;
                while( !material->isLoaded() && count++ < 10 )
                {
                    Thread::sleep( 0.01 );
                }
            }

            if( m_index != index )
            {
                m_index = index;
                dirty = true;
            }

            if( dirty )
            {
                updateImageComponent();
                updateDependentComponents();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Material::updateFlags()
    {
        updateMaterial();

        if( auto actor = getActor() )
        {
            auto meshRenderer = actor->getComponent<MeshRenderer>();
            if( meshRenderer )
            {
                meshRenderer->updateMaterials();
            }

            auto uiComponent = actor->getComponent<UIComponent>();
            if( uiComponent )
            {
                uiComponent->updateMaterials();
            }

            auto skybox = actor->getComponent<Skybox>();
            if( skybox )
            {
                skybox->updateMaterials();
            }
        }
    }

    auto Material::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        Array<SmartPtr<ISharedObject>> objects;

        if( m_material )
        {
            objects.emplace_back( m_material );
        }

        return objects;
    }

    auto Material::getMaterialPath() const -> String
    {
        return m_materialPath;
    }

    void Material::setMaterialPath( const String &materialPath )
    {
        m_materialPath = materialPath;

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto resourceDatabase = applicationManager->getResourceDatabase();
        if( resourceDatabase )
        {
            auto material = resourceDatabase->createOrRetrieveByType<render::IMaterial>( materialPath );
            setMaterial( material.first );
        }
    }

    auto Material::getMaterial() const -> SmartPtr<render::IMaterial>
    {
        return m_material;
    }

    void Material::setMaterial( SmartPtr<render::IMaterial> material )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();

        if( m_material != material )
        {
            if( m_material )
            {
                WP_ASSERT( getMaterial() && getMaterial()->isValid() );

                if( auto materialObjectListener = getMaterialObjectListener() )
                {
                    m_material->removeObjectListener( materialObjectListener );
                }

                if( auto materialStateObject = m_material->getStateContext() )
                {
                    materialStateObject->removeStateListener( m_materialListener );
                }
            }

            m_material = material;

            if( m_material )
            {
                WP_ASSERT( getMaterial() && getMaterial()->isValid() );

                if( auto materialObjectListener = getMaterialObjectListener() )
                {
                    m_material->addObjectListener( materialObjectListener );
                }

                if( auto materialStateObject = m_material->getStateContext() )
                {
                    materialStateObject->addStateListener( m_materialListener );
                }

                graphicsSystem->loadObject( m_material );
            }

            updateMaterial();
            updateImageComponent();
            updateDependentComponents();
        }
    }

    void Material::updateMaterial()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto resourceDatabase = applicationManager->getResourceDatabasePtr();
        if( !resourceDatabase )
        {
            WP_LOG_ERROR( "Material::updateMaterial: resourceDatabase is nullptr" );
            return;
        }

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        if( !graphicsSystem )
        {
            WP_LOG_ERROR( "Material::updateMaterial: graphicsSystem is nullptr" );
            return;
        }

        auto material = getMaterial();
        if( !material )
        {
            auto materialPath = getMaterialPath();
            if( !StringUtil::isNullOrEmpty( materialPath ) )
            {
                auto newMaterial =
                    resourceDatabase->createOrRetrieveByType<render::IMaterial>( materialPath );
                setMaterial( newMaterial.first );
            }
        }
        else
        {
            if( !material->isLoaded() )
            {
                auto materialPath = getMaterialPath();
                if( !StringUtil::isNullOrEmpty( materialPath ) )
                {
                    graphicsSystem->loadObject( material );
                }
            }
        }
    }

    void Material::updateDependentComponents()
    {
        ScopedLock lock( this );

        if( auto actor = getActor() )
        {
            auto components = actor->getComponents();
            for( auto &component : components )
            {
                if( component )
                {
                    component->updateMaterials();
                }
            }
        }
    }

    void Material::updateImageComponent()
    {
        if( auto material = getMaterial() )
        {
            if( auto actor = getActor() )
            {
                auto imageComponent = actor->getComponent<Image>();
                if( imageComponent )
                {
                    if( auto imageTexture = imageComponent->getTexture() )
                    {
                        material->setTexture( imageTexture, 0 );
                    }
                }
            }
        }
    }

    auto Material::getIndex() const -> u32
    {
        return m_index;
    }

    void Material::setIndex( u32 index )
    {
        m_index = index;
    }

    auto Material::getMaterialObjectListener() const -> SmartPtr<IEventListener>
    {
        return m_materialObjectListener;
    }

    void Material::setMaterialObjectListener( SmartPtr<IEventListener> materialObjectListener )
    {
        m_materialObjectListener = materialObjectListener;
    }

    Material::MaterialStateObjectListener::MaterialStateObjectListener() = default;

    Material::MaterialStateObjectListener::~MaterialStateObjectListener() = default;

    auto Material::MaterialStateObjectListener::getOwner() const -> SmartPtr<Material>
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void Material::MaterialStateObjectListener::setOwner( SmartPtr<Material> owner )
    {
        m_owner = owner;
    }

    Material::MaterialStateListener::MaterialStateListener() = default;

    Material::MaterialStateListener::MaterialStateListener( Material *owner )
    {
        m_owner = owner;
    }

    Material::MaterialStateListener::~MaterialStateListener() = default;

    auto Material::MaterialStateObjectListener::handleEvent( EventType eventType, hash_type eventValue,
                                                             const Array<Parameter> &arguments,
                                                             SmartPtr<ISharedObject> sender,
                                                             SmartPtr<ISharedObject> object,
                                                             SmartPtr<IEvent> event ) -> Parameter
    {
        auto task = Thread::getCurrentTask();
        if( task == TaskId::Application )
        {
            if( eventValue == IEvent::loadingStateChanged )
            {
                if( arguments[1].getS32() == static_cast<u32>( LoadingState::Loaded ) )
                {
                    auto owner = getOwner();
                    if( owner )
                    {
                        if( owner->getMaterial() == sender )
                        {
                            if( owner->getLoadingState() == LoadingState::Loaded )
                            {
                                //owner->updateMaterial();
                                owner->updateImageComponent();
                                owner->updateDependentComponents();
                            }
                        }
                    }
                }
            }
        }

        return {};
    }

    void Material::MaterialStateObjectListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    bool Material::MaterialStateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        auto owner = getOwner();
        if( owner )
        {
            if( owner->isLoaded() )
            {
                //owner->updateMaterial();
                //owner->updateImageComponent();
                //owner->updateDependentComponents();
            }
        }

        return false;
    }

    bool Material::MaterialStateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( message->isExactly<StateMessageLoad>() )
        {
            auto messageLoad = workphone::static_pointer_cast<StateMessageLoad>( message );
            auto messageType = messageLoad->getType();

            if( messageType == StateMessageLoad::LOADED_HASH )
            {
                auto owner = getOwner();
                if( owner )
                {
                    if( owner->getLoadingState() == LoadingState::Loaded )
                    {
                        owner->updateImageComponent();
                        owner->updateDependentComponents();
                    }
                }
            }
        }

        return false;
    }

    auto Material::MaterialStateListener::getOwner() const -> SmartPtr<Material>
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void Material::MaterialStateListener::setOwner( SmartPtr<Material> owner )
    {
        m_owner = owner;
    }

    String Material::getMainTextureStr() const
    {
        return m_mainTextureStr;
    }

    void Material::setMainTextureStr( const String &key )
    {
        m_mainTextureStr = key;
    }

    String Material::getMaterialStr() const
    {
        return m_materialStr;
    }

    void Material::setMaterialStr( const String &key )
    {
        m_materialStr = key;
    }

    String Material::getMaterialPathStr() const
    {
        return m_materialPathStr;
    }

    void Material::setMaterialPathStr( const String &key )
    {
        m_materialPathStr = key;
    }

    String Material::getIndexStr() const
    {
        return m_indexStr;
    }

    void Material::setIndexStr( const String &key )
    {
        m_indexStr = key;
    }

}  // namespace workphone::scene
