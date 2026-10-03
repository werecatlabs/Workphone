#ifndef SceneNodeHelper_h__
#define SceneNodeHelper_h__

#include <WPPythonBind/WPPythonBindPrerequisites.hpp>

namespace fb
{

    class SceneNodeHelper
    {
    public:
        static void _setVisibilityFlags( SmartPtr<render::ISceneNode> node, python_Integer flag );

        static python_Integer _getVisibilityFlags( SmartPtr<render::ISceneNode> node );

        static SmartPtr<render::ISceneNode> _addChildSceneNode( SmartPtr<render::ISceneNode> node );

        static SmartPtr<render::ISceneNode> _addChildSceneNodeNamed( SmartPtr<render::ISceneNode> node,
                                                                     const char *name );

        static void _attachObject( SmartPtr<render::ISceneNode> node,
                                   SmartPtr<ISharedObject> const &obj );

        static void _attachMesh( SmartPtr<render::ISceneNode> node,
                                 SmartPtr<render::IGraphicsMesh> const &obj );

        static void _attachCamera( SmartPtr<render::ISceneNode> node,
                                   SmartPtr<render::ICamera> const &obj );

        static void _setVisible( SmartPtr<render::ISceneNode> node, bool isVisible );
    };

}  // end namespace fb

#endif  // SceneNodeHelper_h__
