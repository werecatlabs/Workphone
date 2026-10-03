#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUISeparator.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUISeparator, IUIElement );

    IUISeparator::IUISeparator( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUISeparator::IUISeparator() : IUIElement( IUISeparator::typeInfo() )
    {
    }

    IUISeparator::~IUISeparator() = default;

}  // namespace workphone::ui
