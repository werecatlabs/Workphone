#ifndef WORKPHONE_SCENE_UI_HIERARCHY_UPDATE_HPP
#define WORKPHONE_SCENE_UI_HIERARCHY_UPDATE_HPP

namespace workphone::scene::ui_hierarchy
{
    // Actor updates own the subtree walk. Components still cascade when called
    // directly, but must not repeat the actor's walk for every component/node.
    inline thread_local unsigned actorOrderWalkDepth = 0;

    class ActorOrderWalk
    {
    public:
        ActorOrderWalk() { ++actorOrderWalkDepth; }
        ~ActorOrderWalk() { --actorOrderWalkDepth; }
        ActorOrderWalk( const ActorOrderWalk & ) = delete;
        ActorOrderWalk &operator=( const ActorOrderWalk & ) = delete;
    };

    inline bool actorOwnsOrderWalk() { return actorOrderWalkDepth != 0; }
}

#endif
