//
// Created by Zane Desir on 22/11/2021.
//

#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Scene/IGamePrefabManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, IGamePrefabManager, IResourceManager );

    IGamePrefabManager::~IGamePrefabManager() = default;

}  // namespace workphone::scene
