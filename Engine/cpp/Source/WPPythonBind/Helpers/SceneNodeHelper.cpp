#include <WPPythonBind/WPPythonBindPCH.hpp>
#include <WPPythonBind/Helpers/SceneNodeHelper.hpp>
#include <Workphone/Workphone.hpp>

namespace fb
{
    void SceneNodeHelper::_setVisibilityFlags( SmartPtr<render::ISceneNode> node, python_Integer flag )
    {
        u32 mask = *reinterpret_cast<u32 *>(&flag);
        //node->setVisibilityFlags(mask);
    }

    python_Integer SceneNodeHelper::_getVisibilityFlags( SmartPtr<render::ISceneNode> node )
    {
        u32 mask = 0;
        return *reinterpret_cast<python_Integer *>(&mask);
    }

    SmartPtr<render::ISceneNode> SceneNodeHelper::_addChildSceneNode(
        SmartPtr<render::ISceneNode> node )
    {
        return node->addChildSceneNode();
    }

    SmartPtr<render::ISceneNode> SceneNodeHelper::_addChildSceneNodeNamed(
        SmartPtr<render::ISceneNode> node, const char *name )
    {
        return node->addChildSceneNode( name );
    }

    void SceneNodeHelper::_attachObject( SmartPtr<render::ISceneNode> node,
                                         const SmartPtr<ISharedObject> &obj )
    {
        node->attachObject( obj );
    }

    void SceneNodeHelper::_attachMesh( SmartPtr<render::ISceneNode> node,
                                       const SmartPtr<render::IGraphicsMesh> &obj )
    {
        node->attachObject( obj );
    }

    void SceneNodeHelper::_attachCamera( SmartPtr<render::ISceneNode> node,
                                         const SmartPtr<render::ICamera> &obj )
    {
        node->attachObject( obj );
    }

    void SceneNodeHelper::_setVisible( SmartPtr<render::ISceneNode> node, bool isVisible )
    {
        node->setVisible( isVisible );
    }
} // namespace fb
