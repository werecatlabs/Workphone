#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiTerrainEditor.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>
#include <imgui_internal.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiTerrainEditor, ImGuiElement<IUITerrainEditor> );

    ImGuiTerrainEditor::ImGuiTerrainEditor( int width, int height ) :
        m_width( width ),
        m_height( height )
    {
        m_heightmap.resize( m_width * m_height );
        SetupStyle();
    }

    ImGuiTerrainEditor::ImGuiTerrainEditor() = default;

    void ImGuiTerrainEditor::SetupStyle()
    {
        ImGuiStyle &style = ImGui::GetStyle();

        // Modern color scheme
        ImVec4 *colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4( 0.08f, 0.08f, 0.08f, 1.00f );
        colors[ImGuiCol_Header] = ImVec4( 0.20f, 0.20f, 0.20f, 1.00f );
        colors[ImGuiCol_HeaderHovered] = ImVec4( 0.26f, 0.26f, 0.26f, 1.00f );
        colors[ImGuiCol_HeaderActive] = ImVec4( 0.30f, 0.30f, 0.30f, 1.00f );
        colors[ImGuiCol_Button] = ImVec4( 0.20f, 0.20f, 0.20f, 1.00f );
        colors[ImGuiCol_ButtonHovered] = ImVec4( 0.26f, 0.26f, 0.26f, 1.00f );
        colors[ImGuiCol_ButtonActive] = ImVec4( 0.30f, 0.30f, 0.30f, 1.00f );

        // Modern spacing and sizing
        style.WindowPadding = ImVec2( 15, 15 );
        style.FramePadding = ImVec2( 5, 5 );
        style.ItemSpacing = ImVec2( 12, 8 );
        style.ItemInnerSpacing = ImVec2( 8, 6 );
        style.IndentSpacing = 25.0f;
        style.ScrollbarSize = 15.0f;
        style.GrabMinSize = 5.0f;

        // Modern rounding
        style.WindowRounding = 5.0f;
        style.ChildRounding = 4.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 4.0f;
        style.ScrollbarRounding = 4.0f;
        style.GrabRounding = 4.0f;
    }

    void ImGuiTerrainEditor::update()
    {
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 10, 10 ) );

        if( ImGui::BeginTabBar( "Tabs", ImGuiTabBarFlags_Reorderable ) )
        {
            if( ImGui::BeginTabItem( ICON_FA_MOUNTAIN " Heightmap" ) )
            {
                ShowEditor();
                ImGui::EndTabItem();
            }
            if( ImGui::BeginTabItem( ICON_FA_MOUNTAIN " Layers" ) )
            {
                ShowLayermapTab();
                ImGui::EndTabItem();
            }
            if( ImGui::BeginTabItem( ICON_FA_TREE " Trees" ) )
            {
                ShowTreesTab();
                ImGui::EndTabItem();
            }
            if( ImGui::BeginTabItem( ICON_FA_MOUNTAIN " Grass" ) )
            {
                ShowGrassTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::PopStyleVar();
    }

    void ImGuiTerrainEditor::ShowEditor()
    {
        // Left panel - Tools and Settings
        ImGui::BeginChild( "ToolsPanel", ImVec2( 250, 0 ), true );

        // Heightmap settings
        ImGui::Text( "Heightmap Settings" );
        ImGui::Separator();

        ImGui::PushItemWidth( -1 );
        if( ImGui::InputInt( "Width", &m_width ) )
            ResizeHeightmap();
        if( ImGui::InputInt( "Height", &m_height ) )
            ResizeHeightmap();
        ImGui::PopItemWidth();

        if( ImGui::Button( "Resize Heightmap", ImVec2( -1, 0 ) ) )
        {
            ResizeHeightmap();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Tool selection
        ImGui::Text( "Tools" );
        ImGui::Separator();

        const char *tools[] = { "Raise", "Lower", "Smooth", "Flatten", "Noise" };
        for( int i = 0; i < 5; i++ )
        {
            if( ImGui::Selectable( tools[i], m_tool == static_cast<Tool>( i ), ImGuiSelectableFlags_None,
                                   ImVec2( -1, 30 ) ) )
            {
                m_tool = static_cast<Tool>( i );
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Tool settings
        ImGui::Text( "Tool Settings" );
        ImGui::Separator();

        ImGui::PushItemWidth( -1 );
        ImGui::SliderFloat( "Brush Size", &m_brushSize, 1.0f, 50.0f, "%.1f" );
        ImGui::SliderFloat( "Brush Strength", &m_brushStrength, 0.0f, 1.0f, "%.2f" );
        ImGui::SliderFloat( "Smooth Factor", &m_smoothFactor, 0.0f, 1.0f, "%.2f" );
        ImGui::PopItemWidth();

        ImGui::EndChild();

        ImGui::SameLine();

        // Right panel - Heightmap preview
        ImGui::BeginChild( "PreviewPanel", ImVec2( 0, 0 ), true );
        ImGui::Text( "Heightmap Preview" );
        ImGui::Separator();

        // TODO: Add heightmap preview rendering here
        ImGui::Text( "Heightmap preview will be rendered here" );

        ImGui::EndChild();
    }

    void ImGuiTerrainEditor::ShowLayermapTab()
    {
        ImGui::BeginChild( "LayerSettings", ImVec2( 250, 0 ), true );

        ImGui::Text( "Layer Settings" );
        ImGui::Separator();

        ImGui::PushItemWidth( -1 );
        if( ImGui::InputInt( "Number of Layers", &m_num_layers ) )
        {
            ResizeLayermap();
        }
        ImGui::PopItemWidth();

        if( ImGui::Button( "Update Layers", ImVec2( -1, 0 ) ) )
        {
            ResizeLayermap();
        }

        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild( "LayerList", ImVec2( 0, 0 ), true );

        for( int i = 0; i < m_num_layers; i++ )
        {
            ImGui::PushID( i );

            ImGui::BeginChild( ( "Layer" + StringUtil::toString( i ) ).c_str(), ImVec2( 0, 100 ), true );

            ImGui::Text( "Layer %d", i );
            ImGui::Separator();

            if( ImGui::Button( "Select Texture", ImVec2( -1, 0 ) ) )
            {
                SelectTexture( i );
            }

            // Layer properties
            ImGui::PushItemWidth( -1 );
            ImGui::SliderFloat( "Scale", &m_layer_scales[i], 0.1f, 10.0f, "%.2f" );
            ImGui::SliderFloat( "Blend", &m_layer_blends[i], 0.0f, 1.0f, "%.2f" );
            ImGui::PopItemWidth();

            ImGui::EndChild();

            ImGui::Spacing();
            ImGui::PopID();
        }

        ImGui::EndChild();
    }

    void ImGuiTerrainEditor::ShowTreesTab()
    {
        ImGui::BeginChild( "TreeSettings", ImVec2( 250, 0 ), true );

        ImGui::Text( "Tree Settings" );
        ImGui::Separator();

        ImGui::PushItemWidth( -1 );
        if( ImGui::InputInt( "Number of Trees", &m_num_trees ) )
        {
            ResizeTrees();
        }
        ImGui::PopItemWidth();

        if( ImGui::Button( "Update Trees", ImVec2( -1, 0 ) ) )
        {
            ResizeTrees();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Tree placement tools
        ImGui::Text( "Placement Tools" );
        ImGui::Separator();

        if( ImGui::Button( "Add Tree", ImVec2( -1, 0 ) ) )
        {
            // TODO: Implement tree placement
        }

        if( ImGui::Button( "Remove Tree", ImVec2( -1, 0 ) ) )
        {
            // TODO: Implement tree removal
        }

        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild( "TreeList", ImVec2( 0, 0 ), true );

        for( int i = 0; i < m_num_trees; i++ )
        {
            ImGui::PushID( i );

            ImGui::BeginChild( ( "Tree" + StringUtil::toString( i ) ).c_str(), ImVec2( 0, 80 ), true );

            ImGui::Text( "Tree %d", i );
            ImGui::Separator();

            ImGui::PushItemWidth( -1 );
            ImGui::InputFloat3( "Position", &m_tree_positions[i][0], "%.2f" );
            ImGui::SliderFloat( "Scale", &m_tree_scales[i], 0.1f, 5.0f, "%.2f" );
            ImGui::PopItemWidth();

            ImGui::EndChild();

            ImGui::Spacing();
            ImGui::PopID();
        }

        ImGui::EndChild();
    }

    void ImGuiTerrainEditor::ShowGrassTab()
    {
        ImGui::BeginChild( "GrassSettings", ImVec2( 250, 0 ), true );

        ImGui::Text( "Grass Settings" );
        ImGui::Separator();

        ImGui::PushItemWidth( -1 );
        if( ImGui::InputInt( "Number of Blades", &m_num_grass_blades ) )
        {
            ResizeGrass();
        }
        ImGui::PopItemWidth();

        if( ImGui::Button( "Update Grass", ImVec2( -1, 0 ) ) )
        {
            ResizeGrass();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Grass properties
        ImGui::Text( "Grass Properties" );
        ImGui::Separator();

        ImGui::PushItemWidth( -1 );
        ImGui::SliderFloat( "Min Height", &m_grass_min_height, 0.1f, 2.0f, "%.2f" );
        ImGui::SliderFloat( "Max Height", &m_grass_max_height, 0.1f, 2.0f, "%.2f" );
        ImGui::SliderFloat( "Density", &m_grass_density, 0.0f, 1.0f, "%.2f" );
        ImGui::PopItemWidth();

        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild( "GrassList", ImVec2( 0, 0 ), true );

        for( int i = 0; i < m_num_grass_blades; i++ )
        {
            ImGui::PushID( i );

            ImGui::BeginChild( ( "Grass" + StringUtil::toString( i ) ).c_str(), ImVec2( 0, 80 ), true );

            ImGui::Text( "Blade %d", i );
            ImGui::Separator();

            ImGui::PushItemWidth( -1 );
            ImGui::InputFloat2( "Position", &m_grass_blade_positions[i][0], "%.2f" );
            ImGui::InputFloat( "Height", &m_grass_blade_heights[i], 0.1f, 2.0f, "%.2f" );
            ImGui::PopItemWidth();

            ImGui::EndChild();

            ImGui::Spacing();
            ImGui::PopID();
        }

        ImGui::EndChild();
    }

    SmartPtr<scene::IComponent> ImGuiTerrainEditor::getTerrain() const
    {
        return m_terrainRenderer;
    }

    void ImGuiTerrainEditor::setTerrain( SmartPtr<scene::IComponent> terrainRenderer )
    {
        m_terrainRenderer = terrainRenderer;

        if( m_terrainRenderer )
        {
            auto numLayers = m_terrainRenderer->calculateNumLayers();

            m_num_layers = numLayers;
            m_layer_textures.resize( m_num_layers );
        }
    }

    void ImGuiTerrainEditor::ResizeHeightmap()
    {
        m_heightmap.resize( m_width * m_height );
    }

    void ImGuiTerrainEditor::SelectTexture( int layer )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto listeners = getObjectListeners();
        for( auto listener : listeners )
        {
            Array<Parameter> arguments;
            arguments.resize( 1 );

            auto subComponents = m_terrainRenderer->getSubComponentsByType<scene::TerrainLayer>();

            if( layer < subComponents.size() )
            {
                arguments[0].object = subComponents[layer];
            }

            listener->handleEvent( EventType::UI, selectTerrainTextureHash, arguments, this, this,
                                   nullptr );
        }
    }

    void ImGuiTerrainEditor::ResizeLayermap()
    {
        //m_layermap.resize( m_num_layers );
        m_layer_textures.resize( m_num_layers );

        if( m_terrainRenderer )
        {
            m_terrainRenderer->setNumLayers( m_num_layers );
            m_terrainRenderer->resizeLayermap();
        }
    }

    void ImGuiTerrainEditor::ResizeTrees()
    {
        m_tree_positions.resize( m_num_trees );
    }

    void ImGuiTerrainEditor::ResizeGrass()
    {
        m_grass_blade_positions.resize( m_num_grass_blades );
        m_grass_blade_heights.resize( m_num_grass_blades );
    }

    ImGuiTerrainEditor::~ImGuiTerrainEditor() = default;
}  // namespace workphone::ui
