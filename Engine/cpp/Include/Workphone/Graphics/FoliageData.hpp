#ifndef WPFoliageData_h__
#define WPFoliageData_h__

#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <array>
#include <memory>

namespace workphone::render
{
    inline constexpr u32 foliageDataFormatVersion = 1;
    inline constexpr u32 foliageMaximumSpecies = 64;
    inline constexpr u32 foliageMaximumInstances = 1048576;
    inline constexpr u32 foliageMaximumPageInstances = 65536;
    inline constexpr u32 foliageMaximumPages = 2048;
    inline constexpr u32 foliageMaximumMeshVertices = 262144;
    inline constexpr u32 foliageMaximumMeshIndices = 1572864;
    inline constexpr u64 foliageMaximumSourceBytes = 128ull << 20;

    enum class FoliageKind : u32
    {
        Tree,
        Bush,
        Grass,
        GroundCover
    };
    enum FoliageInstanceFlags : u32
    {
        FoliageManual = 1,
        FoliageLocked = 2,
        FoliageRemoved = 4,
        FoliagePromoted = 8
    };
    struct FoliageMaterial
    {
        String albedo, normal, orm;
        ColourF tint = ColourF::White;
        f32 roughness = 1, metalness = 0, alphaCutoff = .5f;
        bool twoSided = true;
    };
    struct FoliageMeshSection
    {
        u32 firstIndex = 0, indexCount = 0, material = 0;
    };
    struct FoliageMeshData
    {
        Array<Vector3F> positions, normals;
        Array<Vector2F> uvs;
        Array<ColourF> colours;  // Empty means white.
        Array<u32> indices;
        Array<FoliageMeshSection> sections;
    };
    struct FoliageWind
    {
        f32 amplitude = 0, frequency = 1, flutter = 0;
        // Vertex weights are derived from species-local height above this pivot.
        f32 bendStart = 0, bendEnd = 1;
        f32 maximumInteractionBend = 0;
    };
    struct FoliageImpostorView
    {
        Vector3F direction = Vector3F( 0, 0, 1 );
        Vector2F uvMinimum = Vector2F( 0, 0 ), uvMaximum = Vector2F( 1, 1 );
    };
    struct FoliageImpostor
    {
        String albedo, normal, orm, depth;
        Array<FoliageImpostorView> views;
        u32 width = 0, height = 0, gutter = 2;
        bool relightable = false;
    };
    struct FoliageSpecies
    {
        String id;
        FoliageKind kind = FoliageKind::Tree;
        Vector3F boundsMinimum = Vector3F( -.5f, 0, -.5f );
        Vector3F boundsMaximum = Vector3F( .5f, 1, .5f );
        Vector3F pivot = Vector3F::zero();
        std::array<FoliageMeshData, 3> lods;
        Array<FoliageMaterial> materials;
        // Fraction of viewport height; select the first qualifying level.
        std::array<f32, 3> screenHeightThresholds = { .12f, .035f, 0 };
        std::array<f32, 3> densityFractions = { 1, 1, 1 };
        f32 maximumDistance = 500, shadowDistance = 150, hysteresis = .15f;
        u32 shadowLodBias = 1;
        bool castShadows = true, farBillboard = false;
        FoliageWind wind;
        FoliageImpostor impostor;
        // Optional nearby capsule/box proxy policy, independent of visual LOD.
        f32 collisionRadius = 0, collisionHeight = 0, collisionDistance = 32;
    };
    struct FoliageCellKey
    {
        s32 x = 0, z = 0;
        bool operator==( const FoliageCellKey &o ) const
        {
            return x == o.x && z == o.z;
        }
        bool operator!=( const FoliageCellKey &o ) const
        {
            return !( *this == o );
        }
        bool operator<( const FoliageCellKey &o ) const
        {
            return x == o.x ? z < o.z : x < o.x;
        }
    };
    struct FoliageInstance
    {
        u64 id = 0;
        u32 species = 0;
        // Metres in the owning world's local space; cell ownership is floor(x/z / pageSize).
        Vector3F position = Vector3F::zero();
        QuaternionF rotation = QuaternionF::identity();
        Vector3F scale = Vector3F( 1, 1, 1 );
        ColourF tint = ColourF::White;
        u32 seed = 0, flags = 0;
    };
    struct FoliagePageData
    {
        FoliageCellKey cell;
        Array<FoliageInstance> instances;
        u64 revision = 0;
    };
    struct FoliageWorldData
    {
        f32 pageSize = 64;
        Array<std::shared_ptr<const FoliageSpecies>> species;
        Array<FoliageInstance> instances;
        u64 revision = 0;
    };
    using FoliageSnapshot = std::shared_ptr<const FoliageWorldData>;

    WPCore_API bool validateFoliageSpecies( const FoliageSpecies &, String &error );
    WPCore_API bool validateFoliageWorld( const FoliageWorldData &, String &error );
    // The immutable definition must already have passed validateFoliageWorld.
    WPCore_API bool validateFoliagePage( const FoliagePageData &, const FoliageWorldData &definition,
                                         String &error );
    WPCore_API bool foliageCellForPosition( const Vector3F &, f32 pageSize, FoliageCellKey &,
                                            String &error );
    WPCore_API u64 foliageCellIdentity( FoliageCellKey );
    WPCore_API u64 foliageStableHash( u64 value );
    WPCore_API u64 foliageSourceBytes( const FoliageWorldData & );
    // Definition/instances must already be validated. Geometry includes off-centre
    // bounds and flutter; bend accounts for nonuniform instance scale projection.
    WPCore_API bool foliageInstanceRadii( const FoliageWorldData &definition,
                                          const Array<FoliageInstance> &instances, f32 &geometryRadius,
                                          f32 &bendRadius, String &error );
    WPCore_API bool serializeFoliageSpecies( const FoliageSpecies &, String &output, String &error );
    WPCore_API bool deserializeFoliageSpecies( const String &, FoliageSpecies &output, String &error );
    WPCore_API bool serializeFoliageWorld( const FoliageWorldData &, String &output, String &error );
    WPCore_API bool deserializeFoliageWorld( const String &, FoliageWorldData &output, String &error );
}  // namespace workphone::render
#endif
