#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIToggle.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIToggle, IUIButton );

    IUIToggle::IUIToggle( u32 poolTypeId ) : IUIButton( poolTypeId )
    {
    }

    IUIToggle::IUIToggle() : IUIButton( IUIToggle::typeInfo() )
    {
    }

    IUIToggle::~IUIToggle() = default;
}  // namespace workphone::ui
