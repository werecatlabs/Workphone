#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIDialogBox.hpp>

namespace workphone::ui
{

    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIDialogBox, IUIElement );

    IUIDialogBox::IUIDialogBox( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUIDialogBox::IUIDialogBox() : IUIElement( IUIDialogBox::typeInfo() )
    {
    }

    IUIDialogBox::~IUIDialogBox() = default;

}  // namespace workphone::ui
