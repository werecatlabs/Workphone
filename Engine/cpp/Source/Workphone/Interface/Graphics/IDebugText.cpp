#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IDebugText.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/TypeManager.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone, IDebugText, ISharedObject );

    IDebugText::~IDebugText() = default;

}  // namespace workphone::render
