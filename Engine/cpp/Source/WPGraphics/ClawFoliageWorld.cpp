#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawFoliageWorld.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <WPGraphics/Resources/FoliageSourceTexture.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/ITextureManager.hpp>
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <set>
#include <thread>

namespace workphone::render
{
    namespace
    {
        using Microsoft::WRL::ComPtr;

        Vector3F point( const Matrix4F &m, const Vector3F &p )
        {
            return { m[0][0] * p.x + m[0][1] * p.y + m[0][2] * p.z + m[0][3],
                     m[1][0] * p.x + m[1][1] * p.y + m[1][2] * p.z + m[1][3],
                     m[2][0] * p.x + m[2][1] * p.y + m[2][2] * p.z + m[2][3] };
        }

        Vector3F vector( const Matrix4F &m, const Vector3F &p )
        {
            return { m[0][0] * p.x + m[0][1] * p.y + m[0][2] * p.z,
                     m[1][0] * p.x + m[1][1] * p.y + m[1][2] * p.z,
                     m[2][0] * p.x + m[2][1] * p.y + m[2][2] * p.z };
        }

        bool finite( const Vector3F &p )
        {
            return std::isfinite( p.x ) && std::isfinite( p.y ) && std::isfinite( p.z );
        }

        bool affine( const Matrix4F &m )
        {
            wp_instance_pntc_dx11 instance{};
            std::memcpy( instance.transform.m, m.ptr(), sizeof( instance.transform ) );
            instance.tint = { 1, 1, 1, 1 };
            return wp_renderer_dx11_validate_instances_pntc( &instance, 1 ) != 0;
        }

        f32 maximumScale( const Matrix4F &m )
        {
            // Induced norm bound remains conservative under shear and is exact for identity.
            double rows = 0, columns = 0;
            for( int y = 0; y < 3; ++y )
            {
                double r = 0, c = 0;
                for( int x = 0; x < 3; ++x )
                {
                    r += std::abs( double( m[y][x] ) );
                    c += std::abs( double( m[x][y] ) );
                }
                rows = std::max( rows, r );
                columns = std::max( columns, c );
            }
            return f32( std::sqrt( rows * columns ) );
        }

        f32 deformedRadius( const FoliageSpecies &species, const Matrix4F &world )
        {
            const auto scale = maximumScale( world );
            const auto inverse = world.inverse();
            // The shader projects world-space wind and interactions into the
            // local XZ bend plane. M * projectionXZ * inverse(M) need not have
            // unit norm for rotated, nonuniformly scaled or sheared instances.
            const auto x = vector( world, { inverse[0][0], 0, inverse[2][0] } );
            const auto z = vector( world, { inverse[0][2], 0, inverse[2][2] } );
            const auto wind = species.wind.amplitude * std::max( 1.0f, x.length() );
            const auto interaction =
                species.wind.maximumInteractionBend *
                std::max( 1.0f, std::sqrt( x.lengthSquared() + z.lengthSquared() ) );
            // A far card has an orthogonal basis, so the unit lower bounds
            // above also cover every camera-facing impostor orientation.
            return ( ( species.boundsMaximum - species.boundsMinimum ).length() * .5f +
                     species.wind.flutter * 1.415f ) *
                       scale +
                   wind + interaction;
        }

