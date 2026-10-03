#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IDecalCursor.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/TypeManager.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone, IDecalCursor, ISharedObject );

    IDecalCursor::~IDecalCursor() = default;

}  // namespace workphone::render
