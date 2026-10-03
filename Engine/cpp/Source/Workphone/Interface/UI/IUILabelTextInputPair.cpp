#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUILabelTextInputPair.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUILabelTextInputPair, IUIElement );

    IUILabelTextInputPair::IUILabelTextInputPair( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUILabelTextInputPair::IUILabelTextInputPair() : IUIElement( IUILabelTextInputPair::typeInfo() )
    {
    }

    IUILabelTextInputPair::~IUILabelTextInputPair() = default;

}  // namespace workphone::ui
