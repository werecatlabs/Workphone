#pragma once

#include "ClawFoliageWorldContracts.hpp"
#include <Workphone/Math/Matrix3.hpp>
#include <array>
#include <map>

namespace claw_foliage_matched_acceptance
{
    using namespace claw_foliage_contracts;
    using Clock = std::chrono::steady_clock;

    struct Placement
    {
        u32 species;
        FoliageCellKey cell;
        Matrix4F transform;
    };

    struct Patch
    {
        std::vector<wp_vertex_pntc> vertices;
        std::array<std::vector<u32>, 2> indices;
    };

    struct MergedPatch
    {
        Geometry geometry;
        std::array<u32, 2> sectionLengths;
    };

    struct Sample
    {
        u64 draws = 0, triangles = 0, uploadBytes = 0;
        double cpuMs = 0;
        std::vector<unsigned char> image;
    };

    inline double milliseconds( Clock::duration duration )
    {
        return std::chrono::duration<double, std::milli>( duration ).count();
    }

    inline wp_mat4f nativeMatrix( const Matrix4F &matrix )
    {
        wp_mat4f result{};
        std::memcpy( result.m, matrix.ptr(), sizeof( result.m ) );
        return result;
    }

    inline std::vector<wp_vertex_pntc> vertices( const FoliageMeshData &mesh )
    {
        std::vector<wp_vertex_pntc> result;
        result.reserve( mesh.positions.size() );
        const auto byte = []( f32 value ) {
            return u32( std::lround( std::clamp( value, 0.f, 1.f ) * 255.f ) );
        };
        for( size_t i = 0; i < mesh.positions.size(); ++i )
        {
            const auto p = mesh.positions[i], n = mesh.normals[i];
            const auto uv = mesh.uvs[i];
            const auto tint = mesh.colours.empty() ? ColourF::White : mesh.colours[i];
            result.push_back( { { p.x, p.y, p.z }, { n.x, n.y, n.z }, { uv.x, uv.y },
                                byte( tint.r ) << 24 | byte( tint.g ) << 16 |
                                    byte( tint.b ) << 8 | byte( tint.a ) } );
        }
        return result;
    }

    inline void setSurface( wp_renderer_dx11 *native, const FoliageMaterial &surface, void *leafView )
    {
        auto settings = material( { surface.tint.r, surface.tint.g, surface.tint.b, surface.tint.a } );
        settings.specular_color = { .04f, .04f, .04f, 0 };
        settings.surface = { surface.metalness, surface.roughness, 1, 1 };
        settings.controls.z = surface.alphaCutoff;
        settings.uv_transform.z = 1;
        settings.texture_sources = { 4, 3, 0, 0 };
        settings.ambient_color = { 1, 1, 1, 1 };
        settings.camera_position.w = 1;
        settings.light_direction = { 0, -1, 0, 0 };
        wp_renderer_dx11_set_material( native, &settings );
        wp_renderer_dx11_set_texture_native( native, surface.albedo.empty() ? nullptr : leafView );
        wp_renderer_dx11_set_cull_mode( native, surface.twoSided ? WORKPHONE_CULL_MODE_NONE
                                                               : WORKPHONE_CULL_MODE_BACK );
    }

    inline bool visiblePixel( const std::vector<unsigned char> &image, size_t p )
    {
        return image[p] || image[p + 1] || image[p + 2];
    }

    inline size_t coverage( const std::vector<unsigned char> &image )
    {
        size_t count = 0;
        for( size_t p = 0; p < image.size(); p += 4 ) count += visiblePixel( image, p );
        return count;
    }

    inline void compareCoverage( const Sample &reference, const Sample &other )
    {
        require( reference.image.size() == other.image.size(), "matched fixture target changed" );
        size_t different = 0;
        for( size_t p = 0; p < reference.image.size(); p += 4 )
            different += visiblePixel( reference.image, p ) != visiblePixel( other.image, p );
        const auto covered = coverage( reference.image );
        require( covered > 100 && different <= std::max<size_t>( 32, covered / 100 ),
                 "matched foliage coverage diverged beyond the raster tolerance" );
    }

    template <class Draw>
    Sample measure( ClawRendererDX11 &renderer, wp_renderer_dx11 *native, Draw draw )
    {
        Sample result;
        std::array<double, 3> timings{};
        for( size_t run = 0; run <= timings.size(); ++run )
        {
            renderer.clear( ColourF::Black );
            const auto before = statistics( native );
            const auto start = Clock::now();
            draw();
            const auto submitted = Clock::now();
            const auto after = statistics( native );
            // Readback drains the frame outside the measured CPU submission interval.
            result.image = pixels( native );
            if( run == 0 ) continue;
            timings[run - 1] = milliseconds( submitted - start );
            result.draws = after.draws - before.draws;
            result.triangles = after.triangles - before.triangles;
            result.uploadBytes = after.instance_upload_bytes - before.instance_upload_bytes;
        }
        std::sort( timings.begin(), timings.end() );
        result.cpuMs = timings[1];
        return result;
    }

