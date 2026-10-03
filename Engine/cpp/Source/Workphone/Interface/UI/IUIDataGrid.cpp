#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIDataGrid.hpp>

namespace workphone::ui
{

    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIDataGrid, IUIElement );

    IUIDataGrid::IUIDataGrid( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUIDataGrid::IUIDataGrid() : IUIElement( IUIDataGrid::typeInfo() )
    {
    }

    IUIDataGrid::~IUIDataGrid() = default;

}  // namespace workphone::ui
