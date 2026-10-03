#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIImageArray.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIImageArray, IUIElement );

    IUIImageArray::IUIImageArray( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUIImageArray::IUIImageArray() : IUIElement( IUIImageArray::typeInfo() )
    {
    }

    IUIImageArray::~IUIImageArray() = default;

}  // namespace workphone::ui
