#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIAnimator.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{

    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIAnimator, IUIElement );

    IUIAnimator::IUIAnimator( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUIAnimator::IUIAnimator() : IUIElement( IUIAnimator::typeInfo() )
    {
    }

    IUIAnimator::~IUIAnimator() = default;

}  // namespace workphone::ui
