#pragma once

namespace cityReserveTrees
{
    struct Tree
    {
        Vector3F position;
        float radius = 0.f;
    };

    struct Candidate
    {
        Tree tree;
        float width, height, priority;
        u64 id, site;
        u32 species;
    };

    bool clearSightLines( Vector3F point, float crown, const CityRoadTerrainMesh &footways,
                          const procedural::CityRoadSplineSource &source )
    {
        for( const auto &crossing : footways.crossings )
        {
            const auto delta = point - crossing.centre;
            if( std::abs( delta.dotProduct( crossing.across ) ) < crossing.width * .5f + 4.15f + crown &&
                std::abs( delta.dotProduct( crossing.outward ) ) < crossing.depth * .5f + 6.65f + crown )
            {
                return false;
            }
        }
        for( const auto &junction : source.roundabouts )
        {
            const auto delta = point - junction.center;
            const float radius = junction.outerRadius + 4.75f + crown;
            if( delta.x * delta.x + delta.z * delta.z < radius * radius )
            {
                return false;
            }
            for( const auto &port : junction.ports )
            {
                const auto along = Vector3F( port.outward.x, 0, port.outward.z ).normaliseCopy();
                const Vector3F across( -along.z, 0, along.x );
                const float station = delta.dotProduct( along ) - junction.outerRadius;
                if( station > -1.45f - crown && station < port.length + 4.5f + crown &&
                    std::abs( delta.dotProduct( across ) ) < port.width * .5f + 4.80f + crown )
                {
                    return false;
                }
            }
        }
        return true;
    }

