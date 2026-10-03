#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Ai/IAiGoalEvaluator.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAiGoalEvaluator, ISharedObject );

    IAiGoalEvaluator::~IAiGoalEvaluator() = default;

}  // namespace workphone
