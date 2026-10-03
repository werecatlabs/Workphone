#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Animation/IAnimationTimeIndex.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IAnimationTimeIndex, ISharedObject );

    IAnimationTimeIndex::~IAnimationTimeIndex() = default;

}  // namespace workphone
