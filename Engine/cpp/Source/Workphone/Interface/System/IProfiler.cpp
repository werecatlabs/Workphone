#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IProfiler.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IProfiler, ISharedObject );

    IProfiler::~IProfiler() = default;
}  // namespace workphone
