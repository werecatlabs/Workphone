#pragma once
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <stdexcept>

namespace runtime_scene_membership_contracts
{
    using namespace workphone;
    inline void require(bool valid,const char *message){if(!valid)throw std::runtime_error(message);}

    // Exercise real scene-root publication without granting the child its own
    // InScene bit. Restore the original root/state before subsequent draw tests.
    template<class Inspect>
    void nested(SmartPtr<scene::IGameManager> manager,SmartPtr<scene::IGameActor> actor,Inspect inspect)
    {
        auto owner=actor?actor->getScene():nullptr;
        require(manager&&actor&&owner&&actor->getFlag(scene::IGameActor::ActorFlagInScene)&&actor->isEnabled(),
            "Nested membership requires its enabled, published fixture root");
        const auto state=actor->getState();
        auto parent=manager->createActor();require(parent!=nullptr,"Nested membership parent must allocate");
        struct Cleanup {SmartPtr<scene::IGameManager> manager;SmartPtr<scene::IGameActor> actor;~Cleanup(){if(actor)manager->destroyActor(actor);}} cleanup{manager,parent};
        parent->setName("RuntimeMembershipAnchor");parent->setState(state);
        owner->removeActor(actor);inspect(false);
        parent->addChild(actor);
        require(!actor->getFlag(scene::IGameActor::ActorFlagInScene),"Child publication must not manufacture its own root flag");
        inspect(false);
        owner->addActor(parent);inspect(true);
        require(!actor->getFlag(scene::IGameActor::ActorFlagInScene),"Nested native visibility must come from the ancestor anchor");
        parent->setEnabled(false);inspect(false);
        parent->setEnabled(true);inspect(true);
        owner->removeActor(parent);inspect(false);
        parent->removeChild(actor);owner->addActor(actor);inspect(true);
        require(actor->getState()==state&&actor->isEnabled()&&actor->getFlag(scene::IGameActor::ActorFlagInScene)&&!actor->getParent(),
            "Nested membership regression must restore the original root state and ownership");
    }
}
