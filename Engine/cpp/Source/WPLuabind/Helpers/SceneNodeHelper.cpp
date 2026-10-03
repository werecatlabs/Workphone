#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuaBind/Helpers/SceneNodeHelper.hpp"
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IGraphicsMesh.hpp>
#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>
#include <luabind/luabind.hpp>

namespace workphone
{
    using namespace render;

    void SceneNodeHelper::setMaterialName( IGraphicsSceneNode *node, const String &materialName )
    {
        // node->setMaterialName( materialName );
    }

    void SceneNodeHelper::setMaterialNameCascade( IGraphicsSceneNode *node, const String &materialName,
                                                  bool cascade )
    {
        // node->setMaterialName( materialName, cascade );
    }

    void SceneNodeHelper::_setVisibilityFlags( IGraphicsSceneNode *node, lua_Integer flag )
    {
        u32 mask = *reinterpret_cast<u32 *>( &flag );
        // node->setVisibilityFlags( mask );
    }

    lua_Integer SceneNodeHelper::_getVisibilityFlags( IGraphicsSceneNode *node )
    {
        u32 mask = 0;
        return *reinterpret_cast<lua_Integer *>( &mask );
    }

    SmartPtr<IGraphicsSceneNode> SceneNodeHelper::_addChildSceneNode( IGraphicsSceneNode *node )
    {
        return node->addChildSceneNode();
    }

    SmartPtr<IGraphicsSceneNode> SceneNodeHelper::_addChildSceneNodeNamed( IGraphicsSceneNode *node,
                                                                           const char         *name )
    {
        return node->addChildSceneNode( name );
    }

    void SceneNodeHelper::_attachObject( IGraphicsSceneNode *node, const SmartPtr<IGraphicsObject> &obj )
    {
        node->attachObject( obj );
    }

    void SceneNodeHelper::_attachMesh( IGraphicsSceneNode *node, const SmartPtr<IGraphicsMesh> &obj )
    {
        node->attachObject( obj );
    }

    void SceneNodeHelper::_attachCamera( IGraphicsSceneNode *node, const SmartPtr<IGraphicsCamera> &obj )
    {
        node->attachObject( obj );
    }

    void SceneNodeHelper::_setVisible( IGraphicsSceneNode *node, bool isVisible )
    {
        // node->setVisible( isVisible );
    }
} // namespace workphone
