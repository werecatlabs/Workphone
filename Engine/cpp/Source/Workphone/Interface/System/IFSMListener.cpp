#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IFSMListener.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/TypeManager.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IFSMListener, ISharedObject );

    IFSMListener::~IFSMListener() = default;
}  // namespace workphone
