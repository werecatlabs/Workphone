#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Bindings/GraphicsBind.hpp"
#include <luabind/luabind.hpp>
#include "WPLuabind/ParamConverter.hpp"
#include "WPLuabind/SmartPtrConverter.hpp"
#include "WPLuabind/Helpers/AnimationControllerHelper.hpp"
#include "WPLuabind/Helpers/SceneNodeHelper.hpp"
#include "WPLuabind/Helpers/ViewportHelper.hpp"
#include "WPLuabind/Helpers/SceneManagerHelper.hpp"
#include "WPLuabind/Helpers/GraphicsSystemHelper.hpp"
#include "WPLuabind/Helpers/GraphicsObjectHelper.hpp"
#include "WPLuabind/Helpers/GraphicsContainerHelper.hpp"
#include "WPLuabind/Helpers/ResourceManagerHelper.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Graphics/IComputeShader.hpp>

namespace workphone
{
    using namespace render;

    // Material file selection needs a synchronous load before reading pass values.
    // The shared-object overload takes a SmartPtr argument that Lua nil cannot
    // convert, so expose the no-data form with graphics synchronization here.
    void IMaterial_load( IMaterial *material )
    {
        if( material )
        {
            auto application = core::IApplicationManager::instance();
            ScopedLock lock( application->getGraphicsSystem() );
            if( !material->isLoaded() )
                material->load( nullptr );
        }
    }

    lua_Integer IParticleSystem_getStateLua( IParticleSystem *particleSystem )
    {
        if( !particleSystem )
        {
            return static_cast<lua_Integer>( ParticleSystemState::Stopped );
        }

        return static_cast<lua_Integer>( particleSystem->getState() );
    }

    void IParticleSystem_setStateLua( IParticleSystem *particleSystem, lua_Integer state )
    {
        if( particleSystem )
        {
            particleSystem->setState( static_cast<ParticleSystemState>( static_cast<s32>( state ) ) );
        }
    }

    void IParticleSystem_play( IParticleSystem *particleSystem )
    {
        IParticleSystem_setStateLua( particleSystem,
                                     static_cast<lua_Integer>( ParticleSystemState::Started ) );
    }

    void IParticleSystem_stop( IParticleSystem *particleSystem )
    {
        IParticleSystem_setStateLua( particleSystem,
                                     static_cast<lua_Integer>( ParticleSystemState::Stopped ) );
    }

    void IParticleSystem_pause( IParticleSystem *particleSystem )
    {
        IParticleSystem_setStateLua( particleSystem,
                                     static_cast<lua_Integer>( ParticleSystemState::Paused ) );
    }

    SmartPtr<IParticleTechnique> IParticleSystem_addTechniqueNamed( IParticleSystem *particleSystem,
                                                                    const String    &name )
    {
        return particleSystem ? particleSystem->addTechnique( name ) : nullptr;
    }

    SmartPtr<IParticleTechnique> IParticleSystem_addTechniqueUnnamed( IParticleSystem *particleSystem )
    {
        return particleSystem ? particleSystem->addTechnique() : nullptr;
    }

    SmartPtr<IParticleEmitter> IParticleTechnique_addEmitterNamed( IParticleTechnique *technique,
                                                                   const String       &name )
    {
        return technique ? technique->addEmitter( name ) : nullptr;
    }

    SmartPtr<IParticleEmitter> IParticleTechnique_addEmitterUnnamed( IParticleTechnique *technique )
    {
        return technique ? technique->addEmitter() : nullptr;
    }

    SmartPtr<IParticleEmitter> IParticleTechnique_getEmitterByIndex( IParticleTechnique *technique,
                                                                     lua_Integer         index )
    {
        if( !technique || index < 0 )
        {
            return nullptr;
        }

        auto emitters = technique->getParticleEmitters();
        auto emitterIndex = static_cast<size_t>( index );
        return emitterIndex < emitters.size() ? emitters[emitterIndex] : nullptr;
    }

    SmartPtr<IGraphicsScene> IGraphicsSystem_getGraphicsSceneUnnamed( IGraphicsSystem *sys )
    {
        return sys ? sys->getGraphicsScene() : nullptr;
    }

    SmartPtr<IGraphicsScene> IGraphicsSystem_getGraphicsSceneNamed( IGraphicsSystem *sys,
                                                                    const String    &name )
    {
        return sys ? sys->getGraphicsScene( name ) : nullptr;
    }

    lua_Integer IGraphicsSystem_getStateTaskLua( IGraphicsSystem *sys )
    {
        return sys ? static_cast<lua_Integer>( sys->getStateTask() ) : 0;
    }

    lua_Integer IGraphicsSystem_getRenderTaskLua( IGraphicsSystem *sys )
    {
        return sys ? static_cast<lua_Integer>( sys->getRenderTask() ) : 0;
    }

    bool validateComputeDispatch( const render::IComputeShader               *shader,
                                  const render::IComputeShader::DispatchSize &size )
    {
        return shader->validateDispatch( size );
    }

