#include <WPLuabind/WPLuabindPCH.hpp>
#include <WPLuabind/Bindings/SceneBind.hpp>
#include <luabind/luabind.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <WPLuabind/ParamConverter.hpp>
#include <WPLua/LuaManager.hpp>
#include <WPLua/LuaObjectData.hpp>
#include <WPLuabind/ScriptObjectFunctions.hpp>
#include <WPLuaBind/Helpers/Projectile3Helper.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    using namespace scene;

    SmartPtr<IComponent> getComponentHash( SmartPtr<IGameActor> entity, lua_Integer hash )
    {
        if( !entity )
        {
            return nullptr;
        }

        auto typeInfo = static_cast<u32>( hash );
        auto handleId = hash;
        auto components = entity->getComponents();

        for( auto &component : components )
        {
            if( component )
            {
                if( component->derived( typeInfo ) )
                {
                    return component;
                }

                if( auto handle = component->getHandle() )
                {
                    if( handle->getId() == handleId )
                    {
                        return component;
                    }
                }
            }
        }

        return nullptr;
    }

    SmartPtr<IComponent> getComponent( SmartPtr<IGameActor> actor, const char *type )
    {
        if( !actor || !type )
        {
            return nullptr;
        }

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();
        if( factoryManager )
        {
            if( auto factory = factoryManager->getFactoryByName( type ) )
            {
                auto typeId = factory->getObjectTypeId();
                if( auto component = getComponentHash( actor, typeId ) )
                {
                    return component;
                }
            }
        }

        auto hash = StringUtil::getHash( type );
        return actor->getComponent( hash );
    }

    luabind::object addComponent( IGameActor *entity, const String &className )
    {
        if( !entity )
        {
            WP_LOG_ERROR( "addComponent: entity is null." );
            return luabind::object();
        }

        if( className.empty() )
        {
            WP_LOG_ERROR( "addComponent: className is empty." );
            return luabind::object();
        }

        auto applicationManager = core::ApplicationManager::instance();
        auto scriptManager =
            workphone::static_pointer_cast<LuaManager>( applicationManager->getScriptManager() );
        auto factoryManager = applicationManager->getFactoryManager();

        WP_ASSERT( scriptManager );
        WP_ASSERT( factoryManager );

        auto classNames = scriptManager->getClassNames();

        if( std::find( classNames.begin(), classNames.end(), className ) != classNames.end() )
        {
            auto component = factoryManager->make_ptr<Script>();
            if( !component )
            {
                WP_LOG_ERROR( "addComponent: failed to create Script." );
                return luabind::object();
            }

            component->setClassName( className );
            entity->addComponentInstance( component );
            component->load( nullptr );

            auto scriptObject =
                workphone::static_pointer_cast<LuaObjectData>( component->getScriptData() );
            if( !scriptObject )
            {
                WP_LOG_ERROR( "addComponent: script data unavailable for UserComponent." );
                return luabind::object();
            }

            return scriptObject->getObject();
        }
        auto component = factoryManager->createObjectFromType<IComponent>( className );
        if( !component )
        {
            WP_LOG_ERROR( "addComponent: no factory found for class." );
            return luabind::object();
        }

        entity->addComponentInstance( component );
        component->load( nullptr );
        return luabind::object( scriptManager->getLuaState(), component );
    }

    luabind::object addComponentById( IGameActor *entity, lua_Integer typeId )
    {
        if( !entity )
        {
            WP_LOG_ERROR( "addComponentById: entity is null." );
            return luabind::object();
        }

        auto applicationManager = core::ApplicationManager::instance();
        auto scriptManager =
            workphone::static_pointer_cast<LuaManager>( applicationManager->getScriptManager() );
        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto object = factoryManager->createById( static_cast<u32>( typeId ) );
        if( object )
        {
            auto component = workphone::static_pointer_cast<IComponent>( object );
            if( component )
            {
                entity->addComponentInstance( component );
                component->load( nullptr );
                return luabind::object( scriptManager->getLuaState(), component );
            }
        }

        return luabind::object();
    }

    lua_Integer IGameActor_getState( IGameActor *actor )
    {
        if( !actor )
        {
            return 0;
        }

        return static_cast<lua_Integer>( actor->getState() );
    }

    void IGameActor_setState( IGameActor *actor, lua_Integer state, bool cascade )
    {
        if( actor )
        {
            actor->setState( static_cast<IGameActor::State>( state ), cascade );
        }
    }

    lua_Integer ITransform_getTask( ITransform *transform )
    {
        return transform ? static_cast<lua_Integer>( transform->getTask() ) : 0;
    }

    void ITransform_setTask( ITransform *transform, lua_Integer task )
    {
        if( transform )
        {
            transform->setTask( static_cast<TaskId>( task ) );
        }
    }

    void bindScene( lua_State *L )
    {
        using namespace luabind;

        module( L )[class_<IGameEditor, ISharedObject>( "IEditor" )
                        .def( "getParent", &IGameEditor::getParent )
                        .def( "setParent", &IGameEditor::setParent )
                        .def( "getParentWindow", &IGameEditor::getParentWindow )
                        .def( "setParentWindow", &IGameEditor::setParentWindow )
                        .def( "getDebugWindow", &IGameEditor::getDebugWindow )
                        .def( "setDebugWindow", &IGameEditor::setDebugWindow )
                        .def( "isWindowVisible", &IGameEditor::isWindowVisible )
                        .def( "setWindowVisible", &IGameEditor::setWindowVisible )
                        .def( "updateSelection", &IGameEditor::updateSelection )
                        .def( "getEventListener", &IGameEditor::getEventListener )
                        .def( "setEventListener", &IGameEditor::setEventListener )
                        .def( "getWindowDragSource", &IGameEditor::getWindowDragSource )
                        .def( "setWindowDragSource", &IGameEditor::setWindowDragSource )
                        .def( "getWindowDropTarget", &IGameEditor::getWindowDropTarget )
                        .def( "setWindowDropTarget", &IGameEditor::setWindowDropTarget )
                        .def( "setDraggable", &IGameEditor::setDraggable )
                        .def( "isDraggable", &IGameEditor::isDraggable )
                        .def( "setDroppable", &IGameEditor::setDroppable )
                        .def( "isDroppable", &IGameEditor::isDroppable )
                        .def( "setHandleEvents", &IGameEditor::setHandleEvents )
                        .def( "getHandleEvents", &IGameEditor::getHandleEvents )
                        .def( "handleEvent", &IGameEditor::handleEvent )
                        .scope[def( "typeInfo", IGameEditor::typeInfo )]];

        module( L )[class_<IBuildDirector, IResource>( "IBuildDirector" )
                        .def( "getProperties", &IBuildDirector::getProperties )
                        .def( "setProperties", &IBuildDirector::setProperties )
                        .def( "getParent", &IBuildDirector::getParent )
                        .def( "setParent", &IBuildDirector::setParent )
                        .def( "addChild", &IBuildDirector::addChild )
                        .def( "removeChild", &IBuildDirector::removeChild )
                        .def( "removeChildren", &IBuildDirector::removeChildren )
                        .def( "findChild", &IBuildDirector::findChild )
                        .def( "getChildren", &IBuildDirector::getChildren )
                        .scope[def( "typeInfo", IBuildDirector::typeInfo )]];

        module( L )[class_<IGameActor::State>( "ActorState" )
                        .enum_( "constants" )
                            [value( "Create", static_cast<u32>( IGameActor::State::Create ) ),
                             value( "Destroyed", static_cast<u32>( IGameActor::State::Destroyed ) ),
                             value( "Edit", static_cast<u32>( IGameActor::State::Edit ) ),
                             value( "Play", static_cast<u32>( IGameActor::State::Play ) ),
                             value( "Count", static_cast<u32>( IGameActor::State::Count ) )]];

        module(
            L )[class_<IGameActor, IResource, SmartPtr<ISharedObject>>( "IActor" )
                    .def( "updateDirty", &IGameActor::updateDirty )
                    .def( "getLocalTransform", static_cast<Transform3<real_Num> ( IGameActor::* )()
                                                               const>( &IGameActor::getLocalTransform ) )
                    .def( "getWorldTransform", static_cast<Transform3<real_Num> ( IGameActor::* )()
                                                               const>( &IGameActor::getWorldTransform ) )
                    .def( "getLocalPosition", &IGameActor::getLocalPosition )
                    .def( "setLocalPosition", &IGameActor::setLocalPosition )
                    .def( "getLocalScale", &IGameActor::getLocalScale )
                    .def( "setLocalScale", &IGameActor::setLocalScale )
                    .def( "getLocalOrientation", &IGameActor::getLocalOrientation )
                    .def( "setLocalOrientation", &IGameActor::setLocalOrientation )
                    .def( "getLocalRotation", &IGameActor::getLocalRotation )
                    .def( "setLocalRotation", &IGameActor::setLocalRotation )
                    .def( "getPosition", &IGameActor::getPosition )
                    .def( "lookAt", static_cast<void ( IGameActor::* )( const Vector3<real_Num> & )>(
                                        &IGameActor::lookAt ) )
                    .def( "lookAt", static_cast<void ( IGameActor::* )( const Vector3<real_Num> &,
                                                                        const Vector3<real_Num> & )>(
                                        &IGameActor::lookAt ) )
                    .def( "setPosition", &IGameActor::setPosition )
                    .def( "getScale", &IGameActor::getScale )
                    .def( "setScale", &IGameActor::setScale )
                    .def( "getOrientation", &IGameActor::getOrientation )
                    .def( "setOrientation", &IGameActor::setOrientation )
                    .def( "getRotation", &IGameActor::getRotation )
                    .def( "setRotation", &IGameActor::setRotation )
                    .def( "levelWasLoaded", &IGameActor::levelWasLoaded )
                    .def( "hierarchyChanged", &IGameActor::hierarchyChanged )
                    .def( "childAdded", &IGameActor::childAdded )
                    .def( "childRemoved", &IGameActor::childRemoved )
                    .def( "childAddedInHierarchy", &IGameActor::childAddedInHierarchy )
                    .def( "childRemovedInHierarchy", &IGameActor::childRemovedInHierarchy )
                    .def( "destroyChildren", &IGameActor::destroyChildren )
                    .def( "getName", &IGameActor::getName )
                    .def( "setName", &IGameActor::setName )
                    .def( "getPerpetual", &IGameActor::getPerpetual )
                    .def( "setPerpetual", &IGameActor::setPerpetual )
                    .def( "addComponent", addComponent )
                    .def( "addComponentById", addComponentById )
                    .def( "addComponentInstance", &IGameActor::addComponentInstance )
                    .def( "removeComponentInstance", &IGameActor::removeComponentInstance )
                    .def( "getComponent", getComponent )
                    .def( "getComponentByType", getComponentHash )
                    .def( "hasComponent",
                          static_cast<bool ( IGameActor::* )( hash_type )>( &IGameActor::hasComponent ) )
                    .def( "getComponents", &IGameActor::getComponents )

                    .def( "getChildByIndex", &IGameActor::getChildByIndex )
                    .def( "getNumChildren", &IGameActor::getNumChildren )
                    .def( "getSiblingIndex", &IGameActor::getSiblingIndex )
                    .def( "setSiblingIndex", &IGameActor::setSiblingIndex )
                    .def( "setChildSiblingIndex", &IGameActor::setChildSiblingIndex )
                    .def( "triggerEnter", &IGameActor::triggerEnter )
                    .def( "triggerLeave", &IGameActor::triggerLeave )
                    .def( "componentLoaded", &IGameActor::componentLoaded )
                    .def( "compareTag", &IGameActor::compareTag )
                    .def( "getTags", &IGameActor::getTags )
                    .def( "setTags", &IGameActor::setTags )
                    .def( "addTag", &IGameActor::addTag )
                    .def( "removeTag", &IGameActor::removeTag )
                    .def( "hasTag", &IGameActor::hasTag )
                    .def( "clearTags", &IGameActor::clearTags )
                    .def( "getLayer", &IGameActor::getLayer )
                    .def( "setLayer", &IGameActor::setLayer )
                    .def( "getParent", &IGameActor::getParent )
                    .def( "setParent", &IGameActor::setParent )
                    .def( "addChild", &IGameActor::addChild )
                    .def( "removeChild", &IGameActor::removeChild )
                    .def( "removeChildren", &IGameActor::removeChildren )
                    .def( "findChild", &IGameActor::findChild )
                    .def( "getAllChildren", &IGameActor::getAllChildren )
                    .def( "isMine", &IGameActor::isMine )
                    .def( "setMine", &IGameActor::setMine )
                    .def( "isStatic", &IGameActor::isStatic )
                    .def( "setStatic", &IGameActor::setStatic )
                    .def( "isEnabledInScene", &IGameActor::isEnabledInScene )
                    .def( "isEnabled", &IGameActor::isEnabled )
                    .def( "setEnabled", &IGameActor::setEnabled )
                    .def( "isVisible", &IGameActor::isVisible )
                    .def( "setVisible", &IGameActor::setVisible )
                    .def( "isDirty", &IGameActor::isDirty )
                    .def( "setDirty", &IGameActor::setDirty )
                    .def( "isSmoothMotion", &IGameActor::isSmoothMotion )
                    .def( "setSmoothMotion", &IGameActor::setSmoothMotion )
                    .def( "getCollisionMask", &IGameActor::getCollisionMask )
                    .def( "setCollisionMask", &IGameActor::setCollisionMask )
                    .def( "updateTransform", &IGameActor::updateTransform )
                    .def( "getSceneRoot", &IGameActor::getSceneRoot )
                    .def( "getSceneLevel", &IGameActor::getSceneLevel )
                    .def( "getTransform", &IGameActor::getTransform )
                    .def( "setTransform", &IGameActor::setTransform )
                    .def( "setState", IGameActor_setState )
                    .def( "getState", IGameActor_getState )
                    .def( "getFlags", &IGameActor::getFlags )
                    .def( "setFlags", &IGameActor::setFlags )
                    .def( "getPreviousFlags", &IGameActor::getPreviousFlags )
                    .def( "setPreviousFlags", &IGameActor::setPreviousFlags )
                    .def( "getFlag", &IGameActor::getFlag )
                    .def( "setFlag", &IGameActor::setFlag )
                    .def( "getScene", &IGameActor::getScene )
                    .def( "setScene", &IGameActor::setScene )
                    .def( "updateVisibility", &IGameActor::updateVisibility )
                    .def( "updateOrder", &IGameActor::updateOrder )
                    .def( "updateComponentsState", &IGameActor::updateComponentsState )
                    .def( "getLocalAABB", &IGameActor::getLocalAABB )
                    .def( "setLocalAABB", &IGameActor::setLocalAABB )
                    .def( "getRadius", &IGameActor::getRadius )
                    .def( "setRadius", &IGameActor::setRadius )
                    .scope[def( "typeInfo", IGameActor::typeInfo )]];
    }

    void bindSceneManager( lua_State *L )
    {
        using namespace luabind;

        module(
            L )[luabind::class_<IGameScene::SpatialPartitioningConfig>( "SpatialPartitioningConfig" )
                    .property( "nearDistance", &IGameScene::SpatialPartitioningConfig::nearDistance )
                    .property( "midDistance", &IGameScene::SpatialPartitioningConfig::midDistance )
                    .property( "farDistance", &IGameScene::SpatialPartitioningConfig::farDistance )
                    .property( "nearUpdateRate", &IGameScene::SpatialPartitioningConfig::nearUpdateRate )
                    .property( "midUpdateRate", &IGameScene::SpatialPartitioningConfig::midUpdateRate )
                    .property( "farUpdateRate", &IGameScene::SpatialPartitioningConfig::farUpdateRate )
                    .property( "sleepDistance", &IGameScene::SpatialPartitioningConfig::sleepDistance )];

        module(
            L )[luabind::class_<IGameScene, ISharedObject>( "IGameScene" )
                    .def( "loadScene", &IGameScene::loadScene )
                    .def( "loadSceneDataStr", &IGameScene::loadSceneDataStr )
                    .def( "saveScene", static_cast<void ( IGameScene::* )( const String & )>(
                                           &IGameScene::saveScene ) )
                    .def( "saveScene", static_cast<void ( IGameScene::* )()>( &IGameScene::saveScene ) )
                    .def( "clear", &IGameScene::clear )
                    .def( "getLabel", &IGameScene::getLabel )
                    .def( "setLabel", &IGameScene::setLabel )
                    .def( "addActor", &IGameScene::addActor )
                    .def( "removeActor", &IGameScene::removeActor )
                    .def( "removeAllActors", &IGameScene::removeAllActors )
                    .def( "findActorByName", &IGameScene::findActorByName )
                    .def( "findActorById", &IGameScene::findActorById )
                    .def( "getActors", &IGameScene::getActors )
                    .def( "setActors", &IGameScene::setActors )
                    .def( "registerUpdates", &IGameScene::registerUpdates )
                    .def( "registerAllUpdates", &IGameScene::registerAllUpdates )
                    .def( "unregisterAll", &IGameScene::unregisterAll )
                    .def( "getFilePath", &IGameScene::getFilePath )
                    .def( "setFilePath", &IGameScene::setFilePath )];

        module( L )[class_<IGameManager, ISharedObject, SmartPtr<IGameManager>>( "IGameManager" )
                        .def( "loadScene", &IGameManager::loadScene )
                        .def( "loadSceneDataStr", &IGameManager::loadSceneDataStr )
                        .def( "clear", &IGameManager::clear )
                        .def( "getFsmManager", &IGameManager::getFsmManager )
                        .def( "setFsmManager", &IGameManager::setFsmManager )
                        .def( "getCurrentScene", &IGameManager::getCurrentScene )
                        .def( "setCurrentScene", &IGameManager::setCurrentScene )
                        .def( "getActors", &IGameManager::getActors )
                        .def( "createActor", &IGameManager::createActor )
                        .def( "destroyActor", &IGameManager::destroyActor )
                        .def( "destroyActors", &IGameManager::destroyActors )
                        .def( "play", &IGameManager::play )
                        .def( "edit", &IGameManager::edit )
                        .def( "stop", &IGameManager::stop )
                        .def( "addComponent", &IGameManager::addComponent )
                        .def( "removeComponent", &IGameManager::removeComponent )
                        .def( "getActor", &IGameManager::getActor )
                        .def( "getActorByName", &IGameManager::getActorByName )
                        .def( "getActorsByName", &IGameManager::getActorsByName )
                        .def( "getActorByFileId", &IGameManager::getActorByFileId )
                        .def( "createTransform", &IGameManager::createTransform )
                        .def( "destroyTransform", &IGameManager::destroyTransform )
                        .def( "queueProperties", &IGameManager::queueProperties )
                        .def( "getComponents",
                              static_cast<Array<SmartPtr<IComponent>> ( IGameManager::* )( u32 ) const>(
                                  &IGameManager::getComponents ) )
                        .def( "getNumActors", &IGameManager::getNumActors )
                        .def( "makeActorTransformsDirty", &IGameManager::makeActorTransformsDirty )
                        .def( "addDirtyActor", &IGameManager::addDirtyActor )
                        .def( "addDirtyTransform", &IGameManager::addDirtyTransform )
                        .def( "addDirtyComponent", &IGameManager::addDirtyComponent )
                        .scope[def( "typeInfo", IGameManager::typeInfo )]];

        module( L )[class_<ICameraManager, ISharedObject, SmartPtr<ICameraManager>>( "ICameraManager" )
                        .def( "addCamera", &ICameraManager::addCamera )
                        .def( "removeCamera", &ICameraManager::removeCamera )
                        .def( "findCamera", &ICameraManager::findCamera )
                        .def( "getCameras", &ICameraManager::getCameras )
                        .def( "reset", &ICameraManager::reset )
                        .def( "getEditorCameraPtr", &ICameraManager::getEditorCameraPtr )
                        .def( "getEditorCamera", &ICameraManager::getEditorCamera )
                        .def( "setEditorCamera", &ICameraManager::setEditorCamera )
                        .def( "isEditorCameraEnabled", &ICameraManager::isEditorCameraEnabled )
                        .def( "setEnabled", &ICameraManager::setEnabled )
                        .def( "isEnabled", &ICameraManager::isEnabled )
                        .def( "getEditorRTT", &ICameraManager::getEditorRTT )
                        .def( "setEditorRTT", &ICameraManager::setEditorRTT )];

        module( L )[class_<IGamePrefab, ISharedObject, SmartPtr<IGamePrefab>>( "IGamePrefab" )
                        .def( "createActor", &IGamePrefab::createActor )
                        .def( "getData", &IGamePrefab::getData )
                        .def( "setData", &IGamePrefab::setData )];
        module( L )[class_<IGamePrefabManager, ISharedObject, SmartPtr<IGamePrefabManager>>(
                        "IGamePrefabManager" )
                        .def( "createInstance", &IGamePrefabManager::createInstance )
                        .def( "loadActor", &IGamePrefabManager::loadActor )
                        .def( "loadPrefab", &IGamePrefabManager::loadPrefab )
                        .def( "savePrefab", &IGamePrefabManager::savePrefab )];

        module( L )[class_<IGameSceneBuilder, ISharedObject, SmartPtr<IGameSceneBuilder>>(
                        "IGameSceneBuilder" )
                        .def( "getScene", &IGameSceneBuilder::getScene )
                        .def( "setScene", &IGameSceneBuilder::setScene )
                        .def( "getDirector", &IGameSceneBuilder::getDirector )
                        .def( "setDirector", &IGameSceneBuilder::setDirector )
                        .def( "getActor", &IGameSceneBuilder::getActor )
                        .def( "setActor", &IGameSceneBuilder::setActor )
                        .def( "getActors", &IGameSceneBuilder::getActors )
                        .def( "setActors", &IGameSceneBuilder::setActors )];

        module(
            L )[class_<scene::IComponentSystem, ISharedObject, SmartPtr<scene::IComponentSystem>>(
                    "IComponentSystem" )
                    .def( "addComponent", &scene::IComponentSystem::addComponent )
                    .def( "removeComponent",
                          (void ( scene::IComponentSystem::* )(
                              SmartPtr<IComponent> ))&scene::IComponentSystem::removeComponent )
                    .def( "removeComponentById", (void ( scene::IComponentSystem::* )(
                                                     u32 ))&scene::IComponentSystem::removeComponent )
                    .def( "reserve", &scene::IComponentSystem::reserve )
                    .def( "getSize", &scene::IComponentSystem::getSize )
                    .def( "setSize", &scene::IComponentSystem::setSize )
                    .def( "getGrowSize", &scene::IComponentSystem::getGrowSize )
                    .def( "setGrowSize", &scene::IComponentSystem::setGrowSize )
                    .def( "isDirty", &scene::IComponentSystem::isDirty )
                    .def( "setDirty", &scene::IComponentSystem::setDirty )
                    .def( "makeDirty", &scene::IComponentSystem::makeDirty )
                    .def( "addDirtyComponent", &scene::IComponentSystem::addDirtyComponent )
                    .def( "removeDirtyComponent", &scene::IComponentSystem::removeDirtyComponent )];

        module( L )[class_<scene::ITransform, ISharedObject, SmartPtr<scene::ITransform>>( "ITransform" )
                        .def( "getActorPtr", &scene::ITransform::getActorPtr )
                        .def( "getActor", &scene::ITransform::getActor )
                        .def( "setActor", &scene::ITransform::setActor )
                        .def( "getLocalPosition", &scene::ITransform::getLocalPosition )
                        .def( "setLocalPosition", &scene::ITransform::setLocalPosition )
                        .def( "getLocalScale", &scene::ITransform::getLocalScale )
                        .def( "setLocalScale", &scene::ITransform::setLocalScale )
                        .def( "getLocalOrientation", &scene::ITransform::getLocalOrientation )
                        .def( "setLocalOrientation", &scene::ITransform::setLocalOrientation )
                        .def( "getLocalRotation", &scene::ITransform::getLocalRotation )
                        .def( "setLocalRotation", &scene::ITransform::setLocalRotation )
                        .def( "getPosition", &scene::ITransform::getPosition )
                        .def( "setPosition", &scene::ITransform::setPosition )
                        .def( "getScale", &scene::ITransform::getScale )
                        .def( "setScale", &scene::ITransform::setScale )
                        .def( "getOrientation", &scene::ITransform::getOrientation )
                        .def( "setOrientation", &scene::ITransform::setOrientation )
                        .def( "getRotation", &scene::ITransform::getRotation )
                        .def( "setRotation", &scene::ITransform::setRotation )
                        .def( "getLocalTransform", &scene::ITransform::getLocalTransform )
                        .def( "setLocalTransform", &scene::ITransform::setLocalTransform )
                        .def( "getWorldTransform", &scene::ITransform::getWorldTransform )
                        .def( "setWorldTransform", &scene::ITransform::setWorldTransform )
                        .def( "updateWorldFromLocal", &scene::ITransform::updateWorldFromLocal )
                        .def( "updateLocalFromWorld", &scene::ITransform::updateLocalFromWorld )
                        .def( "isLocalDirty", &scene::ITransform::isLocalDirty )
                        .def( "setLocalDirty", &scene::ITransform::setLocalDirty )
                        .def( "isDirty", &scene::ITransform::isDirty )
                        .def( "setDirty", &scene::ITransform::setDirty )
                        .def( "getSmoothMotion", &scene::ITransform::getSmoothMotion )
                        .def( "setSmoothMotion", &scene::ITransform::setSmoothMotion )
                        .def( "parentChanged", &scene::ITransform::parentChanged )
                        .def( "getFrameTime", &scene::ITransform::getFrameTime )
                        .def( "setFrameTime", &scene::ITransform::setFrameTime )
                        .def( "getFrameDeltaTime", &scene::ITransform::getFrameDeltaTime )
                        .def( "setFrameDeltaTime", &scene::ITransform::setFrameDeltaTime )
                        .def( "getTask", ITransform_getTask )
                        .def( "setTask", ITransform_setTask )
                        .def( "lookAt", (void ( scene::ITransform::* )(
                                            const Vector3<real_Num> & ))&scene::ITransform::lookAt )
                        .def( "getForward", &scene::ITransform::getForward )
                        .def( "getUp", &scene::ITransform::getUp )
                        .def( "getRight", &scene::ITransform::getRight )
                        .def( "getFlags", &scene::ITransform::getFlags )
                        .def( "setFlags", &scene::ITransform::setFlags )
                        .def( "addTransformReference", &scene::ITransform::addTransformReference )
                        .def( "removeTransformReference", &scene::ITransform::removeTransformReference )
                        .def( "getTransformReferences", &scene::ITransform::getTransformReferences )
                        .def( "setTransformReferences", &scene::ITransform::setTransformReferences )];
    }
} // namespace workphone
