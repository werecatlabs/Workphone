#ifndef WPFoliageImpostorContracts_h__
#define WPFoliageImpostorContracts_h__
#include <Workphone/Graphics/FoliageImpostorBaker.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
inline void runFoliageImpostorContracts()
{
    using namespace workphone;
    using namespace workphone::render;
    const auto require = []( bool value, const char *message ) { if( !value ) throw std::runtime_error( message ); };
    FoliageSpecies species; species.id = "fixture/impostor"; species.boundsMinimum = { -1, 0, -.01f }; species.boundsMaximum = { 1, 2, .01f };
    FoliageMaterial material; material.albedo = "leaf"; material.normal = "normal"; material.orm = "orm"; material.roughness = .5f; material.metalness = .8f; material.tint = { .5f, .5f, .5f, 1 };
    species.materials = { material };
    for( auto &mesh : species.lods )
    {
        mesh.positions = { { -1, 0, 0 }, { 1, 0, 0 }, { 1, 2, 0 }, { -1, 2, 0 } };
        mesh.normals = { { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 } };
        mesh.uvs = { { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 } }; mesh.indices = { 0, 1, 2, 0, 2, 3 }; mesh.sections = { { 0, 6, 0 } };
    }
    auto albedo = std::make_shared<TextureMipLevel>(); albedo->width = 2; albedo->height = 1; albedo->bgra = { 10, 30, 230, 255, 0, 255, 0, 0 };
    auto normal = std::make_shared<TextureMipLevel>(); normal->width = normal->height = 1; normal->bgra = { 230, 128, 210, 255 };
    auto orm = std::make_shared<TextureMipLevel>(); orm->width = orm->height = 1; orm->bgra = { 192, 64, 128, 255 };
    std::map<String, std::shared_ptr<const TextureMipLevel>> images{ { "leaf", albedo }, { "normal", normal }, { "orm", orm } };
    FoliageBakeTextureResolver resolver = [&]( const String &id, String & ) { return images[id]; };
    FoliageImpostorBakeOptions options; options.tileSize = 32; options.gutter = 3; options.directions = { { 0, 0, 1 }, { 0, 0, -1 } }; options.resourcePrefix = "data://impostor/leaf";
    FoliageImpostorBake baked; String error;
    require( bakeFoliageImpostor( species, resolver, options, baked, error ), error.c_str() );
    require( baked.metadata.relightable && baked.metadata.views.size() == 2 && baked.albedo.size() > 1 && baked.normal.size() == baked.albedo.size(), "Impostor bake emits relightable multi-view atlas mip chains." );
    require( baked.radius > 1.4f && baked.centre == Vector3F( 0, 1, 0 ), "Capture and runtime share the enclosing-sphere square." );
    const auto &base = baked.albedo.front(); u32 covered = 0, neutral = 0, maximumRed = 0;
    for( u32 y = 0; y < 32; ++y ) for( u32 x = 0; x < 64; ++x )
    {
        const auto index = ( size_t( y ) * 64 + x ) * 4;
        if( x % 32 < 3 || x % 32 >= 29 || y < 3 || y >= 29 ) require( base.bgra[index + 3] == 0, "Gutters do not invent alpha coverage." );
        if( base.bgra[index + 3] )
        {
            ++covered; maximumRed = std::max( maximumRed, u32( base.bgra[index + 2] ) ); require( base.bgra[index + 2] > base.bgra[index], "Baked albedo samples the authored leaf image, not a fixed colour." );
            require( baked.orm[0].bgra[index] >= 152 && baked.orm[0].bgra[index] <= 155 && baked.orm[0].bgra[index + 1] >= 31 && baked.orm[0].bgra[index + 1] <= 33 && baked.orm[0].bgra[index + 2] == 128,
                "Baked ORM retains AO and folds roughness/metallic factors." );
            neutral += baked.normal[0].bgra[index + 2] == 128 && baked.normal[0].bgra[index] == 255;
            require( baked.depth[0].bgra[index] >= 127 && baked.depth[0].bgra[index] <= 128, "Depth encodes normalized capture distance." );
        }
    }
    require( covered > 150 && covered < 600, "Textured alpha preserves a partial leaf silhouette in both views." );
    require( maximumRed >= 90 && maximumRed <= 116, "Nonwhite tint matches the native sRGB material multiplication convention." );
    require( neutral < covered, "Source normal map contributes to the relightable normal atlas." );
    for( const auto &mip : baked.albedo ) require( mip.width % 2 == 0 && mip.width >= 2, "Mip chains stop before neighbouring captures merge." );
    auto retained = baked.albedo[0].bgra; std::atomic<bool> cancelled{ true };
    require( !bakeFoliageImpostor( species, resolver, options, baked, error, &cancelled ) && baked.albedo[0].bgra == retained, "Cancelled bake retains prior output." );
    options.maximumRasterSamples = 1;
    require( !bakeFoliageImpostor( species, resolver, options, baked, error ) && baked.albedo[0].bgra == retained, "Bounded raster failure retains prior output." );
    const auto parent = std::filesystem::temp_directory_path();
    const auto folder = parent / ( std::string( "workphone_foliage_bake_" ) + StringUtil::getUUID().c_str() );
    String speciesPath;
    try
    {
        require( saveFoliageImpostorSource( species, baked, folder.u8string().c_str(), speciesPath, error ), error.c_str() );
        require( std::filesystem::exists( folder / "leaf_albedo.tga" ) && std::filesystem::exists( folder / "leaf_depth.texres" ) && std::filesystem::exists( folder / "species.folspec" ), "Export writes actual image sources, typed descriptors and species metadata." );
        require( !saveFoliageImpostorSource( species, baked, folder.u8string().c_str(), speciesPath, error ), "Export must not overwrite an existing asset set piecemeal." );
    }
    catch( ... ) { if( folder.parent_path() == parent ) { std::error_code ignored; std::filesystem::remove_all( folder, ignored ); } throw; }
    if( folder.parent_path() == parent ) { std::error_code ignored; std::filesystem::remove_all( folder, ignored ); }
}
#endif
