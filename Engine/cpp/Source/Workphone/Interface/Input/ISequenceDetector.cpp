#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Input/ISequenceDetector.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ISequenceDetector, ISharedObject );

    ISequenceDetector::~ISequenceDetector() = default;

}  // namespace workphone
