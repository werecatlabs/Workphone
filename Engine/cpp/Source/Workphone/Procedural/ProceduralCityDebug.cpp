#include <Workphone/WorkphonePCH.hpp>
#include "ProceduralSceneInternal.hpp"

namespace workphone::scene::race
{
    namespace sceneBuild
    {
        void buildCityLayoutDebug( SceneAssets &assets, const OpenCityLayout &city,
                                   const CityRoadBlockFaces *terrainBlocks )
        {
            assets.cityDebugBlockCount = 0;
            assets.cityDebugLotCount = 0;
            if( !assets.cityDebugBlocks && !assets.cityDebugLots )
            {
                return;
            }

            const auto terrain = assets.cityTerrainRoads && assets.roadAuthoring
                                     ? assets.roadAuthoring->getTerrainSnapshot()
                                     : nullptr;
            const auto placement = assets.cityTerrainRoads && assets.roadAuthoring
                                       ? assets.roadAuthoring->getTerrainPlacement()
                                       : Transform3<real_Num>();
            const auto point = [&]( float x, float z, float fallback, float lift, Vector3F &result ) {
                result = { x, fallback, z };
                if( terrain && !render::sampleTerrainHeight( *terrain, placement, result, result.y ) )
                {
                    return false;
                }
                result.y += lift;
                return true;
            };
            const auto line = [&]( Geometry &geometry, OpenCityLayout::Point a, OpenCityLayout::Point b,
                                   float fallback, float lift, float radius, bool blockBoundary ) {
                const int steps =
                    std::max( 1, int( std::ceil( std::hypot( b.x - a.x, b.z - a.z ) / 6.f ) ) );
                Vector3F previous;
                bool previousValid = point( a.x, a.z, fallback, lift, previous );
                for( int i = 1; i <= steps; ++i )
                {
                    const float t = float( i ) / steps;
                    Vector3F next;
                    const bool valid =
                        point( a.x + ( b.x - a.x ) * t, a.z + ( b.z - a.z ) * t, fallback, lift, next );
                    bool clear = true;
                    if( blockBoundary && previousValid && valid && assets.roadAuthoring )
                    {
                        const auto middle = ( previous + next ) * .5f;
                        for( const auto &roundabout : assets.roadAuthoring->getSource().roundabouts )
                        {
                            const float radiusXZ = roundabout.outerRadius + 6.f;
                            const float dx = middle.x - roundabout.center.x;
                            const float dz = middle.z - roundabout.center.z;
                            if( dx * dx + dz * dz < radiusXZ * radiusXZ )
                            {
                                clear = false;
                                break;
                            }
                        }
                        if( clear )
                        {
                            float roadHeight = 0;
                            bool driveable = false;
                            clear = !assets.roadAuthoring->getRuntimeSurface( middle.x, middle.z,
                                                                              roadHeight, driveable ) ||
                                    !driveable;
                        }
                    }
                    if( previousValid && valid && clear )
                    {
                        geometry.beam( previous, next, radius );
                    }
                    previous = next;
                    previousValid = valid;
                }
            };
            const auto rectangle = [&]( Geometry &geometry, float x, float z, float width, float depth,
                                        float base, float lift, float radius ) {
                const OpenCityLayout::Point corners[] = { { x - width * .5f, z - depth * .5f },
                                                          { x + width * .5f, z - depth * .5f },
                                                          { x + width * .5f, z + depth * .5f },
                                                          { x - width * .5f, z + depth * .5f } };
                for( int i = 0; i < 4; ++i )
                {
                    line( geometry, corners[i], corners[( i + 1 ) % 4], base, lift, radius, false );
                }
            };
            const auto publish = [&]( const String &name, const Geometry &geometry, ColourF colour ) {
                if( geometry.indices.empty() )
                {
                    return;
                }
                auto surface = material( assets, colour, 1.f );
                surface->setEmissive( colour * .22f );
                mesh( assets, name, geometry, surface );
            };

            if( assets.cityDebugBlocks )
            {
                Geometry outlines;
                const auto outline = [&]( const auto &boundary ) {
                    const auto before = outlines.indices.size();
                    const auto inside = insetOpenCityDebugBlock(
                        boundary, city.naturalRoads ? 6.f : OpenCityLayout::roadHalfWidth + 2.5f );
                    for( size_t i = 0; i < inside.size(); ++i )
                    {
                        line( outlines, inside[i], inside[( i + 1 ) % inside.size()], 0.f, 1.15f, .075f,
                              true );
                    }
                    assets.cityDebugBlockCount += outlines.indices.size() > before;
                };
                if( terrainBlocks )
                {
                    for( const auto &boundary : *terrainBlocks )
                    {
                        outline( boundary );
                    }
                }
                else
                {
                    for( const auto &boundary : openCityDebugBlocks( city ) )
                    {
                        outline( boundary );
                    }
                }
                publish( "City debug block boundaries", outlines, { .95f, .23f, .05f, 1 } );
            }
            if( assets.cityDebugLots )
            {
                Geometry occupied, parks, footprints;
                for( const auto &lot : city.lots )
                {
                    rectangle( lot.park ? parks : occupied, lot.parcelX, lot.parcelZ, lot.parcelWidth,
                               lot.parcelDepth, lot.groundHeight, .40f, .055f );
                    if( !lot.park )
                    {
                        rectangle( footprints, lot.x, lot.z, lot.width, lot.depth, lot.groundHeight,
                                   .72f, .045f );
                    }
                }
                publish( "City debug occupied lots", occupied, { .02f, .75f, .95f, 1 } );
                publish( "City debug park lots", parks, { .17f, .94f, .28f, 1 } );
                publish( "City debug building footprints", footprints, { .98f, .75f, .08f, 1 } );
                assets.cityDebugLotCount = static_cast<u32>( city.lots.size() );
            }
        }
    }  // namespace sceneBuild
}  // namespace workphone::scene::race
