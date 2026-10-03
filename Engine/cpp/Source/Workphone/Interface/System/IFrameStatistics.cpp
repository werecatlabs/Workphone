#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IFrameStatistics.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IFrameStatistics, ISharedObject );

    IFrameStatistics::~IFrameStatistics() = default;
}  // namespace workphone
