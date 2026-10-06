#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, IGameManager, ISharedObject );

    const hash_type IGameManager::sceneLoadedHash = StringUtil::getHash( "sceneLoaded" );
    const hash_type IGameManager::sceneUnloadedHash = StringUtil::getHash( "sceneUnloaded" );
    const hash_type IGameManager::scenePlayHash = StringUtil::getHash( "scenePlay" );
    const hash_type IGameManager::sceneEditHash = StringUtil::getHash( "sceneEdit" );
    const hash_type IGameManager::sceneClearHash = StringUtil::getHash( "sceneClear" );

    IGameManager::~IGameManager() = default;

}  // namespace workphone::scene
