#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiDataGrid.hpp>
#include <WPImGui/ImGuiUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiDataGrid, IUIDataGrid );
    u32 ImGuiDataGrid::m_childWindowCount = 0;

    ImGuiDataGrid::ImGuiDataGrid()
    {
        auto name = "DataGrid_" + StringUtil::toString( m_childWindowCount++ );
        setName( name );
    }

    ImGuiDataGrid::~ImGuiDataGrid() = default;

    void ImGuiDataGrid::update()
    {
        if( auto properties = getProperties() )
        {
            createElement( properties, nullptr );
        }
    }

    SmartPtr<Properties> ImGuiDataGrid::getProperties() const
    {
        return m_properties;
    }

    void ImGuiDataGrid::setProperties( SmartPtr<Properties> properties )
    {
        m_properties = properties;
    }

    void ImGuiDataGrid::createElement( SmartPtr<IUIElement> element )
    {
        auto dataGrid = workphone::static_pointer_cast<ImGuiDataGrid>( element );
        if( dataGrid )
        {
            auto properties = dataGrid->getProperties();
            if( properties )
            {
                dataGrid->createElement( properties, nullptr );
            }
        }
    }

    void ImGuiDataGrid::createElement( SmartPtr<Properties> properties, SmartPtr<Properties> parent )
    {
        auto dataGrid = this;

        auto dataGridName = properties->getName();
        if( StringUtil::isNullOrEmpty( dataGridName ) )
        {
            dataGridName = "DataGrid_" + StringUtil::toString( m_childWindowCount++ );
        }

        // Set up table flags for a data grid
        auto flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInner |
                     ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable |
                     ImGuiTableFlags_SortMulti | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;

        // Set cell padding to make cells bigger
        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 10.0f, 6.0f ) );

        // Get column definitions from properties
        auto columns = properties->getChildren();
        if( columns.empty() )
        {
            return;
        }

        auto numColumns = 0;

        for( auto &column : columns )
        {
            auto propertyList = column->getPropertiesAsArray();
            numColumns = static_cast<s32>( propertyList.size() );
            break;
        }

        // Begin the table with the number of columns
        if( ImGui::BeginTable( dataGridName.c_str(), numColumns, flags ) )
        {
            // Set up columns
            for( auto &column : columns )
            {
                auto propertyList = column->getPropertiesAsArray();

                for( auto &property : propertyList )
                {
                    auto columnName = property.getName();
                    ImGui::TableSetupColumn( columnName.c_str(), ImGuiTableColumnFlags_WidthStretch,
                                             150.0f );
                }

                break;
            }

            // Headers
            ImGui::TableHeadersRow();

            // Get data rows
            for( auto &column : columns )
            {
                auto rows = column->getPropertiesAsArray();

                ImGui::TableNextRow();

                auto colCount = 0;
                for( auto &row : rows )
                {
                    // Display each cell in the row
                    ImGui::TableSetColumnIndex( colCount );

                    auto value = row.getValue();
                    ImGui::Text( "%s", value.c_str() );

                    colCount++;
                }
            }

            ImGui::EndTable();
        }

        // Pop the style var we pushed
        ImGui::PopStyleVar();
    }

    void ImGuiDataGrid::handleDataChanged( SmartPtr<IUIDataGrid> dataGrid,
                                           SmartPtr<Properties> properties, const String &name,
                                           const String &value )
    {
        if( dataGrid && properties )
        {
            auto args = Array<Parameter>();
            args.resize( 4 );

            args[0].setStr( name );
            args[1].setStr( value );
            args[2].object = properties;
            args[3].object = dataGrid;
        }
    }
}  // namespace workphone::ui
