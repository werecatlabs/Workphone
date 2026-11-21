#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUICheckbox.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUICheckbox, IUIElement );

    IUICheckbox::IUICheckbox( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUICheckbox::IUICheckbox() : IUIElement( IUICheckbox::typeInfo() )
    {
    }

    IUICheckbox::~IUICheckbox() = default;

}  // namespace workphone::ui
