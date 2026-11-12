#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Terrain/FoliagePresets.hpp>
#include <Workphone/Graphics/GeneratedTextureSource.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace workphone::scene
{
    namespace
    {
        bool isGrass( FoliagePreset preset )
        {
            return preset == FoliagePreset::Grass || preset == FoliagePreset::TallGrass;
        }
    }  // namespace

    // Compact cutout sprites need no project-specific textures. White shades
    // retain the inspector's leaf tint; transparent texels preserve silhouette gaps.
    Array<u8> foliagePresetCutout( FoliagePreset preset )
    {
        constexpr u32 size = foliageCutoutSize;
        auto pixels = makeSourceBudgetSnapshot<u8>();
        pixels.resize( size * size * 4, 0 );
        const bool grass = isGrass( preset );
        const bool pine = preset == FoliagePreset::Pine;
        const bool palm = preset == FoliagePreset::Palm;
        for( u32 y = 0; y < size; ++y )
        {
            for( u32 x = 0; x < size; ++x )
            {
                const float u = ( x + .5f ) / size, v = ( y + .5f ) / size;
                bool covered = false;
                if( grass )
                {
                    for( int blade = 0; blade < 7; ++blade )
                    {
                        const float top = .04f + .055f * ( ( blade * 3 ) % 5 );
                        const float t = std::clamp( ( 1 - v ) / ( 1 - top ), 0.f, 1.f );
                        const float base = .20f + .10f * blade;
                        const float lean = ( blade - 3 ) * .045f;
                        const float centre = base + lean * t * t;
                        covered |=
                            v >= top && v < .99f && std::abs( u - centre ) < .028f * ( 1 - t ) + .0015f;
                    }
                }
                else if( palm )
                {
                    // One feathered frond, mapped continuously over its curved
                    // strip: base at v=1, tip at v=0. Gaps separate the leaflets.
                    covered = v > .025f && v < .99f && std::abs( u - .5f ) < .012f;
                    for( int leaf = 0; leaf < 14; ++leaf )
                    {
                        const float stem = .10f + leaf * .058f;
                        const float reach = .12f + .35f * std::sin( 3.14159265f * stem );
                        const float dx = std::abs( u - .5f );
                        const float dy = v - ( stem - dx * .22f );
                        covered |= dx < reach && std::abs( dy ) < .016f * ( 1 - dx / reach ) + .002f;
                    }
                }
                else
                {
                    covered = v > .10f && v < .96f && std::abs( u - .5f ) < .009f;
                    const int pairs = pine ? 12 : 5;
                    for( int leaf = 0; leaf < pairs; ++leaf )
                    {
                        const float stem = .20f + leaf * ( .65f / pairs );
                        for( int side : { -1, 1 } )
                        {
                            const float dx = ( u - .5f ) * side;
                            const float reach = pine ? .31f : .24f;
                            const float dy = v - ( stem - dx * .6f );
                            if( pine )
                            {
                                covered |= dx > .015f && dx < reach &&
                                           std::abs( dy ) < .007f * ( 1 - dx / reach ) + .002f;
                            }
                            else
                            {
                                const float a = ( dx - .14f ) / .15f;
                                const float b = dy / .050f;
                                covered |= a * a + b * b < 1;
                            }
                        }
                    }
                    // A terminal leaf closes the tip of the broadleaf spray.
                    if( !pine )
                    {
                        const float dx = ( u - .5f ) / .06f;
                        const float dy = ( v - .12f ) / .10f;
                        covered |= dx * dx + dy * dy < 1;
                    }
                }
                const auto index = ( size_t( y ) * size + x ) * 4;
                const auto shade = u8( 205 + 45 * ( 1 - v ) );
                pixels[index] = pixels[index + 1] = pixels[index + 2] = shade;
                pixels[index + 3] = covered ? 255 : 0;
            }
        }
        return pixels;
    }

    FoliagePlantRecipe foliagePresetRecipe( FoliagePreset preset )
    {
        FoliagePlantRecipe recipe;
        recipe.seed = 7919;
        recipe.bark.tint = { .27f, .15f, .065f, 1 };
        recipe.leaves.tint = { .25f, .48f, .12f, 1 };
        switch( preset )
        {
        case FoliagePreset::Oak:
            recipe.height = 6;
            recipe.radius = 2.3f;
            recipe.trunkRadius = .20f;
            recipe.branches = 28;
            break;
        case FoliagePreset::Birch:
            recipe.height = 7;
            recipe.radius = 1.35f;
            recipe.trunkRadius = .10f;
            recipe.branches = 20;
            recipe.bark.tint = { .80f, .78f, .70f, 1 };
            recipe.leaves.tint = { .40f, .58f, .16f, 1 };
            break;
        case FoliagePreset::Pine:
            recipe.height = 8;
            recipe.radius = 2.3f;
            recipe.trunkRadius = .16f;
            recipe.branches = 32;
            recipe.leaves.tint = { .12f, .31f, .16f, 1 };
            break;
        case FoliagePreset::Bush:
            recipe.kind = render::FoliageKind::Bush;
            recipe.height = 1.3f;
            recipe.radius = 1.0f;
            recipe.trunkRadius = .035f;
            recipe.branches = 16;
            break;
        case FoliagePreset::Palm:
            recipe.height = 7;
            recipe.radius = 3;
            recipe.trunkRadius = .18f;
            recipe.branches = 12;
            recipe.bark.tint = { .46f, .34f, .20f, 1 };
            recipe.leaves.tint = { .22f, .43f, .10f, 1 };
            break;
        case FoliagePreset::Grass:
        case FoliagePreset::TallGrass:
            recipe.kind = render::FoliageKind::Grass;
            recipe.height = preset == FoliagePreset::Grass ? .45f : 1.1f;
            recipe.radius = preset == FoliagePreset::Grass ? .40f : .55f;
            recipe.trunkRadius = .01f;
            recipe.branches = 16;
            recipe.leaves.tint = { .36f, .52f, .16f, 1 };
            break;
        }
        return recipe;
    }

    bool generateFoliagePreset( FoliagePreset preset, const FoliagePlantRecipe &recipe,
                                render::FoliageSpecies &output, String &error )
    {
        if( u32( preset ) > u32( FoliagePreset::Palm ) )
        {
            error = "Unknown procedural foliage preset.";
            return false;
        }
        render::FoliageSpecies candidate;
        const auto shape =
            preset == FoliagePreset::Palm ? FoliagePlantShape::Palm : FoliagePlantShape::Broadleaf;
        if( !generateFoliagePlant( recipe, shape, candidate, error ) )
        {
            return false;
        }
        if( preset == FoliagePreset::Pine )
        {
            for( auto &source : candidate.lods )
            {
                // Same conifer taper used by the procedural racing scene. Transform
                // normals by the inverse transpose so crown lighting follows the shape.
                for( size_t i = 0; i < source.positions.size(); ++i )
                {
                    auto &p = source.positions[i];
                    auto &n = source.normals[i];
                    const float k = .68f / recipe.height;
                    const float width = .96f - .68f * std::clamp( p.y / recipe.height, 0.f, 1.f );
                    n = { n.x / width, n.y + k * ( p.x * n.x + p.z * n.z ) / width, n.z / width };
                    n.normalise();
                    p.x *= width;
                    p.z *= width;
                }
            }
        }
        const auto largest = std::numeric_limits<f32>::max();
        auto minimum = Vector3F( largest, largest, largest );
        auto maximum = -minimum;
        for( const auto &mesh : candidate.lods )
        {
            for( const auto &position : mesh.positions )
            {
                minimum.x = std::min( minimum.x, position.x );
                minimum.y = std::min( minimum.y, position.y );
                minimum.z = std::min( minimum.z, position.z );
                maximum.x = std::max( maximum.x, position.x );
                maximum.y = std::max( maximum.y, position.y );
                maximum.z = std::max( maximum.z, position.z );
            }
        }
        candidate.boundsMinimum = minimum;
        candidate.boundsMaximum = maximum;
        if( !render::validateFoliageSpecies( candidate, error ) )
        {
            return false;
        }
        output = std::move( candidate );
        error.clear();
        return true;
    }

    bool saveFoliagePresetCutout( FoliagePreset preset, const String &projectRoot,
                                  const String &assetDirectory, String &textureId, String &error )
    {
        if( u32( preset ) > u32( FoliagePreset::Palm ) )
        {
            error = "Unknown procedural foliage preset.";
            return false;
        }
        auto pixels = foliagePresetCutout( preset );
        procedural::TextureBuffer rgba( foliageCutoutSize, foliageCutoutSize );
        for( size_t i = 0; i < pixels.size(); i += 4 )
        {
            rgba.pixels[i] = pixels[i + 2];
            rgba.pixels[i + 1] = pixels[i + 1];
            rgba.pixels[i + 2] = pixels[i];
            rgba.pixels[i + 3] = pixels[i + 3];
        }
        const char *names[] = { "oak", "birch", "pine", "bush", "grass", "tall-grass", "palm" };
        return render::saveGeneratedTextureSource( rgba, projectRoot, assetDirectory,
                                                   String( "foliage-" ) + names[u32( preset )],
                                                   render::TextureMipFilter::Cutout, textureId, error );
    }
}  // namespace workphone::scene
