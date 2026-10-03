#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Input/ITapDetector.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ITapDetector, ISharedObject );

    ITapDetector::~ITapDetector() = default;

}  // namespace workphone
