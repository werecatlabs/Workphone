#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IDebugLine.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/TypeManager.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone, IDebugLine, ISharedObject );

    IDebugLine::~IDebugLine() = default;

}  // namespace workphone::render
