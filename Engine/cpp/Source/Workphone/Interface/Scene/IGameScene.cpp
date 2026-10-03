#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, IGameScene, IResource );

    IGameScene::IGameScene( u32 poolTypeId ) : IResource( poolTypeId )
    {
    }

    IGameScene::IGameScene() : IResource( IGameScene::typeInfo() )
    {
    }

    IGameScene::~IGameScene() = default;

}  // namespace workphone::scene
