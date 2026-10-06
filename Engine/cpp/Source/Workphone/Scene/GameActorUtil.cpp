#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Scene/GameScene.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <Workphone/WorkphoneHeaders.hpp>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>

namespace workphone::scene
{
    namespace {
        struct LoadBatch {
            std::unordered_map<String, SmartPtr<ISharedObject>> objects;
            Array<Pair<SmartPtr<IComponent>, SmartPtr<Properties>>> components;
        };
        thread_local LoadBatch *activeBatch = nullptr;
    }

    const String GameActorUtil::childStr = String( "child" );
    const String GameActorUtil::nameStr = String( "name" );
    const String GameActorUtil::staticStr = String( "static" );
    const String GameActorUtil::enabledStr = String( "enabled" );
    const String GameActorUtil::smoothMotionStr = String( "smoothMotion" );
    const String GameActorUtil::collisionMaskStr = String( "collisionMask" );
    const String GameActorUtil::layerStr = String( "layer" );
    const String GameActorUtil::tagsStr = String( "tags" );
    const String GameActorUtil::updateTransformStr = String( "updateTransform" );
    const String GameActorUtil::labelStr = String( "label" );
    const String GameActorUtil::uuidStr = String( "uuid" );
    const String GameActorUtil::localTransformStr = String( "localTransform" );
    const String GameActorUtil::worldTransformStr = String( "worldTransform" );
    const String GameActorUtil::componentsStr = String( "components" );
    const String GameActorUtil::componentStr = String( "component" );
    const String GameActorUtil::componentTypeStr = String( "componentType" );
    const String GameActorUtil::childrenStr = String( "children" );

    void GameActorUtil::createRigidStaticMesh()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto selectionManager = applicationManager->getSelectionManager();
            WP_ASSERT( selectionManager );

