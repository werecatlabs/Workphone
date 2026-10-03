#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUILabelSliderPair.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUILabelSliderPair, IUIElement );

    IUILabelSliderPair::IUILabelSliderPair( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUILabelSliderPair::IUILabelSliderPair() : IUIElement( IUILabelSliderPair::typeInfo() )
    {
    }

    IUILabelSliderPair::~IUILabelSliderPair() = default;

}  // namespace workphone::ui
