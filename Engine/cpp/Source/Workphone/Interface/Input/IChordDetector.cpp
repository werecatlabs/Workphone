#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Input/IChordDetector.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IChordDetector, ISharedObject );

    IChordDetector::~IChordDetector() = default;

}  // namespace workphone
