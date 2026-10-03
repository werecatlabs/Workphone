#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IGraphicsSceneNode, ISharedObject );

    const hash_type IGraphicsSceneNode::STATE_QUERY_TYPE_LOCAL_AABB = StringUtil::getHash( "LocalAABB" );
    const hash_type IGraphicsSceneNode::STATE_QUERY_TYPE_WORLD_AABB = StringUtil::getHash( "WorldAABB" );

    const hash_type IGraphicsSceneNode::STATE_MESSAGE_POSITION = StringUtil::getHash( "position" );
    const hash_type IGraphicsSceneNode::STATE_MESSAGE_SCALE = StringUtil::getHash( "scale" );
    const hash_type IGraphicsSceneNode::STATE_MESSAGE_ORIENTATION = StringUtil::getHash( "orientation" );
    const hash_type IGraphicsSceneNode::STATE_MESSAGE_LOOK_AT = StringUtil::getHash( "lookAt" );

    const hash_type IGraphicsSceneNode::STATE_MESSAGE_ADD = StringUtil::getHash( "add" );
    const hash_type IGraphicsSceneNode::STATE_MESSAGE_REMOVE = StringUtil::getHash( "remove" );

    const hash_type IGraphicsSceneNode::STATE_MESSAGE_ADD_CHILD = StringUtil::getHash( "addChild" );
    const hash_type IGraphicsSceneNode::STATE_MESSAGE_REMOVE_CHILD =
        StringUtil::getHash( "removeChild" );
    const hash_type IGraphicsSceneNode::STATE_MESSAGE_ATTACH_OBJECT =
        StringUtil::getHash( "attachObject" );
    const hash_type IGraphicsSceneNode::STATE_MESSAGE_DETACH_OBJECT =
        StringUtil::getHash( "detachObject" );
    const hash_type IGraphicsSceneNode::STATE_MESSAGE_DETACH_ALL_OBJECTS =
        StringUtil::getHash( "detachAllObject" );

    const String IGraphicsSceneNode::nameStr = String( "name" );
    const String IGraphicsSceneNode::numObjectsStr = String( "NumObjects" );
    const String IGraphicsSceneNode::sceneNodePositionStr = String( "sceneNodePosition" );
    const String IGraphicsSceneNode::sceneNodeScaleStr = String( "sceneNodeScale" );
    const String IGraphicsSceneNode::sceneNodeOrientationStr = String( "sceneNodeOrientation" );
    const String IGraphicsSceneNode::stateOrientationStr = String( "stateOrientation" );

    IGraphicsSceneNode::IGraphicsSceneNode() : ISharedObject( IGraphicsSceneNode::typeInfo() )
    {
    }

    IGraphicsSceneNode::IGraphicsSceneNode( u32 poolTypeId ) : ISharedObject( poolTypeId )
    {
    }

    IGraphicsSceneNode::~IGraphicsSceneNode() = default;

}  // namespace workphone::render
