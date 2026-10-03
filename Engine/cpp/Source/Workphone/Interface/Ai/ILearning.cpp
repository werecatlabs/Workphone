#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Ai/ILearning.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ILearning, ISharedObject );

    ILearning::~ILearning() = default;

}  // namespace workphone