    inline void report( const char *path, const Sample &sample, u64 bytes, double setupMs )
    {
        std::printf( "FOLIAGE_MATCHED,%s,%llu,%llu,%llu,%llu,%llu,%.6f,%.6f\n", path,
                     static_cast<unsigned long long>( sample.draws ),
                     static_cast<unsigned long long>( sample.triangles ),
                     static_cast<unsigned long long>( bytes ),
                     static_cast<unsigned long long>( sample.uploadBytes ),
                     static_cast<unsigned long long>( coverage( sample.image ) ),
                     sample.cpuMs, setupMs );
    }

    template <class Camera>
    void run( ClawRendererDX11 &renderer, const FoliageSnapshot &forest, SmartPtr<Camera> camera,
              const SmartPtr<ClawTexture> &texture )
    {
        auto *native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
        renderer.setCamera( camera );
        renderer.disableShadows();
        renderer.setSceneLighting( ColourF::White, { 0, -1, 0 }, ColourF::White, 0 );
        renderer.setSceneFog( 0, ColourF::Black, 0, 0, 1 );
        auto view = camera->configure( 90, { 0, 140, 260 }, { 0, 0, 0 } );
        // Pin the whole population and LOD. The flight fixture exercises automatic
        // culling and LOD independently of this coverage-controlled comparison.
        view.forcedLod = 0;
        view.cull = false;
        const auto viewMatrix = nativeMatrix( Matrix4F( camera->getViewMatrix().ptr() ) );
        const auto projection = nativeMatrix( Matrix4F( camera->getProjectionMatrix().ptr() ) );
        auto *leafView = texture->getNativeTextureView( native );
        require( leafView != nullptr && forest->pageSize == 64 && forest->species.size() == 3,
                 "matched fixture requires the pinned forest materials and 64 m cells" );

        const auto data = std::make_shared<FoliageWorldData>();
        data->pageSize = forest->pageSize;
        data->revision = forest->revision;
        for( u32 i = 0; i < 2; ++i )
        {
            auto tree = std::make_shared<FoliageSpecies>( *forest->species[i] );
            require( tree->kind == FoliageKind::Tree && tree->materials.size() == 2,
                     "matched fixture requires two-section trees" );
            tree->wind = {};
            tree->materials = forest->species[0]->materials;
            data->species.push_back( std::move( tree ) );
        }
        std::vector<Placement> placements;
        String error;
        for( const auto &tree : forest->instances )
        {
            if( tree.species >= data->species.size() ) continue;
            FoliageCellKey cell;
            require( foliageCellForPosition( tree.position, data->pageSize, cell, error ), error.c_str() );
            if( cell.x < -2 || cell.x > 1 || cell.z < -2 || cell.z > 1 ) continue;
            require( tree.tint == ColourF::White, "matched source uses untinted instances" );
            Matrix4F transform;
            transform.makeTransform( tree.position, tree.scale, tree.rotation );
            data->instances.push_back( tree );
            placements.push_back( { tree.species, cell, transform } );
        }

        auto start = Clock::now();
        std::array<std::vector<wp_vertex_pntc>, 2> sourceVertices;
        std::vector<Geometry> meshes;
        u64 sharedBytes = 0;
        for( size_t i = 0; i < data->species.size(); ++i )
        {
            const auto &mesh = data->species[i]->lods[0];
            require( mesh.sections.size() == 2, "matched near mesh must have two material sections" );
            sourceVertices[i] = vertices( mesh );
            const std::vector<u32> indices( mesh.indices.begin(), mesh.indices.end() );
            sharedBytes += sourceVertices[i].size() * sizeof( wp_vertex_pntc ) +
                           indices.size() * sizeof( u32 );
            meshes.push_back( geometry( native, sourceVertices[i], indices ) );
        }
        const auto sharedSetup = milliseconds( Clock::now() - start );

        start = Clock::now();
        std::map<FoliageCellKey, Patch> patches;
        for( const auto &placement : placements )
        {
            auto &patch = patches[placement.cell];
            const auto base = u32( patch.vertices.size() );
            Matrix3F normal;
            placement.transform.inverse().transpose().extract3x3Matrix( normal );
            for( auto vertex : sourceVertices[placement.species] )
            {
                const auto p = placement.transform.transformAffine(
                    Vector3F( vertex.position.x, vertex.position.y, vertex.position.z ) );
                const auto n = ( normal * Vector3F( vertex.normal.x, vertex.normal.y, vertex.normal.z ) )
                                   .normaliseCopy();
                vertex.position = { p.x, p.y, p.z };
                vertex.normal = { n.x, n.y, n.z };
                patch.vertices.push_back( vertex );
            }
            const auto &mesh = data->species[placement.species]->lods[0];
            for( const auto &section : mesh.sections )
            {
                for( u32 i = 0; i < section.indexCount; ++i )
                    patch.indices[section.material].push_back( base + mesh.indices[section.firstIndex + i] );
            }
        }
        std::vector<MergedPatch> merged;
        u64 mergedBytes = 0;
        for( auto &entry : patches )
        {
            auto &patch = entry.second;
            auto &indices = patch.indices[0];
            const std::array<u32, 2> lengths{ u32( indices.size() ), u32( patch.indices[1].size() ) };
            indices.insert( indices.end(), patch.indices[1].begin(), patch.indices[1].end() );
            mergedBytes += patch.vertices.size() * sizeof( wp_vertex_pntc ) +
                           indices.size() * sizeof( u32 );
            merged.push_back( { geometry( native, patch.vertices, indices ), lengths } );
        }
        const auto mergedSetup = milliseconds( Clock::now() - start );
        require( placements.size() == 512 && merged.size() == 16,
                 "matched fixture population and patch partition must stay pinned" );
        patches.clear();

        setup( native );
        wp_renderer_dx11_set_view_matrix( native, &viewMatrix );
        wp_renderer_dx11_set_projection_matrix( native, &projection );
        wp_renderer_dx11_set_depth_test_enabled( native, 1 );
        wp_renderer_dx11_set_depth_write_enabled( native, 1 );
        wp_renderer_dx11_set_depth_func( native, WORKPHONE_DEPTH_FUNC_LEQUAL );
        wp_renderer_dx11_set_material_sampler( native, 0, 0, 2, 1 );
        const auto &surfaces = data->species[0]->materials;
        const auto scalar = measure( renderer, native, [&] {
            for( const auto &placement : placements )
            {
                const auto world = nativeMatrix( placement.transform );
                wp_renderer_dx11_set_world_matrix( native, &world );
                for( const auto &section : data->species[placement.species]->lods[0].sections )
                {
                    setSurface( native, surfaces[section.material], leafView );
                    wp_renderer_dx11_draw_geometry_pntc( native, meshes[placement.species].get(),
                        s32( section.firstIndex ), s32( section.indexCount ), 0 );
                }
            }
        } );
        const auto unit = identity();
        wp_renderer_dx11_set_world_matrix( native, &unit );
        const auto staticPatches = measure( renderer, native, [&] {
            for( const auto &patch : merged )
            {
                u32 first = 0;
                for( u32 section = 0; section < patch.sectionLengths.size(); ++section )
                {
                    setSurface( native, surfaces[section], leafView );
                    wp_renderer_dx11_draw_geometry_pntc( native, patch.geometry.get(), s32( first ),
                        s32( patch.sectionLengths[section] ), 0 );
                    first += patch.sectionLengths[section];
                }
            }
        } );
        wp_renderer_dx11_set_texture_native( native, nullptr );

        start = Clock::now();
        ClawFoliageWorld world;
        require( world.setSource( data, [&]( const String &, String & ) -> SmartPtr<ITexture> {
            return texture;
        }, error ), error.c_str() );
        claw_foliage_world_contracts::prepare( world, renderer, view, data->revision );
        const auto pagedSetup = milliseconds( Clock::now() - start );
        const auto paged = measure( renderer, native, [&] {
            require( world.render( renderer, view, error ), error.c_str() );
        } );
        const auto stats = world.diagnostics();
        std::printf( "FOLIAGE_MATCHED_FIXTURE,%llu,%llu\n",
                     static_cast<unsigned long long>( placements.size() ),
                     static_cast<unsigned long long>( merged.size() ) );
        report( "scalar", scalar, sharedBytes, sharedSetup );
        report( "merged64", staticPatches, mergedBytes, mergedSetup );
        report( "paged", paged, stats.geometryBytes, pagedSetup );
        require( scalar.draws == 1024 && staticPatches.draws == 32 && paged.draws == 4 &&
                     stats.predictedDraws == paged.draws && stats.residentInstances == placements.size(),
                 "matched fixture draw counts or residency differ from the fixed populations" );
        require( scalar.triangles == staticPatches.triangles && scalar.triangles == paged.triangles &&
                     scalar.uploadBytes == 0 && staticPatches.uploadBytes == 0 &&
                     paged.uploadBytes == placements.size() * 2 * 160 && stats.geometryBytes < mergedBytes,
                 "matched fixture must submit identical triangles and account for storage" );
        compareCoverage( scalar, staticPatches );
        compareCoverage( scalar, paged );
        setup( native );
    }
}
