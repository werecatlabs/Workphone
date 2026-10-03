#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/UIAnchorStateData.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, UIAnchorStateData, StateData );

    UIAnchorStateData::UIAnchorStateData() : StateData( UIAnchorStateData::typeInfo() )
    {
    }

    UIAnchorStateData::~UIAnchorStateData() = default;

}  // namespace workphone
