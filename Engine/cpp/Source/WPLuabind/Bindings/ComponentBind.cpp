#include <WPLuabind/WPLuabindPCH.hpp>
#include <WPLuabind/Bindings/ComponentBind.hpp>
#include <luabind/luabind.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <WPLuabind/ParamConverter.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/Components/AnimatedMaterial.hpp>
#include <Workphone/Scene/Components/Animation.hpp>
#include <Workphone/Scene/Components/Animator.hpp>
#include <Workphone/Scene/Components/AudioEmitter.hpp>
#include <Workphone/Scene/Components/Billboard.hpp>
#include <Workphone/Scene/Components/Billboards.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
#include <Workphone/Scene/Components/Camera/CameraController.hpp>
#include <Workphone/Scene/Components/Camera/CameraFollow.hpp>
#include <Workphone/Scene/Components/Camera/CameraTarget.hpp>
#include <Workphone/Scene/Components/Camera/EditorCameraController.hpp>
#include <Workphone/Scene/Components/Camera/FPSCameraController.hpp>
#include <Workphone/Scene/Components/Camera/SphericalCameraController.hpp>
#include <Workphone/Scene/Components/Camera/ThirdPersonCameraController.hpp>
#include <Workphone/Scene/Components/Camera/VehicleCameraController.hpp>
#include <Workphone/Scene/Components/CharacterController.hpp>
#include <Workphone/Scene/Components/CarController.hpp>
#include <Workphone/Scene/Components/Collision.hpp>
#include <Workphone/Scene/Components/CollisionBox.hpp>
#include <Workphone/Scene/Components/CollisionMesh.hpp>
#include <Workphone/Scene/Components/CollisionPlane.hpp>
#include <Workphone/Scene/Components/CollisionSphere.hpp>
#include <Workphone/Scene/Components/CollisionTerrain.hpp>
#include <Workphone/Scene/Components/Constraint.hpp>
#include <Workphone/Scene/Components/ComponentEvent.hpp>
#include <Workphone/Scene/Components/ComponentEventListener.hpp>
#include <Workphone/Scene/Components/Cubemap.hpp>
#include <Workphone/Scene/Components/FiniteStateMachine.hpp>
#include <Workphone/Scene/Components/Light.hpp>
#include <Workphone/Scene/Components/Material.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Scene/Components/NetworkListener.hpp>
#include <Workphone/Scene/Components/NetworkPlayer.hpp>
#include <Workphone/Scene/Components/NetworkStream.hpp>
#include <Workphone/Scene/Components/NetworkView.hpp>
#include <Workphone/Scene/Components/ParticleSystem.hpp>
#include <Workphone/Scene/Components/Renderer.hpp>
#include <Workphone/Scene/Components/RenderTexture.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Scene/Components/RigidbodyListener.hpp>
#include <Workphone/Scene/Components/Skybox.hpp>
#include <Workphone/Scene/Components/SubComponent.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainGrassLayer.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainBlendMap.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainLayer.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainTreeLayer.hpp>
#include <Workphone/Scene/Components/Script.hpp>
#include <Workphone/Scene/Components/VehicleController.hpp>
#include <Workphone/Scene/Components/VideoPlayer.hpp>
#include <Workphone/Scene/Components/WheelController.hpp>
#include <Workphone/Interface/Graphics/ISkybox.hpp>
#include <WPLuabind/Helpers/DestructibleHelper.hpp>

namespace workphone
{
    Parameter Component_handleEvent( scene::IComponent *component, lua_Integer eventType,
                                     lua_Integer eventValue, const Array<Parameter> &arguments,
                                     SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                                     SmartPtr<IEvent> event )
    {
        return component->handleEvent( static_cast<EventType>( eventType ), eventValue, arguments,
                                       sender, object, event );
    }

    lua_Integer Component_getState( const scene::IComponent *component )
    {
        return static_cast<lua_Integer>( component->getState() );
    }

    void Component_setState( scene::IComponent *component, lua_Integer state )
    {
        component->setState( static_cast<scene::IComponent::State>( state ) );
    }

    SmartPtr<ISharedObject> Component_getStateByTypeId( scene::IComponent *component, u32 typeId )
    {
        return component->getStateByTypeId( typeId );
    }

    lua_Integer Renderer_getCastShadows( const scene::Renderer *renderer )
    {
        return static_cast<lua_Integer>( renderer->getCastShadows() );
    }

    void Renderer_setCastShadows( scene::Renderer *renderer, lua_Integer value )
    {
        renderer->setCastShadows( static_cast<scene::Renderer::CastShadows>( value ) );
    }

    lua_Integer Renderer_getRecieveShadows( const scene::Renderer *renderer )
    {
        return static_cast<lua_Integer>( renderer->getRecieveShadows() );
    }

    void Renderer_setRecieveShadows( scene::Renderer *renderer, lua_Integer value )
    {
        renderer->setRecieveShadows( static_cast<scene::Renderer::RecieveShadows>( value ) );
    }

    lua_Integer Renderer_getReflections( const scene::Renderer *renderer )
    {
        return static_cast<lua_Integer>( renderer->getReflections() );
    }

    void Renderer_setReflections( scene::Renderer *renderer, lua_Integer value )
    {
        renderer->setReflections( static_cast<scene::Renderer::Reflections>( value ) );
    }

    lua_Integer Renderer_getOcculsion( const scene::Renderer *renderer )
    {
        return static_cast<lua_Integer>( renderer->getOcculsion() );
    }

    void Renderer_setOcculsion( scene::Renderer *renderer, lua_Integer value )
    {
        renderer->setOcculsion( static_cast<scene::Renderer::Occulsion>( value ) );
    }

    lua_Integer WheelController_getTireModel( const scene::WheelController *controller )
    {
        return static_cast<lua_Integer>( controller->getTireModel() );
    }

    void WheelController_setTireModel( scene::WheelController *controller, lua_Integer value )
    {
        controller->setTireModel( static_cast<TireModel>( value ) );
    }

    u32 CharacterController_getActorFlags( scene::CharacterController *controller )
    {
        return static_cast<u32>( controller->getActorFlags() );
    }

    void CharacterController_setActorFlag( scene::CharacterController *controller, u32 flag, bool value )
    {
        controller->setActorFlag( static_cast<physics::ActorFlagEnum>( flag ), value );
    }

    u32 Constraint_getType( scene::Constraint *constraint )
    {
        return static_cast<u32>( constraint->getType() );
    }

    void Constraint_setType( scene::Constraint *constraint, u32 type )
    {
        constraint->setType( static_cast<scene::Constraint::Type>( type ) );
    }

    u32 Constraint_getAxisX( scene::Constraint *constraint )
    {
        return static_cast<u32>( constraint->getAxisX() );
    }

    void Constraint_setAxisX( scene::Constraint *constraint, u32 axisX )
    {
        constraint->setAxisX( static_cast<physics::D6MotionEnum>( axisX ) );
    }

    u32 Constraint_getAxisY( scene::Constraint *constraint )
    {
        return static_cast<u32>( constraint->getAxisY() );
    }

    void Constraint_setAxisY( scene::Constraint *constraint, u32 axisY )
    {
        constraint->setAxisY( static_cast<physics::D6MotionEnum>( axisY ) );
    }

    u32 Constraint_getAxisZ( scene::Constraint *constraint )
    {
        return static_cast<u32>( constraint->getAxisZ() );
    }

    void Constraint_setAxisZ( scene::Constraint *constraint, u32 axisZ )
    {
        constraint->setAxisZ( static_cast<physics::D6MotionEnum>( axisZ ) );
    }

    u32 Constraint_getSwing1( scene::Constraint *constraint )
    {
        return static_cast<u32>( constraint->getSwing1() );
    }

    void Constraint_setSwing1( scene::Constraint *constraint, u32 swing1 )
    {
        constraint->setSwing1( static_cast<physics::D6MotionEnum>( swing1 ) );
    }

    u32 Constraint_getSwing2( scene::Constraint *constraint )
    {
        return static_cast<u32>( constraint->getSwing2() );
    }

    void Constraint_setSwing2( scene::Constraint *constraint, u32 swing2 )
    {
        constraint->setSwing2( static_cast<physics::D6MotionEnum>( swing2 ) );
    }

    u32 Constraint_getTwist( scene::Constraint *constraint )
    {
        return static_cast<u32>( constraint->getTwist() );
    }

    void Constraint_setTwist( scene::Constraint *constraint, u32 twist )
    {
        constraint->setTwist( static_cast<physics::D6MotionEnum>( twist ) );
    }

