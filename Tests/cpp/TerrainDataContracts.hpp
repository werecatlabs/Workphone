#ifndef WPTerrainDataContracts_h__
#define WPTerrainDataContracts_h__

#include <Workphone/Graphics/Terrain.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <Workphone/Interface/Graphics/ITerrainRayResult.hpp>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace terrain_data_contracts
{
    using namespace workphone;
    using namespace workphone::render;

    inline void require( bool condition, const char *message )
    {
        if( !condition )
            throw std::runtime_error( message );
    }

    inline bool terrainNear( real_Num actual, real_Num expected, real_Num tolerance = 1.e-4 )
    {
        return std::abs( actual - expected ) <= tolerance;
    }

    inline void run()
    {
        TerrainData data;
        data.dimensions = { 3, 5 };
        data.spacing = { 2, 3 };
        data.origin = { -2, -6 };
        data.heightScale = 2;
        data.heights.resize( 15 );
        for( s32 z = 0; z < 5; ++z )
            for( s32 x = 0; x < 3; ++x )
                data.heights[size_t( z ) * 3 + x] = f32( x + 10 * z );
        String error;
        require( validateTerrainData( data, error ), "Asymmetric rectangular terrain must validate" );
        Transform3<real_Num> transform;
        transform.setPosition( { 10, 7, 20 } );
        transform.setScale( { 2, 3, 4 } );
        f32 height = 0;
        require( sampleTerrainHeight( data, transform, { 10, 0, 20 }, height ) && terrainNear( height, 133 ),
                 "Terrain height must use row-major samples, height scale and world Y transform" );
        require( sampleTerrainHeight( data, transform, { 6, 0, -4 }, height ) && terrainNear( height, 7 ) &&
                     sampleTerrainHeight( data, transform, { 14, 0, 44 }, height ) &&
                     terrainNear( height, 259 ),
                 "Terrain world boundaries must match rectangular sample endpoints" );
        require( !sampleTerrainHeight( data, transform, { 14.01, 0, 44 }, height ),
                 "Terrain must reject samples outside its finite footprint" );
        Vector3<real_Num> hit;
        require( intersectTerrain( data, transform, Ray3F( { 10, 300, 20 }, { 0, -1, 0 } ), hit ) &&
                     terrainNear( hit.y, 133 ),
                 "Vertical ray and world height query must agree" );
        require( intersectTerrain( data, transform, Ray3F( { 10, 0, 20 }, { 0, 2, 0 } ), hit ) &&
                     terrainNear( hit.y, 133 ),
                 "Upward non-unit rays must hit the same terrain surface" );
        require( !intersectTerrain( data, transform, Ray3F( { 100, 300, 20 }, { 0, -1, 0 } ), hit ) &&
                     !intersectTerrain( data, transform, Ray3F( { 10, 300, 20 }, { 0, 0, 0 } ), hit ),
                 "Outside and zero-direction rays must miss" );

        auto yawed = transform;
        yawed.setOrientation( { real_Num( std::cos( 0.05 ) ), 0, real_Num( std::sin( 0.05 ) ), 0 } );
        for( const auto x : { 0, 2 } )
            for( const auto z : { 0, 4 } )
            {
                const Vector3<real_Num> local( data.origin.x + x * data.spacing.x,
                                               data.heights[z * 3 + x] * data.heightScale,
                                               data.origin.y + z * data.spacing.y );
                const auto world =
                    yawed.getPosition() + yawed.getOrientation() * ( local * yawed.getScale() );
                require( sampleTerrainHeight( data, yawed, world, height ) && terrainNear( height, world.y ),
                         "Rotated rendered corners must remain inside after inverse float roundoff" );
                require(
                    intersectTerrain(
                        data, yawed,
                        Ray3F( { f32( world.x ), f32( world.y + 300 ), f32( world.z ) }, { 0, -1, 0 } ),
                        hit ) &&
                        terrainNear( hit.y, world.y ),
                    "Picking rotated rendered corners must use the same boundary tolerance" );
            }
        const Vector3<real_Num> outsideLocal( data.origin.x - 0.01f, 0, 0 );
        const auto outsideWorld =
            yawed.getPosition() + yawed.getOrientation() * ( outsideLocal * yawed.getScale() );
        require(
            !sampleTerrainHeight( data, yawed, outsideWorld, height ) &&
                !intersectTerrain(
                    data, yawed,
                    Ray3F( { f32( outsideWorld.x ), 300, f32( outsideWorld.z ) }, { 0, -1, 0 } ), hit ),
            "Float boundary tolerance must not admit a point clearly outside a rotated edge" );

        TerrainData saddle;
        saddle.origin = { 0, 0 };
        saddle.heights = { 0, 0, 0, 4 };
        require(
            sampleTerrainHeight( saddle, {}, { 0.5, 0, 0.5 }, height ) && terrainNear( height, 0 ) &&
                sampleTerrainHeight( saddle, {}, { 0.75, 0, 0.75 }, height ) && terrainNear( height, 2 ),
            "Terrain samples must follow rendered anti-diagonal triangles, not bilinear interpolation" );
        require( intersectTerrain( saddle, {}, Ray3F( { -1, 1, 0.75f }, { 1, 0, 0 } ), hit ) &&
                     terrainNear( hit.x, 0.5 ) && terrainNear( hit.y, 1 ),
                 "Horizontal ray from outside must traverse cells and find the triangle surface" );
        TerrainMeshData geometry;
        require( buildTerrainMeshData( data, geometry, error ) && geometry.positions.size() == 15 &&
                     geometry.indices.size() == 48 && geometry.uvs.front() == Vector2F( 0, 0 ) &&
                     geometry.uvs.back() == Vector2F( 1, 1 ),
                 "CPU terrain mesh must preserve rectangle dimensions and UV endpoints" );
        require( geometry.indices[0] == 0 && geometry.indices[1] == 3 && geometry.indices[2] == 1 &&
                     geometry.indices[3] == 1 && geometry.indices[4] == 3 && geometry.indices[5] == 4,
                 "Terrain mesh topology must match the sampling diagonal and upward winding" );
        for( size_t i = 0; i < geometry.positions.size(); ++i )
        {
            const auto &normal = geometry.normals[i];
            require( normal.y > 0 && terrainNear( normal.dotProduct( normal ), 1 ),
                     "Terrain mesh normals must be finite, normalized and upward" );
        }
        auto negative = saddle;
        negative.heightScale = -2;
        require( sampleTerrainHeight( negative, {}, { 0.75, 0, 0.75 }, height ) && terrainNear( height, -4 ),
                 "Negative height scale must mirror height consistently" );
        negative.heightScale = 0;
        require( sampleTerrainHeight( negative, {}, { 0.75, 0, 0.75 }, height ) && terrainNear( height, 0 ) &&
                     intersectTerrain( negative, {}, Ray3F( { 0.75f, 1, 0.75f }, { 0, -1, 0 } ), hit ) &&
                     terrainNear( hit.y, 0 ),
                 "Zero height scale must produce a flat terrain" );
        auto invalid = data;
        invalid.heights.resize( 14 );
        require( !validateTerrainData( invalid, error ), "Mismatched terrain sample counts must fail" );
        invalid = data;
        invalid.heights[3] = std::numeric_limits<f32>::quiet_NaN();
        require( !validateTerrainData( invalid, error ), "Nonfinite terrain samples must fail" );
        invalid = data;
        invalid.heightScale = std::numeric_limits<f32>::max();
        require( !validateTerrainData( invalid, error ),
                 "Finite samples whose scaled value overflows must fail" );
        invalid = data;
        invalid.spacing.x = std::numeric_limits<f32>::max();
        require( !validateTerrainData( invalid, error ),
                 "Finite spacing whose endpoint overflows must fail" );
        invalid = data;
        invalid.dimensions = { 8193, 8193 };
        require( !validateTerrainData( invalid, error ),
                 "Oversized terrain dimensions must fail before allocating" );
        invalid = saddle;
        invalid.origin.x = 1.e20f;
        require( !validateTerrainData( invalid, error ),
                 "Terrain spacing must not collapse adjacent f32 renderer coordinates" );
        auto steep = saddle;
        steep.heights = { 0, 1.e20f, 0, 0 };
        require(
            validateTerrainData( steep, error ) && buildTerrainMeshData( steep, geometry, error ) &&
                intersectTerrain( steep, {}, Ray3F( { 0.25f, 1.e20f, 0.25f }, { 0, -1, 0 } ), hit ) &&
                std::abs( hit.y / 2.5e19 - 1 ) < 1.e-5,
            "Finite steep terrain must retain valid mesh normals and triangle intersections" );
        auto tiny = saddle;
        tiny.heights.assign( 4, 0.0f );
        tiny.spacing = { std::numeric_limits<f32>::denorm_min(),
                         std::numeric_limits<f32>::denorm_min() };
        require( validateTerrainData( tiny, error ) && buildTerrainMeshData( tiny, geometry, error ) &&
                     geometry.normals[0] == Vector3F( 0, 1, 0 ),
                 "Small representable terrain spacing must not underflow normal calculations" );

        auto terrain = make_ptr<Terrain>();
        require( terrain->applyTerrainData( data, error ),
                 "Headless terrain source publication must work" );
        const auto before = terrain->getTerrainSnapshot();
        require( terrain->applyTerrainData( data, error, before->revision ) &&
                     terrain->getTerrainSnapshot() == before,
                 "Identical terrain publication must preserve its revision and snapshot" );
        terrain->setHeightMapSize( data.dimensions );
        require( terrain->getTerrainSnapshot() == before && before->origin == data.origin &&
                     before->spacing == data.spacing,
                 "Legacy same-size setter must preserve explicit source origin and spacing" );
        auto changed = data;
        changed.heights[4] += 3;
        require( terrain->applyTerrainData( changed, error, before->revision ) &&
                     terrain->getTerrainRevision() > before->revision && before->heights[4] == 11,
                 "Source snapshots must remain detached and immutable after editing" );
        const auto after = terrain->getTerrainSnapshot();
        require( !terrain->applyTerrainData( data, error, before->revision ) &&
                     terrain->getTerrainSnapshot() == after,
                 "A stale terrain edit must leave the complete previous revision intact" );
        require( !terrain->applyTerrainData( invalid, error ) && terrain->getTerrainSnapshot() == after,
                 "Invalid terrain input must preserve the previous source revision" );
        terrain->setWorldTransform( transform );
        require( terrain->getWorldTransform() == transform &&
                     terrainNear( terrain->getHeightAtWorldPosition( { 10, 0, 20 } ), 133 ),
                 "Terrain object's world transform and query must use shared source semantics" );
        auto unsupported = transform;
        unsupported.setScale( { 0, 1, 1 } );
        terrain->setWorldTransform( unsupported );
        require( terrain->getWorldTransform() == transform,
                 "Degenerate terrain transform must preserve the last valid transform" );
        unsupported = transform;
        unsupported.setOrientation( Quaternion<real_Num>::euler( 0.1, 0, 0 ) );
        terrain->setWorldTransform( unsupported );
        require( terrain->getWorldTransform() == transform,
                 "Unsupported pitch must preserve the last valid transform" );
        terrain->setVisible( false );
        require( !terrain->isVisible(), "Headless terrain visibility must retain changes" );
        terrain->setVisible( true );
        require( terrain->isVisible(), "Headless terrain visibility must restore changes" );
        const auto mesh = terrain->getMesh();
        require( mesh && terrain->getMesh() == mesh,
                 "CPU collision/export mesh must exist and be retained for its source revision" );
        TerrainData larger;
        larger.dimensions = { 259, 3 };
        larger.origin = { -129, -1 };
        larger.heights.assign( 259 * 3, 0.0f );
        larger.heights[258] = 2;
        require( buildTerrainMeshData( larger, geometry, error ) &&
                     geometry.positions.size() == 259 * 3 && geometry.positions[258].y == 2,
                 "Terrain mesh must retain samples beyond the former 257-side preview cap" );
    }
}  // namespace terrain_data_contracts

inline void runTerrainDataContracts()
{
    terrain_data_contracts::run();
}

#endif
