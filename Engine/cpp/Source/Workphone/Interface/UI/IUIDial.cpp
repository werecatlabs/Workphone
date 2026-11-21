#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIDial.hpp>

namespace workphone::ui
{

    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIDial, IUIElement );

    IUIDial::IUIDial( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUIDial::IUIDial() : IUIElement( IUIDial::typeInfo() )
    {
    }

    IUIDial::~IUIDial() = default;

}  // namespace workphone::ui
