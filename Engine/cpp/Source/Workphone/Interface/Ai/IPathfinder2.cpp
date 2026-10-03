#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Ai/IPathfinder2.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IPathfinder2, ISharedObject );

    IPathfinder2::~IPathfinder2() = default;

}  // namespace workphone
