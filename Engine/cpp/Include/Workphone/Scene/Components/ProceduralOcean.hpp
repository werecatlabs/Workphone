#pragma once
#include <Workphone/Scene/Components/ProceduralMesh.hpp>
#include <Workphone/Core/ColourF.hpp>

namespace workphone::scene
{
    /// A static water surface with a regenerated plane and dielectric material.
    class WPCore_API ProceduralOcean : public ProceduralMesh
    {
    public:
        ProceduralOcean();
        ~ProceduralOcean() override;

        void setSize( f32 width, f32 depth );
        bool isLandscapeOwned() const;
        void setLandscapeOwned( bool owned );
        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;
        void unload( SmartPtr<ISharedObject> data ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        SmartPtr<IMesh> buildMesh() override;
        void applyGeneratedMaterials() override;

    private:
        f32 m_width = 2048, m_depth = 2048;
        ColourF m_colour = ColourF( .025f, .16f, .24f, 1 );
        f32 m_roughness = .12f;
        bool m_landscapeOwned = false;
        SmartPtr<Material> m_generatedMaterial;
        SmartPtr<render::IMaterial> m_materialResource;
        String m_generatedMaterialId;
    };
}  // namespace workphone::scene
