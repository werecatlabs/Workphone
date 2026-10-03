#include <WPPythonBind/WPPythonBindPCH.hpp>
#include <WPPythonBind/Bindings/BindComponents.hpp>
#include <Workphone/Workphone.hpp>
#include <WPGraphicsOgre/WPGraphicsOgre.hpp>
#include <WPPythonBind/Helpers/GraphicsSystemHelper.hpp>
#include <WPPythonBind/Helpers/SceneManagerHelper.hpp>
#include <WPPythonBind/Helpers/SceneNodeHelper.hpp>
#include <WPPythonBind/Helpers/PythonHelper.hpp>

namespace fb
{

    void bindGraphics()
    {
        using namespace boost::python;
        using namespace fb::render;

        class_<render::IGraphicsSystem, SmartPtr<render::IGraphicsSystem>, bases<ISharedObject>,
               boost::noncopyable>( "IGraphicsSystem", no_init )
            //.def( "configure", _configureDefault )
            //.def( "configure", _configure )

            .def( "addSceneManager", GraphicsSystemHelper::addSceneManager )
            .def( "getSceneManager", GraphicsSystemHelper::getSceneManager )
            .def( "getSceneManagerById", &IGraphicsSystem::getSceneManagerById )
            .def( "getOverlayManager", &IGraphicsSystem::getOverlayManager )
            .def( "getDebug", GraphicsSystemHelper::getDebug )
            //.def( "getCompositorManager", &IGraphicsSystem::getCompositorManager )

            .def( "getResourceGroupManager", &IGraphicsSystem::getResourceGroupManager )
            .def( "getMaterialManager", &IGraphicsSystem::getMaterialManager )
            .def( "getTextureManager", &IGraphicsSystem::getTextureManager )

            .def( "getRenderWindow", GraphicsSystemHelper::getRenderWindow )
            .def( "getRenderWindow", GraphicsSystemHelper::getRenderWindowNamed );

        //.def( "loadResources", &IGraphicsSystem::loadResources )
        //.def( "reloadResources", &IGraphicsSystem::reloadResources );

        class_<IGraphicsSceneManager, SmartPtr<render::IGraphicsSceneManager>, bases<ISharedObject>, boost::noncopyable>(
            "SceneManager", no_init )
            .def( "addGraphicsObject", SceneManagerHelper::_addGraphicsObject )
            .def( "addGraphicsObject", SceneManagerHelper::_addGraphicsObjectNamed )

            .def( "addSceneNode", SceneManagerHelper::_addSceneNode )
            .def( "getSceneNode", &IGraphicsSceneManager::getSceneNode )
            .def( "getSceneNodeById", &IGraphicsSceneManager::getSceneNodeById )
            .def( "getRootSceneNode", SceneManagerHelper::_getRootSceneNode )

            .def( "addMesh", SceneManagerHelper::_addMesh )
            .def( "addMesh", SceneManagerHelper::_addMeshNamed )
            .def( "addParticleSystem", SceneManagerHelper::_addParticleSystem )
            .def( "getParticleSystem", SceneManagerHelper::_getParticleSystem )
            .def( "addCamera", &IGraphicsSceneManager::addCamera )
            .def( "hasCamera", &IGraphicsSceneManager::hasCamera )

            .def( "clearScene", &IGraphicsSceneManager::clearScene )

            .def( "hasAnimation", &IGraphicsSceneManager::hasAnimation )
            .def( "destroyAnimation", &IGraphicsSceneManager::destroyAnimation )

            .def( "createAnimationStateController", &IGraphicsSceneManager::createAnimationStateController )

            .def( "setSkyBox", &IGraphicsSceneManager::setSkyBox )
            .def( "createTerrain", &IGraphicsSceneManager::createTerrain )

            .def( "getEnableShadows", &IGraphicsSceneManager::getEnableShadows )
            .def( "setEnableShadows", &IGraphicsSceneManager::setEnableShadows );

        class_<ISceneNode, SmartPtr<render::ISceneNode>, bases<ISharedObject>, boost::noncopyable>(
            "SceneNode", no_init )
            .def( "add", &ISceneNode::add )
            .def( "remove", &ISceneNode::remove )

            .def( "addChild", &ISceneNode::addChild )
            .def( "addChildSceneNode", SceneNodeHelper::_addChildSceneNode )
            .def( "addChildSceneNode", SceneNodeHelper::_addChildSceneNodeNamed )
            .def( "attachObject", SceneNodeHelper::_attachObject )
            .def( "attachObject", SceneNodeHelper::_attachMesh )
            .def( "attachObject", SceneNodeHelper::_attachCamera )

            //.def("detachObject", &ISceneNode::detachObject )
            .def( "detachAllObjects", &ISceneNode::detachAllObjects )

            .def( "setPosition", &ISceneNode::setPosition )
            .def( "getPosition", &ISceneNode::getPosition )

            .def( "setScale", &ISceneNode::setScale )
            .def( "getScale", &ISceneNode::getScale )

            .def( "setRotationByDegrees", &ISceneNode::setRotationByDegrees )
            .def( "setOrientation", &ISceneNode::setOrientation )
            .def( "getOrientation", &ISceneNode::getOrientation )

            .def( "setVisible", SceneNodeHelper::_setVisible )

            .def( "setVisibilityFlags", SceneNodeHelper::_setVisibilityFlags )
            .def( "getVisibilityFlags", SceneNodeHelper::_getVisibilityFlags )

            //.def("getName",  &ISceneNode::getName )
            ;

        class_<render::IDebug, SmartPtr<render::IDebug>, bases<ISharedObject>, boost::noncopyable>(
            "IDebug", no_init )
            .def( "drawLine", &IDebug::drawLine );

        class_<IDeferredShadingSystem, SmartPtr<render::IDeferredShadingSystem>, bases<ISharedObject>,
               boost::noncopyable>( "IDeferredShadingSystem", no_init )
            .def( "getSSAO", &IDeferredShadingSystem::getSSAO )
            .def( "setSSAO", &IDeferredShadingSystem::setSSAO )
            .def( "getActive", &IDeferredShadingSystem::getActive )
            .def( "setActive", &IDeferredShadingSystem::setActive )
            .def( "getShadowsEnabled", &IDeferredShadingSystem::getShadowsEnabled )
            .def( "setShadowsEnabled", &IDeferredShadingSystem::setShadowsEnabled );

        converter::smart_ptr_to_python<IGraphicsSystem>();
        converter::smart_ptr_to_python<IGraphicsSceneManager>();

        converter::smart_ptr_from_python<IGraphicsSystem>();
        converter::smart_ptr_from_python<IGraphicsSceneManager>();

        PythonHelper::registerPointer<ISceneNode>();
        PythonHelper::registerPointer<IDeferredShadingSystem>();
    }

}  // namespace fb
