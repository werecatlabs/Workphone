#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IStateContext, ISharedObject );

    IStateContext::~IStateContext() = default;

}  // namespace workphone
