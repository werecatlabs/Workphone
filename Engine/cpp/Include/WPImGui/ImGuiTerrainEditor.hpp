#ifndef ImGuiTerrainEditor_h__
#define ImGuiTerrainEditor_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include "Workphone/Interface/UI/IUITerrainEditor.hpp"

namespace workphone
{
    namespace ui
    {
        class ImGuiTerrainEditor : public ImGuiElement<IUITerrainEditor>
        {
        public:
            ImGuiTerrainEditor();
            ImGuiTerrainEditor( int width, int height );
            ~ImGuiTerrainEditor() override;

            void update() override;

            void ShowEditor();
            void ShowLayermapTab();
            void ShowTreesTab();
            void ShowGrassTab();

            SmartPtr<scene::IComponent> getTerrain() const override;
            void setTerrain( SmartPtr<scene::IComponent> terrainRenderer ) override;

            WP_CLASS_REGISTER_DECL;

        private:
            void SetupStyle();
            void ResizeHeightmap();
            void SelectTexture( int layer );
            void ResizeLayermap();
            void ResizeTrees();
            void ResizeGrass();

            enum class Tool
            {
                Raise,
                Lower,
                Smooth,
                Flatten,
                Noise
            };

            // Terrain renderer
            SmartPtr<scene::TerrainSystem> m_terrainRenderer;

            // Heightmap settings
            int m_width = 129;
            int m_height = 129;
            std::vector<float> m_heightmap;
            Tool m_tool = Tool::Raise;

            // Tool settings
            float m_brushSize = 10.0f;
            float m_brushStrength = 0.5f;
            float m_smoothFactor = 0.5f;

            // Layer settings
            int m_num_layers = 0;
            std::vector<SmartPtr<render::ITexture>> m_layer_textures;
            std::vector<float> m_layer_scales;
            std::vector<float> m_layer_blends;

            // Tree settings
            int m_num_trees = 0;
            Array<Vector3F> m_tree_positions;
            std::vector<float> m_tree_scales;

            // Grass settings
            int m_num_grass_blades = 0;
            Array<Vector3F> m_grass_blade_positions;
            Array<f32> m_grass_blade_heights;
            float m_grass_min_height = 0.1f;
            float m_grass_max_height = 1.0f;
            float m_grass_density = 0.5f;

            String selected_filename;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiTerrainEditor_h__
