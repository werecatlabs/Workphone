#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/UIImageStateData.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, UIImageStateData, StateData );

    UIImageStateData::UIImageStateData() : StateData( UIImageStateData::typeInfo() )
    {
    }

    UIImageStateData::~UIImageStateData() = default;

}  // namespace workphone
