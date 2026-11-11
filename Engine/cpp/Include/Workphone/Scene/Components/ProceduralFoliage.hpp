#pragma once
#include <Workphone/Scene/Components/ProceduralMesh.hpp>
#include <Workphone/Scene/Components/Terrain/FoliagePlantGenerator.hpp>

namespace workphone::scene
{
    /// An editable standalone plant using the same generator as terrain foliage.
    class WPCore_API ProceduralFoliage : public ProceduralMesh
    {
    public:
        enum class Preset { Oak, Birch, Pine, Bush, Grass, TallGrass, Palm };

        ProceduralFoliage();
        ~ProceduralFoliage() override;

        void setPreset( Preset preset );
        Preset getPreset() const;
        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;
        void unload( SmartPtr<ISharedObject> data ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        SmartPtr<IMesh> buildMesh() override;
        void applyGeneratedMaterials() override;

    private:
        Preset m_preset = Preset::Oak;
        FoliagePlantRecipe m_recipe;
        u32 m_lod = 0;
        Array<SmartPtr<Material>> m_generatedMaterials;
        Array<SmartPtr<render::IMaterial>> m_materialResources;
        String m_generatedMaterialIds;
        SmartPtr<render::ITexture> m_leafTexture;
        Preset m_texturePreset = Preset::Oak;
    };
}  // namespace workphone::scene
