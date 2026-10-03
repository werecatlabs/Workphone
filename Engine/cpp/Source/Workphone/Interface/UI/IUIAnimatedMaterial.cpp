#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIAnimatedMaterial.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{

    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIAnimatedMaterial, IUIElement );

    IUIAnimatedMaterial::IUIAnimatedMaterial( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUIAnimatedMaterial::IUIAnimatedMaterial() : IUIElement( IUIAnimatedMaterial::typeInfo() )
    {
    }

    IUIAnimatedMaterial::~IUIAnimatedMaterial() = default;

}  // namespace workphone::ui