    void scatter( SceneAssets &assets, const SourceBudgetVector<cityReserveLandscape::Site> &sites,
                  const cityHillside::Exclusions &exclusions, const SourceBudgetVector<float> &radii,
                  const render::TerrainData &terrain, const Transform3<real_Num> &placement,
                  render::FoliageWorldData &world, VehicleAppearanceQuality quality )
    {
        const auto footways = assets.roadAuthoring->getRuntimeFootways();
        if( !footways || footways->sourceRevision != assets.roadAuthoring->getActiveRevision() ||
            footways->terrainRevision != terrain.revision )
        {
            return;
        }
        const auto &source = assets.roadAuthoring->getSource();
        const auto limit = quality == VehicleAppearanceQuality::Preview ? 8u : 18u;
        using cityHillside::Cell;
        using cityHillside::cellFor;
        using cityHillside::unit;
        SourceBudgetMap<Cell, SourceBudgetVector<Tree>> trees;
        for( const auto &instance : world.instances )
        {
            if( world.species[instance.species]->kind == render::FoliageKind::Tree )
            {
                trees[cellFor( instance.position )].push_back(
                    { instance.position,
                      radii[instance.species] * std::max( instance.scale.x, instance.scale.z ) } );
            }
        }
        const auto spaced = [&]( const Tree &tree ) {
            const Vector3F radius( 12.f, 0, 12.f );
            return cityHillside::visitCells(
                tree.position - radius, tree.position + radius, [&]( const Cell &cell ) {
                    const auto found = trees.find( cell );
                    if( found == trees.end() )
                    {
                        return true;
                    }
                    return std::none_of(
                        found->second.begin(), found->second.end(), [&]( const Tree &other ) {
                            const float spacing =
                                std::max( 7.75f, .88f * ( tree.radius + other.radius ) + 2.45f );
                            return cityHillside::segmentDistance( tree.position, other.position,
                                                                  other.position ) < spacing * spacing;
                        } );
                } );
        };
        const auto inBed = [&]( Vector3F point ) {
            return std::any_of( sites.begin(), sites.end(), [&]( const auto &bed ) {
                const auto from = point - bed.centre;
                return std::abs( from.dotProduct( bed.along ) ) <= bed.halfLength + .36f &&
                       std::abs( from.dotProduct( bed.outward ) ) <= bed.halfDepth + .36f;
            } );
        };
        SourceBudgetMap<Cell, Candidate> pockets;
        for( const auto &site : sites )
        {
            if( site.halfLength < 2.05f || site.halfDepth < .92f )
            {
                continue;
            }
            const auto raw = render::foliageStableHash( site.key ^ 0x7374726565747472ull );
            const auto key = raw ? raw : 0x7374726565747472ull;
            const auto species = u32( key & 3ull );
            const float width = .57f + .24f * unit( key + 1 );
            const float height = .62f + .27f * unit( key + 2 );
            const float crown = width * radii[species];
            const float swing = .47f + .25f * unit( key + 3 );
            const float side = unit( key + 4 ) < .5f ? -1.f : 1.f;
            bool offered = false;
            // Use a few side stations and deeper shoulders with the same
            // street orientation as the admitted public planted bed.
            for( const float retreat : { 1.45f, 2.70f, 4.10f, 6.05f } )
            {
                for( const float lateral : { 0.f, side * swing, -side * swing } )
                {
                    auto point = site.centre + site.along * ( site.halfLength * lateral ) +
                                 site.outward * ( site.halfDepth + retreat );
                    const Tree tree{ point, crown };
                    if( inBed( point ) || !exclusions.clear( point, crown + .55f ) ||
                        !clearSightLines( point, crown, *footways, source ) || !spaced( tree ) )
                    {
                        continue;
                    }
                    if( !render::sampleTerrainHeight( terrain, placement, point, point.y ) ||
                        !cityHillside::uninterruptedFooting( terrain, placement, point, .48f ) )
                    {
                        continue;
                    }
                    bool supported = true;
                    for( const auto offset : { site.along, -site.along, site.outward, -site.outward } )
                    {
                        float edge = 0.f;
                        supported = supported &&
                                    render::sampleTerrainHeight( terrain, placement,
                                                                 point + offset * .48f, edge ) &&
                                    std::abs( edge - point.y ) <= .192f;
                    }
                    if( !supported )
                    {
                        continue;
                    }
                    const float extent = std::max( 1.f, assets.cityLayout.extent );
                    const float priority = .64f * std::hypot( point.x, point.z ) / extent +
                                           .024f * retreat + .073f * unit( key + 5 );
                    const Cell cell( s32( std::floor( point.x / 24.f ) ),
                                     s32( std::floor( point.z / 24.f ) ) );
                    const auto prior = pockets.find( cell );
                    if( prior == pockets.end() ||
                        std::tie( priority, key ) <
                            std::tie( prior->second.priority, prior->second.id ) )
                    {
                        pockets.insert_or_assign(
                            cell,
                            Candidate{
                                { point, crown }, width, height, priority, key, site.key, species } );
                    }
                    offered = true;
                    break;
                }
                if( offered )
                {
                    break;
                }
            }
        }
        SourceBudgetVector<Candidate> ordered;
        ordered.reserve( pockets.size() );
        for( const auto &[cell, candidate] : pockets )
        {
            ordered.push_back( candidate );
        }
        std::sort( ordered.begin(), ordered.end(), []( const Candidate &a, const Candidate &b ) {
            return std::tie( a.priority, a.id ) < std::tie( b.priority, b.id );
        } );

        size_t count = 0;
        for( const auto &candidate : ordered )
        {
            if( count >= limit )
            {
                break;
            }
            if( !spaced( candidate.tree ) )
            {
                continue;
            }
            render::FoliageInstance instance;
            instance.id = candidate.id;
            instance.seed = u32( candidate.id );
            instance.species = candidate.species;
            instance.position = candidate.tree.position - Vector3F( 0, .065f, 0 );
            instance.rotation = QuaternionF::eulerDegrees( 0, 360.f * unit( candidate.id + 6 ), 0 );
            instance.scale = { candidate.width, candidate.height, candidate.width };
            const float colour = .94f + .14f * unit( candidate.id + 7 );
            instance.tint = { colour, colour, colour * .94f, 1 };
            world.instances.push_back( instance );
            trees[cellFor( candidate.tree.position )].push_back( candidate.tree );
            ++count;
            WP_LOG( String( "Terrain city street tree site=" ) + StringUtil::toString( candidate.site ) +
                    ", species=" + StringUtil::toString( candidate.species ) +
                    ", x=" + StringUtil::toString( instance.position.x ) +
                    ", y=" + StringUtil::toString( instance.position.y ) +
                    ", z=" + StringUtil::toString( instance.position.z ) +
                    ", crown=" + StringUtil::toString( candidate.tree.radius ) );
        }
        WP_LOG( String( "Terrain city street trees=" ) + StringUtil::toString( count ) +
                ", eligible pockets=" + StringUtil::toString( pockets.size() ) );
    }
}  // namespace cityReserveTrees
