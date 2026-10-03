#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIColourPicker.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{

    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIColourPicker, IUIElement );

    IUIColourPicker::IUIColourPicker( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUIColourPicker::IUIColourPicker() : IUIElement( IUIColourPicker::typeInfo() )
    {
    }

    IUIColourPicker::~IUIColourPicker() = default;

}  // namespace workphone::ui
