#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUILabelTogglePair.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUILabelTogglePair, IUIElement );

    IUILabelTogglePair::IUILabelTogglePair( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUILabelTogglePair::IUILabelTogglePair() : IUIElement( IUILabelTogglePair::typeInfo() )
    {
    }

    IUILabelTogglePair::~IUILabelTogglePair() = default;

}  // namespace workphone::ui
