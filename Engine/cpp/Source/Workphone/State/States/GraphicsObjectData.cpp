#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/GraphicsObjectData.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, GraphicsObjectData, StateData );

    GraphicsObjectData::GraphicsObjectData() : StateData( GraphicsObjectData::typeInfo() )
    {
    }

    GraphicsObjectData::GraphicsObjectData( u32 poolTypeId ) : StateData( poolTypeId )
    {
    }

    GraphicsObjectData::~GraphicsObjectData() = default;

}  // namespace workphone
