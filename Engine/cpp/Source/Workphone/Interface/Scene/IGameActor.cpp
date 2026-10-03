#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, IGameActor, IResource );

    const u32 IGameActor::ActorFlagReserved = ( 1 << 0 );
    const u32 IGameActor::ActorFlagStatic = ( 1 << 1 );
    const u32 IGameActor::ActorFlagVisible = ( 1 << 2 );
    const u32 IGameActor::ActorFlagEnabled = ( 1 << 3 );
    const u32 IGameActor::ActorFlagMine = ( 1 << 4 );
    const u32 IGameActor::ActorFlagPerpetual = ( 1 << 5 );
    const u32 IGameActor::ActorFlagDirty = ( 1 << 6 );
    const u32 IGameActor::ActorFlagAwake = ( 1 << 7 );
    const u32 IGameActor::ActorFlagStarted = ( 1 << 8 );
    const u32 IGameActor::ActorFlagDummy = ( 1 << 9 );
    const u32 IGameActor::ActorFlagInScene = ( 1 << 10 );
    const u32 IGameActor::ActorFlagEnabledInScene = ( 1 << 11 );
    const u32 IGameActor::ActorFlagIsEditor = ( 1 << 12 );
    const u32 IGameActor::ActorFlagSmoothMotion = ( 1 << 13 );
    const u32 IGameActor::ActorFlagHidden = ( 1 << 14 );
    const u32 IGameActor::ActorFlagDontSave = ( 1 << 15 );
    const u32 IGameActor::ActorFlagPrefab = ( 1 << 16 );

    const String IGameActor::actorName = "Actor";

    IGameActor::IGameActor() : IResource( IGameActor::typeInfo() )
    {
    }

    IGameActor::IGameActor( u32 poolTypeId ) : IResource( poolTypeId )
    {
    }

    IGameActor::~IGameActor() = default;

}  // namespace workphone::scene
