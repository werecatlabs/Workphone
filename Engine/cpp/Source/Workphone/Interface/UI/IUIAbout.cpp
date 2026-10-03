#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIAbout.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIAbout, IUIElement );

    IUIAbout::IUIAbout( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUIAbout::IUIAbout() : IUIElement( IUIAbout::typeInfo() )
    {
    }

    IUIAbout::~IUIAbout() = default;
}  // namespace workphone::ui
