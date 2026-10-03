//#include "WPPyPyBind/GraphicsSystemBind.hpp"
#include <Workphone/Workphone.hpp>

namespace fb
{
    using namespace fb::render;

    //---------------------------------------------------------------------------------------------------
    void _setMaterialName( IGraphicsObject *obj, const char *materialName )
    {
        obj->setMaterialName( materialName );
    }

    //---------------------------------------------------------------------------------------------------
    void _setMaterialNameStr( IGraphicsObject *obj, String materialName )
    {
        obj->setMaterialName( materialName );
    }

    //---------------------------------------------------------------------------------------------------
    void _setMaterialNameIndx( IGraphicsObject *obj, const char *materialName, lua_Integer idx )
    {
        obj->setMaterialName( materialName, idx );
    }

    ISceneManager *_getSceneManager( IGraphicsSystem *gfxSystem, const char *sceneManagerName )
    {
        SceneManagerPtr smgr = gfxSystem->getSceneManager( sceneManagerName );
        return smgr.getPtr();
    }

    //
    // SceneNode functions
    //
    void _setVisibilityFlags( ISceneNode *node, lua_Integer flag )
    {
        u32 mask = *reinterpret_cast<u32 *>( &flag );
        node->setVisibilityFlags( mask );
    }

    lua_Integer _getVisibilityFlags( ISceneNode *node )
    {
        u32 mask = node->getVisibilityFlags();
        return *reinterpret_cast<lua_Integer *>( &mask );
    }

    SceneNodePtr _addChildSceneNode( ISceneNode *node )
    {
        return node->addChildSceneNode();
    }

    SceneNodePtr _addChildSceneNodeNamed( ISceneNode *node, const char *name )
    {
        return node->addChildSceneNode( name );
    }

    void _attachObject( ISceneNode *node, GraphicsSmartPtr<IObject> const &obj )
    {
        node->attachObject( obj );
    }

    void _attachMesh( ISceneNode *node, GraphicsMeshPtr const &obj )
    {
        node->attachObject( obj );
    }

    void _attachCamera( ISceneNode *node, CameraPtr const &obj )
    {
        node->attachObject( obj );
    }

    void _setVisible( ISceneNode *node, bool isVisible )
    {
        node->setVisible( isVisible );
    }

    void _checkPointer( GraphicsSmartPtr<IObject> const &obj )
    {
        int halt = 0;
        halt = 0;
    }

    //
    // GraphicsContainer functions
    //
    void GraphicsObject_setVisibilityFlags( IGraphicsObject *object, lua_Integer flag )
    {
        u32 mask = *reinterpret_cast<u32 *>( &flag );
        object->setVisibilityFlags( mask );
    }

    lua_Integer GraphicsObject_getVisibilityFlags( IGraphicsObject *object )
    {
        u32 mask = object->getVisibilityFlags();
        return *reinterpret_cast<lua_Integer *>( &mask );
    }

    void _setObjectSceneNode( GraphicsContainer *component, const char *name, SceneNodePtr const &obj )
    {
        u32 hash = StringUtil::getHashMakeLower( name );
        component->setObject( hash, obj );
    }

    void _setObjectSceneNodeHash( GraphicsContainer *component, lua_Integer hash,
                                  SceneNodePtr const &obj )
    {
        component->setObject( hash, obj );
    }

    void _setObjectGfxObj( GraphicsContainer *component, const char *name, GraphicsSmartPtr<IObject> const &obj )
    {
        u32 hash = StringUtil::getHashMakeLower( name );
        component->setObject( hash, obj );
    }

    void _setObjectGfxObjHash( GraphicsContainer *component, lua_Integer hash,
                               GraphicsSmartPtr<IObject> const &obj )
    {
        component->setObject( hash, obj );
    }

    void _setObjectGfxMesh( GraphicsContainer *component, const char *name, GraphicsMeshPtr const &obj )
    {
        u32 hash = StringUtil::getHashMakeLower( name );
        component->setObject( hash, obj );
    }

