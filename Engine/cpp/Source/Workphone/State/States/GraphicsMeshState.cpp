#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/GraphicsMeshState.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, GraphicsMeshState, GraphicsObjectData );

    GraphicsMeshState::GraphicsMeshState() : GraphicsObjectData( GraphicsMeshState::typeInfo() )
    {
    }

    GraphicsMeshState::~GraphicsMeshState() = default;

}  // namespace workphone
