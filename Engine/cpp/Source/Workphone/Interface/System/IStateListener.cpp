#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IStateListener, ISharedObject );

    IStateListener::~IStateListener() = default;

}  // namespace workphone
