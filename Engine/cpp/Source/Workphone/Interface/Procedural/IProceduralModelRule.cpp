#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IProceduralModelRule.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, IProceduralModelRule, ISharedObject );

    IProceduralModelRule::~IProceduralModelRule() = default;
}  // namespace workphone::procedural