    void _setObjectGfxMeshHash( GraphicsContainer *component, lua_Integer hash,
                                GraphicsMeshPtr const &obj )
    {
        component->setObject( hash, obj );
    }

    void _setObjectParticleSystem( GraphicsContainer *component, const char *name,
                                   ParticleSystemPtr const &obj )
    {
        u32 hash = StringUtil::getHashMakeLower( name );
        component->setObject( hash, obj );
    }

    void _setObjectParticleSystemHash( GraphicsContainer *component, lua_Integer hash,
                                       ParticleSystemPtr const &obj )
    {
        component->setObject( hash, obj );
    }

    void _setObjectAnimationCtrl( GraphicsContainer *component, const char *name,
                                  AnimationControllerPtr const &obj )
    {
        u32 hash = StringUtil::getHashMakeLower( name );
        component->setObject( hash, obj );
    }

    void _setObjectAnimationCtrlHash( GraphicsContainer *component, lua_Integer hash,
                                      AnimationControllerPtr const &obj )
    {
        component->setObject( hash, obj );
    }

    void _setObjectAnimationStateCtrl( GraphicsContainer *component, const char *name,
                                       AnimationStateControllerPtr const &obj )
    {
        u32 hash = StringUtil::getHashMakeLower( name );
        component->setObject( hash, obj );
    }

    void _setObjectAnimationStateCtrlHash( GraphicsContainer *component, lua_Integer hash,
                                           AnimationStateControllerPtr const &obj )
    {
        component->setObject( hash, obj );
    }

    AnimationControllerPtr _getAnimationCtrl( GraphicsContainer *component, const char *name )
    {
        u32 hash = StringUtil::getHashMakeLower( name );
        return component->getAnimationController( hash );
    }

    AnimationStateControllerPtr _getAnimationStateController( GraphicsContainer *component,
                                                              const char *name )
    {
        u32 hash = StringUtil::getHashMakeLower( name );
        return component->getAnimationStateController( hash );
    }

    GraphicsMeshPtr _getMesh( GraphicsContainer *component, const char *name )
    {
        u32 hash = StringUtil::getHashMakeLower( name );
        return component->getGraphicsObject( hash );
    }

    GraphicsMeshPtr _getMeshHash( GraphicsContainer *component, u32 hash )
    {
        return component->getGraphicsObject( hash );
    }

    SceneNodePtr _getSceneNode( GraphicsContainer *component, const char *name )
    {
        u32 hash = StringUtil::getHashMakeLower( name );
        return component->getSceneNode( hash );
    }

    // IViewport functions
    Vector2I _getActualSize( IViewport *vp )
    {
        return Vector2I( vp->getActualWidth(), vp->getActualHeight() );
    }

    void _setClearEveryFrame( IViewport *vp, bool clear )
    {
        vp->setClearEveryFrame( clear );
    }

    void _setClearEveryFrameFlags( IViewport *vp, bool clear, u32 buffers )
    {
        vp->setClearEveryFrame( clear, buffers );
    }

    bool _configureDefault( IGraphicsSystem *system )
    {
        return system->configure( GraphicsSystemConfigurationPtr::NULL_PTR );
    }

    bool _configure( IGraphicsSystem *system, GraphicsSystemConfigurationPtr config )
    {
        return system->configure( config );
    }

    WindowPtr _getRenderWindow( IGraphicsSystem *sys )
    {
        return sys->getRenderWindow();
    }

    WindowPtr _getRenderWindowNamed( IGraphicsSystem *sys, const char *name )
    {
        return sys->getRenderWindow( name );
    }

    ViewportPtr _addViewportMinArgs( IRenderTarget *rt, u32 id, CameraPtr camera )
    {
        return rt->addViewport( id, camera );
    }

    ViewportPtr _addViewportZorderArgs( IRenderTarget *rt, u32 id, CameraPtr camera, s32 zorder )
    {
        return rt->addViewport( id, camera, zorder );
    }