        bool visible( const ClawFoliageView &view, const Vector3F &centre, f32 radius )
        {
            if( !view.cull )
            {
                return true;
            }
            for( int axis = 0; axis < 3; ++axis )
            {
                for( int sign : { -1, 1 } )
                {
                    const auto &m = view.worldToClip;
                    const double a = m[3][0] + sign * m[axis][0], b = m[3][1] + sign * m[axis][1],
                                 c = m[3][2] + sign * m[axis][2];
                    const double d = m[3][3] + sign * m[axis][3];
                    if( a * centre.x + b * centre.y + c * centre.z + d <
                        -double( radius ) * std::sqrt( a * a + b * b + c * c ) )
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        f32 pagingRange( const FoliageSnapshot &source, const Matrix4F &placement, f32 geometryRadius,
                         f32 bendRadius )
        {
            f32 distance = 0;
            for( const auto &species : source->species )
            {
                distance =
                    std::max( distance, std::max( species->maximumDistance, species->shadowDistance ) );
            }
            const auto inverseScale = maximumScale( placement.inverse() );
            // Includes off-centre/scaled geometry, every billboard orientation and
            // world-space bend projected through nonuniform placement.
            return ( distance + bendRadius ) * inverseScale +
                   geometryRadius * maximumScale( placement ) * inverseScale;
        }

        bool residencyRange( const FoliageSnapshot &source, const Matrix4F &placement,
                             f32 geometryRadius, f32 bendRadius, String &error )
        {
            if( !source )
            {
                return true;
            }
            const auto range = pagingRange( source, placement, geometryRadius, bendRadius );
            if( !std::isfinite( range ) || !std::isfinite( source->pageSize ) || source->pageSize <= 0 ||
                range / source->pageSize + 1 > 30 )
            {
                error =
                    "Foliage visibility range exceeds the bounded native pager (30 local page widths); "
                    "increase page size or reduce range.";
                return false;
            }
            return true;
        }

        SmartPtr<ITexture> resolve( const String &id, wp_renderer_dx11 *renderer, String &error )
        {
            auto app = core::IApplicationManager::instancePtr();
            if( id.size() >= 7 && id.substr( id.size() - 7 ) == ".texres" )
            {
#if WP_FOLIAGE_SOURCE_RESOURCES
                return loadFoliageSourceTexture( app ? app->getProjectPath() : String(), id, renderer,
                                                 error );
#else
                error = "Authored foliage texture descriptors require the resource system.";
                return {};
#endif
            }
            // Runtime procedural textures have manager-owned names rather than catalog UUIDs.
            // Resolve the existing resource first; never reload a generated texture from disk.
            auto graphics = app ? app->getGraphicsSystem() : nullptr;
            auto manager = graphics ? graphics->getTextureManager() : nullptr;
            auto texture = manager ? dynamic_pointer_cast<ITexture>( manager->getByName( id ) ) : nullptr;
            auto database = app ? app->getResourceDatabase() : nullptr;
            if( !texture && database )
                texture = database->loadResourceByType<ITexture>( id );
            if( !texture )
            {
                error = "Foliage texture is unavailable: " + id;
            }
            return texture;
        }

        struct Geometry
        {
            StreamBudget budget;
            wp_geometry_dx11 *native = nullptr;
            u64 bytes = 0;

            ~Geometry()
            {
                if( native )
                {
                    wp_renderer_dx11_destroy_geometry( native );
                }
            }
        };

        struct Material
        {
            FoliageMaterial data;
            std::array<SmartPtr<ITexture>, 3> textures;
            std::array<ComPtr<ID3D11ShaderResourceView>, 3> views;
        };

        struct Species
        {
            std::array<std::shared_ptr<Geometry>, 3> meshes;
            Array<Material> materials;
            Material impostor;
            bool materialsReady = false;
        };

        struct Assets
        {
            ClawFoliageWorld::TextureResolver resolver;
            Array<std::shared_ptr<const FoliageSpecies>> descriptors;
            ComPtr<ID3D11Device> device;
            Array<Species> species;
            std::map<String, SmartPtr<ITexture>> resolved;
            u64 bytes = 0;
        };

        bool material( Material &out, const FoliageMaterial &data, Assets &assets,
                       wp_renderer_dx11 *renderer, String &error )
        {
            out.data = data;
            const String *names[] = { &data.albedo, &data.normal, &data.orm };
            for( size_t i = 0; i < 3; ++i )
            {
                if( names[i]->empty() )
                {
                    continue;
                }
                auto found = assets.resolved.find( *names[i] );
                auto texture = found != assets.resolved.end()
                                   ? found->second
                                   : ( assets.resolver ? assets.resolver( *names[i], error )
                                                       : resolve( *names[i], renderer, error ) );
                void *view = nullptr;
                if( auto claw = dynamic_pointer_cast<ClawTexture>( texture ) )
                {
                    view = claw->getNativeTextureView( renderer );
                }
                else if( texture )
                {
                    texture->getTextureFinal( &view );
                }
                if( !view )
                {
                    if( error.empty() )
                    {
                        error = "Foliage texture has no GPU view: " + *names[i];
                    }
                    return false;
                }
                out.textures[i] = texture;
                out.views[i] = static_cast<ID3D11ShaderResourceView *>( view );
                ComPtr<ID3D11Device> device;
                out.views[i]->GetDevice( &device );
                D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
                out.views[i]->GetDesc( &desc );
                if( device.Get() != assets.device.Get() ||
                    desc.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE2D )
                {
                    error = "Foliage requires 2D material maps from the active device.";
                    return false;
                }
                assets.resolved[*names[i]] = texture;
            }
            return true;
        }

        FoliageMeshData billboard( const FoliageSpecies &species )
        {
            FoliageMeshData mesh;
            const f32 radius = ( species.boundsMaximum - species.boundsMinimum ).length() * .5f;
            // Matches the baker's common enclosing-sphere projection for every elevation.
            mesh.positions = { { -radius, -radius, 0 },
                               { radius, -radius, 0 },
                               { radius, radius, 0 },
                               { -radius, radius, 0 } };
            mesh.normals.assign( 4, { 0, 0, 1 } );
            mesh.uvs = { { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 } };
            mesh.indices = { 0, 1, 2, 0, 2, 3 };
            mesh.sections = { { 0, 6, 0 } };
            return mesh;
        }

        std::shared_ptr<Geometry> geometry( wp_renderer_dx11 *renderer, const FoliageMeshData &source,
                                            String &error )
        {
            auto work = WorldStreamingScheduler::shared()->reserve(
                StreamMemoryClass::WorkingCpu, source.positions.size() * sizeof( wp_vertex_pntc ) + 256,
                error );
            if( !work )
            {
                return {};
            }
            auto result = std::make_shared<Geometry>();
            result->bytes = source.positions.size() * sizeof( wp_vertex_pntc ) +
                            source.indices.size() * sizeof( u32 );
            result->budget = WorldStreamingScheduler::shared()->reserve( StreamMemoryClass::Gpu,
                                                                         result->bytes, error );
            if( !result->budget )
            {
                return {};
            }
            Array<wp_vertex_pntc> vertices;
            vertices.reserve( source.positions.size() );
            for( size_t i = 0; i < source.positions.size(); ++i )
            {
                const auto p = source.positions[i], n = source.normals[i];
                const auto uv = source.uvs[i];
                const auto colour = source.colours.empty() ? ColourF::White : source.colours[i];
                const auto byte = []( f32 v ) {
                    return u32( std::lround( std::clamp( v, 0.0f, 1.0f ) * 255 ) );
                };
                vertices.push_back( { { p.x, p.y, p.z },
                                      { n.x, n.y, n.z },
                                      { uv.x, uv.y },
                                      ( byte( colour.r ) << 24 ) | ( byte( colour.g ) << 16 ) |
                                          ( byte( colour.b ) << 8 ) | byte( colour.a ) } );
            }
            result->native = wp_renderer_dx11_create_indexed_geometry_pntc(
                renderer, vertices.data(), s32( vertices.size() ), source.indices.data(),
                s32( source.indices.size() ), 1 );
            if( !result->native )
            {
                error = "Foliage immutable mesh upload failed; active generation retained.";
                return {};
            }
            return result;
        }
    }  // namespace

    struct ClawFoliageWorld::Impl
    {
        std::thread::id owner;
        FoliagePageStream stream;
        FoliageSnapshot requested, active;
        u64 requestedEpoch = 0, activeEpoch = 0;
        TextureResolver resolver;
        PublicationGate publicationGate;
        std::map<FoliageCellKey, std::shared_ptr<const FoliagePageData>> pages, staged;

        struct Bounds
        {
            Vector3F centre;
            f32 radius = 0;
        };

        std::map<FoliageCellKey, Bounds> pageBounds;
        std::shared_ptr<Assets> assets, candidate;
        bool refreshMaterials = true;
        Matrix4F placement = Matrix4F::identity();
        Matrix4F activePlacement = Matrix4F::identity();
        Array<ClawFoliageInteraction> interactions;

        struct History
        {
            StreamBudget budget;
            Array<std::pair<u64, s32>> entries;
        };

        std::map<u64, History> history;
        u64 uploadFrame = 0;
        u32 frameUploads = 0;
        f32 seconds = 0;
        u32 mask = ~u32( 0 );
        String error;
        ClawFoliageDiagnostics diagnostics;

        bool check( String &message )
        {
            if( owner == std::thread::id{} )
            {
                owner = std::this_thread::get_id();
            }
            if( owner == std::this_thread::get_id() )
            {
                return true;
            }
            message = "Foliage world requires its owning render thread.";
            return false;
        }
    };

    ClawFoliageWorld::ClawFoliageWorld() : m_impl( std::make_unique<Impl>() )
    {
    }

    ClawFoliageWorld::~ClawFoliageWorld()
    {
        clear();
    }

    bool ClawFoliageWorld::setSource( FoliageSnapshot source, TextureResolver resolver, String &error )
    {
        if( !m_impl->check( error ) )
        {
            return false;
        }
        if( !source )
        {
            clear();
            error.clear();
            return true;
        }
        f32 geometry = 0, bend = 0;
        if( !validateFoliageWorld( *source, error ) ||
            !foliageInstanceRadii( *source, source->instances, geometry, bend, error ) ||
            !residencyRange( source, m_impl->placement, geometry, bend, error ) )
        {
            return false;
        }
        if( !m_impl->stream.setSource( source, error ) )
        {
            return false;
        }
        m_impl->requested = m_impl->stream.source();
        m_impl->resolver = std::move( resolver );
        ++m_impl->requestedEpoch;
        m_impl->candidate.reset();
        m_impl->refreshMaterials = true;
        m_impl->staged.clear();
        m_impl->diagnostics.currentGenerationReady = false;
        m_impl->error.clear();
        return true;
    }

    bool ClawFoliageWorld::setSource( FoliageStreamSource source, TextureResolver resolver,
                                      String &error )
    {
        if( !m_impl->check( error ) )
        {
            return false;
        }
        if( !source.snapshot || !validateFoliageWorld( *source.snapshot, error ) )
        {
            return false;
        }
        if( source.maximumGeometryRadius < 0 || source.maximumBendRadius < 0 )
        {
            if( source.snapshot->instances.empty() && !source.cells.empty() )
            {
                error = "Lazy foliage sources require conservative instance extent metadata";
                return false;
            }
            if( !foliageInstanceRadii( *source.snapshot, source.snapshot->instances,
                                       source.maximumGeometryRadius, source.maximumBendRadius, error ) )
            {
                return false;
            }
        }
        if( !residencyRange( source.snapshot, m_impl->placement, source.maximumGeometryRadius,
                             source.maximumBendRadius, error ) )
        {
            return false;
        }
        if( !m_impl->stream.setSource( std::move( source ), error ) )
        {
            return false;
        }
        m_impl->requested = m_impl->stream.source();
        m_impl->resolver = std::move( resolver );
        ++m_impl->requestedEpoch;
        m_impl->candidate.reset();
        m_impl->refreshMaterials = true;
        m_impl->staged.clear();
        m_impl->diagnostics.currentGenerationReady = false;
        m_impl->error.clear();
        return true;
    }

    bool ClawFoliageWorld::setPlacement( const Matrix4F &value, String &error )
    {
        if( !m_impl->check( error ) || !affine( value ) )
        {
            if( error.empty() )
            {
                error = "Invalid foliage world transform.";
            }
            return false;
        }
        if( !residencyRange( m_impl->requested, value, m_impl->stream.maximumGeometryRadius(),
                             m_impl->stream.maximumBendRadius(), error ) )
        {
            return false;
        }
        m_impl->placement = value;
        error.clear();
        return true;
    }

    bool ClawFoliageWorld::setInteractions( const Array<ClawFoliageInteraction> &values, String &error )
    {
        if( !m_impl->check( error ) )
        {
            return false;
        }
        if( values.size() > 16 )
        {
            error = "Foliage supports at most sixteen local interaction spheres.";
            return false;
        }
        for( const auto &v : values )
        {
            if( !finite( v.position ) || !std::isfinite( v.radius ) || v.radius <= 0 ||
                !std::isfinite( v.strength ) || v.strength < 0 || v.strength > 1 )
            {
                error = "Invalid foliage interaction.";
                return false;
            }
        }
        m_impl->interactions = values;
        error.clear();
        return true;
    }

    void ClawFoliageWorld::setVisibilityMask( u32 mask )
    {
        m_impl->mask = mask;
    }

    bool ClawFoliageWorld::setPublicationGate( PublicationGate gate, String &error )
    {
        if( !m_impl->check( error ) )
        {
            return false;
        }
        m_impl->publicationGate = std::move( gate );
        error.clear();
        return true;
    }
    bool ClawFoliageWorld::prepare( ClawRendererDX11 &renderer, const Array<ClawFoliageView> &views,
                                    f32 seconds, String &error )
    try
    {
        auto &p = *m_impl;
        error.clear();
        if( !p.check( error ) )
        {
            return false;
        }
        const auto fail = [&]() {
            p.error = error;
            p.diagnostics.currentGenerationReady = false;
            return false;
        };
        if( !p.requested || views.empty() || views.size() > 16 || !std::isfinite( seconds ) )
        {
            error = "Foliage preparation requires a source and one to sixteen finite views.";
            return fail();
        }
        auto native =
            renderer.isLoaded() ? wp_renderer_get_dx11( renderer.getNativeRenderer() ) : nullptr;
        auto device =
            native ? static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( native ) ) : nullptr;
        if( !device || FAILED( device->GetDeviceRemovedReason() ) )
        {
            error = "Foliage device is unavailable.";
            return fail();
        }
        const auto inverse = p.placement.inverse();
        const auto range = pagingRange( p.requested, p.placement, p.stream.maximumGeometryRadius(),
                                        p.stream.maximumBendRadius() );
        Array<FoliagePageView> pageViews;
        for( const auto &view : views )
        {
            if( !finite( view.position ) )
            {
                error = "Nonfinite foliage view position.";
                return fail();
            }
            pageViews.push_back( { point( inverse, view.position ), range } );
        }
        if( !( range > 0 ? p.stream.updateViews( pageViews, error ) : p.stream.setDemand( {}, error ) ) )
        {
            // Obsolete ready/staged results must not pin admission permanently.
            // Active pages remain intact even when replacement needs twice the budget.
            p.staged.clear();
            FoliagePageResult retired;
            while( p.stream.take( retired ) )
            {
                retired = {};
            }
            return fail();
        }
        const auto &demand = p.stream.demand();
        const std::set<FoliageCellKey> needed( demand.begin(), demand.end() );
        for( auto i = p.staged.begin(); i != p.staged.end(); )
        {
            if( !needed.count( i->first ) )
            {
                i = p.staged.erase( i );
            }
            else
            {
                ++i;
            }
        }
        if( p.activeEpoch == p.requestedEpoch )
        {
            for( const auto &entry : p.pages )
            {
                if( needed.count( entry.first ) )
                {
                    p.staged.emplace( entry );
                }
            }
        }
        FoliagePageResult result;
        while( p.stream.take( result ) )
        {
            if( result.source.get() != p.requested.get() || !needed.count( result.cell ) )
            {
                continue;
            }
            if( !result.page )
            {
                ++p.diagnostics.failedPages;
                error = result.error;
                continue;
            }
            p.staged[result.cell] = std::move( result.page );
        }
        if( !error.empty() )
        {
            return fail();
        }
        p.seconds = seconds;
        if( !p.candidate )
        {
            if( p.assets && p.assets->device.Get() == device &&
                p.assets->descriptors == p.requested->species )
            {
                if( !p.refreshMaterials )
                {
                    p.candidate = p.assets;
                }
                else
                {
                    p.candidate = std::make_shared<Assets>( *p.assets );
                    p.candidate->resolver = p.resolver;
                    p.candidate->resolved.clear();
                    for( auto &species : p.candidate->species )
                    {
                        species.materials.clear();
                        species.impostor = {};
                        species.materialsReady = false;
                    }
                }
            }
            else
            {
                p.candidate = std::make_shared<Assets>();
                p.candidate->device = device;
                p.candidate->descriptors = p.requested->species;
                p.candidate->resolver = p.resolver;
                p.candidate->species.resize( p.requested->species.size() );
            }
        }
        if( p.candidate->device.Get() != device )
        {
            p.candidate.reset();
            error = "Foliage device changed during preparation.";
            return fail();
        }
        const auto frame = renderer.terrainFrameToken();
        if( !frame || frame != p.uploadFrame )
        {
            p.uploadFrame = frame;
            p.frameUploads = 0;
        }
        bool assetsReady = true;
        for( size_t i = 0; i < p.requested->species.size(); ++i )
        {
            auto &gpu = p.candidate->species[i];
            const auto &species = *p.requested->species[i];
            if( !gpu.materialsReady )
            {
                gpu.materials.resize( species.materials.size() );
                for( size_t j = 0; j < species.materials.size(); ++j )
                {
                    if( !material( gpu.materials[j], species.materials[j], *p.candidate, native,
                                   error ) )
                    {
                        return fail();
                    }
                }
                if( species.farBillboard )
                {
                    FoliageMaterial atlas;
                    atlas.albedo = species.impostor.albedo;
                    atlas.normal = species.impostor.normal;
                    atlas.orm = species.impostor.orm;
                    atlas.metalness = atlas.orm.empty() ? 0.0f : 1.0f;
                    if( !material( gpu.impostor, atlas, *p.candidate, native, error ) )
                    {
                        return fail();
                    }
                }
                gpu.materialsReady = true;
            }
            for( size_t lod = 0; lod < 3; ++lod )
            {
                if( !gpu.meshes[lod] )
                {
                    if( p.frameUploads >= 4 )
                    {
                        assetsReady = false;
                        continue;
                    }
                    gpu.meshes[lod] = geometry(
                        native,
                        lod == 2 && species.farBillboard ? billboard( species ) : species.lods[lod],
                        error );
                    if( !gpu.meshes[lod] )
                    {
                        return fail();
                    }
                    p.candidate->bytes += gpu.meshes[lod]->bytes;
                    ++p.frameUploads;
                }
            }
        }
        p.diagnostics.pendingPages = u32( needed.size() - p.staged.size() );
        p.diagnostics.currentGenerationReady = assetsReady && p.staged.size() == needed.size();
        if( assetsReady && p.staged.size() == needed.size() )
        {
            const bool changed =
                p.activeEpoch != p.requestedEpoch || p.pages != p.staged ||
                std::memcmp( p.activePlacement.ptr(), p.placement.ptr(), 16 * sizeof( f32 ) ) != 0;
            std::map<FoliageCellKey, Impl::Bounds> candidateBounds;
            if( changed )
            {
                for( const auto &entry : p.staged )
                {
                    if( entry.second->instances.empty() )
                    {
                        candidateBounds[entry.first] = {};
                        continue;
                    }
                    Vector3F lo( 1e30f, 1e30f, 1e30f ), hi( -1e30f, -1e30f, -1e30f );
                    for( const auto &instance : entry.second->instances )
                    {
                        const auto &species = *p.requested->species[instance.species];
                        Matrix4F local;
                        local.makeTransform( instance.position, instance.scale, instance.rotation );
                        const auto transform = p.placement * local;
                        if( !affine( transform ) )
                        {
                            error =
                                "Foliage instance transform exceeds native DX11 limits; active "
                                "generation retained.";
                            return fail();
                        }
                        const auto centre =
                            point( transform, ( species.boundsMinimum + species.boundsMaximum ) * .5f );
                        const auto radius = deformedRadius( species, transform );
                        for( int axis = 0; axis < 3; ++axis )
                        {
                            lo[axis] = std::min( lo[axis], centre[axis] - radius );
                            hi[axis] = std::max( hi[axis], centre[axis] + radius );
                        }
                    }
                    candidateBounds[entry.first] = { ( lo + hi ) * .5f, ( hi - lo ).length() * .5f };
                }
            }
            auto publicationSource = p.requested;
            decltype( p.history ) retiredHistory;
            bool committed = false;
            const std::function<void()> commit = [&]() noexcept {
                if( committed )
                {
                    return;
                }
                if( changed )
                {
                    p.pageBounds.swap( candidateBounds );
                }
                p.pages.swap( p.staged );
                p.assets.swap( p.candidate );
                p.active.swap( publicationSource );
                if( p.activeEpoch != p.requestedEpoch )
                {
                    p.history.swap( retiredHistory );
                }
                p.activeEpoch = p.requestedEpoch;
                p.activePlacement = p.placement;
                p.refreshMaterials = false;
                committed = true;
            };
            if( p.publicationGate )
            {
                if( !p.publicationGate( p.requested, commit, error ) )
                {
                    return fail();
                }
                if( !committed )
                {
                    error = "Foliage publication gate did not commit its prepared generation.";
                    return fail();
                }
            }
            else
            {
                commit();
            }
            // The old GPU/page/source/history owners retire after the external
            // source/physics gate has released its locks.
            p.staged.clear();
            p.candidate.reset();
            p.diagnostics.sourceRevision = p.active->revision;
            p.diagnostics.residentPages = u32( p.pages.size() );
            p.diagnostics.residentInstances = 0;
            for( const auto &entry : p.pages )
            {
                p.diagnostics.residentInstances += entry.second->instances.size();
            }
            p.diagnostics.geometryBytes = p.assets->bytes;
        }
        p.error.clear();
        return true;
    }
    catch( const std::exception &exception )
    {
        error = exception.what();
        m_impl->error = error;
        m_impl->diagnostics.currentGenerationReady = false;
        return false;
    }
    bool ClawFoliageWorld::render( ClawRendererDX11 &renderer, const ClawFoliageView &view,
                                   String &error )
    try
    {
        auto &p = *m_impl;
        error.clear();
        if( !p.check( error ) )
        {
            return false;
        }
        auto &stats = p.diagnostics;
        stats.viewKey = view.key;
        stats.testedInstances = stats.culledInstances = 0;
        stats.lodInstances = {};
        stats.groups = stats.predictedDraws = stats.actualDraws = stats.submittedInstances =
            stats.uploadBytes = 0;
        if( !p.active || !p.assets || !( view.visibilityMask & p.mask ) )
        {
            return true;
        }
        if( !finite( view.position ) || !std::isfinite( view.projectionScale ) ||
            view.projectionScale <= 0 || !std::isfinite( view.lodBias ) || view.lodBias <= 0 ||
            view.forcedLod < -1 || view.forcedLod > 2 )
        {
            error = "Invalid foliage view.";
            p.error = error;
            return false;
        }
        for( int row = 0; row < 4; ++row )
        {
            for( int col = 0; col < 4; ++col )
            {
                if( !std::isfinite( view.worldToClip[row][col] ) )
                {
                    error = "Nonfinite foliage frustum.";
                    p.error = error;
                    return false;
                }
            }
        }
        auto native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
        if( !native || wp_renderer_dx11_get_device( native ) != p.assets->device.Get() )
        {
            error = "Foliage publication belongs to another device.";
            p.error = error;
            return false;
        }
        if( !p.history.count( view.key ) && p.history.size() >= 16 )
        {
            p.history.erase( p.history.begin() );
        }
        constexpr u64 maximumVisible = 262144;
        const auto admitted = std::min<u64>( maximumVisible, p.diagnostics.residentInstances );
        auto workBudget = WorldStreamingScheduler::shared()->reserve(
            StreamMemoryClass::WorkingCpu,
            admitted * ( 2 * sizeof( wp_foliage_instance_dx11 ) + 32 ) + 65536, error );
        if( !workBudget )
        {
            p.error = error;
            return false;
        }
        Impl::History nextHistory;
        nextHistory.budget = WorldStreamingScheduler::shared()->reserve( StreamMemoryClass::SourceCpu,
                                                                         admitted * 32 + 256, error );
        if( !nextHistory.budget )
        {
            p.error = error;
            return false;
        }
        auto &history = p.history[view.key].entries;
        nextHistory.entries.reserve( size_t( admitted ) );
        Array<Array<f32>> thresholds;
        for( const auto &species : p.active->species )
        {
            thresholds.emplace_back( species->screenHeightThresholds.begin(),
                                     species->screenHeightThresholds.end() );
        }
        using Bin = std::pair<u32, u32>;
        std::map<Bin, Array<wp_foliage_instance_dx11>> bins;
        wp_render_statistics_dx11 before{};
        wp_renderer_dx11_get_statistics( native, &before );
        for( const auto &page : p.pages )
        {
            const auto bounds = p.pageBounds.find( page.first );
            if( bounds != p.pageBounds.end() &&
                !visible( view, bounds->second.centre, bounds->second.radius ) )
            {
                stats.culledInstances += page.second->instances.size();
                continue;
            }
            for( const auto &instance : page.second->instances )
            {
                ++stats.testedInstances;
                if( instance.flags & ( FoliageRemoved | FoliagePromoted ) )
                {
                    ++stats.culledInstances;
                    continue;
                }
                const auto &species = *p.active->species[instance.species];
                if( view.shadow && !species.castShadows )
                {
                    ++stats.culledInstances;
                    continue;
                }
                Matrix4F local;
                local.makeTransform( instance.position, instance.scale, instance.rotation );
                auto world = p.activePlacement * local;
                const auto inverse = world.inverse();
                const auto root = point( world, species.pivot );
                const auto centre =
                    point( world, ( species.boundsMinimum + species.boundsMaximum ) * .5f );
                const auto baseRadius = ( species.boundsMaximum - species.boundsMinimum ).length() * .5f;
                const auto maxScale = maximumScale( world );
                const auto radius = deformedRadius( species, world );
                const auto distance = ( centre - view.position ).length();
                if( distance - radius >
                        ( view.shadow ? species.shadowDistance : species.maximumDistance ) ||
                    !visible( view, centre, radius ) )
                {
                    ++stats.culledInstances;
                    continue;
                }
                const auto previous =
                    std::lower_bound( history.begin(), history.end(), instance.id,
                                      []( const auto &a, u64 id ) { return a.first < id; } );
                const auto screen = baseRadius * maxScale * view.projectionScale * view.lodBias /
                                    ( view.orthographic ? 1.0f : std::max( .001f, distance ) );
                auto lod = scene::LODSystem::selectLOD(
                    screen, thresholds[instance.species],
                    previous == history.end() || previous->first != instance.id ? -1 : previous->second,
                    view.forcedLod, species.hysteresis, false );
                if( nextHistory.entries.size() >= maximumVisible )
                {
                    error = "Foliage per-view visible-instance budget exceeded (262144).";
                    p.error = error;
                    return false;
                }
                nextHistory.entries.emplace_back( instance.id, lod );
                if( view.shadow )
                {
                    lod = std::min<s32>( 2, lod + species.shadowLodBias );
                }
                const auto hash = foliageStableHash( instance.id ^ instance.seed );
                const auto fraction = double( hash >> 11 ) * ( 1.0 / 9007199254740992.0 );
                if( fraction >= species.densityFractions[size_t( lod )] )
                {
                    ++stats.culledInstances;
                    continue;
                }
                wp_foliage_instance_dx11 output{};
                output.uv_rect = { 0, 0, 1, 1 };
                if( lod == 2 && species.farBillboard )
                {
                    const auto centreLocal = ( species.boundsMinimum + species.boundsMaximum ) * .5f;
                    auto toward = point( inverse, view.position ) - centreLocal;
                    if( toward.lengthSquared() < 1e-8f )
                    {
                        toward = { 0, 0, 1 };
                    }
                    toward.normalise();
                    size_t selected = 0;
                    f32 best = -2;
                    for( size_t v = 0; v < species.impostor.views.size(); ++v )
                    {
                        const auto d = toward.dotProduct( species.impostor.views[v].direction );
                        if( d > best )
                        {
                            best = d;
                            selected = v;
                        }
                    }
                    const auto &atlas = species.impostor.views[selected];
                    output.uv_rect = { atlas.uvMinimum.x, atlas.uvMinimum.y,
                                       atlas.uvMaximum.x - atlas.uvMinimum.x,
                                       atlas.uvMaximum.y - atlas.uvMinimum.y };
                    auto direction = view.position - point( world, centreLocal );
                    if( direction.lengthSquared() < 1e-8f )
                    {
                        direction = { 0, 0, 1 };
                    }
                    direction.normalise();
                    const auto localUp =
                        std::abs( toward.y ) > .98f ? Vector3F( 0, 0, 1 ) : Vector3F( 0, 1, 0 );
                    auto right =
                        vector( world, localUp.crossProduct( toward ).normaliseCopy() ).normaliseCopy();
                    auto up = direction.crossProduct( right ).normaliseCopy();
                    right = up.crossProduct( direction );
                    const auto origin = point( world, centreLocal );
                    const f32 sx = maxScale, sy = maxScale;
                    world = Matrix4F::identity();
                    for( int axis = 0; axis < 3; ++axis )
                    {
                        world[axis][0] = right[axis] * sx;
                        world[axis][1] = up[axis] * sy;
                        world[axis][2] = direction[axis] * sx;
                        world[axis][3] = origin[axis];
                    }
                }
                std::memcpy( output.instance.transform.m, world.ptr(),
                             sizeof( output.instance.transform ) );
                output.instance.tint = { instance.tint.r, instance.tint.g, instance.tint.b,
                                         instance.tint.a };
                const auto localWind = vector( world.inverse(), { species.wind.amplitude, 0, 0 } );
                const auto phase = std::fmod( double( p.seconds ) * species.wind.frequency +
                                                  double( hash & 65535u ) / 65535.0 * 6.283185307,
                                              6.283185307 );
                output.wind = { localWind.x, localWind.z, f32( phase ), species.wind.flutter };
                Vector3F bend{};
                for( const auto &interaction : p.interactions )
                {
                    auto delta = root - interaction.position;
                    delta.y = 0;
                    const auto length = delta.length();
                    if( length >= interaction.radius )
                    {
                        continue;
                    }
                    if( length < 1e-5f )
                    {
                        delta = { 1, 0, 0 };
                    }
                    else
                    {
                        delta /= length;
                    }
                    bend += delta * ( interaction.strength * ( 1 - length / interaction.radius ) );
                }
                if( bend.lengthSquared() > 1 )
                {
                    bend.normalise();
                }
                bend = vector( world.inverse(), bend * species.wind.maximumInteractionBend );
                output.interaction = { bend.x, bend.z, 0, 0 };
                bins[{ instance.species, u32( lod ) }].push_back( output );
                ++stats.lodInstances[size_t( lod )];
            }
        }
        std::sort( nextHistory.entries.begin(), nextHistory.entries.end() );
        p.history[view.key] = std::move( nextHistory );
        // Validate every group before the first draw: a late invalid transform
        // cannot leave a partially submitted population in the current pass.
        for( const auto &bin : bins )
        {
            const auto &species = *p.active->species[bin.first.first];
            const f32 offset = bin.first.second == 2 && species.farBillboard
                                   ? ( species.boundsMinimum.y + species.boundsMaximum.y ) * .5f
                                   : 0;
            const wp_foliage_parameters_dx11 parameters = { { 0, species.wind.frequency, 0, 0 },
                                                            { species.wind.bendStart - offset,
                                                              species.wind.bendEnd - offset, 0, 0 } };
            if( !wp_renderer_dx11_validate_foliage_instances( bin.second.data(),
                                                              s32( bin.second.size() ), &parameters ) )
            {
                error = "Foliage transformed draw population exceeds native numeric limits.";
                p.error = error;
                return false;
            }
        }
        for( const auto &bin : bins )
        {
            const auto &species = *p.active->species[bin.first.first];
            const auto lod = bin.first.second;
            const auto &gpu = p.assets->species[bin.first.first];
            const auto billboard = lod == 2 && species.farBillboard;
            const auto sections =
                billboard
                    ? Array<FoliageMeshSection>{ { 0, 6, 0 } }
                    : ( species.lods[lod].sections.empty()
                            ? Array<FoliageMeshSection>{ { 0, u32( species.lods[lod].indices.size() ),
                                                           0 } }
                            : species.lods[lod].sections );
            const f32 heightOffset =
                billboard ? ( species.boundsMinimum.y + species.boundsMaximum.y ) * .5f : 0;
            const wp_foliage_parameters_dx11 parameters = {
                { 0, species.wind.frequency, 0, 0 },
                { species.wind.bendStart - heightOffset, species.wind.bendEnd - heightOffset, 0, 0 }
            };
            for( const auto &section : sections )
            {
                const auto &mat = billboard ? gpu.impostor : gpu.materials[section.material];
                void *views[] = { mat.views[0].Get(), mat.views[1].Get(), mat.views[2].Get() };
                ++stats.groups;
                stats.predictedDraws +=
                    ( bin.second.size() + WP_DX11_INSTANCE_CAPACITY - 1 ) / WP_DX11_INSTANCE_CAPACITY;
                const auto submitted = renderer.renderFoliageGeometry(
                    gpu.meshes[lod]->native, section.firstIndex, section.indexCount, bin.second.data(),
                    u32( bin.second.size() ), parameters, mat.data, views,
                    billboard && !species.impostor.relightable, billboard );
                stats.submittedInstances += submitted;
                if( submitted != bin.second.size() )
                {
                    error = "Foliage GPU submission was incomplete.";
                    p.error = error;
                    return false;
                }
            }
        }
        wp_render_statistics_dx11 after{};
        wp_renderer_dx11_get_statistics( native, &after );
        stats.actualDraws = after.draws - before.draws;
        stats.uploadBytes = after.instance_upload_bytes - before.instance_upload_bytes;
        p.error.clear();
        return true;
    }
    catch( const std::exception &exception )
    {
        error = exception.what();
        m_impl->error = error;
        return false;
    }

    FoliageSnapshot ClawFoliageWorld::source() const
    {
        return m_impl->requested;
    }

    ClawFoliageDiagnostics ClawFoliageWorld::diagnostics() const
    {
        return m_impl->diagnostics;
    }

    String ClawFoliageWorld::lastError() const
    {
        return m_impl->error;
    }

    void ClawFoliageWorld::clear()
    {
        auto &p = *m_impl;
        p.stream.drain();
        p.pages.clear();
        p.staged.clear();
        p.pageBounds.clear();
        p.assets.reset();
        p.candidate.reset();
        p.active.reset();
        p.requested.reset();
        p.resolver = {};
        p.publicationGate = {};
        p.history.clear();
        p.interactions.clear();
        p.diagnostics = {};
        p.error.clear();
    }
}  // namespace workphone::render
