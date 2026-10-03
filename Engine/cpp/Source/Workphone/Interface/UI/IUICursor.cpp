#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUICursor.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{

    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUICursor, IUIElement );

    IUICursor::IUICursor( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUICursor::IUICursor() : IUIElement( IUICursor::typeInfo() )
    {
    }

    IUICursor::~IUICursor() = default;

}  // namespace workphone::ui