    ViewportPtr _addViewportAllArgs( IRenderTarget *rt, u32 id, CameraPtr camera, s32 ZOrder, f32 left,
                                     f32 top, f32 width, f32 height )
    {
        return rt->addViewport( id, camera, ZOrder, left, top, width, height );
    }

    //
    // IAnimationController functions
    //
    void _setAnimationEnabled( IAnimationController *ctrl, const char *animationName, bool enabled )
    {
        ctrl->setAnimationEnabled( animationName, enabled );
    }

    void _setAnimationEnabledPosition( IAnimationController *ctrl, const char *animationName,
                                       bool enabled, f32 timePosition )
    {
        ctrl->setAnimationEnabled( animationName, enabled, timePosition );
    }

    void _setAnimationStateEnabled( IAnimationStateController *ctrl, const char *animationName,
                                    bool enabled )
    {
        ctrl->setAnimationEnabled( animationName, enabled );
    }

    void _setAnimationStateEnabledPosition( IAnimationStateController *ctrl, const char *animationName,
                                            bool enabled, f32 timePosition )
    {
        ctrl->setAnimationEnabled( animationName, enabled, timePosition );
    }

    //
    //
    //
    void _loadSceneFile( ISceneManager *smgr, const char *filePath, SceneNodePtr parent )
    {
        smgr->loadSceneFile( filePath, parent );
    }

    SceneManagerPtr _addSceneManager( IGraphicsSystem *graphicsSystem, const char *type,
                                      const char *name )
    {
        return graphicsSystem->addSceneManager( type, name );
    }

