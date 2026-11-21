#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUITextEntry.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUITextEntry, IUIElement );

    IUITextEntry::IUITextEntry( u32 poolTypeId ) : IUIText( poolTypeId )
    {
    }

    IUITextEntry::IUITextEntry() : IUIText( IUITextEntry::typeInfo() )
    {
    }

    IUITextEntry::~IUITextEntry() = default;
}  // namespace workphone::ui
