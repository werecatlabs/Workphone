#ifndef GameActorSystem_h__
#define GameActorSystem_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <memory>

namespace workphone::scene
{
    class GameActor;
    struct GameActorComponentUpdates;

    /** Runs actor component callbacks from reusable, contiguous membership snapshots. */
    class WPCore_API GameActorSystem final
    {
    public:
        enum class Phase
        {
            PreUpdate,
            Update,
            PostUpdate
        };

        static void dispatch( GameActor &actor, Phase phase );

    private:
        static std::shared_ptr<const GameActorComponentUpdates> pinComponents( GameActor &actor );
    };
}  // namespace workphone::scene

#endif