    void bindGraphicsSystem( lua_State *L )
    {
        using namespace luabind;

        module(
            L )[class_<IGraphicsSystem, ISharedObject, SmartPtr<IObject>>( "GraphicsSystem" )
                    .def( "createConfiguration", &IGraphicsSystem::createConfiguration )
                    .def( "configure", &IGraphicsSystem::configure )
                    .def( "messagePump", &IGraphicsSystem::messagePump )
                    .def( "render", &IGraphicsSystem::render )
                    .def( "getDebug", &IGraphicsSystem::getDebug )
                    .def( "setDebug", &IGraphicsSystem::setDebug )
                    .def( "addGraphicsScene", &IGraphicsSystem::addGraphicsScene )
                    .def( "removeGraphicsScene", &IGraphicsSystem::removeGraphicsScene )
                    .def( "removeAllGraphicsScenes", &IGraphicsSystem::removeAllGraphicsScenes )
                    .def( "clearGraphicScenes", &IGraphicsSystem::clearGraphicScenes )
                    .def( "getGraphicsScene", &IGraphicsSystem_getGraphicsSceneUnnamed )
                    .def( "getGraphicsSceneNamed", &IGraphicsSystem_getGraphicsSceneNamed )
                    .def( "getGraphicsSceneById", &IGraphicsSystem::getGraphicsSceneById )
                    .def( "getSceneManagers", &IGraphicsSystem::getSceneManagers )
                    .def( "getOverlayManager", &IGraphicsSystem::getOverlayManager )
                    .def( "getResourceGroupManager", &IGraphicsSystem::getResourceGroupManager )
                    .def( "getMaterialManager", &IGraphicsSystem::getMaterialManager )
                    .def( "getTextureManager", &IGraphicsSystem::getTextureManager )
                    .def( "getInstanceManager", &IGraphicsSystem::getInstanceManager )
                    .def( "getRenderer", &IGraphicsSystem::getRenderer )
                    .def( "getFontManager", &IGraphicsSystem::getFontManager )
                    .def( "createRenderWindow", &IGraphicsSystem::createRenderWindow )
                    .def( "destroyRenderWindow", &IGraphicsSystem::destroyRenderWindow )
                    .def( "getRenderWindow", &IGraphicsSystem::getRenderWindow )
                    .def( "getDefaultWindow", &IGraphicsSystem::getDefaultWindow )
                    .def( "setDefaultWindow", &IGraphicsSystem::setDefaultWindow )
                    .def( "getWindows", &IGraphicsSystem::getWindows )
                    .def( "addDeferredShadingSystem", &IGraphicsSystem::addDeferredShadingSystem )
                    .def( "removeDeferredShadingSystem", &IGraphicsSystem::removeDeferredShadingSystem )
                    .def( "getDeferredShadingSystems", &IGraphicsSystem::getDeferredShadingSystems )
                    .def( "loadObject", &IGraphicsSystem::loadObject )
                    .def( "reloadObject", &IGraphicsSystem::reloadObject )
                    .def( "unloadObject", &IGraphicsSystem::unloadObject )
                    .def( "clearObjectQueues", &IGraphicsSystem::clearObjectQueues )
                    .def( "setupRenderer", &IGraphicsSystem::setupRenderer )
                    .def( "getStateTask", IGraphicsSystem_getStateTaskLua )
                    .def( "getRenderTask", IGraphicsSystem_getRenderTaskLua )
                    .def( "getMeshConverter", &IGraphicsSystem::getMeshConverter )
                    .def( "setMeshConverter", &IGraphicsSystem::setMeshConverter )
                    .def( "getFactoryManager", &IGraphicsSystem::getFactoryManager )
                    .def( "setFactoryManager", &IGraphicsSystem::setFactoryManager )
                    .def( "getStringPool", &IGraphicsSystem::getStringPool )
                    .def( "setStringPool", &IGraphicsSystem::setStringPool )
                    .def( "getLoadPriority", &IGraphicsSystem::getLoadPriority )
                    .def( "getStateContextPtr", &IGraphicsSystem::getStateContextPtr )
                    .scope[def( "typeInfo", IGraphicsSystem::typeInfo )]];

        module( L )[class_<IGraphicsDeferredShading, ISharedObject, SmartPtr<IObject>>(
                        "IGraphicsDeferredShading" )
                        .def( "getSSAO", &IGraphicsDeferredShading::getSSAO )
                        .def( "setSSAO", &IGraphicsDeferredShading::setSSAO )
                        .def( "getActive", &IGraphicsDeferredShading::getActive )
                        .def( "setActive", &IGraphicsDeferredShading::setActive )
                        .def( "getShadowsEnabled", &IGraphicsDeferredShading::getShadowsEnabled )
                        .def( "setShadowsEnabled", &IGraphicsDeferredShading::setShadowsEnabled )
                        .scope[def( "typeInfo", IGraphicsDeferredShading::typeInfo )]];

        module( L )[class_<IViewport, ISharedObject, SmartPtr<IObject>>( "Viewport" )
                        .def( "getCamera", &IViewport::getCamera )
                        .def( "setCamera", &IViewport::setCamera )
                        .def( "getActualSize", ViewportHelper::_getActualSize )
                        .def( "setBackgroundColour", &IViewport::setBackgroundColour )
                        .def( "getBackgroundColour", &IViewport::getBackgroundColour )
                        .def( "setClearEveryFrame", ViewportHelper::_setClearEveryFrame )
                        .def( "setClearEveryFrame", ViewportHelper::_setClearEveryFrameFlags )
                        .def( "getClearEveryFrame", &IViewport::getClearEveryFrame )
                        .def( "getClearBuffers", &IViewport::getClearBuffers )
                        .def( "getViewportId", &IViewport::getViewportId )
                        .def( "setViewportId", &IViewport::setViewportId )
                        .def( "getZOrder", &IViewport::getZOrder )
                        .def( "setZOrder", &IViewport::setZOrder )
                        .def( "getPosition", &IViewport::getPosition )
                        .def( "setPosition", &IViewport::setPosition )
                        .def( "getActualPosition", &IViewport::getActualPosition )
                        .def( "getSize", &IViewport::getSize )
                        .def( "setSize", &IViewport::setSize )
                        .def( "setMaterialScheme", &IViewport::setMaterialScheme )
                        .def( "getMaterialScheme", &IViewport::getMaterialScheme )
                        .def( "setEnableSceneRender", &IViewport::setEnableSceneRender )
                        .def( "getEnableSceneRender", &IViewport::getEnableSceneRender )
                        .def( "setOverlaysEnabled", &IViewport::setOverlaysEnabled )
                        .def( "getOverlaysEnabled", &IViewport::getOverlaysEnabled )
                        .def( "setEnableUI", &IViewport::setEnableUI )
                        .def( "getEnableUI", &IViewport::getEnableUI )
                        .def( "setSkiesEnabled", &IViewport::setSkiesEnabled )
                        .def( "getSkiesEnabled", &IViewport::getSkiesEnabled )
                        .def( "setShadowsEnabled", &IViewport::setShadowsEnabled )
                        .def( "getShadowsEnabled", &IViewport::getShadowsEnabled )
                        .def( "setVisibilityMask", ViewportHelper::setVisibilityMask )
                        .def( "getVisibilityMask", ViewportHelper::getVisibilityMask )
                        .def( "setAutoUpdated", &IViewport::setAutoUpdated )
                        .def( "isAutoUpdated", &IViewport::isAutoUpdated )
                        .def( "getBackgroundTexture", &IViewport::getBackgroundTexture )
                        .def( "setBackgroundTexture", &IViewport::setBackgroundTexture )
                        .def( "getBackgroundTextureName", &IViewport::getBackgroundTextureName )
                        .def( "setBackgroundTextureName", &IViewport::setBackgroundTextureName )
                        .def( "getRenderTarget", &IViewport::getRenderTarget )
                        .def( "setRenderTarget", &IViewport::setRenderTarget )
                        .def( "isActive", &IViewport::isActive )
                        .def( "setActive", &IViewport::setActive )
                        .scope[def( "typeInfo", IViewport::typeInfo )]];

        module( L )[class_<IRenderTarget, ISharedObject, SmartPtr<IObject>>( "RenderTarget" )
                        .def( "addViewport", ViewportHelper::_addViewportMinArgs )
                        .def( "addViewport", ViewportHelper::_addViewportAllArgs )
                        .def( "addViewport", ViewportHelper::_addViewportZorderArgs )
                        .def( "getNumViewports", &IRenderTarget::getNumViewports )
                        .def( "getViewport", &IRenderTarget::getViewport )
                        .def( "getViewportById", &IRenderTarget::getViewportById )
                        .def( "removeViewport", &IRenderTarget::removeViewport )
                        .def( "removeAllViewports", &IRenderTarget::removeAllViewports )
                        .def( "getSize", &IRenderTarget::getSize )
                        .def( "getColourDepth", &IRenderTarget::getColourDepth )

                        .def( "setAutoUpdated", &IRenderTarget::setAutoUpdated )
                        .def( "isAutoUpdated", &IRenderTarget::isAutoUpdated )
                        .scope[def( "typeInfo", IRenderTarget::typeInfo )]];

        module(
            L )[class_<IGraphicsWindow, IRenderTarget, SmartPtr<IObject>>( "Window" )
                    //.def( "setFullscreen", &IGraphicsWindow::setFullscreen )
                    .def( "destroy", &IGraphicsWindow::destroy )
                    .def( "resize", &IGraphicsWindow::resize )
                    .def( "windowMovedOrResized", &IGraphicsWindow::windowMovedOrResized )
                    .def( "reposition", &IGraphicsWindow::reposition )
                    .def( "isVisible", &IGraphicsWindow::isVisible )
                    .def( "setVisible", &IGraphicsWindow::setVisible )
                    .def( "isActive", &IGraphicsWindow::isActive )
                    .def( "isClosed", &IGraphicsWindow::isClosed )
                    .def( "isPrimary", &IGraphicsWindow::isPrimary )
                    .def( "isFullScreen", &IGraphicsWindow::isFullScreen )
                    .def( "isDeactivatedOnFocusChange", &IGraphicsWindow::isDeactivatedOnFocusChange )
                    .def( "setDeactivateOnFocusChange", &IGraphicsWindow::setDeactivateOnFocusChange )
                    .scope[def( "typeInfo", IGraphicsWindow::typeInfo )]];

        module( L )[class_<IGraphicsScene, ISharedObject, SmartPtr<ISharedObject>>( "GraphicsScene" )
                        //.def( "update", &IGraphicsSceneManager::_update )
                        .def( "addGraphicsObject", SceneManagerHelper::_addGraphicsObject )
                        .def( "addGraphicsObject", SceneManagerHelper::_addGraphicsObjectNamed )

                        .def( "addSceneNode", SceneManagerHelper::_addSceneNode )
                        .def( "getSceneNode", &IGraphicsScene::getSceneNode )
                        .def( "getSceneNodeById", &IGraphicsScene::getSceneNodeById )
                        .def( "getRootSceneNode", SceneManagerHelper::_getRootSceneNode )

                        //.def( "addLight", &IGraphicsScene::addLight )
                        .def( "addMesh", SceneManagerHelper::_addMesh )
                        .def( "addMesh", SceneManagerHelper::_addMeshNamed )
                        .def( "addParticleSystem", SceneManagerHelper::_addParticleSystem )
                        .def( "getParticleSystem", SceneManagerHelper::_getParticleSystem )
                        //.def( "addCamera", &IGraphicsScene::addCamera )
                        //.def( "getCamera", &IGraphicsSceneManager::getCamera )
                        //.def( "hasCamera", &IGraphicsScene::hasCamera )

                        .def( "clear", &IGraphicsScene::clear )

                        .def( "hasAnimation", &IGraphicsScene::hasAnimation )

                        .def( "loadSceneFile", SceneManagerHelper::_loadSceneFile )
                        //.def( "loadScene", &IGraphicsSceneManager::loadScene )

                        .def( "setSkyBox",
                              static_cast<void ( IGraphicsScene::* )(
                                  bool, SmartPtr<IMaterial>, f32, bool )>( &IGraphicsScene::setSkyBox ) )

                        .def( "getEnableShadows", &IGraphicsScene::getEnableShadows )
                        .def( "setEnableShadows", &IGraphicsScene::setEnableShadows )

                        //.def( "createInstanceManager", &IGraphicsScene::createInstanceManager )
                        //.def( "createInstancedObject", &IGraphicsScene::createInstancedObject )
                        //.def( "destroyInstancedObject", &IGraphicsScene::destroyInstancedObject )
                        .scope[def( "typeInfo", IGraphicsScene::typeInfo )]];

        module( L )[class_<IGraphicsSceneNode, ISharedObject, SmartPtr<IObject>>( "IGraphicsSceneNode" )
                        .def( "setMaterialName", SceneNodeHelper::setMaterialName )
                        .def( "setMaterialName", SceneNodeHelper::setMaterialNameCascade )
                        .def( "getParent", &IGraphicsSceneNode::getParent )

                        .def( "addChild", &IGraphicsSceneNode::addChild )
                        .def( "addChildSceneNode", SceneNodeHelper::_addChildSceneNode )
                        .def( "addChildSceneNode", SceneNodeHelper::_addChildSceneNodeNamed )
                        .def( "attachObject", SceneNodeHelper::_attachObject )
                        .def( "detachObject", &IGraphicsSceneNode::detachObject )
                        .def( "detachAllObjects", &IGraphicsSceneNode::detachAllObjects )

                        .def( "setPosition", &IGraphicsSceneNode::setPosition )
                        .def( "getPosition", &IGraphicsSceneNode::getPosition )

                        .def( "lookAt", &IGraphicsSceneNode::lookAt )

                        .def( "setScale", &IGraphicsSceneNode::setScale )
                        .def( "getScale", &IGraphicsSceneNode::getScale )

                        .def( "setRotationFromDegrees", &IGraphicsSceneNode::setRotationFromDegrees )
                        .def( "setOrientation", &IGraphicsSceneNode::setOrientation )
                        .def( "getOrientation", &IGraphicsSceneNode::getOrientation )

                        .def( "setVisible", SceneNodeHelper::_setVisible )

                        .def( "setVisibilityFlags", SceneNodeHelper::_setVisibilityFlags )
                        .def( "getVisibilityFlags", SceneNodeHelper::_getVisibilityFlags )
                        .scope[def( "typeInfo", IGraphicsSceneNode::typeInfo )]];

        // graphics objects
        module( L )[class_<IGraphicsObject, ISharedObject, SharedPtr<ISharedObject>>( "IGraphicsObject" )
                        .def( "setMaterialName", GraphicsObjectHelper::_setMaterialName )
                        //.def("setMaterialName", GraphicsObjectHelper::_setMaterialNameStr )
                        .def( "setMaterialName", GraphicsObjectHelper::_setMaterialNameIndx )
                        // .def( "getMaterialName", &IGraphicsObject::getMaterialName )

                        .def( "getCastShadows", &IGraphicsObject::getCastShadows )
                        .def( "setCastShadows", &IGraphicsObject::setCastShadows )
                        //.def( "getRecieveShadows", &IGraphicsObject::getRecieveShadows )
                        //.def( "setRecieveShadows", &IGraphicsObject::setRecieveShadows )
                        .def( "setVisible", &IGraphicsObject::setVisible )
                        .def( "isVisible", &IGraphicsObject::isVisible )
                        .def( "setZOrder", &IGraphicsObject::setZOrder )
                        .def( "getZOrder", &IGraphicsObject::getZOrder )

                        .def( "setVisibilityFlags", GraphicsObjectHelper::_setVisibilityFlags )
                        .def( "getVisibilityFlags", GraphicsObjectHelper::_getVisibilityFlags )

                        .scope[def( "typeInfo", IGraphicsObject::typeInfo )]];

        module(
            L )[class_<IInstancedObject, IGraphicsObject, SharedPtr<ISharedObject>>( "IInstancedObject" )
                    .def( "setPosition", &IInstancedObject::setPosition )
                    .def( "getPosition", &IInstancedObject::getPosition )
                    .def( "setScale", &IInstancedObject::setScale )
                    .def( "getScale", &IInstancedObject::getScale )
                    .def( "setOrientation", &IInstancedObject::setOrientation )
                    .def( "getOrientation", &IInstancedObject::getOrientation )

                    .def( "setCustomParam", &IInstancedObject::setCustomParam )
                    .def( "getCustomParam", &IInstancedObject::getCustomParam )
                    .scope[def( "typeInfo", IInstancedObject::typeInfo )]];

        module( L )[class_<IFrustum, IGraphicsObject, SharedPtr<ISharedObject>>( "IFrustum" )
                        .def( "getNearClipDistance", &IFrustum::getNearClipDistance )
                        .def( "setNearClipDistance", &IFrustum::setNearClipDistance )
                        .def( "getFarClipDistance", &IFrustum::getFarClipDistance )
                        .def( "setFarClipDistance", &IFrustum::setFarClipDistance )
                        .def( "getAspectRatio", &IFrustum::getAspectRatio )
                        .def( "setAspectRatio", &IFrustum::setAspectRatio )
                        .scope[def( "typeInfo", IFrustum::typeInfo )]];

        module( L )[class_<IGraphicsCamera, IFrustum, SharedPtr<ISharedObject>>( "IGraphicsCamera" )
                        .def( "setNearClipDistance", &IGraphicsCamera::setNearClipDistance )
                        .def( "setFarClipDistance", &IGraphicsCamera::setFarClipDistance )
                        .scope[def( "typeInfo", IGraphicsCamera::typeInfo )]];

        module( L )[class_<IGraphicsMesh, IGraphicsObject, SharedPtr<ISharedObject>>( "IGraphicsMesh" )
                        .def( "getAnimationController", &IGraphicsMesh::getAnimationController )
                        .scope[def( "typeInfo", IGraphicsMesh::typeInfo )]];

        module( L )[class_<IGraphicsLight, IGraphicsObject, SharedPtr<ISharedObject>>( "IGraphicsLight" )
                        //.def( "setType", &ILight::setType )
                        //.def( "getType", &ILight::getType )

                        .def( "setDiffuseColour", &IGraphicsLight::setDiffuseColour )
                        .def( "getDiffuseColour", &IGraphicsLight::getDiffuseColour )

                        .def( "setSpecularColour", &IGraphicsLight::setSpecularColour )
                        .def( "getSpecularColour", &IGraphicsLight::getSpecularColour )

                        .def( "setAttenuation", &IGraphicsLight::setAttenuation )

                        .def( "getAttenuationRange", &IGraphicsLight::getAttenuationRange )
                        .def( "getAttenuationConstant", &IGraphicsLight::getAttenuationConstant )
                        .def( "getAttenuationLinear", &IGraphicsLight::getAttenuationLinear )
                        .def( "getAttenuationQuadric", &IGraphicsLight::getAttenuationQuadric )
                        .scope[def( "typeInfo", IGraphicsLight::typeInfo )]];

        //
        // Animation
        //
        module( L )[class_<IAnimationController, ISharedObject, SharedPtr<ISharedObject>>(
                        "AnimationController" )
                        //.def( "setAnimationEnabled", AnimationControllerHelper::_setAnimationEnabled )
                        //.def( "setAnimationEnabled",
                        //     AnimationControllerHelper::_setAnimationEnabledPosition )
                        .def( "isAnimationEnabled", &IAnimationController::isAnimationEnabled )
                        .def( "stopAllAnimations", &IAnimationController::stopAllAnimations )
                        .def( "hasAnimationEnded", &IAnimationController::hasAnimationEnded )
                        .def( "hasAnimation", &IAnimationController::hasAnimation )
                        .def( "setAnimationLoop", &IAnimationController::setAnimationLoop )
                        .def( "isAnimationLooping", &IAnimationController::isAnimationLooping )
                        .def( "setAnimationReversed", &IAnimationController::setAnimationReversed )
                        .def( "isAnimationReversed", &IAnimationController::isAnimationReversed )
                        .def( "setTimePosition", &IAnimationController::setTimePosition )
                        .def( "getTimePosition", &IAnimationController::getTimePosition )
                        .def( "getAnimationLength", &IAnimationController::getAnimationLength )
                        .scope[def( "typeInfo", IAnimationController::typeInfo )]];

        module(
            L )[class_<IAnimationStateController, ISharedObject, SharedPtr<ISharedObject>>(
                    "AnimationStateController" )
                    //.def( "setAnimationEnabled", AnimationControllerHelper::_setAnimationStateEnabled )
                    //.def( "setAnimationEnabled",
                    //      AnimationControllerHelper::_setAnimationStateEnabledPosition )
                    .def( "isAnimationEnabled", &IAnimationStateController::isAnimationEnabled )
                    .def( "stopAllAnimations", &IAnimationStateController::stopAllAnimations )
                    .def( "hasAnimationEnded", &IAnimationStateController::hasAnimationEnded )
                    .def( "hasAnimation", &IAnimationStateController::hasAnimation )
                    .def( "setAnimationLoop", &IAnimationStateController::setAnimationLoop )
                    .def( "isAnimationLooping", &IAnimationStateController::isAnimationLooping )
                    .def( "setAnimationReversed", &IAnimationStateController::setAnimationReversed )
                    .def( "isAnimationReversed", &IAnimationStateController::isAnimationReversed )
                    .def( "setTimePosition", &IAnimationStateController::setTimePosition )
                    .def( "getTimePosition", &IAnimationStateController::getTimePosition )
                    //.def("getAnimationLength", &IAnimationStateController::getAnimationLength )
                    .scope[def( "typeInfo", IAnimationStateController::typeInfo )]];

        module(
            L )[class_<ParticleSystemState>( "ParticleSystemState" )
                    .enum_( "constants" )
                        [value( "Stopped", static_cast<u32>( ParticleSystemState::Stopped ) ),
                         value( "Started", static_cast<u32>( ParticleSystemState::Started ) ),
                         value( "Paused", static_cast<u32>( ParticleSystemState::Paused ) ),
                         value( "PausedForTime",
                                static_cast<u32>( ParticleSystemState::PausedForTime ) ),
                         value( "StoppedFade", static_cast<u32>( ParticleSystemState::StoppedFade ) )]];

        module(
            L )[class_<IParticleNode, ISharedObject, SharedPtr<ISharedObject>>( "ParticleNode" )
                    .def( "addChild", static_cast<void ( IParticleNode::* )( SmartPtr<IParticleNode> )>(
                                          &IParticleNode::addChild ) )
                    .def( "removeChild", &IParticleNode::removeChild )
                    .def( "remove", &IParticleNode::remove )
                    .def( "getNumChildren", &IParticleNode::getNumChildren )
                    .def( "getChildByIndex", &IParticleNode::getChildByIndex )
                    .def( "getChildren", &IParticleNode::getChildren )
                    .def( "getParent", &IParticleNode::getParent )
                    .def( "setParent", &IParticleNode::setParent )
                    .def( "setPosition", &IParticleNode::setPosition )
                    .def( "getPosition", &IParticleNode::getPosition )
                    .def( "getAbsolutePosition", &IParticleNode::getAbsolutePosition )
                    .def( "getParticleSystem", &IParticleNode::getParticleSystem )
                    .def( "setParticleSystem", &IParticleNode::setParticleSystem )
                    .scope[def( "typeInfo", IParticleNode::typeInfo )]];

        module(
            L )[class_<IParticleSystem, IGraphicsObject, SharedPtr<ISharedObject>>( "IParticleSystem" )
                    .def( "reload", &IParticleSystem::reload )
                    //.def( "prepare", &IParticleSystem::prepare )
                    .def( "play", IParticleSystem_play )
                    .def( "stop", IParticleSystem_stop )
                    //.def( "stopFade", &IParticleSystem::stopFade )

                    .def( "pause", IParticleSystem_pause )
                    //.def( "pause", ( void( IParticleSystem::* )( float ) ) & IParticleSystem::pause )
                    .def( "resume", IParticleSystem_play )

                    .def( "setFastForward", &IParticleSystem::setFastForward )
                    .def( "getFastForwardTime", &IParticleSystem::getFastForwardTime )
                    .def( "getFastForwardInterval", &IParticleSystem::getFastForwardInterval )

                    .def( "setTemplateName", &IParticleSystem::setTemplateName )
                    .def( "getTemplateName", &IParticleSystem::getTemplateName )

                    .def( "setScale", &IParticleSystem::setScale )
                    .def( "getScale", &IParticleSystem::getScale )

                    .def( "getState", IParticleSystem_getStateLua )
                    .def( "setState", IParticleSystem_setStateLua )

                    .def( "addTechnique", IParticleSystem_addTechniqueUnnamed )
                    .def( "addTechnique", IParticleSystem_addTechniqueNamed )
                    .def( "removeTechnique", &IParticleSystem::removeTechnique )
                    .def( "getTechnique", &IParticleSystem::getTechnique )
                    .def( "addParticle", &IParticleSystem::addParticle )
                    .scope[def( "typeInfo", IParticleSystem::typeInfo )]];

        module( L )[class_<IParticleTechnique, IParticleNode, SharedPtr<ISharedObject>>(
                        "ParticleTechnique" )
                        .def( "addEmitter", IParticleTechnique_addEmitterUnnamed )
                        .def( "addEmitter", IParticleTechnique_addEmitterNamed )
                        .def( "removeEmitter", &IParticleTechnique::removeEmitter )
                        .def( "getEmitterByIndex", IParticleTechnique_getEmitterByIndex )
                        .def( "getEmitterByName", &IParticleTechnique::getEmitterByName )
                        .def( "getParticleEmitters", &IParticleTechnique::getParticleEmitters )
                        .def( "clearParticles", &IParticleTechnique::clearParticles )
                        .scope[def( "typeInfo", IParticleTechnique::typeInfo )]];

        module(
            L )[class_<IParticleEmitter, IParticleNode, SharedPtr<ISharedObject>>( "ParticleEmitter" )
                    .def( "getParticleSize", &IParticleEmitter::getParticleSize )
                    .def( "setParticleSize", &IParticleEmitter::setParticleSize )
                    .def( "getParticleSizeMin", &IParticleEmitter::getParticleSizeMin )
                    .def( "setParticleSizeMin", &IParticleEmitter::setParticleSizeMin )
                    .def( "getParticleSizeMax", &IParticleEmitter::getParticleSizeMax )
                    .def( "setParticleSizeMax", &IParticleEmitter::setParticleSizeMax )
                    .def( "getDirection", &IParticleEmitter::getDirection )
                    .def( "setDirection", &IParticleEmitter::setDirection )
                    .def( "getEmissionRate", &IParticleEmitter::getEmissionRate )
                    .def( "setEmissionRate", &IParticleEmitter::setEmissionRate )
                    .def( "getEmissionRateMin", &IParticleEmitter::getEmissionRateMin )
                    .def( "setEmissionRateMin", &IParticleEmitter::setEmissionRateMin )
                    .def( "getEmissionRateMax", &IParticleEmitter::getEmissionRateMax )
                    .def( "setEmissionRateMax", &IParticleEmitter::setEmissionRateMax )
                    .def( "getTimeToLive", &IParticleEmitter::getTimeToLive )
                    .def( "setTimeToLive", &IParticleEmitter::setTimeToLive )
                    .def( "getTimeToLiveMin", &IParticleEmitter::getTimeToLiveMin )
                    .def( "setTimeToLiveMin", &IParticleEmitter::setTimeToLiveMin )
                    .def( "getTimeToLiveMax", &IParticleEmitter::getTimeToLiveMax )
                    .def( "setTimeToLiveMax", &IParticleEmitter::setTimeToLiveMax )
                    .def( "getVelocity", &IParticleEmitter::getVelocity )
                    .def( "setVelocity", &IParticleEmitter::setVelocity )
                    .def( "getVelocityMin", &IParticleEmitter::getVelocityMin )
                    .def( "setVelocityMin", &IParticleEmitter::setVelocityMin )
                    .def( "getVelocityMax", &IParticleEmitter::getVelocityMax )
                    .def( "setVelocityMax", &IParticleEmitter::setVelocityMax )
                    .scope[def( "typeInfo", IParticleEmitter::typeInfo )]];

        //
        // Material system
        //
        // module( L )[class_<IMaterialManager, IResourceManager, SmartPtr<IObject>>(
        //                "MaterialManager" )
        //                .def( "cloneMaterial", &IMaterialManager::cloneMaterial )];

        module( L )[class_<MaterialType>( "MaterialType" )
                        .enum_( "values" )[value( "Standard", MaterialType::Standard ),
                                           value( "StandardSpecular", MaterialType::StandardSpecular ),
                                           value( "StandardTriPlanar", MaterialType::StandardTriPlanar ),
                                           value( "TerrainStandard", MaterialType::TerrainStandard ),
                                           value( "TerrainSpecular", MaterialType::TerrainSpecular ),
                                           value( "TerrainDiffuse", MaterialType::TerrainDiffuse ),
                                           value( "Skybox", MaterialType::Skybox ),
                                           value( "SkyboxCubemap", MaterialType::SkyboxCubemap ),
                                           value( "UI", MaterialType::UI ),
                                           value( "Custom", MaterialType::Custom ),
                                           value( "Count", MaterialType::Count )]];

        module(
            L )[class_<IMaterial, IResource, SmartPtr<ISharedObject>>( "IMaterial" )
                    .def( "load", &IMaterial_load )
                    .def( "getRoot", &IMaterial::getRoot )
                    .def( "setRoot", &IMaterial::setRoot )
                    .def( "createTechnique", &IMaterial::createTechnique )
                    .def( "removeTechnique", &IMaterial::removeTechnique )
                    .def( "removeAllTechniques", &IMaterial::removeAllTechniques )
                    .def( "getTechniques", &IMaterial::getTechniques )
                    .def( "setTechniques", &IMaterial::setTechniques )
                    .def( "getTechniqueByScheme", &IMaterial::getTechniqueByScheme )
                    .def( "getNumTechniques", &IMaterial::getNumTechniques )
                    .def( "setTexture", static_cast<void ( IMaterial::* )( SmartPtr<ITexture>, u32 )>(
                                            &IMaterial::setTexture ) )
                    .def( "setTexture", static_cast<void ( IMaterial::* )( const String &, u32 )>(
                                            &IMaterial::setTexture ) )
                    .def( "getTextureName", &IMaterial::getTextureName )
                    .def( "getTexture", &IMaterial::getTexture )
                    //.def( "setCubicTexture", static_cast<void ( IMaterial::* )(
                    //                             Array<SmartPtr<render::ITexture>> &,
                    //                             u32 )>( &IMaterial::setCubicTexture ) )
                    .def( "setCubicTexture",
                          static_cast<void ( IMaterial::* )( const String &, bool, u32 )>(
                              &IMaterial::setCubicTexture ) )
                    .def( "setScale", &IMaterial::setScale )
                    .def( "getMetalness", &IMaterial::getMetalness )
                    .def( "setMetalness", &IMaterial::setMetalness )
                    .def( "getRoughness", &IMaterial::getRoughness )
                    .def( "setRoughness", &IMaterial::setRoughness )
                    .def( "getDiffuse", &IMaterial::getDiffuse )
                    .def( "setDiffuse", &IMaterial::setDiffuse )
                    .def( "getSpecular", &IMaterial::getSpecular )
                    .def( "setSpecular", &IMaterial::setSpecular )
                    .def( "getEmissive", &IMaterial::getEmissive )
                    .def( "setEmissive", &IMaterial::setEmissive )

                    .def( "isTransparent", &IMaterial::isTransparent )
                    .def( "setTransparent", &IMaterial::setTransparent )
                    .def( "isCutout", &IMaterial::isCutout )
                    .def( "setCutout", &IMaterial::setCutout )
                    .def( "isEmissionEnabled", &IMaterial::isEmissionEnabled )
                    .def( "setEmissionEnabled", &IMaterial::setEmissionEnabled )
                    .def( "isRefractionEnabled", &IMaterial::isRefractionEnabled )
                    .def( "setRefractionEnabled", &IMaterial::setRefractionEnabled )
                    .def( "setEditorFloat", &IMaterial::setEditorFloat )
                    .def( "getEditorFloat", &IMaterial::getEditorFloat )
                    .def( "setEditorUInt", &IMaterial::setEditorUInt )
                    .def( "getEditorUInt", &IMaterial::getEditorUInt )
                    .def( "setEditorBool", &IMaterial::setEditorBool )
                    .def( "getEditorBool", &IMaterial::getEditorBool )
                    .def( "setEditorString", &IMaterial::setEditorString )
                    .def( "getEditorString", &IMaterial::getEditorString )
                    .def( "setRenderMode", &IMaterial::setRenderMode )
                    .def( "getRenderMode", &IMaterial::getRenderMode )
                    .def( "setWorkflow", &IMaterial::setWorkflow )
                    .def( "getWorkflow", &IMaterial::getWorkflow )
                    .def( "setSpecularAmount", &IMaterial::setSpecularAmount )
                    .def( "getSpecularAmount", &IMaterial::getSpecularAmount )
                    .def( "setNormalStrength", &IMaterial::setNormalStrength )
                    .def( "getNormalStrength", &IMaterial::getNormalStrength )
                    .def( "setDetailNormalStrength", &IMaterial::setDetailNormalStrength )
                    .def( "getDetailNormalStrength", &IMaterial::getDetailNormalStrength )
                    .def( "setAlphaClip", &IMaterial::setAlphaClip )
                    .def( "getAlphaClip", &IMaterial::getAlphaClip )
                    .def( "setOpacity", &IMaterial::setOpacity )
                    .def( "getOpacity", &IMaterial::getOpacity )
                    .def( "setBlendMode", &IMaterial::setBlendMode )
                    .def( "getBlendMode", &IMaterial::getBlendMode )
                    .def( "setDepthWrite", &IMaterial::setDepthWrite )
                    .def( "getDepthWrite", &IMaterial::getDepthWrite )
                    .def( "setDepthTest", &IMaterial::setDepthTest )
                    .def( "getDepthTest", &IMaterial::getDepthTest )
                    .def( "setCullMode", &IMaterial::setCullMode )
                    .def( "getCullMode", &IMaterial::getCullMode )
                    .def( "setUVTiling", &IMaterial::setUVTiling )
                    .def( "getUVTiling", &IMaterial::getUVTiling )
                    .def( "setUVOffset", &IMaterial::setUVOffset )
                    .def( "getUVOffset", &IMaterial::getUVOffset )
                    .def( "setUVRotation", &IMaterial::setUVRotation )
                    .def( "getUVRotation", &IMaterial::getUVRotation )
                    .def( "setUVProjection", &IMaterial::setUVProjection )
                    .def( "getUVProjection", &IMaterial::getUVProjection )
                    .def( "setUVSet", &IMaterial::setUVSet )
                    .def( "getUVSet", &IMaterial::getUVSet )
                    .def( "setTriplanarScale", &IMaterial::setTriplanarScale )
                    .def( "getTriplanarScale", &IMaterial::getTriplanarScale )
                    //.def( "getAlphaCutoff", &IMaterial::getAlphaCutoff )
                    //.def( "setAlphaCutoff", &IMaterial::setAlphaCutoff )

                    .def( "getMaterialType",
                          reinterpret_cast<u32 ( IMaterial::* )() const>( &IMaterial::getMaterialType ) )
                    .def( "setMaterialType",
                          reinterpret_cast<void ( IMaterial::* )( u32 )>( &IMaterial::setMaterialType ) )
                    .scope[def( "typeInfo", IMaterial::typeInfo )]];

        // module( L )[class_<IMaterialNodePass, IScriptObject, SmartPtr<IObject>>( "Pass" )
        //                 .def( "getName", &IMaterialNodePass::getName )
        //                 .def( "setName", &IMaterialNodePass::setName )
        //                 .def( "setSceneBlending", &IMaterialNodePass::setSceneBlending )
        //                 .def( "isDepthCheckEnabled", &IMaterialNodePass::isDepthCheckEnabled )
        //                 .def( "setDepthCheckEnabled", &IMaterialNodePass::setDepthCheckEnabled )

        //                .def( "isDepthWriteEnabled", &IMaterialNodePass::isDepthWriteEnabled )
        //                .def( "setDepthWriteEnabled", &IMaterialNodePass::setDepthWriteEnabled )];

        // module( L )[class_<IMaterialNodeTexture, IScriptObject, SmartPtr<IObject>>(
        //                 "TextureUnit" )
        //                 .def( "getTextureName", &IMaterialNodeTexture::getTextureName )
        //                 .def( "setTextureName", &IMaterialNodeTexture::setTextureName )];

        // module( L )[class_<ICompositorManager, IScriptObject, SmartPtr<IObject>>(
        //                 "ICompositorManager" )
        //                 .def( "registerCompositors", &ICompositorManager::registerCompositors )
        //                 .def( "registerCompositor", &ICompositorManager::registerCompositor )
        //                 .def( "removeCompositor", &ICompositorManager::removeCompositor )
        //                 .def( "setCompositorEnabled", &ICompositorManager::setCompositorEnabled )
        //                 .def( "isCompositorEnabled", &ICompositorManager::isCompositorEnabled )
        //                 .def( "setCompositorProperties", &ICompositorManager::setCompositorProperties
        //                 ) .def( "getCompositorProperties",
        //                 &ICompositorManager::getCompositorProperties )];

        using ArrayU8 = Array<u8>;
        // typedef ArrayFunctions<ArrayU8> ArrayU8Functions;

        module( L )[class_<ArrayU8>( "ArrayU8" )
                        .def( constructor<>() )

                        .def( "reserve", &ArrayU8::reserve )
                    //.def("set_used", &ArrayU8::set_used )

                    //.def("push_back", &ArrayU8::push_back )

                    //.def("erase", (void (ArrayU8::*)(u32))&ArrayU8::erase_element_index )
                    //.def("erase_element", &ArrayU8::erase_element )

                    //.def("get", ArrayU8Functions::get )
        ];

        module( L )[class_<IGraphicsTerrain, ISharedObject, SmartPtr<IObject>>( "Terrain" )
                        .def( "load", &IGraphicsTerrain::load )
                        .def( "getHeightAtWorldPosition", &IGraphicsTerrain::getHeightAtWorldPosition )
                        .def( "getSize", &IGraphicsTerrain::getSize )
                        .def( "getTerrainSpacePosition", &IGraphicsTerrain::getTerrainSpacePosition )
                        .def( "isVisible", &IGraphicsTerrain::isVisible )
                        .def( "setVisible", &IGraphicsTerrain::setVisible )
                        .def( "getMaterialName", &IGraphicsTerrain::getMaterialName )
                        .def( "setMaterialName", &IGraphicsTerrain::setMaterialName )
                        .def( "getHeightData", &IGraphicsTerrain::getHeightData )
                        .scope[def( "typeInfo", IGraphicsTerrain::typeInfo )]];

        module( L )[class_<IGraphicsWater, ISharedObject, SmartPtr<IObject>>( "Water" )
                        .def( "getSceneManager", &IGraphicsWater::getSceneManager )
                        .def( "setSceneManager", &IGraphicsWater::setSceneManager )
                        .def( "getCamera", &IGraphicsWater::getCamera )
                        .def( "setCamera", &IGraphicsWater::setCamera )
                        .def( "getViewport", &IGraphicsWater::getViewport )
                        .def( "setViewport", &IGraphicsWater::setViewport )

                        .def( "getPosition", &IGraphicsWater::getPosition )
                        .def( "setPosition", &IGraphicsWater::setPosition )
                        .scope[def( "typeInfo", IGraphicsWater::typeInfo )]];

        module( L )[class_<ITextureManager, IResourceManager, SmartPtr<IObject>>( "ITextureManager" )
                        .def( "createManual", &ITextureManager::createManual )
                        .scope[def( "typeInfo", ITextureManager::typeInfo )]];

        module( L )[class_<ITexture, IResource, SmartPtr<IObject>>( "ITexture" )
                        .def( "getRenderTarget", &ITexture::getRenderTarget )
                        .scope[def( "typeInfo", ITexture::typeInfo )]];

        module( L )[class_<IBillboard, SmartPtr<IBillboard>>( "IBillboard" )];
        module( L )[class_<IBillboardSet, SmartPtr<IBillboardSet>>( "IBillboardSet" )];

        module( L )[class_<ISky, SmartPtr<ISky>>( "ISky" )];
        module( L )[class_<ISkySphere, SmartPtr<ISkySphere>>( "ISkySphere" )];
        module( L )[class_<ISkybox, SmartPtr<ISkybox>>( "ISkybox" )];
        module( L )[class_<ISkyboxCube, SmartPtr<ISkyboxCube>>( "ISkyboxCube" )];
        module( L )[class_<ISkyboxPlane, SmartPtr<ISkyboxPlane>>( "ISkyboxPlane" )];

        module( L )[class_<IDebug, SmartPtr<IDebug>>( "IDebug" )];
        module( L )[class_<IDebugCircle, SmartPtr<IDebugCircle>>( "IDebugCircle" )];
        module( L )[class_<IDebugLine, SmartPtr<IDebugLine>>( "IDebugLine" )];
        module( L )[class_<IDebugText, SmartPtr<IDebugText>>( "IDebugText" )];
        module( L )[class_<IDecalCursor, SmartPtr<IDecalCursor>>( "IDecalCursor" )];

        module( L )[class_<IGraphicsBone, SmartPtr<IGraphicsBone>>( "IGraphicsBone" )];
        module( L )[class_<IGraphicsCubemap, SmartPtr<IGraphicsCubemap>>( "IGraphicsCubemap" )];
        module( L )[class_<IGraphicsNode, SmartPtr<IGraphicsNode>>( "IGraphicsNode" )];
        module( L )[class_<GraphicsSettings, SmartPtr<IBuildDirector>>( "GraphicsSettings" )];
        module( L )[class_<IGraphicsSkeleton, SmartPtr<IGraphicsSkeleton>>( "IGraphicsSkeleton" )];
        module( L )[class_<render::IGraphicsWindowEvent, SmartPtr<render::IGraphicsWindowEvent>>(
            "IGraphicsWindowEvent" )];
        module( L )[class_<render::IGraphicsWindowListener, SmartPtr<render::IGraphicsWindowListener>>(
            "IGraphicsWindowListener" )];

        module( L )[class_<ILightmap, SmartPtr<ILightmap>>( "ILightmap" )];
        module( L )[class_<ILightmapper, SmartPtr<ILightmapper>>( "ILightmapper" )];

        module( L )[class_<IMaterialNode, SmartPtr<IMaterialNode>>( "IMaterialNode" )];
        module( L )[class_<IMaterialNodeAnimatedTexture, SmartPtr<IMaterialNodeAnimatedTexture>>(
            "IMaterialNodeAnimatedTexture" )];
        module( L )[class_<IMaterialPass, SmartPtr<IMaterialPass>>( "IMaterialPass" )];
        module( L )[class_<IMaterialShader, SmartPtr<IMaterialShader>>( "IMaterialShader" )];
        module( L )[class_<IMaterialTechnique, SmartPtr<IMaterialTechnique>>( "IMaterialTechnique" )];
        module( L )[class_<IMaterialTexture, SmartPtr<IMaterialTexture>>( "IMaterialTexture" )];
        module( L )[class_<IMeshConverter, SmartPtr<IMeshConverter>>( "IMeshConverter" )];

        module( L )[class_<IRenderTexture, SmartPtr<IRenderTexture>>( "IRenderTexture" )];
        module( L )[class_<IRenderer, SmartPtr<IRenderer>>( "IRenderer" )];
        module( L )[class_<IRenderer2, SmartPtr<IRenderer2>>( "IRenderer2" )];
        module( L )[class_<IRenderer3, SmartPtr<IRenderer3>>( "IRenderer3" )];

        module( L )[class_<ITerrainBlendMap, SmartPtr<ITerrainBlendMap>>( "ITerrainBlendMap" )];
        module( L )[class_<procedural::ITerrainGenerator, SmartPtr<procedural::ITerrainGenerator>>(
            "ITerrainGenerator" )];
        module( L )[class_<ITerrainRayResult, SmartPtr<ITerrainRayResult>>( "ITerrainRayResult" )];

        module( L )[class_<IVideoMaterial, SmartPtr<IVideoMaterial>>( "IVideoMaterial" )];
        module( L )[class_<IVideoStream, SmartPtr<IVideoStream>>( "IVideoStream" )];
        module( L )[class_<IVideoTexture, SmartPtr<IVideoTexture>>( "IVideoTexture" )];
        module( L )[class_<IVolumeRenderer, SmartPtr<IVolumeRenderer>>( "IVolumeRenderer" )];

        module( L )[class_<IOverlay, SmartPtr<IOverlay>>( "IOverlay" )];
        module( L )[class_<IOverlayElement, SmartPtr<IOverlayElement>>( "IOverlayElement" )];
        module( L )[class_<IOverlayElementContainer, SmartPtr<IOverlayElementContainer>>(
            "IOverlayElementContainer" )];
        module( L )[class_<IOverlayElementText, SmartPtr<IOverlayElementText>>( "IOverlayElementText" )];
        module( L )[class_<IOverlayElementVector, SmartPtr<IOverlayElementVector>>(
            "IOverlayElementVector" )];
        module( L )[class_<IOverlayManager, SmartPtr<IOverlayManager>>( "IOverlayManager" )];
        module( L )[class_<IParticle, SmartPtr<IParticle>>( "IParticle" )];
        module( L )[class_<IParticleAffector, SmartPtr<IParticleAffector>>( "IParticleAffector" )];
        module( L )[class_<IParticleManager, SmartPtr<IParticleManager>>( "IParticleManager" )];
        module( L )[class_<IParticleRenderer, SmartPtr<IParticleRenderer>>( "IParticleRenderer" )];

        module( L )[class_<IFont, SmartPtr<IFont>>( "IFont" )];
        module( L )[class_<IFontManager, SmartPtr<IFontManager>>( "IFontManager" )];

        module(
            L )[class_<render::IShader, ISharedObject, SmartPtr<render::IShader>>( "IShader" )
                    .def( "getSource", &render::IShader::getSource )
                    .def( "setSource", &render::IShader::setSource )
                    .def( "getEntryPoint", &render::IShader::getEntryPoint )
                    .def( "setEntryPoint", &render::IShader::setEntryPoint )
                    .def( "getProfile", &render::IShader::getProfile )
                    .def( "setProfile", &render::IShader::setProfile )
                    .def( "setDefine", &render::IShader::setDefine )
                    .def( "removeDefine", &render::IShader::removeDefine )
                    .def( "clearDefines", &render::IShader::clearDefines )
                    .def( "compile", &render::IShader::compile )
                    .def( "reload", &render::IShader::reload )
                    .def( "invalidate", &render::IShader::invalidate )
                    .def( "isCompiled", &render::IShader::isCompiled )
                    .def( "clearDiagnostics", &render::IShader::clearDiagnostics )
                    .def( "hasResource", &render::IShader::hasResource )
                    .def( "setSpecializationConstant", &render::IShader::setSpecializationConstant )];

        module( L )[class_<render::IComputeShader::WorkgroupSize>( "ComputeWorkgroupSize" )
                        .def_readwrite( "x", &render::IComputeShader::WorkgroupSize::x )
                        .def_readwrite( "y", &render::IComputeShader::WorkgroupSize::y )
                        .def_readwrite( "z", &render::IComputeShader::WorkgroupSize::z )];
        module( L )[class_<render::IComputeShader::DispatchSize>( "ComputeDispatchSize" )
                        .def_readwrite( "x", &render::IComputeShader::DispatchSize::x )
                        .def_readwrite( "y", &render::IComputeShader::DispatchSize::y )
                        .def_readwrite( "z", &render::IComputeShader::DispatchSize::z )];
        module( L )[class_<render::IComputeShader::ComputeBinding>( "ComputeBinding" )
                        .def_readwrite( "name", &render::IComputeShader::ComputeBinding::name )
                        .def_readwrite( "set", &render::IComputeShader::ComputeBinding::set )
                        .def_readwrite( "binding", &render::IComputeShader::ComputeBinding::binding )
                        .def_readwrite( "arraySize", &render::IComputeShader::ComputeBinding::arraySize )
                        .def_readwrite( "byteSize", &render::IComputeShader::ComputeBinding::byteSize )
                        .def_readwrite( "optional", &render::IComputeShader::ComputeBinding::optional )];
        module( L )[class_<render::IComputeShader, render::IShader, SmartPtr<render::IComputeShader>>(
                        "IComputeShader" )
                        .def( "getWorkgroupSize", &render::IComputeShader::getWorkgroupSize )
                        .def( "setWorkgroupSize", &render::IComputeShader::setWorkgroupSize )
                        .def( "getDispatchSize", &render::IComputeShader::getDispatchSize )
                        .def( "setDispatchSize", &render::IComputeShader::setDispatchSize )
                        .def( "validateDispatch", &validateComputeDispatch )
                        .def( "hasComputeBinding", &render::IComputeShader::hasComputeBinding )
                        .def( "setBindingOverride", &render::IComputeShader::setBindingOverride )
                        .def( "clearBindingOverrides", &render::IComputeShader::clearBindingOverrides )
                        .def( "getSharedMemorySize", &render::IComputeShader::getSharedMemorySize )
                        .def( "setSharedMemorySize", &render::IComputeShader::setSharedMemorySize )
                        .def( "createPipeline", &render::IComputeShader::createPipeline )
                        .def( "destroyPipeline", &render::IComputeShader::destroyPipeline )
                        .def( "hasPipeline", &render::IComputeShader::hasPipeline )
                        .def( "clearIndirectDispatchSource",
                              &render::IComputeShader::clearIndirectDispatchSource )
                        .def( "hasIndirectDispatchSource",
                              &render::IComputeShader::hasIndirectDispatchSource )];
        module(
            L )[class_<render::IShaderManager, SmartPtr<render::IShaderManager>>( "IShaderManager" )];

        module( L )[class_<render::IDynamicLines, SmartPtr<render::IDynamicLines>>( "IDynamicLines" )];
        module( L )[class_<render::IDynamicMesh, SmartPtr<render::IDynamicMesh>>( "IDynamicMesh" )];

        module( L )[class_<render::IInstanceManager, SmartPtr<render::IInstanceManager>>(
            "IInstanceManager" )];

        module( L )[class_<render::ISprite, SmartPtr<render::ISprite>>( "ISprite" )];

        // module( L )[class_<GraphicsTypes>( "GraphicsTypes" )];

        // LUA_CONST_START( GraphicsTypes )
        // LUA_CONST( GraphicsTypes, PF_R8G8B8A8 );

        // LUA_CONST( GraphicsTypes, TU_STATIC );
        // LUA_CONST( GraphicsTypes, TU_DYNAMIC );
        // LUA_CONST( GraphicsTypes, TU_WRITE_ONLY );
        // LUA_CONST( GraphicsTypes, TU_STATIC_WRITE_ONLY );
        // LUA_CONST( GraphicsTypes, TU_DYNAMIC_WRITE_ONLY );
        // LUA_CONST( GraphicsTypes, TU_DYNAMIC_WRITE_ONLY_DISCARDABLE );
        // LUA_CONST( GraphicsTypes, TU_AUTOMIPMAP );
        // LUA_CONST( GraphicsTypes, TU_RENDERTARGET );
        // LUA_CONST( GraphicsTypes, TU_DEFAULT );

        // LUA_CONST( GraphicsTypes, LT_POINT );
        // LUA_CONST_END;

        // LUA_CONST_START( ITexture )
        // LUA_CONST( ITexture, TU_STATIC );
        // LUA_CONST( ITexture, TU_DYNAMIC );
        // LUA_CONST( ITexture, TU_WRITE_ONLY );
        // LUA_CONST( ITexture, TU_STATIC_WRITE_ONLY );
        // LUA_CONST( ITexture, TU_DYNAMIC_WRITE_ONLY );
        // LUA_CONST( ITexture, TU_DYNAMIC_WRITE_ONLY_DISCARDABLE );
        // LUA_CONST( ITexture, TU_AUTOMIPMAP );
        // LUA_CONST( ITexture, TU_RENDERTARGET );
        // LUA_CONST( ITexture, TU_DEFAULT );

        // LUA_CONST( ITexture, TEX_TYPE_1D );
        // LUA_CONST( ITexture, TEX_TYPE_2D );
        // LUA_CONST( ITexture, TEX_TYPE_3D );
        // LUA_CONST( ITexture, TEX_TYPE_CUBE_MAP );
        // LUA_CONST( ITexture, TEX_TYPE_2D_ARRAY );

        // LUA_CONST( ITexture, MIP_UNLIMITED );
        // LUA_CONST( ITexture, MIP_DEFAULT );
        // LUA_CONST_END;
    }
} // namespace workphone
