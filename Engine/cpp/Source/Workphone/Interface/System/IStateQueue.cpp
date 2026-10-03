#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IStateQueue.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IStateQueue, IObject );

    IStateQueue::~IStateQueue() = default;

}  // namespace workphone
