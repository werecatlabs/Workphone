#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUICollapsingHeader.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{

    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUICollapsingHeader, IUIElement );

    IUICollapsingHeader::IUICollapsingHeader( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUICollapsingHeader::IUICollapsingHeader() : IUIElement( IUICollapsingHeader::typeInfo() )
    {
    }

    IUICollapsingHeader::~IUICollapsingHeader() = default;

}  // namespace workphone::ui
