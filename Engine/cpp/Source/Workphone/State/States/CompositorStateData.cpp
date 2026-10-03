#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/CompositorStateData.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, CompositorStateData, StateData );

    CompositorStateData::CompositorStateData() : StateData( CompositorStateData::typeInfo() )
    {
    }

    CompositorStateData::~CompositorStateData() = default;

}  // namespace workphone
