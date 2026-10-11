#ifndef WPFoliageAuthoring_h__
#define WPFoliageAuthoring_h__

#include <Workphone/Graphics/FoliageData.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <atomic>

namespace workphone::scene
{
    struct FoliagePlacementRule
    {
        u32 species = 0;
        bool enabled = true, randomYaw = true, alignToNormal = false;
        f32 weight = 1, density = .1f, minimumSpacing = 1;
        f32 minimumHeight = -100000, maximumHeight = 100000;
        f32 minimumSlope = 0, maximumSlope = 90;
        f32 minimumScale = .8f, maximumScale = 1.2f, heightOffset = 0;
        u32 allowedLayers = 15;
        f32 minimumLayerWeight = 0, minimumMoisture = 0, maximumMoisture = 1;
    };
    struct FoliageExclusion
    {
        // Polygon when radius==0; a road/river polyline corridor otherwise.
        Array<Vector2F> points;
        f32 radius = 0;
    };
    struct FoliagePlacementRecipe
    {
        u32 version = 1, seed = 1;
        f32 pageSize = 64, candidateSpacing = 1;
        Array<FoliagePlacementRule> rules;
        Array<FoliageExclusion> exclusions;
        Vector2I maskDimensions = Vector2I( 0, 0 );
        // Terrain-normalized density and biome masks; species mask values are rule indices.
        Array<f32> densityMask;
        Array<u32> biomeMask;
    };

    /** Bounded synchronized authoring. Immutable published snapshots contain stable
     * generated IDs and manual overrides/tombstones. A stroke owns one undo entry;
     * cancel, parse, validation and generation failure retain the previous state.
     * Runtime publication is a separate TerrainSystem step against active ground.
     */
    class WPCore_API FoliageAuthoring final : public ISharedObject
    {
    public:
        static constexpr u32 maximumCandidates = 262144;
        static constexpr u32 maximumAuthoredInstances = 65536;
        FoliageAuthoring();
        ~FoliageAuthoring() override;
        render::FoliageSnapshot getSnapshot() const;
        render::FoliageSnapshot getGroundedSnapshot( render::TerrainSnapshot terrain, String &error ) const;
        FoliagePlacementRecipe getRecipe() const;
        bool configure( const FoliagePlacementRecipe &, String &error );
        bool setSpecies( Array<std::shared_ptr<const render::FoliageSpecies>>, String &error );
        bool regenerate( render::TerrainSnapshot, const Array<render::FoliageCellKey> &cells,
                         String &error, const std::atomic<bool> *cancel = nullptr );
        bool regenerateAll( render::TerrainSnapshot, String &error,
                            const std::atomic<bool> *cancel = nullptr );
        bool resample( render::TerrainSnapshot, String &error );
        bool beginStroke( String &error );
        bool commitStroke( String &error );
        bool cancelStroke( String &error );
        bool isStrokeActive() const;
        bool paint( render::TerrainSnapshot, u32 species, Vector2F centre, f32 radius,
                    u32 count, u32 seed, String &error );
        bool erase( Vector2F centre, f32 radius, String &error );
        Array<u64> select( const Array<Vector2F> &polygon ) const;
        bool transformSelection( const Array<u64> &, Vector3F translation, f32 yaw,
                                 f32 scale, String &error );
        bool replaceSpecies( const Array<u64> &, u32 species, String &error );
        bool setInstanceFlags( const Array<u64> &, u32 mask, bool enabled, String &error );
        bool undo( String &error );
        bool redo( String &error );
        bool serialize( String &output, String &error ) const;
        bool deserialize( const String &json, String &error );
        bool save( const String &path, String &error ) const;
        bool load( const String &path, String &error );
        WP_CLASS_REGISTER_DECL;
    private:
        struct State;
        std::unique_ptr<State> m_state;
    };
}
#endif
