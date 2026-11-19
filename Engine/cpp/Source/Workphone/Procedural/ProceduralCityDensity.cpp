#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Procedural/ProceduralSceneInternal.hpp"

namespace workphone::scene::procedural_scene
{
    bool selectCityBuilding( u64 lotId, u32 seed, f32 centreDistanceFraction, s32 densityPercent )
    {
        if( densityPercent >= 100 )
        {
            return true;
        }
        if( densityPercent <= 0 )
        {
            return false;
        }
        const auto hash =
            render::foliageStableHash( lotId ^ ( u64( seed ) << 32 ) ^ 0x4349545944454e53ull );
        const auto random = f32( hash >> 40 ) / 16777216.f;
        const auto distance = std::clamp( centreDistanceFraction, 0.f, 1.f );
        return .7f * random + .3f * distance < densityPercent / 100.f;
    }

    void applyCityBuildingDensity( procedural::OpenCityLayout &city, u32 seed, s32 densityPercent )
    {
        if( densityPercent == 100 || city.lots.empty() )
        {
            return;
        }
        float minX = city.lots.front().parcelX, maxX = minX;
        float minZ = city.lots.front().parcelZ, maxZ = minZ;
        for( const auto &lot : city.lots )
        {
            minX = std::min( minX, lot.parcelX );
            maxX = std::max( maxX, lot.parcelX );
            minZ = std::min( minZ, lot.parcelZ );
            maxZ = std::max( maxZ, lot.parcelZ );
        }
        const float centreX = ( minX + maxX ) * .5f, centreZ = ( minZ + maxZ ) * .5f;
        const float radius = std::max( 1.f, std::hypot( maxX - minX, maxZ - minZ ) * .5f );
        if( densityPercent > 100 )
        {
            decltype( city.lots ) expanded;
            expanded.reserve( city.lots.size() * 2 );
            for( const auto &source : city.lots )
            {
                const bool alongX = source.frontage == procedural::OpenCityLayout::Frontage::North ||
                                    source.frontage == procedural::OpenCityLayout::Frontage::South;
                const float span = alongX ? source.parcelWidth : source.parcelDepth;
                const float cross = alongX ? source.parcelDepth : source.parcelWidth;
                const float distance =
                    std::hypot( source.parcelX - centreX, source.parcelZ - centreZ ) / radius;
                if( source.park || span < 18.5f || cross < 9.f ||
                    !selectCityBuilding( source.id, seed, distance, densityPercent - 100 ) )
                {
                    expanded.push_back( source );
                    continue;
                }
                constexpr float serviceGap = 1.5f;
                const float childSpan = ( span - serviceGap ) * .5f;
                for( u32 child = 0; child < 2; ++child )
                {
                    auto lot = source;
                    const float offset = ( child ? 1.f : -1.f ) * ( childSpan + serviceGap ) * .5f;
                    if( alongX )
                    {
                        lot.parcelX += offset;
                        lot.parcelWidth = childSpan;
                    }
                    else
                    {
                        lot.parcelZ += offset;
                        lot.parcelDepth = childSpan;
                    }
                    const float widthRatio = std::clamp( source.width / source.parcelWidth, .7f, .93f );
                    const float depthRatio = std::clamp( source.depth / source.parcelDepth, .7f, .93f );
                    lot.width = lot.parcelWidth * widthRatio;
                    lot.depth = lot.parcelDepth * depthRatio;
                    lot.x = lot.parcelX;
                    lot.z = lot.parcelZ;
                    if( lot.frontage == procedural::OpenCityLayout::Frontage::West )
                    {
                        lot.x -= ( lot.parcelWidth - lot.width ) * .5f;
                    }
                    else if( lot.frontage == procedural::OpenCityLayout::Frontage::East )
                    {
                        lot.x += ( lot.parcelWidth - lot.width ) * .5f;
                    }
                    else if( lot.frontage == procedural::OpenCityLayout::Frontage::North )
                    {
                        lot.z -= ( lot.parcelDepth - lot.depth ) * .5f;
                    }
                    else
                    {
                        lot.z += ( lot.parcelDepth - lot.depth ) * .5f;
                    }
                    if( child )
                    {
                        lot.id = render::foliageStableHash( source.id ^ ( u64( seed ) << 32 ) ^
                                                            0x53504c4954424c44ull );
                        lot.variant = u32( render::foliageStableHash( lot.id ) );
                        lot.height = std::max(
                            6.4f, source.height * ( .82f + .30f * f32( lot.variant >> 24 ) / 256.f ) );
                    }
                    expanded.push_back( lot );
                }
            }
            city.lots = std::move( expanded );
            return;
        }
        for( auto &lot : city.lots )
        {
            const float distance = std::hypot( lot.parcelX - centreX, lot.parcelZ - centreZ ) / radius;
            if( lot.park || selectCityBuilding( lot.id, seed, distance, densityPercent ) )
            {
                continue;
            }
            lot.park = true;
            lot.x = lot.parcelX;
            lot.z = lot.parcelZ;
            lot.width = lot.parcelWidth;
            lot.depth = lot.parcelDepth;
        }
    }
}  // namespace workphone::scene::race
