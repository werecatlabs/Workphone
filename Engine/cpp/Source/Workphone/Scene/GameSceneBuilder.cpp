#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/GameSceneBuilder.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/IBuildDirector.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, GameSceneBuilder, IGameSceneBuilder );

    GameSceneBuilder::GameSceneBuilder() = default;

    GameSceneBuilder::~GameSceneBuilder() = default;

    SmartPtr<IGameScene> GameSceneBuilder::getScene() const
    {
        return m_scene;
    }

    void GameSceneBuilder::setScene( SmartPtr<IGameScene> scene )
    {
        m_scene = scene;
    }

    SmartPtr<IBuildDirector> GameSceneBuilder::getDirector() const
    {
        return m_director;
    }

    void GameSceneBuilder::setDirector( SmartPtr<IBuildDirector> director )
    {
        m_director = director;
    }

    SmartPtr<ISharedObject> GameSceneBuilder::create()
    {
        return nullptr;
    }

    void GameSceneBuilder::setActors( const Array<SmartPtr<IGameActor>> &actors )
    {
        m_actors = actors;
    }

    Array<SmartPtr<IGameActor>> GameSceneBuilder::getActors() const
    {
        return m_actors;
    }

    void GameSceneBuilder::setActor( SmartPtr<IGameActor> actor )
    {
        m_actor = actor;
    }

    SmartPtr<IGameActor> GameSceneBuilder::getActor() const
    {
        return m_actor;
    }
}  // namespace workphone::scene
