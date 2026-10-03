#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/ILSystemRule.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone, ILSystemRule, ISharedObject );

    ILSystemRule::~ILSystemRule() = default;

}  // namespace workphone::procedural