    //---------------------------------------------------------------------------------------------------
    void bindGraphicsSystem( lua_State *L )
    {
        using namespace luabind;

        module( L )[class_<IGraphicsSystem, IScriptObject, GraphicsSystemPtr>( "GraphicsSystem" )
                        .def( "configure", _configureDefault )
                        .def( "configure", _configure )

                        .def( "addSceneManager", _addSceneManager )
                        .def( "getSceneManager", _getSceneManager )
                        .def( "getSceneManagerById", &IGraphicsSystem::getSceneManagerById )
                        .def( "getOverlayManager", &IGraphicsSystem::getOverlayManager )
                        .def( "getCompositorManager", &IGraphicsSystem::getCompositorManager )

                        .def( "getResourceGroupManager", &IGraphicsSystem::getResourceGroupManager )
                        .def( "getMaterialManager", &IGraphicsSystem::getMaterialManager )
                        .def( "getTextureManager", &IGraphicsSystem::getTextureManager )

                        .def( "getRenderWindow", _getRenderWindow )
                        .def( "getRenderWindow", _getRenderWindowNamed )

                        .def( "loadResources", &IGraphicsSystem::loadResources )
                        .def( "reloadResources", &IGraphicsSystem::reloadResources )];

        module( L )[class_<IViewport, IScriptObject, ViewportPtr>( "Viewport" )
                        .def( "getActualSize", _getActualSize )
                        .def( "setBackgroundColour", &IViewport::setBackgroundColour )
                        .def( "getBackgroundColour", &IViewport::getBackgroundColour )
                        .def( "setClearEveryFrame", _setClearEveryFrame )
                        .def( "setClearEveryFrame", _setClearEveryFrameFlags )
                        .def( "getClearEveryFrame", &IViewport::getClearEveryFrame )
                        .def( "getActualWidth", &IViewport::getActualWidth )
                        .def( "getActualHeight", &IViewport::getActualHeight )
                        .def( "getOverlaysEnabled", &IViewport::getOverlaysEnabled )
                        .def( "setOverlaysEnabled", &IViewport::setOverlaysEnabled )
                        .def( "getSkiesEnabled", &IViewport::getSkiesEnabled )
                        .def( "setSkiesEnabled", &IViewport::setSkiesEnabled )
                        .def( "getShadowsEnabled", &IViewport::getShadowsEnabled )
                        .def( "setShadowsEnabled", &IViewport::setShadowsEnabled )
                        .def( "getVisibilityMask", &IViewport::getVisibilityMask )
                        .def( "setVisibilityMask", &IViewport::setVisibilityMask )];

        module( L )[class_<IRenderTarget, IScriptObject, RenderTargetPtr>( "RenderTarget" )
                        .def( "getViewportById", &IRenderTarget::getViewportById )
                        .def( "addViewport", _addViewportMinArgs )
                        .def( "addViewport", _addViewportAllArgs )
                        .def( "addViewport", _addViewportZorderArgs )
                        .def( "getNumViewports", &IRenderTarget::getNumViewports )
                        .def( "getViewport", &IRenderTarget::getViewport )
                        .def( "getViewportById", &IRenderTarget::getViewportById )
                        .def( "removeViewport", &IRenderTarget::removeViewport )
                        .def( "removeAllViewports", &IRenderTarget::removeAllViewports )
                        .def( "getWidth", &IRenderTarget::getWidth )
                        .def( "getHeight", &IRenderTarget::getHeight )
                        .def( "getColourDepth", &IRenderTarget::getColourDepth )];

        module( L )[class_<IWindow, IRenderTarget, WindowPtr>( "Window" )
                        .def( "setFullscreen", &IWindow::setFullscreen )
                        .def( "destroy", &IWindow::destroy )
                        .def( "resize", &IWindow::resize )
                        .def( "windowMovedOrResized", &IWindow::windowMovedOrResized )
                        .def( "reposition", &IWindow::reposition )
                        .def( "isVisible", &IWindow::isVisible )
                        .def( "setVisible", &IWindow::setVisible )
                        .def( "isActive", &IWindow::isActive )
                        .def( "isClosed", &IWindow::isClosed )
                        .def( "isPrimary", &IWindow::isPrimary )
                        .def( "isFullScreen", &IWindow::isFullScreen )
                        .def( "suggestPixelFormat", &IWindow::suggestPixelFormat )
                        .def( "isDeactivatedOnFocusChange", &IWindow::isDeactivatedOnFocusChange )
                        .def( "setDeactivateOnFocusChange", &IWindow::setDeactivateOnFocusChange )];

        module( L )[class_<ISceneManager, IScriptObject, SceneManagerPtr>( "SceneManager" )
                        .def( "update", &ISceneManager::_update )
                        .def( "addGraphicsObject", SceneManagerHelper::_addGraphicsObject )
                        .def( "addGraphicsObject", SceneManagerHelper::_addGraphicsObjectNamed )

                        .def( "addSceneNode", SceneManagerHelper::_addSceneNode )
                        .def( "getSceneNode", &ISceneManager::getSceneNode )
                        .def( "getSceneNodeById", &ISceneManager::getSceneNodeById )
                        .def( "getRootSceneNode", SceneManagerHelper::_getRootSceneNode )

                        .def( "addMesh", SceneManagerHelper::_addMesh )
                        .def( "addMesh", SceneManagerHelper::_addMeshNamed )
                        .def( "addParticleSystem", SceneManagerHelper::_addParticleSystem )
                        .def( "getParticleSystem", SceneManagerHelper::_getParticleSystem )
                        .def( "addCamera", &ISceneManager::addCamera )
                        .def( "getCamera", &ISceneManager::getCamera )
                        .def( "hasCamera", &ISceneManager::hasCamera )

                        .def( "clearScene", &ISceneManager::clearScene )

                        .def( "hasAnimation", &ISceneManager::hasAnimation )
                        .def( "destroyAnimation", &ISceneManager::destroyAnimation )

                        .def( "createAnimationStateController",
                              &ISceneManager::createAnimationStateController )

                        .def( "loadSceneFile", _loadSceneFile )
                        .def( "loadScene", &ISceneManager::loadScene )
                        .def( "setSkyBox", &ISceneManager::setSkyBox )
                        .def( "createTerrain", &ISceneManager::createTerrain )

                        .def( "getEnableShadows", &ISceneManager::getEnableShadows )
                        .def( "setEnableShadows", &ISceneManager::setEnableShadows )];

        module( L )[class_<ISceneNode, IScriptObject, SceneNodePtr>( "SceneNode" )

                        .def( "setMaterialName", &ISceneNode::setMaterialName )

                        .def( "add", &ISceneNode::add )
                        .def( "remove", &ISceneNode::remove )

                        .def( "addChild", &ISceneNode::addChild )
                        .def( "addChildSceneNode", _addChildSceneNode )
                        .def( "addChildSceneNode", _addChildSceneNodeNamed )
                        .def( "attachObject", _attachObject )
                        .def( "attachObject", _attachMesh )
                        .def( "attachObject", _attachCamera )

                        //.def("detachObject", &ISceneNode::detachObject )
                        .def( "detachAllObjects", &ISceneNode::detachAllObjects )

                        .def( "setPosition", &ISceneNode::setPosition )
                        .def( "getPosition", &ISceneNode::getPosition )

                        .def( "setScale", &ISceneNode::setScale )
                        .def( "getScale", &ISceneNode::getScale )

                        .def( "setRotationByDegrees", &ISceneNode::setRotationByDegrees )
                        .def( "setOrientation", &ISceneNode::setOrientation )
                        .def( "getOrientation", &ISceneNode::getOrientation )

                        .def( "setVisible", _setVisible )

                        .def( "setVisibilityFlags", _setVisibilityFlags )
                        .def( "getVisibilityFlags", _getVisibilityFlags )

                        .def( "getName", &ISceneNode::getName )];

        //graphics objects
        module( L )[class_<IGraphicsObject, IScriptObject, GraphicsSmartPtr<IObject>>( "GraphicsObject" )
                        .def( "setTestOcclusion", &IGraphicsObject::setTestOcclusion )
                        .def( "getTestOcclusion", &IGraphicsObject::getTestOcclusion )
                        .def( "setIsOccluder", &IGraphicsObject::setIsOccluder )
                        .def( "isOccluder", &IGraphicsObject::isOccluder )

                        .def( "getName", &IGraphicsObject::getName )
                        .def( "getId", &IGraphicsObject::getId )

                        .def( "setMaterialName", _setMaterialName )
                        //.def("setMaterialName", _setMaterialNameStr )
                        .def( "setMaterialName", _setMaterialNameIndx )
                        .def( "getMaterialName", &IGraphicsObject::getMaterialName )

                        .def( "getCastShadows", &IGraphicsObject::getCastShadows )
                        .def( "setCastShadows", &IGraphicsObject::setCastShadows )
                        .def( "getRecieveShadows", &IGraphicsObject::getRecieveShadows )
                        .def( "setRecieveShadows", &IGraphicsObject::setRecieveShadows )
                        .def( "setVisible", &IGraphicsObject::setVisible )
                        .def( "isVisible", &IGraphicsObject::isVisible )
                        .def( "setRenderQueueGroup", &IGraphicsObject::setRenderQueueGroup )

                        .def( "setVisibilityFlags", GraphicsObject_setVisibilityFlags )
                        .def( "getVisibilityFlags", GraphicsObject_getVisibilityFlags )

        ];

        module( L )[class_<IFrustum, IGraphicsObject, GraphicsSmartPtr<IObject>>( "Frustum" )
                        .def( "getNearClipDistance", &IFrustum::getNearClipDistance )
                        .def( "setNearClipDistance", &IFrustum::setNearClipDistance )
                        .def( "getFarClipDistance", &IFrustum::getFarClipDistance )
                        .def( "setFarClipDistance", &IFrustum::setFarClipDistance )
                        .def( "getAspectRatio", &IFrustum::getAspectRatio )
                        .def( "setAspectRatio", &IFrustum::setAspectRatio )];

        module( L )[class_<ICamera, IFrustum, GraphicsSmartPtr<IObject>>( "Camera" )
                        .def( "setPosition", &ICamera::setPosition )
                        .def( "lookAt", &ICamera::lookAt )
                        .def( "setNearClipDistance", &ICamera::setNearClipDistance )
                        .def( "setFarClipDistance", &ICamera::setFarClipDistance )];

        //
        // Animation
        //
        module( L )[class_<IAnimationController, IScriptObject, AnimationControllerPtr>(
                        "AnimationController" )
                        .def( "setAnimationEnabled", _setAnimationEnabled )
                        .def( "setAnimationEnabled", _setAnimationEnabledPosition )
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
                        .def( "getAnimationLength", &IAnimationController::getAnimationLength )];

        module( L )[class_<IAnimationStateController, IScriptObject, AnimationStateControllerPtr>(
                        "AnimationStateController" )
                        .def( "setAnimationEnabled", _setAnimationStateEnabled )
                        .def( "setAnimationEnabled", _setAnimationStateEnabledPosition )
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
        ];

        module( L )[class_<IGraphicsMesh, IGraphicsObject, GraphicsSmartPtr<IObject>>( "GraphicsMesh" )
                        .def( "getAnimationController", &IGraphicsMesh::getAnimationController )];

        module( L )[class_<IParticleSystem, IGraphicsObject, GraphicsSmartPtr<IObject>>( "ParticleSystem" )
                        .def( "reload", &IParticleSystem::reload )
                        .def( "prepare", &IParticleSystem::prepare )
                        .def( "start", &IParticleSystem::start )
                        .def( "stop", &IParticleSystem::stop )
                        .def( "stopFade", &IParticleSystem::stopFade )

                        .def( "pause", ( void( IParticleSystem::* )( void ) ) & IParticleSystem::pause )
                        .def( "pause", ( void( IParticleSystem::* )( float ) ) & IParticleSystem::pause )
                        .def( "resume", &IParticleSystem::resume )

                        .def( "setFastForward", &IParticleSystem::setFastForward )
                        .def( "getFastForwardTime", &IParticleSystem::getFastForwardTime )
                        .def( "getFastForwardInterval", &IParticleSystem::getFastForwardInterval )

                        .def( "setTemplateName", &IParticleSystem::setTemplateName )
                        .def( "getTemplateName", &IParticleSystem::getTemplateName )

                        .def( "setScale", &IParticleSystem::setScale )
                        .def( "getScale", &IParticleSystem::getScale )

                        .def( "getState", &IParticleSystem::getState )

                        .def( "getTechnique", &IParticleSystem::getTechnique )];

        module(
            L )[class_<IParticleTechnique, IScriptObject, ParticleTechniquePtr>( "ParticleTechnique" )];

        module(
            L )[class_<GraphicsContainer, IComponent, ComponentPtr>( "GraphicsContainer" )
                    .def( "setObject", _setObjectSceneNode )
                    .def( "setObject", _setObjectSceneNodeHash )
                    .def( "setObject", _setObjectGfxObj )
                    .def( "setObject", _setObjectGfxObjHash )
                    .def( "setObject", _setObjectGfxMesh )
                    .def( "setObject", _setObjectGfxMeshHash )
                    .def( "setObject", _setObjectParticleSystem )
                    .def( "setObject", _setObjectParticleSystemHash )
                    .def( "setObject", _setObjectAnimationCtrl )
                    .def( "setObject", _setObjectAnimationCtrlHash )
                    .def( "setObject", _setObjectAnimationStateCtrl )
                    .def( "setObject", _setObjectAnimationStateCtrlHash )
                    .def( "getSceneNode", _getSceneNode )
                    .def( "getSceneNode", &GraphicsContainer::getSceneNode )
                    .def( "getGraphicsObject", &GraphicsContainer::getGraphicsObject )
                    .def( "getAnimationController", &GraphicsContainer::getAnimationController )
                    .def( "getAnimationStateControl", _getAnimationStateController )
                    .def( "getAnimationStateControl", &GraphicsContainer::getAnimationStateController )
                    .def( "getAnimationController", _getAnimationCtrl )
                    .def( "getMesh", _getMesh )
                    .def( "getMesh", _getMeshHash )];

        module( L )[def( "checkPointer", _checkPointer )];

        //
        // Material system
        //
        module( L )[class_<IMaterialManager, IScriptObject, MaterialManagerPtr>( "MaterialManager" )
                        .def( "getMaterial", &IMaterialManager::getMaterial )
                        .def( "cloneMaterial", &IMaterialManager::cloneMaterial )];

        module( L )[class_<IMaterial, IScriptObject, MaterialPtr>( "Material" )
                        .def( "reload", &IMaterial::reload )];

        module( L )[class_<IPass, IScriptObject, PassPtr>( "Pass" )
                        .def( "getName", &IPass::getName )
                        .def( "setName", &IPass::setName )
                        .def( "setSceneBlending", &IPass::setSceneBlending )
                        .def( "isDepthCheckEnabled", &IPass::isDepthCheckEnabled )
                        .def( "setDepthCheckEnabled", &IPass::setDepthCheckEnabled )

                        .def( "isDepthWriteEnabled", &IPass::isDepthWriteEnabled )
                        .def( "setDepthWriteEnabled", &IPass::setDepthWriteEnabled )];

        module( L )[class_<ITextureUnit, IScriptObject, PassPtr>( "TextureUnit" )
                        .def( "getTextureName", &ITextureUnit::getTextureName )
                        .def( "setTextureName", &ITextureUnit::setTextureName )];

        typedef Array<u8> ArrayU8;
        typedef ArrayFunctions<ArrayU8> ArrayU8Functions;

        module( L )[class_<ArrayU8>( "ArrayU8" )
                        .def( constructor<>() )

                        .def( "reallocate", &ArrayU8::reallocate )
                        .def( "set_used", &ArrayU8::set_used )

                        .def( "push_back", &ArrayU8::push_back )
                        .def( "push_front", &ArrayU8::push_front )

                        .def( "erase", ( void( ArrayU8::* )( u32 ) ) & ArrayU8::erase )
                        .def( "erase_element", &ArrayU8::erase_element )

                        .def( "get", ArrayU8Functions::get )];

        module( L )[class_<ITerrain, IScriptObject, TerrainPtr>( "Terrain" )
                        .def( "load", &ITerrain::load )
                        .def( "getHeightAtWorldPosition", &ITerrain::getHeightAtWorldPosition )
                        .def( "getSize", &ITerrain::getSize )
                        .def( "getTerrainSpacePosition", &ITerrain::getTerrainSpacePosition )
                        .def( "isVisible", &ITerrain::isVisible )
                        .def( "setVisible", &ITerrain::setVisible )
                        .def( "getMaterialName", &ITerrain::getMaterialName )
                        .def( "setMaterialName", &ITerrain::setMaterialName )
                        .def( "getHeightData", &ITerrain::getHeightData )];

        module( L )[class_<IWater, IScriptObject, WaterPtr>( "Water" )
                        .def( "getSceneManager", &IWater::getSceneManager )
                        .def( "setSceneManager", &IWater::setSceneManager )
                        .def( "getCamera", &IWater::getCamera )
                        .def( "setCamera", &IWater::setCamera )
                        .def( "getViewport", &IWater::getViewport )
                        .def( "setViewport", &IWater::setViewport )

                        .def( "getPosition", &IWater::getPosition )
                        .def( "setPosition", &IWater::setPosition )];

        module( L )[class_<WaterStandard, IWater, WaterPtr>( "WaterStandard" )];

        module( L )[class_<HydraxWater, IWater, WaterPtr>( "HydraxWater" )];

        module( L )[class_<IGraphicsScene, IScriptObject, GraphicsScenePtr>( "GraphicsScene" )];
    }

}  // end namespace fb
