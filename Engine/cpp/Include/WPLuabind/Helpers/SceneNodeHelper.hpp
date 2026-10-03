#ifndef SceneNodeHelper_h__
#define SceneNodeHelper_h__

#include "WPLuabind/WPLuabindPrerequisites.hpp"
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>

namespace workphone
{
    class SceneNodeHelper
    {
    public:
        static void setMaterialName( render::IGraphicsSceneNode *node, const String &materialName );
        static void setMaterialNameCascade( render::IGraphicsSceneNode *node, const String &materialName,
                                            bool cascade );

        static void _setVisibilityFlags( render::IGraphicsSceneNode *node, lua_Integer flag );

        static lua_Integer _getVisibilityFlags( render::IGraphicsSceneNode *node );

        static SmartPtr<render::IGraphicsSceneNode> _addChildSceneNode(
            render::IGraphicsSceneNode *node );
        static SmartPtr<render::IGraphicsSceneNode> _addChildSceneNodeNamed(
            render::IGraphicsSceneNode *node, const char *name );

        static void _attachObject( render::IGraphicsSceneNode              *node,
                                   const SmartPtr<render::IGraphicsObject> &obj );

        static void _attachMesh( render::IGraphicsSceneNode            *node,
                                 const SmartPtr<render::IGraphicsMesh> &obj );

        static void _attachCamera( render::IGraphicsSceneNode              *node,
                                   const SmartPtr<render::IGraphicsCamera> &obj );

        static void _setVisible( render::IGraphicsSceneNode *node, bool isVisible );
    };
} // namespace workphone

#endif // SceneNodeHelper_h__
