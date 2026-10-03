#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IGraphicsScene, ISharedObject );

    const String IGraphicsScene::sceneNodeStr = "SceneNode";
    const String IGraphicsScene::sceneNodePrefix = "SceneNode_";

    const u32 IGraphicsScene::VIEWPORT_MASK_TERRAIN = ( 1 << 27 );
    const u32 IGraphicsScene::VIEWPORT_MASK_OCCLUDER = ( 1 << 28 );
    const u32 IGraphicsScene::VIEWPORT_MASK_USER = ( 1 << 29 );
    const u32 IGraphicsScene::VIEWPORT_MASK_SHADOW = ( 1 << 30 );

    const u32 IGraphicsScene::enableShadowsFlag = 1 << 1;
    const u32 IGraphicsScene::depthShadowsFlag = 1 << 2;
    const u32 IGraphicsScene::clearingFlag = 1 << 3;

    IGraphicsScene::~IGraphicsScene() = default;

}  // namespace workphone::render