            auto selection = selectionManager->getSelection();
            for( auto selected : selection )
            {
                if( selected->isDerived<IGameActor>() )
                {
                    auto actor = workphone::static_pointer_cast<IGameActor>( selected );
                    createRigidStaticMesh( actor, true );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GameActorUtil::createRigidDynamicMesh()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto selectionManager = applicationManager->getSelectionManager();
            WP_ASSERT( selectionManager );

            auto selection = selectionManager->getSelection();
            for( auto selected : selection )
            {
                if( selected->isDerived<IGameActor>() )
                {
                    auto actor = workphone::static_pointer_cast<IGameActor>( selected );
                    createRigidDynamicMesh( actor, true );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GameActorUtil::createRigidStaticMesh( SmartPtr<IGameActor> actor, bool recursive )
    {
        try
        {
            if( !actor )
            {
                return;
            }

            actor->setStatic( true );

            auto meshComponent = actor->getComponent<Mesh>();
            if( meshComponent )
            {
                auto collisionMesh = actor->getComponent<CollisionMesh>();
                if( !collisionMesh )
                {
                    collisionMesh = actor->addComponent<CollisionMesh>();
                }

                if( collisionMesh )
                {
                    auto meshPath = meshComponent->getMeshPath();
                    collisionMesh->setMeshPath( meshPath );
                }

                auto rigidbody = actor->getComponent<Rigidbody>();
                if( !rigidbody )
                {
                    rigidbody = actor->addComponent<Rigidbody>();
                }
            }

            auto children = actor->getChildren();
            for( auto child : children )
            {
                createRigidStaticMesh( child, recursive );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GameActorUtil::createRigidDynamicMesh( SmartPtr<IGameActor> actor, bool recursive )
    {
        try
        {
            if( !actor )
            {
                return;
            }

            actor->setStatic( false );

            auto meshComponent = actor->getComponent<Mesh>();
            if( meshComponent )
            {
                auto collisionMesh = actor->getComponent<CollisionMesh>();
                if( !collisionMesh )
                {
                    collisionMesh = actor->addComponent<CollisionMesh>();
                }

                if( collisionMesh )
                {
                    auto meshPath = meshComponent->getMeshPath();
                    collisionMesh->setMeshPath( meshPath );
                }

                auto rigidbody = actor->getComponent<Rigidbody>();
                if( !rigidbody )
                {
                    rigidbody = actor->addComponent<Rigidbody>();
                }
            }

            auto children = actor->getChildren();
            for( auto child : children )
            {
                createRigidDynamicMesh( child, recursive );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    AABB3<real_Num> GameActorUtil::getActorLocalAABB( SmartPtr<IGameActor> actor )
    {
        AABB3<real_Num> result;
        bool hasResult = false;

        try
        {
            if( !actor )
            {
                return result;
            }

            // Include bounding boxes provided by renderer-type components (renderers typically
            // expose a precomputed local-space AABB for the actor geometry).
            const auto components = actor->getComponents();
            for( const auto &component : components )
            {
                if( !component )
                    continue;

                // Many render components expose a local bounding box via getBoundingBox().
                if( component->isDerived<Renderer>() )
                {
                    auto renderer = workphone::dynamic_pointer_cast<Renderer>( component );
                    if( renderer )
                    {
                        const auto aabb = renderer->getBoundingBox();
                        if( aabb.isFinite() )
                        {
                            if( !hasResult )
                            {
                                result = aabb;
                                hasResult = true;
                            }
                            else
                            {
                                result.merge( aabb.getMinimum() );
                                result.merge( aabb.getMaximum() );
                            }
                        }
                    }
                }
            }

            // Include children's local AABBs transformed into this actor's local space.
            const auto &children = actor->getChildren();
            for( auto child : children )
            {
                if( !child )
                    continue;

                // Recursively compute child's local AABB (in child's local space).
                const auto childLocalAABB = getActorLocalAABB( child );
                if( !childLocalAABB.isFinite() )
                    continue;

                // Transform child's local AABB into this actor's local space using the child's local transform.
                const auto childLocalTransform = child->getLocalTransform();
                const auto transformed = childLocalTransform.transformAABB( childLocalAABB );

                if( !transformed.isFinite() )
                    continue;

                if( !hasResult )
                {
                    result = transformed;
                    hasResult = true;
                }
                else
                {
                    result.merge( transformed.getMinimum() );
                    result.merge( transformed.getMaximum() );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return result;
    }

    AABB3<real_Num> GameActorUtil::getActorAABB( SmartPtr<IGameActor> actor )
    {
        AABB3<real_Num> result;

        try
        {
            if( !actor )
            {
                return result;
            }

            // Compute actor's local AABB (includes renderer components and children in local space)
            const auto localAABB = getActorLocalAABB( actor );

            // If the local AABB is finite, transform it by the actor's world transform to get world-space AABB.
            if( localAABB.isFinite() )
            {
                const auto worldTransform = actor->getWorldTransform();
                result = worldTransform.transformAABB( localAABB );
            }
            else
            {
                // Fallback: try to compute by merging children's world AABBs.
                bool hasResult = false;
                const auto &children = actor->getChildren();
                for( auto child : children )
                {
                    if( !child )
                        continue;

                    const auto childAABB = getActorAABB( child );
                    if( !childAABB.isFinite() )
                        continue;

                    if( !hasResult )
                    {
                        result = childAABB;
                        hasResult = true;
                    }
                    else
                    {
                        result.merge( childAABB );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return result;
    }

    namespace
    {
        bool getDirectStringProperty( const Properties *properties, const String &name, String &value )
        {
            if( properties && properties->hasProperty( name ) )
            {
                value = properties->getPropertyObject( name ).getValue();
                return true;
            }

            return false;
        }

        void addUniqueProperties( Array<SmartPtr<Properties>> &propertiesList,
                                  const SmartPtr<Properties> &properties )
        {
            if( properties && std::find( propertiesList.begin(), propertiesList.end(), properties ) ==
                                  propertiesList.end() )
            {
                propertiesList.push_back( properties );
            }
        }

        void addComponentData( Array<SmartPtr<Properties>> &componentsData,
                               const SmartPtr<Properties> &componentData )
        {
            if( !componentData )
            {
                return;
            }

            if( componentData->hasProperty( GameActorUtil::componentTypeStr ) )
            {
                addUniqueProperties( componentsData, componentData );
                return;
            }

            const auto childComponents = componentData->getChildrenByName( GameActorUtil::componentStr );
            for( const auto &childComponent : childComponents )
            {
                addComponentData( componentsData, childComponent );
            }
        }

        bool hasDirectActorData( const SmartPtr<Properties> &actorData )
        {
            if( !actorData )
            {
                return false;
            }

            return actorData->getNumProperties() > 0 ||
                   actorData->hasChild( GameActorUtil::localTransformStr ) ||
                   actorData->hasChild( GameActorUtil::worldTransformStr ) ||
                   actorData->hasChild( GameActorUtil::componentStr ) ||
                   actorData->hasChild( GameActorUtil::componentsStr );
        }

        void addActorData( Array<SmartPtr<Properties>> &childrenData,
                           const SmartPtr<Properties> &childData )
        {
            if( !childData )
            {
                return;
            }

            if( hasDirectActorData( childData ) )
            {
                addUniqueProperties( childrenData, childData );
                return;
            }

            for( const auto &childActor : childData->getChildren() )
            {
                if( childActor->getName() == GameActorUtil::childStr ||
                    childActor->getName() == GameActorUtil::childrenStr )
                    addActorData( childrenData, childActor );
            }
        }
    }  // namespace

    SmartPtr<Properties> GameActorUtil::createInstanceData( SmartPtr<Properties> data )
    {
        if( !data )
            return nullptr;
        std::unordered_map<String, String> identities;
        std::function<void( SmartPtr<Properties> )> index = [&]( SmartPtr<Properties> object ) {
            String uuid;
            if( getDirectStringProperty( object.get(), uuidStr, uuid ) && !uuid.empty() )
            {
                if( !identities.emplace( uuid, StringUtil::getUUID() ).second )
                    throw std::runtime_error( "Duplicate prefab UUID: " + uuid );
            }
            Array<SmartPtr<Properties>> components;
            Array<SmartPtr<Properties>> children;
            for( auto child : object->getChildren() )
            {
                if( child->getName() == componentStr || child->getName() == componentsStr )
                    addComponentData( components, child );
                else if( child->getName() == childStr || child->getName() == childrenStr )
                    addActorData( children, child );
            }
            for( auto component : components )
            {
                if( getDirectStringProperty( component.get(), uuidStr, uuid ) && !uuid.empty() )
                    if( !identities.emplace( uuid, StringUtil::getUUID() ).second )
                        throw std::runtime_error( "Duplicate prefab UUID: " + uuid );
            }
            for( auto child : children )
                index( child );
        };
        index( data );
        std::function<SmartPtr<Properties>( SmartPtr<Properties> )> copy =
            [&]( SmartPtr<Properties> source ) {
                auto result = workphone::make_ptr<Properties>();
                result->setName( source->getName() );
                for( auto property : source->getPropertiesAsArray() )
                {
                    auto found = identities.find( property.getValue() );
                    if( found != identities.end() )
                        property.setValue( found->second );
                    // Serialized reference arrays may arrive as plain semicolon-separated strings.
                    else if( property.getValue().find( ';' ) != String::npos )
                    {
                        Array<String> values;
                        StringUtil::parseArray( property.getValue(), values );
                        bool changed = false;
                        for( auto &value : values )
                        {
                            auto entry = identities.find( value );
                            if( entry != identities.end() )
                            {
                                value = entry->second;
                                changed = true;
                            }
                        }
                        if( changed )
                            property.setValue( StringUtil::toString( values ) );
                    }
                    result->setProperty( property );
                }
                for( auto child : source->getChildren() )
                    result->addChild( copy( child ) );
                return result;
            };
        return copy( data );
    }

    Array<SmartPtr<IGameActor>> GameActorUtil::loadSceneActors( const Array<SmartPtr<Properties>> &data,
                                                                SmartPtr<IGameScene> target )
    {
        if( activeBatch )
            throw std::logic_error( "Reentrant scene loading" );
        auto concrete = workphone::dynamic_pointer_cast<GameScene>( target );
        const auto generation = concrete ? concrete->getLoadGeneration() : 0;
        auto checkCurrent = [&] {
            if( target &&
                ( !target->isLoaded() || ( concrete && concrete->getLoadGeneration() != generation ) ) )
                throw std::runtime_error( "Scene load cancelled during graph construction" );
        };
        LoadBatch batch;
        // Include the pinned destination's existing graph, never the global current scene.
        std::function<void( SmartPtr<IGameActor> )> indexActor = [&]( SmartPtr<IGameActor> actor ) {
            if( !actor )
                return;
            batch.objects.emplace( actor->getHandle()->getUUIDAsString(), actor );
            for( const auto &component : actor->getComponents() )
                batch.objects.emplace( component->getHandle()->getUUIDAsString(), component );
            for( const auto &child : actor->getChildren() )
                indexActor( child );
        };
        if( target )
            for( const auto &actor : target->getActors() )
                indexActor( actor );
        std::unordered_set<String> identities;
        for( const auto &entry : batch.objects )
            identities.insert( entry.first );
        auto validateIdentity = [&]( const SmartPtr<Properties> &properties ) {
            String uuid;
            if( getDirectStringProperty( properties.get(), uuidStr, uuid ) && !uuid.empty() )
            {
                auto identity = StringUtil::toString( StringUtil::parseUUID( uuid ) );
                if( !identities.insert( identity ).second )
                    throw std::runtime_error( "Duplicate scene UUID: " + uuid );
            }
        };
        size_t actorCount = 0;
        std::function<void( const SmartPtr<Properties> & )> validateActor =
            [&]( const SmartPtr<Properties> &properties ) {
                if( !properties )
                    throw std::invalid_argument( "Missing actor properties" );
                ++actorCount;
                validateIdentity( properties );
                Array<SmartPtr<Properties>> components;
                Array<SmartPtr<Properties>> children;
                for( const auto &child : properties->getChildren() )
                {
                    const auto name = child->getName();
                    if( name == componentStr || name == componentsStr )
                        addComponentData( components, child );
                    else if( name == childStr || name == childrenStr )
                        addActorData( children, child );
                }
                for( const auto &component : components )
                    validateIdentity( component );
                for( const auto &child : children )
                    validateActor( child );
            };
        for( const auto &properties : data )
            validateActor( properties );
        auto manager = core::IApplicationManager::instancePtr()->getGameManager();
        const auto capacity = manager->getActors().size();
        const auto used = static_cast<size_t>( manager->getNumActors() );
        if( used > capacity || actorCount > capacity - used )
            throw std::length_error( "Actor capacity exhausted before scene commit" );
        if( target )
        {
            const auto roots = target->getActors().size();
            if( roots > WP_MAX_ACTORS || data.size() > WP_MAX_ACTORS - roots )
                throw std::length_error( "Scene root capacity exhausted before scene commit" );
        }
        struct Scope
        {
            Properties::ObjectResolver previous;
            ~Scope()
            {
                Properties::exchangeObjectResolver( std::move( previous ) );
                activeBatch = nullptr;
            }
        } scope{ Properties::exchangeObjectResolver(
            [&batch]( const String &uuid ) -> SmartPtr<ISharedObject> {
                auto it = batch.objects.find( uuid );
                return it == batch.objects.end() ? nullptr : it->second;
            } ) };
        activeBatch = &batch;
        Array<SmartPtr<IGameActor>> roots;
        try
        {
            for( const auto &properties : data )
            {
                checkCurrent();
                auto actor = manager->createActor();
                if( !actor )
                    throw std::runtime_error( "Actor capacity exhausted" );
                roots.push_back( actor );
                actor->setScene( target );
                loadFromData( actor, properties, true );
            }
            for( const auto &entry : batch.components )
            {
                checkCurrent();
                manager->loadObject( entry.first, entry.second, false );
                if( entry.first->getLoadingState() == LoadingState::Error )
                    throw std::runtime_error( "Component initialization failed" );
            }
            checkCurrent();
            return roots;
        }
        catch( ... )
        {
            for( auto &actor : roots )
                manager->destroyActor( actor );
            throw;
        }
    }

    bool GameActorUtil::isEditorCameraData( SmartPtr<IGameActor> editorCamera,
                                          SmartPtr<Properties> actorData )
    {
        if( !editorCamera || !actorData )
            return false;

        String label;
        if( !getDirectStringProperty( actorData.get(), labelStr, label ) )
        {
            getDirectStringProperty( actorData.get(), nameStr, label );
        }
        if( label != editorCamera->getName() )
            return false;

        Array<SmartPtr<Properties>> componentsData;
        for( const auto &child : actorData->getChildren() )
        {
            if( child->getName() == componentStr || child->getName() == componentsStr )
            {
                addComponentData( componentsData, child );
            }
        }
        return std::any_of( componentsData.begin(), componentsData.end(), []( const auto &component ) {
            String type;
            getDirectStringProperty( component.get(), componentTypeStr, type );
            auto names = StringUtil::split( type, "::" );
            return !names.empty() && ( names.back() == "SphericalCameraController" ||
                                      names.back() == "EditorCameraController" );
        } );
    }

    void GameActorUtil::restoreEditorCameraData( SmartPtr<IGameActor> editorCamera,
                                               SmartPtr<Properties> actorData )
    {
        if( !editorCamera || !actorData )
            return;

        // The editor actor and its components are created by the editor. Loading
        // this data as a new actor appends cameras and loses controller UI bindings.
        auto actorProperties = workphone::make_ptr<Properties>();
        actorProperties->setPropertiesAsArray( editorCamera->getProperties()->getPropertiesAsArray() );
        for( const auto &property : actorData->getPropertiesAsArray() )
        {
            actorProperties->setProperty( property );
        }
        Array<SmartPtr<Properties>> componentsData;
        for( const auto &child : actorData->getChildren() )
        {
            const auto name = child->getName();
            if( name == localTransformStr || name == worldTransformStr )
            {
                actorProperties->addChild( child );
            }
            else if( name == componentStr || name == componentsStr )
            {
                addComponentData( componentsData, child );
            }
        }
        editorCamera->setProperties( actorProperties );

        auto typeManager = TypeManager::instance();
        auto components = editorCamera->getComponents();
        for( const auto &componentData : componentsData )
        {
            String type;
            getDirectStringProperty( componentData.get(), componentTypeStr, type );
            auto typeNames = StringUtil::split( type, "::" );
            if( typeNames.empty() )
                continue;

            for( auto component : components )
            {
                String name = typeManager->getName( component->getTypeInfo() );
                name = StringUtil::replaceAll( name, "class ", "" );
                auto names = StringUtil::split( name, "::" );
                if( !names.empty() && names.back() == typeNames.back() )
                {
                    auto camera = workphone::dynamic_pointer_cast<Camera>( component );
                    auto targetTexture = camera ? camera->getTargetTexture() : nullptr;
                    component->setProperties( componentData );
                    if( camera )
                    {
                        // Output render textures are serialized separately from
                        // the editor-owned viewport target. Keep the latter bound.
                        camera->setTargetTexture( targetTexture );
                    }
                    break;
                }
            }
        }
    }

    void GameActorUtil::loadFromData( SmartPtr<IGameActor> actor, SmartPtr<Properties> actorData,
                                      bool cascade )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto gameManager = applicationManager->getGameManagerPtr();
            WP_ASSERT( gameManager );

            auto pActor = actor.get();
            WP_ASSERT( pActor );

            auto properties = actorData.get();
            WP_ASSERT( properties );

            auto label = String();
            auto enabled = true;
            auto visible = true;
            auto bStatic = false;
            auto bSmoothMotion = false;
            auto collisionMask = u32( 0 );
            auto layer = String();
            auto tags = Array<String>();

            if( !getDirectStringProperty( properties, labelStr, label ) )
            {
                getDirectStringProperty( properties, nameStr, label );
            }

            properties->getPropertyValue( staticStr, bStatic );
            properties->getPropertyValue( enabledStr, enabled );
            properties->getPropertyValue( "visible", visible );
            properties->getPropertyValue( smoothMotionStr, bSmoothMotion );
            properties->getPropertyValue( collisionMaskStr, collisionMask );
            getDirectStringProperty( properties, layerStr, layer );
            properties->getPropertyValue( tagsStr, tags );

            pActor->setName( label );
            pActor->setEnabled( enabled );
            pActor->setVisible( visible );
            pActor->setStatic( bStatic );
            pActor->setSmoothMotion( bSmoothMotion );
            pActor->setCollisionMask( collisionMask );
            pActor->setLayer( layer );
            pActor->setTags( tags );

            if( auto handle = pActor->getHandle() )
            {
                auto uuid = String();
                getDirectStringProperty( properties, uuidStr, uuid );

                if( StringUtil::isNullOrEmpty( uuid ) )
                {
                    uuid = StringUtil::getUUID();
                }

                handle->setUUID( uuid );
                if( activeBatch &&
                    !activeBatch->objects.emplace( handle->getUUIDAsString(), actor ).second )
                    throw std::runtime_error("Duplicate actor UUID: " + uuid);
            }

            if( auto transform = pActor->getTransform() )
            {
                if( auto localTransformChild = properties->getChild( localTransformStr ) )
                {
                    auto localTransform = transform->getLocalTransform();
                    localTransform.setProperties( localTransformChild );

                    transform->setLocalTransform( localTransform );
                }

                if( auto worldTransformChild = properties->getChild( worldTransformStr ) )
                {
                    auto worldTransform = transform->getWorldTransform();
                    worldTransform.setProperties( worldTransformChild );

                    transform->setWorldTransform( worldTransform );
                }
            }

            auto componentsData = Array<SmartPtr<Properties>>();

            const auto componentsDataContainers = properties->getChildrenByName( componentsStr );
            for( const auto &componentData : componentsDataContainers )
            {
                addComponentData( componentsData, componentData );
            }

            const auto componentsDataAlt = properties->getChildrenByName( componentStr );
            for( const auto &componentData : componentsDataAlt )
            {
                addComponentData( componentsData, componentData );
            }

            auto components = Array<Pair<SmartPtr<IComponent>, SmartPtr<Properties>>>();
            components.reserve( componentsData.size() );

            for( auto &componentData : componentsData )
            {
                auto componentType = String();
                if( !getDirectStringProperty( componentData.get(), componentTypeStr, componentType ) )
                {
                    continue;
                }

                auto pComponent = factoryManager->createObjectFromType<IComponent>( componentType );
                if( !pComponent )
                {
                    auto nameSplit = StringUtil::split( componentType, "::" );
                    std::reverse( nameSplit.begin(), nameSplit.end() );

                    for( auto componentTypeName : nameSplit )
                    {
                        pComponent =
                            factoryManager->createObjectFromType<IComponent>( componentTypeName );
                        if( pComponent )
                        {
                            break;
                        }
                    }
                }

                if( !pComponent )
                {
                    componentType = gameManager->getComponentFactoryType( componentType );
                    pComponent = factoryManager->createObjectFromType<IComponent>( componentType );
                }

                if( !pComponent && activeBatch )
                    throw std::runtime_error( "Unknown component type: " + componentType );
                if( pComponent )
                {
                    components.emplace_back( pComponent, componentData );
                    if( activeBatch ) {
                        String uuid;
                        getDirectStringProperty(componentData.get(), uuidStr, uuid);
                        if( !uuid.empty() ) {
                            pComponent->getHandle()->setUUID(uuid);
                            if( !activeBatch->objects
                                     .emplace( pComponent->getHandle()->getUUIDAsString(), pComponent )
                                     .second )
                                throw std::runtime_error("Duplicate component UUID: " + uuid);
                        }
                    }
                }
            }

            std::sort( components.begin(), components.end(),
                       []( const Pair<SmartPtr<IComponent>, SmartPtr<Properties>> &a,
                           const Pair<SmartPtr<IComponent>, SmartPtr<Properties>> &b ) {
                           return ApplicationUtil::getCreationOrder( a.first ) >
                                  ApplicationUtil::getCreationOrder( b.first );
                       } );

            for( auto &c : components )
            {
                try
                {
                    if( c.first )
                    {
                        pActor->addComponentInstance( c.first );
                        if( activeBatch )
                        {
                            auto attached = pActor->getComponents();
                            if( std::find( attached.begin(), attached.end(), c.first ) ==
                                attached.end() )
                                throw std::runtime_error( "Failed to attach component" );
                        }
                    }
                }
                catch( std::exception &e )
                {
                    if( activeBatch ) throw;
                    WP_LOG_EXCEPTION( e );
                }
            }

            for( auto &c : components )
            {
                try
                {
                    if( auto &component = c.first )
                    {
                        component->setActor( pActor );
                        if( activeBatch ) activeBatch->components.emplace_back(component, c.second);
                        else gameManager->loadObject( component, c.second, false );
                    }
                }
                catch( std::exception &e )
                {
                    if( activeBatch ) throw;
                    WP_LOG_EXCEPTION( e );
                }
            }

            if( cascade )
            {
                auto childrenData = Array<SmartPtr<Properties>>();

                for( const auto &childData : properties->getChildren() )
                {
                    if( childData->getName() == childStr || childData->getName() == childrenStr )
                        addActorData( childrenData, childData );
                }

                for( auto &childData : childrenData )
                {
                    auto childActor = gameManager->createActor();
                    if( !childActor && activeBatch ) throw std::runtime_error("Actor capacity exhausted");
                    if( childActor )
                    {
                        pActor->addChild( childActor );
                        GameActorUtil::loadFromData( childActor, childData, cascade );
                    }
                }
            }

            gameManager->addDirtyActor( pActor );
        }
        catch( std::exception &e )
        {
            if( activeBatch ) throw;
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<Properties> GameActorUtil::toData( SmartPtr<IGameActor> actor )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto typeManager = TypeManager::instance();

        //WP_ASSERT( getLoadingState() == LoadingState::Unloaded );
        //WP_ASSERT( getHandle()->getInstanceId() != std::numeric_limits<u32>::max() );

        auto actorData = factoryManager->make_ptr<Properties>();

        auto label = actor->getName();
        auto bStatic = actor->isStatic();
        auto visible = actor->isVisible();
        auto enabled = actor->isEnabled();

        actorData->setProperty( GameActorUtil::labelStr, label );
        actorData->setProperty( GameActorUtil::staticStr, bStatic );
        actorData->setProperty( GameActorUtil::enabledStr, enabled );
        actorData->setProperty( "visible", visible );
        actorData->setProperty( GameActorUtil::smoothMotionStr, actor->isSmoothMotion() );
        actorData->setProperty( GameActorUtil::collisionMaskStr, actor->getCollisionMask() );
        actorData->setProperty( GameActorUtil::layerStr, actor->getLayer() );
        actorData->setProperty( GameActorUtil::tagsStr, actor->getTags() );

        if( auto handle = actor->getHandle() )
        {
            auto uuid = handle->getUUIDAsString();
            if( StringUtil::isNullOrEmpty( uuid ) )
            {
                uuid = StringUtil::getUUID();
            }

            actorData->setProperty( GameActorUtil::uuidStr, uuid );
        }

        if( auto transform = actor->getTransformPtr() )
        {
            auto localTransform = transform->getLocalTransform();
            auto worldTransform = transform->getWorldTransform();

            auto localTransformProperties = localTransform.getProperties();
            localTransformProperties->setName( GameActorUtil::localTransformStr );
            actorData->addChild( localTransformProperties );

            auto worldTransformProperties = worldTransform.getProperties();
            worldTransformProperties->setName( GameActorUtil::worldTransformStr );
            actorData->addChild( worldTransformProperties );
        }

        auto children = actor->getChildren();
        for( auto child : children )
        {
            if( child )
            {
#if _DEBUG
                auto handle = child->getHandle();
                auto instanceId = handle->getInstanceId();
                WP_ASSERT( instanceId != std::numeric_limits<u32>::max() );
#endif

                auto hidden = child->getFlag( IGameActor::ActorFlagDontSave );
                if( !hidden )
                {
                    auto childActorData = child->toData();
                    auto childActorProperties =
                        workphone::static_pointer_cast<Properties>( childActorData );

                    childActorProperties->setName( GameActorUtil::childStr );
                    actorData->addChild( childActorProperties );
                }
            }
        }

        const auto components = actor->getComponents();
        for( const auto &component : components )
        {
            if( component )
            {
                auto componentData = workphone::static_pointer_cast<Properties>( component->toData() );
                if( componentData )
                {
                    componentData->setName( GameActorUtil::componentStr );

                    auto typeinfo = component->getTypeInfo();
                    auto className = typeManager->getName( typeinfo );

                    auto componentType = StringUtil::replaceAll( className, "class ", "" );
                    componentData->setProperty( GameActorUtil::componentTypeStr, componentType );

                    actorData->addChild( componentData );
                }
            }
        }

        return actorData;
    }

}  // namespace workphone::scene