    void bindComponent( lua_State *L )
    {
        using namespace scene;
        using namespace luabind;

        // Bind the State enum class
        module( L )[class_<IComponent::State>( "State" ).enum_(
            "constants" )[value( "None", IComponent::State::None ),
                          value( "Create", IComponent::State::Create ),
                          value( "Destroyed", IComponent::State::Destroyed ),
                          value( "Edit", IComponent::State::Edit ),
                          value( "Play", IComponent::State::Play ),
                          value( "Reset", IComponent::State::Reset ),
                          value( "Count", IComponent::State::Count )]];

        module( L )[class_<ISubComponent, IResource, SmartPtr<ISubComponent>>( "ISubComponent" )
                        .def( "getParentComponent", &ISubComponent::getParentComponent )
                        .def( "setParentComponent", &ISubComponent::setParentComponent )
                        .def( "getParent", &ISubComponent::getParent )
                        .def( "setParent", &ISubComponent::setParent )
                        .def( "addChildByType", &ISubComponent::addChildByType )
                        .def( "addChild", &ISubComponent::addChild )
                        .def( "removeChild", &ISubComponent::removeChild )
                        .def( "getChildren", &ISubComponent::getChildren )
                        .scope[def( "typeInfo", ISubComponent::typeInfo )]];

        module( L )[class_<SubComponent, ISubComponent, SmartPtr<SubComponent>>( "SubComponent" )
                        .scope[def( "typeInfo", SubComponent::typeInfo )]];

        module( L )[class_<IComponent, IResource, SmartPtr<IComponent>>( "IComponent" )
                        .def( "updateFlags", &IComponent::updateFlags )
                        .def( "getActorPtr", &IComponent::getActorPtr )
                        .def( "getActor", &IComponent::getActor )
                        .def( "setActor", &IComponent::setActor )
                        .def( "getProperties", &IComponent::getProperties )
                        .def( "setProperties", &IComponent::setProperties )
                        .def( "updateTransform",
                              static_cast<void ( IComponent::* )()>( &IComponent::updateTransform ) )
                        .def( "updateTransform",
                              static_cast<void ( IComponent::* )( const Transform3<real_Num> & )>(
                                  &IComponent::updateTransform ) )
                        .def( "updateVisibility", &IComponent::updateVisibility )
                        .def( "updateOrder", &IComponent::updateOrder )
                        .def( "updateMaterials", &IComponent::updateMaterials )
                        .def( "updateDependentComponents", &IComponent::updateDependentComponents )
                        .def( "updateSmoothTransformState", &IComponent::updateSmoothTransformState )
                        .def( "setState", Component_setState )
                        .def( "getState", Component_getState )
                        .def( "getComponentFlags", &IComponent::getComponentFlags )
                        .def( "setComponentFlags", &IComponent::setComponentFlags )
                        .def( "setComponentFlag", &IComponent::setComponentFlag )
                        .def( "getComponentFlag", &IComponent::getComponentFlag )
                        .def( "setEnabled", &IComponent::setEnabled )
                        .def( "isEnabled", &IComponent::isEnabled )
                        .def( "getEvents", &IComponent::getEvents )
                        .def( "setEvents", &IComponent::setEvents )
                        .def( "addEvent", &IComponent::addEvent )
                        .def( "removeEvent", &IComponent::removeEvent )
                        .def( "removeEvents", &IComponent::removeEvents )
                        .def( "addSubComponent", &IComponent::addSubComponent )
                        .def( "removeSubComponent", &IComponent::removeSubComponent )
                        .def( "removeSubComponentByIndex", &IComponent::removeSubComponentByIndex )
                        .def( "getNumSubComponents", &IComponent::getNumSubComponents )
                        .def( "getSubComponentByIndex", &IComponent::getSubComponentByIndex )
                        .def( "getSubComponents", &IComponent::getSubComponents )
                        .def( "setSubComponents", &IComponent::setSubComponents )
                        .def( "getParent", &IComponent::getParent )
                        .def( "setParent", &IComponent::setParent )
                        .def( "compareTag", &IComponent::compareTag )
                        .def( "handleEvent", Component_handleEvent )
                        .def( "addState", &IComponent::addState )
                        .def( "removeState", &IComponent::removeState )
                        .def( "getStateByTypeId", Component_getStateByTypeId )
                        .def( "getStates", &IComponent::getStates )
                        .def( "getBoundingBox", &IComponent::getBoundingBox )
                        .def( "getComponentSystemPtr", &IComponent::getComponentSystemPtr )
                        .def( "getComponentSystem", &IComponent::getComponentSystem )
                        .def( "setComponentSystem", &IComponent::setComponentSystem )
                        .def( "getFsmPtr", &IComponent::getFsmPtr )
                        .def( "getFsm", &IComponent::getFsm )
                        .def( "setFsm", &IComponent::setFsm )
                        .def( "updateStatic", &IComponent::updateStatic )
                        .scope[def( "typeInfo", IComponent::typeInfo )]];

        module( L )[class_<Component, IComponent, SmartPtr<Component>>( "Component" )
                        .scope[def( "typeInfo", Component::typeInfo )]];

        module( L )[class_<IComponentEvent::EventType>( "ComponentEventType" )
                        .enum_( "constants" )[value( "Loading", IComponentEvent::EventType::Loading ),
                                              value( "Object", IComponentEvent::EventType::Object ),
                                              value( "UI", IComponentEvent::EventType::UI ),
                                              value( "Count", IComponentEvent::EventType::Count )]];

        module( L )[class_<IComponentEvent, IEvent, SmartPtr<IComponentEvent>>( "IComponentEvent" )
                        .def( "addListener", &IComponentEvent::addListener )
                        .def( "removeListener", &IComponentEvent::removeListener )
                        .def( "removeListeners", &IComponentEvent::removeListeners )
                        .def( "getListeners", &IComponentEvent::getListeners )
                        .def( "setListeners", &IComponentEvent::setListeners )
                        .def( "getLabel", &IComponentEvent::getLabel )
                        .def( "setLabel", &IComponentEvent::setLabel )
                        .def( "getEventHash", &IComponentEvent::getEventHash )
                        .def( "setEventHash", &IComponentEvent::setEventHash )
                        .scope[def( "typeInfo", IComponentEvent::typeInfo )]];

        module( L )[class_<IComponentEventListener::Type>( "ComponentEventListenerType" )
                        .enum_( "constants" )[value( "Loading", IComponentEventListener::Type::Loading ),
                                              value( "Object", IComponentEventListener::Type::Object ),
                                              value( "UI", IComponentEventListener::Type::UI ),
                                              value( "Count", IComponentEventListener::Type::Count )]];

        module( L )[class_<IComponentEventListener, IEventListener, SmartPtr<IComponentEventListener>>(
                        "IComponentEventListener" )
                        .def( "getActor", &IComponentEventListener::getActor )
                        .def( "setActor", &IComponentEventListener::setActor )
                        .def( "getComponent", &IComponentEventListener::getComponent )
                        .def( "setComponent", &IComponentEventListener::setComponent )
                        .def( "getFunction", &IComponentEventListener::getFunction )
                        .def( "setFunction", &IComponentEventListener::setFunction )
                        .def( "getProperties", &IComponentEventListener::getProperties )
                        .def( "setProperties", &IComponentEventListener::setProperties )
                        .scope[def( "typeInfo", IComponentEventListener::typeInfo )]];

        module( L )[class_<Material, Component, SmartPtr<Material>>( "Material" )
                        .def( "getMaterialPath", &Material::getMaterialPath )
                        .def( "setMaterialPath", &Material::setMaterialPath )
                        .def( "getMaterial", &Material::getMaterial )
                        .def( "setMaterial", &Material::setMaterial )
                        .def( "updateMaterial", &Material::updateMaterial )
                        .def( "updateDependentComponents", &Material::updateDependentComponents )
                        .def( "updateImageComponent", &Material::updateImageComponent )
                        .def( "getIndex", &Material::getIndex )
                        .def( "setIndex", &Material::setIndex )
                        .scope[def( "typeInfo", Material::typeInfo )]];

        module( L )[class_<Script, Component, SmartPtr<Script>>( "Script" )
                        .def( "getInvoker", &Script::getInvoker )
                        .def( "setInvoker", &Script::setInvoker )
                        .def( "getReceiver", &Script::getReceiver )
                        .def( "setReceiver", &Script::setReceiver )
                        .def( "getProperties", &Script::getProperties )
                        .def( "setProperties", &Script::setProperties )
                        .def( "getUpdateInEditMode", &Script::getUpdateInEditMode )
                        .def( "setUpdateInEditMode", &Script::setUpdateInEditMode )
                        .def( "getUpdateInPlayMode", &Script::getUpdateInPlayMode )
                        .def( "setUpdateInPlayMode", &Script::setUpdateInPlayMode )
                        .def( "getScriptClass", &Script::getScriptClass )
                        .def( "setScriptClass", &Script::setScriptClass )
                        .def( "getClassName", &Script::getClassName )
                        .def( "setClassName", &Script::setClassName )
                        .def( "generate", &Script::generate )
                        .scope[def( "typeInfo", Script::typeInfo )]];

        module(
            L )[class_<TerrainSystem, Component, SmartPtr<TerrainSystem>>( "TerrainSystem" )
                    .def( "load", &TerrainSystem::load )
                    .def( "unload", &TerrainSystem::unload )
                    .def( "getProperties", &TerrainSystem::getProperties )
                    .def( "setProperties", &TerrainSystem::setProperties )
                    .def( "updateTransform", &TerrainSystem::updateTransform )
                    .def( "calculateNumLayers", &TerrainSystem::calculateNumLayers )
                    .def( "getNumLayers", &TerrainSystem::getNumLayers )
                    .def( "addLayer", &TerrainSystem::addLayer )
                    .def( "addTreeLayer", &TerrainSystem::addTreeLayer )
                    .def( "addGrassLayer", &TerrainSystem::addGrassLayer )
                    .def( "removeLayerIndex",
                          static_cast<void ( TerrainSystem::* )( s32 )>( &TerrainSystem::removeLayer ) )
                    .def( "removeLayerPtr",
                          static_cast<void ( TerrainSystem::* )( SmartPtr<TerrainLayer> )>(
                              &TerrainSystem::removeLayer ) )
                    .def( "setNumLayers", &TerrainSystem::setNumLayers )
                    .def( "resizeLayermap", &TerrainSystem::resizeLayermap )
                    .def( "updateLayers", &TerrainSystem::updateLayers )
                    .def( "getHeightMap", &TerrainSystem::getHeightMap )
                    .def( "setHeightMap", &TerrainSystem::setHeightMap )
                    .def( "rebuild", &TerrainSystem::rebuild )
                    .def( "generateHeightMap", &TerrainSystem::generateHeightMap )
                    .def( "getHeightScale", &TerrainSystem::getHeightScale )
                    .def( "setHeightScale", &TerrainSystem::setHeightScale )
                    .def( "getHeightMapSize", &TerrainSystem::getHeightMapSize )
                    .def( "setHeightMapSize", &TerrainSystem::setHeightMapSize )
                    .def( "getShowWireframe", &TerrainSystem::getShowWireframe )
                    .def( "setShowWireframe", &TerrainSystem::setShowWireframe )
                    .def( "getGeneratedHeightMapWidth", &TerrainSystem::getGeneratedHeightMapWidth )
                    .def( "setGeneratedHeightMapWidth", &TerrainSystem::setGeneratedHeightMapWidth )
                    .def( "getGeneratedHeightMapHeight", &TerrainSystem::getGeneratedHeightMapHeight )
                    .def( "setGeneratedHeightMapHeight", &TerrainSystem::setGeneratedHeightMapHeight )
                    .def( "getGeneratedHeightMapValueScale",
                          &TerrainSystem::getGeneratedHeightMapValueScale )
                    .def( "setGeneratedHeightMapValueScale",
                          &TerrainSystem::setGeneratedHeightMapValueScale )
                    .def( "getGeneratedHeightMapType", &TerrainSystem::getGeneratedHeightMapType )
                    .def( "setGeneratedHeightMapType", &TerrainSystem::setGeneratedHeightMapType )
                    .def( "getDefaultMetalnessValue", &TerrainSystem::getDefaultMetalnessValue )
                    .def( "setDefaultMetalnessValue", &TerrainSystem::setDefaultMetalnessValue )
                    .def( "getTreesEnabled", &TerrainSystem::getTreesEnabled )
                    .def( "setTreesEnabled", &TerrainSystem::setTreesEnabled )
                    .def( "getTreeDensity", &TerrainSystem::getTreeDensity )
                    .def( "setTreeDensity", &TerrainSystem::setTreeDensity )
                    .def( "getGrassEnabled", &TerrainSystem::getGrassEnabled )
                    .def( "setGrassEnabled", &TerrainSystem::setGrassEnabled )
                    .def( "getGrassDensity", &TerrainSystem::getGrassDensity )
                    .def( "setGrassDensity", &TerrainSystem::setGrassDensity )
                    .def( "getPreviewTreeCount", &TerrainSystem::getPreviewTreeCount )
                    .def( "setPreviewTreeCount", &TerrainSystem::setPreviewTreeCount )
                    .def( "getGeneratedTreeCount", &TerrainSystem::getGeneratedTreeCount )
                    .def( "setGeneratedTreeCount", &TerrainSystem::setGeneratedTreeCount )
                    .def( "getTreePrefabName", &TerrainSystem::getTreePrefabName )
                    .def( "setTreePrefabName", &TerrainSystem::setTreePrefabName )
                    .def( "getTerrain", &TerrainSystem::getTerrain )
                    .def( "setTerrain", &TerrainSystem::setTerrain )
                    .scope[def( "typeInfo", TerrainSystem::typeInfo )]];

        module( L )[class_<TerrainLayer, SubComponent, SmartPtr<TerrainLayer>>( "TerrainLayer" )
                        .def( "load", &TerrainLayer::load )
                        .def( "unload", &TerrainLayer::unload )
                        .def( "getProperties", &TerrainLayer::getProperties )
                        .def( "setProperties", &TerrainLayer::setProperties )
                        .def( "getBaseTexture", &TerrainLayer::getBaseTexture )
                        .def( "setBaseTexture", &TerrainLayer::setBaseTexture )
                        .def( "getIndex", &TerrainLayer::getIndex )
                        .def( "setIndex", &TerrainLayer::setIndex )
                        .scope[def( "typeInfo", TerrainLayer::typeInfo )]];

        module(
            L )[class_<TerrainTreeLayer, SubComponent, SmartPtr<TerrainTreeLayer>>( "TerrainTreeLayer" )
                    .def( "load", &TerrainTreeLayer::load )
                    .def( "unload", &TerrainTreeLayer::unload )
                    .def( "getProperties", &TerrainTreeLayer::getProperties )
                    .def( "setProperties", &TerrainTreeLayer::setProperties )
                    .def( "getBaseTexture", &TerrainTreeLayer::getBaseTexture )
                    .def( "setBaseTexture", &TerrainTreeLayer::setBaseTexture )
                    .def( "getIndex", &TerrainTreeLayer::getIndex )
                    .def( "setIndex", &TerrainTreeLayer::setIndex )
                    .def( "getPrefab", &TerrainTreeLayer::getPrefab )
                    .def( "setPrefab", &TerrainTreeLayer::setPrefab )
                    .def( "getPrefabPath", &TerrainTreeLayer::getPrefabPath )
                    .def( "setPrefabPath", &TerrainTreeLayer::setPrefabPath )
                    .def( "getDensity", &TerrainTreeLayer::getDensity )
                    .def( "setDensity", &TerrainTreeLayer::setDensity )
                    .scope[def( "typeInfo", TerrainTreeLayer::typeInfo )]];

        module( L )[class_<TerrainGrassLayer, SubComponent, SmartPtr<TerrainGrassLayer>>(
                        "TerrainGrassLayer" )
                        .def( "load", &TerrainGrassLayer::load )
                        .def( "unload", &TerrainGrassLayer::unload )
                        .def( "getProperties", &TerrainGrassLayer::getProperties )
                        .def( "setProperties", &TerrainGrassLayer::setProperties )
                        .def( "getBaseTexture", &TerrainGrassLayer::getBaseTexture )
                        .def( "setBaseTexture", &TerrainGrassLayer::setBaseTexture )
                        .def( "getIndex", &TerrainGrassLayer::getIndex )
                        .def( "setIndex", &TerrainGrassLayer::setIndex )
                        .def( "getPrefab", &TerrainGrassLayer::getPrefab )
                        .def( "setPrefab", &TerrainGrassLayer::setPrefab )
                        .def( "getPrefabPath", &TerrainGrassLayer::getPrefabPath )
                        .def( "setPrefabPath", &TerrainGrassLayer::setPrefabPath )
                        .def( "getDensity", &TerrainGrassLayer::getDensity )
                        .def( "setDensity", &TerrainGrassLayer::setDensity )
                        .scope[def( "typeInfo", TerrainGrassLayer::typeInfo )]];

        module( L )[class_<Light, Component, SmartPtr<Light>>( "Light" )
                        .def( "getLightType",
                              reinterpret_cast<u32 ( Light::* )() const>( &Light::getLightType ) )
                        .def( "setLightType",
                              reinterpret_cast<void ( Light::* )( u32 )>( &Light::setLightType ) )
                        .def( "getDiffuseColour", &Light::getDiffuseColour )
                        .def( "setDiffuseColour", &Light::setDiffuseColour )
                        .def( "getSpecularColour", &Light::getSpecularColour )
                        .def( "setSpecularColour", &Light::setSpecularColour )
                        .def( "getIntensity", &Light::getIntensity )
                        .def( "setIntensity", &Light::setIntensity )
                        .def( "getAttenuationRange", &Light::getAttenuationRange )
                        .def( "setAttenuationRange", &Light::setAttenuationRange )
                        .def( "getAttenuationConstant", &Light::getAttenuationConstant )
                        .def( "setAttenuationConstant", &Light::setAttenuationConstant )
                        .def( "getAttenuationLinear", &Light::getAttenuationLinear )
                        .def( "setAttenuationLinear", &Light::setAttenuationLinear )
                        .def( "getAttenuationQuadratic", &Light::getAttenuationQuadratic )
                        .def( "setAttenuationQuadratic", &Light::setAttenuationQuadratic )
                        .def( "getDebugColour", &Light::getDebugColour )
                        .def( "setDebugColour", &Light::setDebugColour )
                        .def( "isVisible", &Light::isVisible )
                        .def( "setVisible", &Light::setVisible )
                        .def( "getLight", &Light::getLight )
                        .def( "setLight", &Light::setLight )
                        .def( "getSceneNode", static_cast<SmartPtr<render::IGraphicsSceneNode> &(
                                                  Light::*)()>( &Light::getSceneNode ) )
                        .def( "setSceneNode", &Light::setSceneNode )
                        .scope[def( "typeInfo", Light::typeInfo )]];

        module( L )[class_<Collision, Component, SmartPtr<Collision>>( "Collision" )
                        .def( "load", &Collision::load )
                        .def( "unload", &Collision::unload )
                        .def( "getChildObjects", &Collision::getChildObjects )
                        .def( "getProperties", &Collision::getProperties )
                        .def( "setProperties", &Collision::setProperties )
                        .def( "getExtents", &Collision::getExtents )
                        .def( "setExtents", &Collision::setExtents )
                        .def( "getPosition", &Collision::getPosition )
                        .def( "setPosition", &Collision::setPosition )
                        .def( "getRadius", &Collision::getRadius )
                        .def( "setRadius", &Collision::setRadius )
                        .def( "getStaticFriction", &Collision::getStaticFriction )
                        .def( "setStaticFriction", &Collision::setStaticFriction )
                        .def( "getDynamicFriction", &Collision::getDynamicFriction )
                        .def( "setDynamicFriction", &Collision::setDynamicFriction )
                        .def( "getRestitution", &Collision::getRestitution )
                        .def( "setRestitution", &Collision::setRestitution )
                        .def( "getRigidBody", &Collision::getRigidBody )
                        .def( "setRigidBody", &Collision::setRigidBody )
                        .def( "getMaterial", &Collision::getMaterial )
                        .def( "setMaterial", &Collision::setMaterial )
                        .def( "getShape", &Collision::getShape )
                        .def( "setShape", &Collision::setShape )
                        .def( "isTrigger", &Collision::isTrigger )
                        .def( "setTrigger", &Collision::setTrigger )
                        .def( "isValid", &Collision::isValid )
                        .def( "updateTransform", &Collision::updateTransform )
                        .def( "getBoundingBox", &Collision::getBoundingBox )
                        .scope[def( "typeInfo", Collision::typeInfo )]];

        module( L )[class_<CollisionBox, Collision, SmartPtr<CollisionBox>>( "CollisionBox" )
                        .def( "load", &CollisionBox::load )
                        .def( "unload", &CollisionBox::unload )
                        .def( "reload", &CollisionBox::reload )
                        .def( "getProperties", &CollisionBox::getProperties )
                        .def( "setProperties", &CollisionBox::setProperties )
                        .def( "setExtents", &CollisionBox::setExtents )
                        .def( "isValid", &CollisionBox::isValid )
                        .def( "updateTransform", &CollisionBox::updateTransform )
                        .def( "getBoundingBox", &CollisionBox::getBoundingBox )
                        .scope[def( "typeInfo", CollisionBox::typeInfo )]];

        module( L )[class_<scene::CollisionMesh, Collision, SmartPtr<scene::CollisionMesh>>(
                        "CollisionMesh" )
                        .def( "load", &scene::CollisionMesh::load )
                        .def( "unload", &scene::CollisionMesh::unload )
                        .def( "getProperties", &scene::CollisionMesh::getProperties )
                        .def( "setProperties", &scene::CollisionMesh::setProperties )
                        .def( "getMeshPath", &scene::CollisionMesh::getMeshPath )
                        .def( "setMeshPath", &scene::CollisionMesh::setMeshPath )
                        .def( "getMeshResource", &scene::CollisionMesh::getMeshResource )
                        .def( "setMeshResource", &scene::CollisionMesh::setMeshResource )
                        .def( "isConvex", &scene::CollisionMesh::isConvex )
                        .def( "setConvex", &scene::CollisionMesh::setConvex )
                        .def( "isValid", &scene::CollisionMesh::isValid )
                        .scope[def( "typeInfo", scene::CollisionMesh::typeInfo )]];

        module( L )[class_<CollisionPlane, Collision, SmartPtr<CollisionPlane>>( "CollisionPlane" )
                        .def( "load", &CollisionPlane::load )
                        .def( "unload", &CollisionPlane::unload )
                        .scope[def( "typeInfo", CollisionPlane::typeInfo )]];

        module( L )[class_<CollisionSphere, Collision, SmartPtr<CollisionSphere>>( "CollisionSphere" )
                        .def( "load", &CollisionSphere::load )
                        .def( "unload", &CollisionSphere::unload )
                        .scope[def( "typeInfo", CollisionSphere::typeInfo )]];

        module( L )[class_<CollisionTerrain, Collision, SmartPtr<CollisionTerrain>>( "CollisionTerrain" )
                        .def( "load", &CollisionTerrain::load )
                        .def( "unload", &CollisionTerrain::unload )
                        .def( "getProperties", &CollisionTerrain::getProperties )
                        .def( "setProperties", &CollisionTerrain::setProperties )
                        .def( "isValid", &CollisionTerrain::isValid )
                        .def( "updateTransform", &CollisionTerrain::updateTransform )
                        .def( "getTerrainWidth", &CollisionTerrain::getTerrainWidth )
                        .def( "setTerrainWidth", &CollisionTerrain::setTerrainWidth )
                        .def( "getTerrainDepth", &CollisionTerrain::getTerrainDepth )
                        .def( "setTerrainDepth", &CollisionTerrain::setTerrainDepth )
                        .def( "getTerrainScale", &CollisionTerrain::getTerrainScale )
                        .def( "setTerrainScale", &CollisionTerrain::setTerrainScale )
                        .scope[def( "typeInfo", CollisionTerrain::typeInfo )]];

        module( L )[class_<scene::Animation, SubComponent, SmartPtr<scene::Animation>>( "Animation" )
                        .def( "load", &scene::Animation::load )
                        .def( "unload", &scene::Animation::unload )
                        .def( "getProperties", &scene::Animation::getProperties )
                        .def( "setProperties", &scene::Animation::setProperties )
                        .def( "getName", &scene::Animation::getName )
                        .def( "setName", &scene::Animation::setName )
                        .def( "getLength", &scene::Animation::getLength )
                        .def( "setLength", &scene::Animation::setLength )
                        .def( "isLooping", &scene::Animation::isLooping )
                        .def( "setLooping", &scene::Animation::setLooping )
                        .def( "getSpeed", &scene::Animation::getSpeed )
                        .def( "setSpeed", &scene::Animation::setSpeed )
                        .scope[def( "typeInfo", scene::Animation::typeInfo )]];

        module(
            L )[class_<scene::Animator, Component, SmartPtr<scene::Animator>>( "Animator" )
                    .def( "load", &scene::Animator::load )
                    .def( "unload", &scene::Animator::unload )
                    .def( "update", &scene::Animator::update )
                    .def( "play", &scene::Animator::play )
                    .def( "pause", &scene::Animator::pause )
                    .def( "stop", &scene::Animator::stop )
                    .def( "isPlaying", &scene::Animator::isPlaying )
                    .def( "isPaused", &scene::Animator::isPaused )
                    .def( "isStopped", &scene::Animator::isStopped )
                    .def( "onPlay", &scene::Animator::onPlay )
                    .def( "onPause", &scene::Animator::onPause )
                    .def( "onResume", &scene::Animator::onResume )
                    .def( "onStop", &scene::Animator::onStop )
                    .def( "onUpdate", &scene::Animator::onUpdate )
                    .def( "getAnimationTime", &scene::Animator::getAnimationTime )
                    .def( "setAnimationTime", &scene::Animator::setAnimationTime )
                    .def( "getAnimationSpeed", &scene::Animator::getAnimationSpeed )
                    .def( "setAnimationSpeed", &scene::Animator::setAnimationSpeed )
                    .def( "getAnimationScale", &scene::Animator::getAnimationScale )
                    .def( "setAnimationScale", &scene::Animator::setAnimationScale )
                    .def( "getAnimationWeight", &scene::Animator::getAnimationWeight )
                    .def( "setAnimationWeight", &scene::Animator::setAnimationWeight )
                    .def( "isLooping", &scene::Animator::isLooping )
                    .def( "setLooping", &scene::Animator::setLooping )
                    .def( "getAutoPlay", &scene::Animator::getAutoPlay )
                    .def( "setAutoPlay", &scene::Animator::setAutoPlay )
                    .def( "getResetTimeOnPlay", &scene::Animator::getResetTimeOnPlay )
                    .def( "setResetTimeOnPlay", &scene::Animator::setResetTimeOnPlay )
                    .def( "getResetTimeOnStop", &scene::Animator::getResetTimeOnStop )
                    .def( "setResetTimeOnStop", &scene::Animator::setResetTimeOnStop )
                    .def( "getClampTimeToClip", &scene::Animator::getClampTimeToClip )
                    .def( "setClampTimeToClip", &scene::Animator::setClampTimeToClip )
                    .def( "getApplyOnPropertyChange", &scene::Animator::getApplyOnPropertyChange )
                    .def( "setApplyOnPropertyChange", &scene::Animator::setApplyOnPropertyChange )
                    .def( "getSelectedAnimationIndex", &scene::Animator::getSelectedAnimationIndex )
                    .def( "setSelectedAnimationIndex", &scene::Animator::setSelectedAnimationIndex )
                    .def( "getSelectedAnimationLength", &scene::Animator::getSelectedAnimationLength )
                    .def( "getSelectedAnimationName", &scene::Animator::getSelectedAnimationName )
                    .def( "getSelectedAnimation", &scene::Animator::getSelectedAnimation )
                    .def( "getSelectedAnimationClip", &scene::Animator::getSelectedAnimationClip )
                    .def( "addAnimation", static_cast<void ( scene::Animator::* )(
                                              SmartPtr<IAnimation> )>( &scene::Animator::addAnimation ) )
                    .def( "removeAnimation", &scene::Animator::removeAnimation )
                    .def( "clearAnimations", &scene::Animator::clearAnimations )
                    .def( "getAnimations", &scene::Animator::getAnimations )
                    .def( "getAnimationClips", &scene::Animator::getAnimationClips )
                    .def( "setAnimationClips", &scene::Animator::setAnimationClips )
                    .def( "getNumAnimationClips", &scene::Animator::getNumAnimationClips )
                    .def( "setAnimations", &scene::Animator::setAnimations )
                    .def( "getProperties", &scene::Animator::getProperties )
                    .def( "setProperties", &scene::Animator::setProperties )
                    .def( "getChildObjects", &scene::Animator::getChildObjects )
                    .def( "getSkeleton", &scene::Animator::getSkeleton )
                    .def( "setSkeleton", &scene::Animator::setSkeleton )
                    .scope[def( "typeInfo", scene::Animator::typeInfo )]];

        module( L )[class_<AnimatedMaterial, Component, SmartPtr<AnimatedMaterial>>( "AnimatedMaterial" )
                        .def( "load", &AnimatedMaterial::load )
                        .def( "unload", &AnimatedMaterial::unload )
                        .def( "update", &AnimatedMaterial::update )
                        .def( "play", &AnimatedMaterial::play )
                        .def( "pause", &AnimatedMaterial::pause )
                        .def( "stop", &AnimatedMaterial::stop )
                        .def( "isPlaying", &AnimatedMaterial::isPlaying )
                        .def( "getMaterialName", &AnimatedMaterial::getMaterialName )
                        .def( "setMaterialName", &AnimatedMaterial::setMaterialName )
                        .def( "getProperties", &AnimatedMaterial::getProperties )
                        .def( "setProperties", &AnimatedMaterial::setProperties )
                        .def( "getAnimator", &AnimatedMaterial::getAnimator )
                        .def( "setAnimator", &AnimatedMaterial::setAnimator )
                        .def( "getChildObjects", &AnimatedMaterial::getChildObjects )
                        .scope[def( "typeInfo", AnimatedMaterial::typeInfo )]];

        module( L )[class_<Billboard, Component, SmartPtr<Billboard>>( "Billboard" )
                        .def( "load", &Billboard::load )
                        .def( "unload", &Billboard::unload )
                        .def( "getProperties", &Billboard::getProperties )
                        .def( "setProperties", &Billboard::setProperties )
                        .def( "getTexture", &Billboard::getTexture )
                        .def( "setTexture", &Billboard::setTexture )
                        .def( "getSize", &Billboard::getSize )
                        .def( "setSize", &Billboard::setSize )
                        .def( "getColor", &Billboard::getColor )
                        .def( "setColor", &Billboard::setColor )
                        .def( "updateBillboard", &Billboard::updateBillboard )
                        .scope[def( "typeInfo", Billboard::typeInfo )]];

        module( L )[class_<Billboards, Component, SmartPtr<Billboards>>( "Billboards" )
                        .def( "load", &Billboards::load )
                        .def( "unload", &Billboards::unload )
                        .def( "update", &Billboards::update )
                        .def( "getBillboardSet", &Billboards::getBillboardSet )
                        .def( "setBillboardSet", &Billboards::setBillboardSet )
                        .scope[def( "typeInfo", Billboards::typeInfo )]];

        // AudioEmitter binding
        module( L )[class_<AudioEmitter, Component, SmartPtr<AudioEmitter>>( "AudioEmitter" )
                        .def( "getSound", &AudioEmitter::getSound )
                        .def( "setSound", &AudioEmitter::setSound )
                        .def( "play", &AudioEmitter::play )
                        .def( "stop", &AudioEmitter::stop )
                        .def( "pause", &AudioEmitter::pause )
                        .def( "unpause", &AudioEmitter::unpause )
                        .scope[def( "typeInfo", AudioEmitter::typeInfo )]];

        // Camera binding
        module( L )[class_<Camera, Component, SmartPtr<Camera>>( "Camera" )
                        .def( "load", &Camera::load )
                        .def( "unload", &Camera::unload )
                        .def( "updateTransform",
                              static_cast<void ( Camera::* )()>( &Camera::updateTransform ) )
                        .def( "updateTransformWith",
                              static_cast<void ( Camera::* )( const Transform3<real_Num> & )>(
                                  &Camera::updateTransform ) )
                        .def( "getProperties", &Camera::getProperties )
                        .def( "setProperties", &Camera::setProperties )
                        .def( "getChildObjects", &Camera::getChildObjects )
                        .def( "updateOrder", &Camera::updateOrder )
                        .def( "getTargetTexture", &Camera::getTargetTexture )
                        .def( "setTargetTexture", &Camera::setTargetTexture )
                        .def( "getCamera", &Camera::getCamera )
                        .def( "setCamera", &Camera::setCamera )
                        .def( "getNode", &Camera::getNode )
                        .def( "setNode", &Camera::setNode )
                        .def( "isActive", &Camera::isActive )
                        .def( "setActive", &Camera::setActive )
                        .def( "getZOrder", static_cast<u32 ( Camera::* )() const>( &Camera::getZOrder ) )
                        .def( "setZOrder", &Camera::setZOrder )
                        .def( "getCameraToViewportRay", &Camera::getCameraToViewportRay )
                        .def( "isInFrustum", &Camera::isInFrustum )
                        .def( "getRenderTarget", &Camera::getRenderTarget )
                        .def( "getViewport", &Camera::getViewport )
                        .def( "setViewport", &Camera::setViewport )
                        .def( "getEnableShadows", &Camera::getEnableShadows )
                        .def( "setEnableShadows", &Camera::setEnableShadows )
                        .def( "getEnableSceneRender", &Camera::getEnableSceneRender )
                        .def( "setEnableSceneRender", &Camera::setEnableSceneRender )
                        .def( "getEnableUI", &Camera::getEnableUI )
                        .def( "setEnableUI", &Camera::setEnableUI )
                        .def( "getFOV", &Camera::getFOV )
                        .def( "setFOV", &Camera::setFOV )
                        .def( "getNearClipDistance", &Camera::getNearClipDistance )
                        .def( "setNearClipDistance", &Camera::setNearClipDistance )
                        .def( "getFarClipDistance", &Camera::getFarClipDistance )
                        .def( "setFarClipDistance", &Camera::setFarClipDistance )
                        .def( "getAutoUpdated", &Camera::getAutoUpdated )
                        .def( "setAutoUpdated", &Camera::setAutoUpdated )
                        .def( "getClearEveryFrame", &Camera::getClearEveryFrame )
                        .def( "setClearEveryFrame", &Camera::setClearEveryFrame )
                        .def( "getOverlaysEnabled", &Camera::getOverlaysEnabled )
                        .def( "setOverlaysEnabled", &Camera::setOverlaysEnabled )
                        .def( "getViewportBackgroundColour", &Camera::getViewportBackgroundColour )
                        .def( "setViewportBackgroundColour", &Camera::setViewportBackgroundColour )
                        .def( "getOrthoWindowWidth", &Camera::getOrthoWindowWidth )
                        .def( "setOrthoWindowWidth", &Camera::setOrthoWindowWidth )
                        .def( "getOrthoWindowHeight", &Camera::getOrthoWindowHeight )
                        .def( "setOrthoWindowHeight", &Camera::setOrthoWindowHeight )
                        .scope[def( "typeInfo", Camera::typeInfo )]];

        // Mesh binding
        module( L )[class_<scene::Mesh, Component, SmartPtr<scene::Mesh>>( "Mesh" )
                        .def( "load", &scene::Mesh::load )
                        .def( "unload", &scene::Mesh::unload )
                        .def( "getMeshPath", &scene::Mesh::getMeshPath )
                        .def( "setMeshPath", &scene::Mesh::setMeshPath )
                        .def( "getMeshResource", &scene::Mesh::getMeshResource )
                        .def( "setMeshResource", &scene::Mesh::setMeshResource )
                        .def( "getSkeleton", &scene::Mesh::getSkeleton )
                        .def( "setSkeleton", &scene::Mesh::setSkeleton )
                        .def( "getFsmPriority", &scene::Mesh::getFsmPriority )
                        .def( "setFsmPriority", &scene::Mesh::setFsmPriority )
                        .def( "getProperties", &scene::Mesh::getProperties )
                        .def( "setProperties", &scene::Mesh::setProperties )
                        .scope[def( "typeInfo", scene::Mesh::typeInfo )]];

        // ParticleSystem binding
        module( L )[class_<ParticleSystem, Component, SmartPtr<ParticleSystem>>( "ParticleSystem" )
                        .def( "load", &ParticleSystem::load )
                        .def( "unload", &ParticleSystem::unload )
                        .def( "getChildObjects", &ParticleSystem::getChildObjects )
                        .def( "getProperties", &ParticleSystem::getProperties )
                        .def( "setProperties", &ParticleSystem::setProperties )
                        .def( "getGraphicsObject", &ParticleSystem::getGraphicsObject )
                        .def( "setGraphicsObject", &ParticleSystem::setGraphicsObject )
                        .def( "getGraphicsNode", &ParticleSystem::getGraphicsNode )
                        .def( "setGraphicsNode", &ParticleSystem::setGraphicsNode )
                        .def( "getParticleSystem", &ParticleSystem::getParticleSystem )
                        .def( "setParticleSystem", &ParticleSystem::setParticleSystem )
                        .def( "getTemplateName", &ParticleSystem::getTemplateName )
                        .def( "setTemplateName", &ParticleSystem::setTemplateName )
                        .def( "getTechniqueName", &ParticleSystem::getTechniqueName )
                        .def( "setTechniqueName", &ParticleSystem::setTechniqueName )
                        .def( "getEmitterName", &ParticleSystem::getEmitterName )
                        .def( "setEmitterName", &ParticleSystem::setEmitterName )
                        .def( "getPlayOnLoad", &ParticleSystem::getPlayOnLoad )
                        .def( "setPlayOnLoad", &ParticleSystem::setPlayOnLoad )
                        .def( "getLifetime", &ParticleSystem::getLifetime )
                        .def( "setLifetime", &ParticleSystem::setLifetime )
                        .def( "getDuration", &ParticleSystem::getDuration )
                        .def( "setDuration", &ParticleSystem::setDuration )
                        .def( "isLooping", &ParticleSystem::isLooping )
                        .def( "setLooping", &ParticleSystem::setLooping )
                        .def( "getFastForwardTime", &ParticleSystem::getFastForwardTime )
                        .def( "setFastForwardTime", &ParticleSystem::setFastForwardTime )
                        .def( "getFastForwardInterval", &ParticleSystem::getFastForwardInterval )
                        .def( "setFastForwardInterval", &ParticleSystem::setFastForwardInterval )
                        .def( "getStartLifetime", &ParticleSystem::getStartLifetime )
                        .def( "setStartLifetime", &ParticleSystem::setStartLifetime )
                        .def( "getStartSize", &ParticleSystem::getStartSize )
                        .def( "setStartSize", &ParticleSystem::setStartSize )
                        .def( "getScale", &ParticleSystem::getScale )
                        .def( "setScale", &ParticleSystem::setScale )
                        .def( "getRate", &ParticleSystem::getRate )
                        .def( "setRate", &ParticleSystem::setRate )
                        .def( "getRateVariance", &ParticleSystem::getRateVariance )
                        .def( "setRateVariance", &ParticleSystem::setRateVariance )
                        .def( "getAngle", &ParticleSystem::getAngle )
                        .def( "setAngle", &ParticleSystem::setAngle )
                        .def( "getAngleVariance", &ParticleSystem::getAngleVariance )
                        .def( "setAngleVariance", &ParticleSystem::setAngleVariance )
                        .def( "getShapeType", &ParticleSystem::getShapeType )
                        .def( "setShapeType", &ParticleSystem::setShapeType )
                        .def( "getShapeSize", &ParticleSystem::getShapeSize )
                        .def( "setShapeSize", &ParticleSystem::setShapeSize )
                        .def( "getShapeSizeVariance", &ParticleSystem::getShapeSizeVariance )
                        .def( "setShapeSizeVariance", &ParticleSystem::setShapeSizeVariance )
                        .def( "play", &ParticleSystem::play )
                        .def( "stop", &ParticleSystem::stop )
                        .def( "pause", &ParticleSystem::pause )
                        .def( "resume", &ParticleSystem::resume )
                        .def( "rebuild", &ParticleSystem::rebuild )
                        .def( "isPlaying", &ParticleSystem::isPlaying )
                        .scope[def( "typeInfo", ParticleSystem::typeInfo )]];

        module( L )[class_<Cubemap, Component, SmartPtr<Cubemap>>( "Cubemap" )
                        .def( "load", &Cubemap::load )
                        .def( "reload", &Cubemap::reload )
                        .def( "unload", &Cubemap::unload )
                        .def( "update", &Cubemap::update )
                        .def( "getRenderCubemap", &Cubemap::getRenderCubemap )
                        .def( "setRenderCubemap", &Cubemap::setRenderCubemap )
                        .def( "getCameraDistance", &Cubemap::getCameraDistance )
                        .def( "setCameraDistance", &Cubemap::setCameraDistance )
                        .def( "getEnableDistanceTheshold", &Cubemap::getEnableDistanceTheshold )
                        .def( "setEnableDistanceTheshold", &Cubemap::setEnableDistanceTheshold )
                        .def( "getProperties", &Cubemap::getProperties )
                        .def( "setProperties", &Cubemap::setProperties )
                        .def( "getChildObjects", &Cubemap::getChildObjects )
                        .def( "isEnabled", &Cubemap::isEnabled )
                        .def( "setEnabled", &Cubemap::setEnabled )
                        .def( "isDirty", &Cubemap::isDirty )
                        .def( "markDirty", &Cubemap::markDirty )
                        .def( "requestCapture", &Cubemap::requestCapture )
                        .def( "isCaptureRequested", &Cubemap::isCaptureRequested )
                        .def( "clearCaptureRequest", &Cubemap::clearCaptureRequest )
                        .def( "getCubemapPath", &Cubemap::getCubemapPath )
                        .def( "setCubemapPath", &Cubemap::setCubemapPath )
                        .def( "getResolution", &Cubemap::getResolution )
                        .def( "setResolution", &Cubemap::setResolution )
                        .def( "getNearClipDistance", &Cubemap::getNearClipDistance )
                        .def( "setNearClipDistance", &Cubemap::setNearClipDistance )
                        .def( "getFarClipDistance", &Cubemap::getFarClipDistance )
                        .def( "setFarClipDistance", &Cubemap::setFarClipDistance )
                        .def( "getIntensity", &Cubemap::getIntensity )
                        .def( "setIntensity", &Cubemap::setIntensity )
                        .def( "getBlendDistance", &Cubemap::getBlendDistance )
                        .def( "setBlendDistance", &Cubemap::setBlendDistance )
                        .def( "getUpdateInterval", &Cubemap::getUpdateInterval )
                        .def( "setUpdateInterval", &Cubemap::setUpdateInterval )
                        .def( "getUseHDR", &Cubemap::getUseHDR )
                        .def( "setUseHDR", &Cubemap::setUseHDR )
                        .def( "getGenerateMipmaps", &Cubemap::getGenerateMipmaps )
                        .def( "setGenerateMipmaps", &Cubemap::setGenerateMipmaps )
                        .def( "getProbeName", &Cubemap::getProbeName )
                        .def( "setProbeName", &Cubemap::setProbeName )
                        .scope[def( "typeInfo", Cubemap::typeInfo )]];

        module( L )[class_<RenderTexture, Component, SmartPtr<RenderTexture>>( "RenderTexture" )
                        .def( "load", &RenderTexture::load )
                        .def( "unload", &RenderTexture::unload )
                        .def( "getChildObjects", &RenderTexture::getChildObjects )
                        .def( "getProperties", &RenderTexture::getProperties )
                        .def( "setProperties", &RenderTexture::setProperties )
                        .def( "update", &RenderTexture::update )
                        .def( "getRenderTexture", &RenderTexture::getRenderTexture )
                        .def( "setRenderTexture", &RenderTexture::setRenderTexture )
                        .def( "getTexture", &RenderTexture::getTexture )
                        .def( "setTexture", &RenderTexture::setTexture )
                        .def( "getWidth", &RenderTexture::getWidth )
                        .def( "setWidth", &RenderTexture::setWidth )
                        .def( "getHeight", &RenderTexture::getHeight )
                        .def( "setHeight", &RenderTexture::setHeight )
                        .def( "getTextureName", &RenderTexture::getTextureName )
                        .def( "setTextureName", &RenderTexture::setTextureName )
                        .def( "getAutoUpdate", &RenderTexture::getAutoUpdate )
                        .def( "setAutoUpdate", &RenderTexture::setAutoUpdate )
                        .scope[def( "typeInfo", RenderTexture::typeInfo )]];

        // Renderer binding
        module( L )[class_<Renderer, Component, SmartPtr<Renderer>>( "Renderer" )
                        .def( "load", &Renderer::load )
                        .def( "unload", &Renderer::unload )
                        .def( "updateFlags", &Renderer::updateFlags )
                        .def( "getChildObjects", &Renderer::getChildObjects )
                        .def( "getProperties", &Renderer::getProperties )
                        .def( "setProperties", &Renderer::setProperties )
                        .def( "getSharedMaterial", &Renderer::getSharedMaterial )
                        .def( "setSharedMaterial", &Renderer::setSharedMaterial )
                        .def( "getMaterialName", &Renderer::getMaterialName )
                        .def( "setMaterialName", &Renderer::setMaterialName )
                        .def( "getGraphicsObject", &Renderer::getGraphicsObject )
                        .def( "setGraphicsObject", &Renderer::setGraphicsObject )
                        .def( "getGraphicsNode", &Renderer::getGraphicsNode )
                        .def( "setGraphicsNode", &Renderer::setGraphicsNode )
                        .def( "getCastShadows", Renderer_getCastShadows )
                        .def( "setCastShadows", Renderer_setCastShadows )
                        .def( "getRecieveShadows", Renderer_getRecieveShadows )
                        .def( "setRecieveShadows", Renderer_setRecieveShadows )
                        .def( "getReflections", Renderer_getReflections )
                        .def( "setReflections", Renderer_setReflections )
                        .def( "getOcculsion", Renderer_getOcculsion )
                        .def( "setOcculsion", Renderer_setOcculsion )
                        .def( "getVisibilityFlags", &Renderer::getVisibilityFlags )
                        .def( "setVisibilityFlags", &Renderer::setVisibilityFlags )
                        .def( "getZOrder", &Renderer::getZOrder )
                        .def( "setZOrder", &Renderer::setZOrder )
                        .def( "getBoundingBox", &Renderer::getBoundingBox )
                        .def( "updateMaterials", &Renderer::updateMaterials )
                        .def( "updateVisibility", &Renderer::updateVisibility )
                        .def( "updateTransform",
                              static_cast<void ( Renderer::* )()>( &Renderer::updateTransform ) )
                        .def( "updateTransformWith",
                              static_cast<void ( Renderer::* )( const Transform3<real_Num> & )>(
                                  &Renderer::updateTransform ) )
                        .scope[def( "typeInfo", Renderer::typeInfo )]];

        module( L )[class_<MeshRenderer, Renderer, SmartPtr<MeshRenderer>>( "MeshRenderer" )
                        .def( "load", &MeshRenderer::load )
                        .def( "unload", &MeshRenderer::unload )
                        .def( "updateFlags", &MeshRenderer::updateFlags )
                        .def( "getChildObjects", &MeshRenderer::getChildObjects )
                        .def( "getProperties", &MeshRenderer::getProperties )
                        .def( "setProperties", &MeshRenderer::setProperties )
                        .def( "updateMaterials", &MeshRenderer::updateMaterials )
                        .def( "getMeshNode", &MeshRenderer::getMeshNode )
                        .def( "setMeshNode", &MeshRenderer::setMeshNode )
                        .def( "updateMesh", &MeshRenderer::updateMesh )
                        .scope[def( "typeInfo", MeshRenderer::typeInfo )]];

        module( L )[class_<Skybox, Component, SmartPtr<Skybox>>( "Skybox" )
                        .def( "load", &Skybox::load )
                        .def( "unload", &Skybox::unload )
                        .def( "updateFlags", &Skybox::updateFlags )
                        .def( "getProperties", &Skybox::getProperties )
                        .def( "setProperties", &Skybox::setProperties )
                        .def( "getMaterial", &Skybox::getMaterial )
                        .def( "getDistance", &Skybox::getDistance )
                        .def( "setDistance", &Skybox::setDistance )
                        .def( "getTextures", &Skybox::getTextures )
                        .def( "setTextures", &Skybox::setTextures )
                        .def( "getTexture", &Skybox::getTexture )
                        .def( "setTexture", &Skybox::setTexture )
                        .def( "setTextureByName", &Skybox::setTextureByName )
                        .def( "updateMaterials", &Skybox::updateMaterials )
                        .def( "getSwapLeftRight", &Skybox::getSwapLeftRight )
                        .def( "setSwapLeftRight", &Skybox::setSwapLeftRight )
                        .def( "setupMaterial", &Skybox::setupMaterial )
                        .def( "getSkybox", &Skybox::getSkybox )
                        .def( "setSkybox", &Skybox::setSkybox )
                        .def( "updateVisibility", &Skybox::updateVisibility )
                        .scope[def( "typeInfo", Skybox::typeInfo )]];

        module(
            L )[class_<VehicleController, Component, SmartPtr<VehicleController>>( "VehicleController" )
                    .def( "getChassis", &VehicleController::getChassis )
                    .def( "setChassis", &VehicleController::setChassis )
                    .def( "getCollision", &VehicleController::getCollision )
                    .def( "setCollision", &VehicleController::setCollision )
                    .def( "getMOI", &VehicleController::getMOI )
                    .def( "setMOI", &VehicleController::setMOI )
                    .def( "getCg", &VehicleController::getCg )
                    .def( "setCg", &VehicleController::setCg )
                    .def( "getMass", &VehicleController::getMass )
                    .def( "setMass", &VehicleController::setMass )
                    .def( "getVehicleType", &VehicleController::getVehicleType )
                    .def( "setVehicleType", &VehicleController::setVehicleType )
                    .def( "isAircraftPhysicsEnabled", &VehicleController::isAircraftPhysicsEnabled )
                    .def( "getAircraftController", &VehicleController::getAircraftController )
                    .def( "setAircraftControls", &VehicleController::setAircraftControls )
                    .def( "getAircraftThrottle", &VehicleController::getAircraftThrottle )
                    .def( "setAircraftThrottle", &VehicleController::setAircraftThrottle )
                    .def( "getAircraftPitch", &VehicleController::getAircraftPitch )
                    .def( "setAircraftPitch", &VehicleController::setAircraftPitch )
                    .def( "getAircraftRoll", &VehicleController::getAircraftRoll )
                    .def( "setAircraftRoll", &VehicleController::setAircraftRoll )
                    .def( "getAircraftYaw", &VehicleController::getAircraftYaw )
                    .def( "setAircraftYaw", &VehicleController::setAircraftYaw )
                    .def( "getAirDensity", &VehicleController::getAirDensity )
                    .def( "setAirDensity", &VehicleController::setAirDensity )
                    .def( "getAerodynamicSectionMultiplier",
                          &VehicleController::getAerodynamicSectionMultiplier )
                    .def( "setAerodynamicSectionMultiplier",
                          &VehicleController::setAerodynamicSectionMultiplier )
                    .def( "getRollwiseDamping", &VehicleController::getRollwiseDamping )
                    .def( "setRollwiseDamping", &VehicleController::setRollwiseDamping )
                    .scope[def( "typeInfo", VehicleController::typeInfo )]];

        module( L )[class_<CarController, VehicleController, SmartPtr<CarController>>( "CarController" )
                        .def( "getThrottle", &CarController::getThrottle )
                        .def( "setThrottle", &CarController::setThrottle )
                        .def( "getBrake", &CarController::getBrake )
                        .def( "setBrake", &CarController::setBrake )
                        .def( "getSteering", &CarController::getSteering )
                        .def( "setSteering", &CarController::setSteering )
                        .def( "getVehicleController", &CarController::getVehicleController )
                        .scope[def( "typeInfo", CarController::typeInfo )]];

        module( L )[class_<ProceduralRaceScene, Component, SmartPtr<ProceduralRaceScene>>( "ProceduralRaceScene" )
                        .def( "regenerate", &ProceduralRaceScene::regenerate )
                        .def( "clearGeneratedScene", &ProceduralRaceScene::clearGeneratedScene )
                        .def( "configurePhysics", &ProceduralRaceScene::configurePhysics )
                        .def( "setControls", &ProceduralRaceScene::setControls )
                        .def( "reset", &ProceduralRaceScene::reset )
                        .def( "isGenerated", &ProceduralRaceScene::isGenerated )
                        .def( "isPhysicsConfigured", &ProceduralRaceScene::isPhysicsConfigured )
                        .def( "getGenerationError", &ProceduralRaceScene::getGenerationError )
                        .def( "getSeed", &ProceduralRaceScene::getSeed )
                        .def( "setSeed", &ProceduralRaceScene::setSeed )
                        .def( "getQuality", &ProceduralRaceScene::getQuality )
                        .def( "setQuality", &ProceduralRaceScene::setQuality )
                        .def( "getCircuitSampleCount", &ProceduralRaceScene::getCircuitSampleCount )
                        .def( "getCircuitLength", &ProceduralRaceScene::getCircuitLength )
                        .def( "nearestCircuitSample", &ProceduralRaceScene::nearestCircuitSample )
                        .def( "getCircuitPosition", &ProceduralRaceScene::getCircuitPosition )
                        .def( "getCircuitRight", &ProceduralRaceScene::getCircuitRight )
                        .def( "getWheelbase", &ProceduralRaceScene::getWheelbase )
                        .def( "getMaxSteeringAngle", &ProceduralRaceScene::getMaxSteeringAngle )
                        .def( "getWheelActor", &ProceduralRaceScene::getWheelActor )
                        .def( "getBodyActor", &ProceduralRaceScene::getBodyActor )
                        .def( "getCarController", &ProceduralRaceScene::getCarController )
                        .def( "setView", &ProceduralRaceScene::setView )
                        .def( "validateReflection", &ProceduralRaceScene::validateReflection )
                        .scope[def( "typeInfo", ProceduralRaceScene::typeInfo )]];

        module( L )[class_<CharacterController, Component, SmartPtr<CharacterController>>(
                        "CharacterController" )
                        .def( "setScene", &CharacterController::setScene )
                        .def( "getTransform", &CharacterController::getTransform )
                        .def( "setTransform", &CharacterController::setTransform )
                        .def( "setActorFlag", CharacterController_setActorFlag )
                        .def( "getActorFlags", CharacterController_getActorFlags )
                        .def( "getMass", &CharacterController::getMass )
                        .def( "setMass", &CharacterController::setMass )
                        .def( "getCollisionType", &CharacterController::getCollisionType )
                        .def( "setCollisionType", &CharacterController::setCollisionType )
                        .def( "getCollisionMask", &CharacterController::getCollisionMask )
                        .def( "setCollisionMask", &CharacterController::setCollisionMask )
                        .def( "getKinematicMode", &CharacterController::getKinematicMode )
                        .def( "setKinematicMode", &CharacterController::setKinematicMode )
                        .def( "wakeUp", &CharacterController::wakeUp )
                        .def( "getPosition", &CharacterController::getPosition )
                        .def( "setPosition", &CharacterController::setPosition )
                        .def( "getOrientation", &CharacterController::getOrientation )
                        .def( "setOrientation", &CharacterController::setOrientation )
                        .def( "getMoveSpeed", &CharacterController::getMoveSpeed )
                        .def( "setMoveSpeed", &CharacterController::setMoveSpeed )
                        .def( "getJump", &CharacterController::getJump )
                        .def( "setJump", &CharacterController::setJump )
                        .def( "isGrounded", &CharacterController::isGrounded )
                        .def( "setWalkVector", &CharacterController::setWalkVector )
                        .def( "stop", &CharacterController::stop )
                        .scope[def( "typeInfo", CharacterController::typeInfo )]];

        module( L )[class_<Constraint, Component, SmartPtr<Constraint>>( "Constraint" )
                        .enum_( "Type" )[value( "D6", static_cast<int>( Constraint::Type::D6 ) ),
                                         value( "Fixed", static_cast<int>( Constraint::Type::Fixed ) ),
                                         value( "Count", static_cast<int>( Constraint::Type::Count ) )]
                        .def( "load", &Constraint::load )
                        .def( "reload", &Constraint::reload )
                        .def( "unload", &Constraint::unload )
                        .def( "updateFlags", &Constraint::updateFlags )
                        .def( "getProperties", &Constraint::getProperties )
                        .def( "setProperties", &Constraint::setProperties )
                        .def( "getChildObjects", &Constraint::getChildObjects )
                        .def( "getBodyA", &Constraint::getBodyA )
                        .def( "setBodyA", &Constraint::setBodyA )
                        .def( "getBodyB", &Constraint::getBodyB )
                        .def( "setBodyB", &Constraint::setBodyB )
                        .def( "getType", Constraint_getType )
                        .def( "setType", Constraint_setType )
                        .def( "getConstraint", &Constraint::getConstraint )
                        .def( "setConstraint", &Constraint::setConstraint )
                        .def( "getAxisX", Constraint_getAxisX )
                        .def( "setAxisX", Constraint_setAxisX )
                        .def( "getAxisY", Constraint_getAxisY )
                        .def( "setAxisY", Constraint_setAxisY )
                        .def( "getAxisZ", Constraint_getAxisZ )
                        .def( "setAxisZ", Constraint_setAxisZ )
                        .def( "getSwing1", Constraint_getSwing1 )
                        .def( "setSwing1", Constraint_setSwing1 )
                        .def( "getSwing2", Constraint_getSwing2 )
                        .def( "setSwing2", Constraint_setSwing2 )
                        .def( "getTwist", Constraint_getTwist )
                        .def( "setTwist", Constraint_setTwist )
                        .def( "getBreakForce", &Constraint::getBreakForce )
                        .def( "setBreakForce", &Constraint::setBreakForce )
                        .def( "getBreakTorque", &Constraint::getBreakTorque )
                        .def( "setBreakTorque", &Constraint::setBreakTorque )
                        .def( "updatePhysicsState", &Constraint::updatePhysicsState )
                        .scope[def( "typeInfo", Constraint::typeInfo )]];

        module( L )[class_<Rigidbody, Component, SmartPtr<Rigidbody>>( "Rigidbody" )
                        .def( "load", &Rigidbody::load )
                        .def( "unload", &Rigidbody::unload )
                        .def( "preUpdate", &Rigidbody::preUpdate )
                        .def( "update", &Rigidbody::update )
                        .def( "postUpdate", &Rigidbody::postUpdate )
                        .def( "updateKinematicState", &Rigidbody::updateKinematicState )
                        .def( "getRigidDynamic", &Rigidbody::getRigidDynamic )
                        .def( "setRigidDynamic", &Rigidbody::setRigidDynamic )
                        .def( "getRigidStatic", &Rigidbody::getRigidStatic )
                        .def( "setRigidStatic", &Rigidbody::setRigidStatic )
                        .def( "hasPhysicsBody", &Rigidbody::hasPhysicsBody )
                        .def( "hasRigidDynamic", &Rigidbody::hasRigidDynamic )
                        .def( "hasRigidStatic", &Rigidbody::hasRigidStatic )
                        .def( "isStatic", &Rigidbody::isStatic )
                        .def( "isKinematic", &Rigidbody::isKinematic )
                        .def( "setKinematic", &Rigidbody::setKinematic )
                        .def( "getGroupMask", &Rigidbody::getGroupMask )
                        .def( "setGroupMask", &Rigidbody::setGroupMask )
                        .def( "getCollisionMask", &Rigidbody::getCollisionMask )
                        .def( "setCollisionMask", &Rigidbody::setCollisionMask )
                        .def( "hasCollisionMaskOverride", &Rigidbody::hasCollisionMaskOverride )
                        .def( "setCollisionMaskOverride", &Rigidbody::setCollisionMaskOverride )
                        .def( "getLocalBounds", &Rigidbody::getLocalBounds )
                        .def( "setLocalBounds", &Rigidbody::setLocalBounds )
                        .def( "getScene", &Rigidbody::getScene )
                        .def( "setScene", &Rigidbody::setScene )
                        .def( "getMaterial", &Rigidbody::getMaterial )
                        .def( "setMaterial", &Rigidbody::setMaterial )
                        .def( "getChildObjects", &Rigidbody::getChildObjects )
                        .def( "getProperties", &Rigidbody::getProperties )
                        .def( "setProperties", &Rigidbody::setProperties )
                        .def( "updateTransform", &Rigidbody::updateTransform )
                        .def( "updateFlags", &Rigidbody::updateFlags )
                        .def( "updateSmoothTransformState", &Rigidbody::updateSmoothTransformState )
                        .def( "getLinearVelocity", &Rigidbody::getLinearVelocity )
                        .def( "setLinearVelocity", &Rigidbody::setLinearVelocity )
                        .def( "getAngularVelocity", &Rigidbody::getAngularVelocity )
                        .def( "setAngularVelocity", &Rigidbody::setAngularVelocity )
                        .def( "getMaxLinearVelocity", &Rigidbody::getMaxLinearVelocity )
                        .def( "setMaxLinearVelocity", &Rigidbody::setMaxLinearVelocity )
                        .def( "getMaxAngularVelocity", &Rigidbody::getMaxAngularVelocity )
                        .def( "setMaxAngularVelocity", &Rigidbody::setMaxAngularVelocity )
                        .def( "getLocalAngularVelocity", &Rigidbody::getLocalAngularVelocity )
                        .def( "getLocalLinearVelocity", &Rigidbody::getLocalLinearVelocity )
                        .def( "addForce", &Rigidbody::addForce )
                        .def( "addTorque", &Rigidbody::addTorque )
                        .def( "setMass", &Rigidbody::setMass )
                        .def( "getMass", &Rigidbody::getMass )
                        .def( "setMassProps", &Rigidbody::setMassProps )
                        .def( "getPointVelocity", &Rigidbody::getPointVelocity )
                        .def( "getLocalPointVelocity", &Rigidbody::getLocalPointVelocity )
                        .def( "getMassSpaceInertiaTensor", &Rigidbody::getMassSpaceInertiaTensor )
                        .def( "setMassSpaceInertiaTensor", &Rigidbody::setMassSpaceInertiaTensor )
                        .def( "getTransform", &Rigidbody::getTransform )
                        .def( "getTransformReferences", &Rigidbody::getTransformReferences )
                        .def( "setTransformReferences", &Rigidbody::setTransformReferences )
                        .def( "getNumShapes", &Rigidbody::getNumShapes )
                        .def( "getNumConstraints", &Rigidbody::getNumConstraints )
                        .def( "getClonedActor", &Rigidbody::getClonedActor )
                        .def( "setClonedActor", &Rigidbody::setClonedActor )
                        .def( "applyActorCollisionMask", &Rigidbody::applyActorCollisionMask )
                        .def( "updateShapes", &Rigidbody::updateShapes )
                        .def( "addConstraint", &Rigidbody::addConstraint )
                        .def( "removeConstraint", &Rigidbody::removeConstraint )
                        .def( "hasConstraint", &Rigidbody::hasConstraint )
                        .def( "hasConstraints", &Rigidbody::hasConstraints )
                        .def( "getConstraints", &Rigidbody::getConstraints )
                        .def( "setConstraints", &Rigidbody::setConstraints )
                        .def( "getRigidbodyListener", &Rigidbody::getRigidbodyListener )
                        .def( "setRigidbodyListener", &Rigidbody::setRigidbodyListener )
                        .scope[def( "updateComponents", &Rigidbody::updateComponents ),
                               def( "typeInfo", Rigidbody::typeInfo )]];

        module( L )[class_<FiniteStateMachine, Component, SmartPtr<FiniteStateMachine>>(
                        "FiniteStateMachine" )
                        .def( "load", &FiniteStateMachine::load )
                        .def( "unload", &FiniteStateMachine::unload )
                        .def( "reload", &FiniteStateMachine::reload )
                        .def( "getChildObjects", &FiniteStateMachine::getChildObjects )
                        .def( "getProperties", &FiniteStateMachine::getProperties )
                        .def( "setProperties", &FiniteStateMachine::setProperties )
                        .scope[def( "typeInfo", FiniteStateMachine::typeInfo )]];

        module( L )[class_<NetworkListener, SmartPtr<NetworkListener>>( "NetworkListener" )
                        .def( constructor<>() )
                        .def( "handlePacket", &NetworkListener::handlePacket )
                        .def( "connect", &NetworkListener::connect )
                        .def( "disconnect", &NetworkListener::disconnect )
                        .def( "addListener", &NetworkListener::addListener )
                        .def( "removeListener", &NetworkListener::removeListener )
                        .def( "clearListeners", &NetworkListener::clearListeners )
                        .def( "hasListener", &NetworkListener::hasListener )
                        .def( "getListenerCount", &NetworkListener::getListenerCount )
                        .scope[def( "typeInfo", NetworkListener::typeInfo )]];

        module( L )[class_<NetworkPlayer, SmartPtr<NetworkPlayer>>( "NetworkPlayer" )
                        .def( constructor<>() )
                        .def( "getActorNumber", &NetworkPlayer::getActorNumber )
                        .def( "setActorNumber", &NetworkPlayer::setActorNumber )
                        .def( "getNickName", &NetworkPlayer::getNickName )
                        .def( "setNickName", &NetworkPlayer::setNickName )
                        .def( "getUserId", &NetworkPlayer::getUserId )
                        .def( "setUserId", &NetworkPlayer::setUserId )
                        .def( "getPing", &NetworkPlayer::getPing )
                        .def( "setPing", &NetworkPlayer::setPing )
                        .def( "isLocal", &NetworkPlayer::isLocal )
                        .def( "setLocal", &NetworkPlayer::setLocal )
                        .def( "isMasterClient", &NetworkPlayer::isMasterClient )
                        .def( "setMasterClient", &NetworkPlayer::setMasterClient )
                        .def( "getCustomProperties", &NetworkPlayer::getCustomProperties )
                        .def( "setCustomProperties", &NetworkPlayer::setCustomProperties )
                        .scope[def( "typeInfo", NetworkPlayer::typeInfo )]];

        module( L )[class_<NetworkStream, SmartPtr<NetworkStream>>( "NetworkStream" )
                        .def( constructor<>() )
                        .def( constructor<bool>() )
                        .def( "isWriting", &NetworkStream::isWriting )
                        .def( "isReading", &NetworkStream::isReading )
                        .def( "getSize", &NetworkStream::getSize )
                        .def( "getPosition", &NetworkStream::getPosition )
                        .def( "reset", &NetworkStream::reset )
                        .def( "getData", &NetworkStream::getData )
                        .def( "setData", &NetworkStream::setData )
                        .scope[def( "typeInfo", NetworkStream::typeInfo )]];

        module( L )[class_<NetworkView, Component, SmartPtr<NetworkView>>( "NetworkView" )
                        .def( "load", &NetworkView::load )
                        .def( "unload", &NetworkView::unload )
                        .def( "getProperties", &NetworkView::getProperties )
                        .def( "setProperties", &NetworkView::setProperties )
                        .def( "RPC", &NetworkView::RPC )
                        .def( "serializeView", &NetworkView::serializeView )
                        .def( "deserializeView", &NetworkView::deserializeView )
                        .def( "onSerializeView", &NetworkView::onSerializeView )
                        .def( "getViewId", &NetworkView::getViewId )
                        .def( "setViewId", &NetworkView::setViewId )
                        .def( "getOwnerId", &NetworkView::getOwnerId )
                        .def( "setOwnerId", &NetworkView::setOwnerId )
                        .def( "isMine", &NetworkView::isMine )
                        .def( "transferOwnership", &NetworkView::transferOwnership )
                        .def( "requestOwnership", &NetworkView::requestOwnership )
                        .def( "isSceneView", &NetworkView::isSceneView )
                        .scope[def( "typeInfo", NetworkView::typeInfo )]];

        module( L )[class_<VideoPlayer, Component, SmartPtr<VideoPlayer>>( "VideoPlayer" )
                        .def( "load", &VideoPlayer::load )
                        .def( "unload", &VideoPlayer::unload )
                        .def( "getProperties", &VideoPlayer::getProperties )
                        .def( "setProperties", &VideoPlayer::setProperties )
                        .def( "getChildObjects", &VideoPlayer::getChildObjects )
                        .def( "play", &VideoPlayer::play )
                        .def( "pause", &VideoPlayer::pause )
                        .def( "stop", &VideoPlayer::stop )
                        .def( "isPlaying", &VideoPlayer::isPlaying )
                        .def( "getLoop", &VideoPlayer::getLoop )
                        .def( "setLoop", &VideoPlayer::setLoop )
                        .def( "getAutoPlay", &VideoPlayer::getAutoPlay )
                        .def( "setAutoPlay", &VideoPlayer::setAutoPlay )
                        .def( "getAutoUpdate", &VideoPlayer::getAutoUpdate )
                        .def( "setAutoUpdate", &VideoPlayer::setAutoUpdate )
                        .scope[def( "typeInfo", VideoPlayer::typeInfo )]];

        module( L )[class_<WheelController, Component, SmartPtr<WheelController>>( "WheelController" )
                        .def( "load", &WheelController::load )
                        .def( "unload", &WheelController::unload )
                        .def( "getProperties", &WheelController::getProperties )
                        .def( "setProperties", &WheelController::setProperties )
                        .def( "getChildObjects", &WheelController::getChildObjects )
                        .def( "getWheelController", &WheelController::getWheelController )
                        .def( "setWheelController", &WheelController::setWheelController )
                        .def( "getRadius", &WheelController::getRadius )
                        .def( "setRadius", &WheelController::setRadius )
                        .def( "getSuspensionDistance", &WheelController::getSuspensionDistance )
                        .def( "setSuspensionDistance", &WheelController::setSuspensionDistance )
                        .def( "getWheelDamping", &WheelController::getWheelDamping )
                        .def( "setWheelDamping", &WheelController::setWheelDamping )
                        .def( "getTireModel", WheelController_getTireModel )
                        .def( "setTireModel", WheelController_setTireModel )
                        .def( "setMassFraction", &WheelController::setMassFraction )
                        .def( "reset", &WheelController::reset )
                        .scope[def( "typeInfo", WheelController::typeInfo )]];

        module( L )[class_<CameraController, Component, SmartPtr<CameraController>>( "CameraController" )
                        .def( "setCameraFlag", &CameraController::setCameraFlag )
                        .def( "getCameraFlag", &CameraController::getCameraFlag )
                        .def( "setViewportId", &CameraController::setViewportId )
                        .def( "getViewportId", &CameraController::getViewportId )
                        .def( "handleSetActive", &CameraController::handleSetActive )
                        .def( "focusSelection", &CameraController::focusSelection )
                        .def( "focusOnBounds", &CameraController::focusOnBounds )
                        .def( "resetCamera", &CameraController::resetCamera )
                        .def( "isMainCamera", &CameraController::isMainCamera )
                        .def( "setMainCamera", &CameraController::setMainCamera )
                        .def( "isOrthographic", &CameraController::isOrthographic )
                        .def( "setOrthographic", &CameraController::setOrthographic )
                        .def( "getUiWindow", &CameraController::getUiWindow )
                        .def( "setUiWindow", &CameraController::setUiWindow )
                        .scope[def( "typeInfo", CameraController::typeInfo )]];

        module( L )[class_<CameraFollow, Component, SmartPtr<CameraFollow>>( "CameraFollow" )
                        .def( "getTarget", &CameraFollow::getTarget )
                        .def( "setTarget", &CameraFollow::setTarget )
                        .def( "getFollowObject", &CameraFollow::getFollowObject )
                        .def( "setFollowObject", &CameraFollow::setFollowObject )
                        .scope[def( "typeInfo", CameraFollow::typeInfo )]];

        module( L )[class_<CameraTarget, Component, SmartPtr<CameraTarget>>( "CameraTarget" )
                        .def( "getOffsetPosition", &CameraTarget::getOffsetPosition )
                        .def( "setOffsetPosition", &CameraTarget::setOffsetPosition )
                        .def( "getOffsetRotation", &CameraTarget::getOffsetRotation )
                        .def( "setOffsetRotation", &CameraTarget::setOffsetRotation )
                        .scope[def( "typeInfo", CameraTarget::typeInfo )]];

        module( L )[class_<EditorCameraController, CameraController, SmartPtr<EditorCameraController>>(
                        "EditorCameraController" )
                        .def( "handleInputEvent", &EditorCameraController::handleInputEvent )
                        .def( "setPosition", &EditorCameraController::setPosition )
                        .def( "getPosition", &EditorCameraController::getPosition )
                        .def( "setTargetPosition", &EditorCameraController::setTargetPosition )
                        .def( "getTargetPosition", &EditorCameraController::getTargetPosition )
                        .def( "setOrientation", &EditorCameraController::setOrientation )
                        .def( "getOrientation", &EditorCameraController::getOrientation )
                        .def( "setDirection", &EditorCameraController::setDirection )
                        .def( "getDirection", &EditorCameraController::getDirection )
                        .def( "getUp", &EditorCameraController::getUp )
                        .def( "getRight", &EditorCameraController::getRight )
                        .def( "getRotationSpeed", &EditorCameraController::getRotationSpeed )
                        .def( "setRotationSpeed", &EditorCameraController::setRotationSpeed )
                        .def( "getInvert", &EditorCameraController::getInvert )
                        .def( "setInvert", &EditorCameraController::setInvert )
                        .def( "getMoveSpeed", &EditorCameraController::getMoveSpeed )
                        .def( "setMoveSpeed", &EditorCameraController::setMoveSpeed )
                        .def( "getTranslationSpeed", &EditorCameraController::getTranslationSpeed )
                        .def( "setTranslationSpeed", &EditorCameraController::setTranslationSpeed )
                        .def( "getOrbitDistance", &EditorCameraController::getOrbitDistance )
                        .def( "setOrbitDistance", &EditorCameraController::setOrbitDistance )
                        .def( "getPanSpeed", &EditorCameraController::getPanSpeed )
                        .def( "setPanSpeed", &EditorCameraController::setPanSpeed )
                        .def( "getDollySpeed", &EditorCameraController::getDollySpeed )
                        .def( "setDollySpeed", &EditorCameraController::setDollySpeed )
                        .def( "focusOnTarget", &EditorCameraController::focusOnTarget )
                        .scope[def( "typeInfo", EditorCameraController::typeInfo )]];

        module( L )[class_<FpsCameraController, CameraController, SmartPtr<FpsCameraController>>(
                        "FpsCameraController" )
                        .def( "handleInputEvent", &FpsCameraController::handleInputEvent )
                        .def( "setPosition", &FpsCameraController::setPosition )
                        .def( "getPosition", &FpsCameraController::getPosition )
                        .def( "setTargetPosition", &FpsCameraController::setTargetPosition )
                        .def( "getTargetPosition", &FpsCameraController::getTargetPosition )
                        .def( "setOrientation", &FpsCameraController::setOrientation )
                        .def( "getOrientation", &FpsCameraController::getOrientation )
                        .def( "getDirection", &FpsCameraController::getDirection )
                        .def( "getMoveSpeed", &FpsCameraController::getMoveSpeed )
                        .def( "setMoveSpeed", &FpsCameraController::setMoveSpeed )
                        .def( "getRotationSpeed", &FpsCameraController::getRotationSpeed )
                        .def( "setRotationSpeed", &FpsCameraController::setRotationSpeed )
                        .def( "getMouseSensitivity", &FpsCameraController::getMouseSensitivity )
                        .def( "setMouseSensitivity", &FpsCameraController::setMouseSensitivity )
                        .def( "getYaw", &FpsCameraController::getYaw )
                        .def( "setYaw", &FpsCameraController::setYaw )
                        .def( "getPitch", &FpsCameraController::getPitch )
                        .def( "setPitch", &FpsCameraController::setPitch )
                        .scope[def( "typeInfo", FpsCameraController::typeInfo )]];

        module(
            L )[class_<SphericalCameraController, CameraController, SmartPtr<SphericalCameraController>>(
                    "SphericalCameraController" )
                    .def( "getSphericalCoords",
                          ( Vector3<real_Num> ( SphericalCameraController::* )() const ) &
                              SphericalCameraController::getSphericalCoords )
                    .def( "setSphericalCoords", &SphericalCameraController::setSphericalCoords )
                    .def( "setPosition", &SphericalCameraController::setPosition )
                    .def( "getPosition", &SphericalCameraController::getPosition )
                    .def( "setTargetPosition", &SphericalCameraController::setTargetPosition )
                    .def( "getTargetPosition", &SphericalCameraController::getTargetPosition )
                    .def( "setOrientation", &SphericalCameraController::setOrientation )
                    .def( "getOrientation", &SphericalCameraController::getOrientation )
                    .def( "setDirection", &SphericalCameraController::setDirection )
                    .def( "getDirection", &SphericalCameraController::getDirection )
                    .def( "getRotationSpeed", &SphericalCameraController::getRotationSpeed )
                    .def( "setRotationSpeed", &SphericalCameraController::setRotationSpeed )
                    .def( "getMaxDistance", &SphericalCameraController::getMaxDistance )
                    .def( "setMaxDistance", &SphericalCameraController::setMaxDistance )
                    .def( "getNearDistance", &SphericalCameraController::getNearDistance )
                    .def( "setNearDistance", &SphericalCameraController::setNearDistance )
                    .def( "getZoomSpeed", &SphericalCameraController::getZoomSpeed )
                    .def( "setZoomSpeed", &SphericalCameraController::setZoomSpeed )
                    .def( "getMoveSpeed", &SphericalCameraController::getMoveSpeed )
                    .def( "setMoveSpeed", &SphericalCameraController::setMoveSpeed )
                    .def( "isViewDirty", &SphericalCameraController::isViewDirty )
                    .def( "setViewDirty", &SphericalCameraController::setViewDirty )
                    .scope[def( "typeInfo", SphericalCameraController::typeInfo )]];

        module( L )[class_<ThirdPersonCameraController, CameraController,
                           SmartPtr<ThirdPersonCameraController>>( "ThirdPersonCameraController" )
                        .def( "getTarget", &ThirdPersonCameraController::getTarget )
                        .def( "setTarget", &ThirdPersonCameraController::setTarget )
                        .def( "getDistance", &ThirdPersonCameraController::getDistance )
                        .def( "setDistance", &ThirdPersonCameraController::setDistance )
                        .def( "getHeight", &ThirdPersonCameraController::getHeight )
                        .def( "setHeight", &ThirdPersonCameraController::setHeight )
                        .def( "getRotationSpeed", &ThirdPersonCameraController::getRotationSpeed )
                        .def( "setRotationSpeed", &ThirdPersonCameraController::setRotationSpeed )
                        .def( "getZoomSpeed", &ThirdPersonCameraController::getZoomSpeed )
                        .def( "setZoomSpeed", &ThirdPersonCameraController::setZoomSpeed )
                        .def( "getDamping", &ThirdPersonCameraController::getDamping )
                        .def( "setDamping", &ThirdPersonCameraController::setDamping )
                        .def( "getLookAtOffset", &ThirdPersonCameraController::getLookAtOffset )
                        .def( "setLookAtOffset", &ThirdPersonCameraController::setLookAtOffset )
                        .def( "getMinDistance", &ThirdPersonCameraController::getMinDistance )
                        .def( "setMinDistance", &ThirdPersonCameraController::setMinDistance )
                        .def( "getMaxDistance", &ThirdPersonCameraController::getMaxDistance )
                        .def( "setMaxDistance", &ThirdPersonCameraController::setMaxDistance )
                        .scope[def( "typeInfo", ThirdPersonCameraController::typeInfo )]];

        module( L )[class_<VehicleCameraController, CameraController, SmartPtr<VehicleCameraController>>(
                        "VehicleCameraController" )
                        .def( "getTarget", &VehicleCameraController::getTarget )
                        .def( "setTarget", &VehicleCameraController::setTarget )
                        .def( "getDistance", &VehicleCameraController::getDistance )
                        .def( "setDistance", &VehicleCameraController::setDistance )
                        .def( "getMinDistance", &VehicleCameraController::getMinDistance )
                        .def( "setMinDistance", &VehicleCameraController::setMinDistance )
                        .def( "getMaxDistance", &VehicleCameraController::getMaxDistance )
                        .def( "setMaxDistance", &VehicleCameraController::setMaxDistance )
                        .def( "getDistanceMultiplier", &VehicleCameraController::getDistanceMultiplier )
                        .def( "setDistanceMultiplier", &VehicleCameraController::setDistanceMultiplier )
                        .def( "getPositionLerpFactor", &VehicleCameraController::getPositionLerpFactor )
                        .def( "setPositionLerpFactor", &VehicleCameraController::setPositionLerpFactor )
                        .def( "getHeight", &VehicleCameraController::getHeight )
                        .def( "setHeight", &VehicleCameraController::setHeight )
                        .def( "getLookAtHeight", &VehicleCameraController::getLookAtHeight )
                        .def( "setLookAtHeight", &VehicleCameraController::setLookAtHeight )
                        .def( "getZoomSpeed", &VehicleCameraController::getZoomSpeed )
                        .def( "setZoomSpeed", &VehicleCameraController::setZoomSpeed )
                        .scope[def( "typeInfo", VehicleCameraController::typeInfo )]];

        module( L )[class_<ComponentEvent, IComponentEvent, SmartPtr<ComponentEvent>>( "ComponentEvent" )
                        .def( "addListener", &ComponentEvent::addListener )
                        .def( "removeListener", &ComponentEvent::removeListener )
                        .def( "removeListeners", &ComponentEvent::removeListeners )
                        .def( "getListeners", &ComponentEvent::getListeners )
                        .def( "setListeners", &ComponentEvent::setListeners )
                        .def( "getLabel", &ComponentEvent::getLabel )
                        .def( "setLabel", &ComponentEvent::setLabel )
                        .def( "getEventHash", &ComponentEvent::getEventHash )
                        .def( "setEventHash", &ComponentEvent::setEventHash )
                        .scope[def( "typeInfo", ComponentEvent::typeInfo )]];

        module( L )[class_<ComponentEventListener, IComponentEventListener,
                           SmartPtr<ComponentEventListener>>( "ComponentEventListener" )
                        .def( "getActor", &ComponentEventListener::getActor )
                        .def( "setActor", &ComponentEventListener::setActor )
                        .def( "getComponent", &ComponentEventListener::getComponent )
                        .def( "setComponent", &ComponentEventListener::setComponent )
                        .def( "getFunction", &ComponentEventListener::getFunction )
                        .def( "setFunction", &ComponentEventListener::setFunction )
                        .def( "getEvent", &ComponentEventListener::getEvent )
                        .def( "setEvent", &ComponentEventListener::setEvent )
                        .scope[def( "typeInfo", ComponentEventListener::typeInfo )]];

        module( L )[class_<RigidbodyListener, IEventListener, SmartPtr<RigidbodyListener>>(
                        "RigidbodyListener" )
                        .def( "getOwner", &RigidbodyListener::getOwner )
                        .def( "setOwner", &RigidbodyListener::setOwner )
                        .scope[def( "typeInfo", RigidbodyListener::typeInfo )]];

        module( L )[class_<TerrainBlendMap, SubComponent, SmartPtr<TerrainBlendMap>>( "TerrainBlendMap" )
                        .def( "getBlendMap", &TerrainBlendMap::getBlendMap )
                        .def( "setBlendMap", &TerrainBlendMap::setBlendMap )
                        .scope[def( "typeInfo", TerrainBlendMap::typeInfo )]];

        /*
        module( L )[class_<Component::ComponentFSMListener, FSMListener, SmartPtr<IFSMListener>>(
            "ComponentFSMListener" )];

        module( L )[class_<CameraController::EventListener, IEventListener, SmartPtr<IEventListener>>(
            "CameraControllerEventListener" )];

        module( L )[class_<EditorCameraController::EditorCameraInputListener, IEventListener,
                           SmartPtr<IEventListener>>( "EditorCameraInputListener" )];

        module( L )[class_<CarController::InputListener, IEventListener, SmartPtr<IEventListener>>(
            "CarControllerInputListener" )];

        module( L )[class_<CarController::VehicleCallback, vehicle::IVehicleCallback,
                           SmartPtr<vehicle::IVehicleCallback>>( "CarControllerVehicleCallback" )];
        module(
            L )[class_<Material::MaterialStateObjectListener, IEventListener, SmartPtr<IEventListener>>(
            "MaterialStateObjectListener" )];

        module( L )[class_<Material::MaterialStateListener, IStateListener, SmartPtr<IStateListener>>(
            "MaterialStateListener" )];

        module( L )[class_<Skybox::MaterialSharedListener, IEventListener, SmartPtr<IEventListener>>(
            "SkyboxMaterialSharedListener" )];

        module( L )[class_<UserComponent::ScriptReceiver, IScriptReceiver, SmartPtr<IScriptReceiver>>(
            "UserComponentScriptReceiver" )];

        module( L )[class_<NetworkView::CNetworkView, INetworkView, SmartPtr<INetworkView>>(
            "ComponentNetworkView" )];

        module( L )[class_<NetworkView::Listener, INetworkListener, SmartPtr<INetworkListener>>(
            "ComponentNetworkListener" )];
            */
    }
} // namespace workphone
