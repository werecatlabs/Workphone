#pragma once
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>

namespace workphone::scene::detail
{
    // Child actors inherit a published scene anchor; only scene roots need the
    // InScene bit. Reject detached, unloaded, foreign and cyclic parent chains.
    inline bool runtimeActorPublished(SmartPtr<IGameActor> actor)
    {
        SmartPtr<IGameScene> scene;
        bool anchored=false;
        size_t depth=0;
        for(;actor&&depth<4096;actor=actor->getParent(),++depth){
            if(!actor->isLoaded())return false;
            if(auto owner=actor->getScene()){
                if(scene&&scene!=owner)return false;
                scene=owner;
                anchored|=actor->getFlag(IGameActor::ActorFlagInScene);
            }
        }
        return !actor&&anchored&&scene&&scene->isLoaded();
    }
}
