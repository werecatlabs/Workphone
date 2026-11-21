#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIFrame.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIFrame, IUIElement );

    IUIFrame::IUIFrame( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUIFrame::IUIFrame() : IUIElement( IUIFrame::typeInfo() )
    {
    }

    IUIFrame::~IUIFrame() = default;

}  // namespace workphone::ui
