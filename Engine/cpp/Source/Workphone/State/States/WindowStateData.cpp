#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/WindowStateData.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WindowStateData, StateData );

    WindowStateData::WindowStateData() : StateData( WindowStateData::typeInfo() )
    {
    }

    WindowStateData::~WindowStateData() = default;

}  // namespace workphone
