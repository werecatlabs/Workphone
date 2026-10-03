#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Sound/ISoundPlayer.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ISoundPlayer, ISharedObject );

    ISoundPlayer::~ISoundPlayer() = default;

}  // namespace workphone
