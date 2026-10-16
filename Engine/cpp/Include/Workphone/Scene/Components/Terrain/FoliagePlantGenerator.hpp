#ifndef WPFoliagePlantGenerator_h__
#define WPFoliagePlantGenerator_h__
#include <Workphone/Graphics/FoliageData.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>

namespace workphone::scene
{
    struct FoliagePlantRecipe
    {
        String speciesId = "generated-tree";
        render::FoliageKind kind = render::FoliageKind::Tree;
        u32 seed = 1, branches = 12;
        f32 height = 5, radius = 1.5f, trunkRadius = .12f;
        render::FoliageMaterial bark, leaves;
        render::FoliageWind wind;
    };

    // Native, deterministic finite meshes with independently generated LOD topology.
    // Leaf/grass atlas resource references are supplied by the caller and preserved.
    WPCore_API bool generateFoliagePlant( const FoliagePlantRecipe &, render::FoliageSpecies &output,
                                          String &error );
    WPCore_API bool importFoliageMesh( SmartPtr<IMesh> mesh, const String &speciesId,
                                       const Array<render::FoliageMaterial> &materials,
                                       render::FoliageSpecies &output, String &error );
}  // namespace workphone::scene
#endif
