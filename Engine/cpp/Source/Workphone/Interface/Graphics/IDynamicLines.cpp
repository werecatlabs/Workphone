#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IDynamicLines.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/TypeManager.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone, IDynamicLines, IGraphicsObject );

    IDynamicLines::~IDynamicLines() = default;

}  // namespace workphone::render
