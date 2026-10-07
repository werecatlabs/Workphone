#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawHammerSystem.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/ClawTerrain.hpp>
#include <WPGraphics/ClawWindow.hpp>
#include <WPGraphics/ClawUtil.hpp>
#include <Workphone/Workphone.hpp>
#include "workphone_graphics_renderer.h"
#include <workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <algorithm>
#include <cstring>
#include <limits>
#include <unordered_map>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawRendererDX11, IRenderer3 );

    namespace
    {
        constexpr u16 k_quadIndices[] = { 0, 1, 2, 0, 2, 3 };

        struct MeshVertex
        {
            wp_vec3f position{};
            wp_vec3f normal{};
            wp_vec2f uv{};
            u32 colour = 0xFFFFFFFFu;
            bool hasNormal = false;
        };

        struct MeshVertexCache
        {
            const void *source = nullptr;
            u32 count = 0;
            u32 stride = 0;
            wp_vertex_format format = WORKPHONE_VERTEX_FORMAT_P;
            Array<wp_vertex_pntc> vertices;
            wp_geometry_dx11 *geometry = nullptr;
            wp_renderer_dx11 *renderer = nullptr;
        };

        std::unordered_map<const wp_graphics_mesh *, MeshVertexCache> g_meshVertexCaches;

        struct SkyEnvironmentEntry
        {
            WeakPtr<ISky> sky;
            ClawCubemap cubemap;
        };
        struct SkyEnvironmentCache
        {
            std::unordered_map<const ISky *, SkyEnvironmentEntry> skies;
            const ClawCubemap *active = nullptr;
        };
        // A scene may draw several skies. One shared filtered cube would be
        // invalidated by every switch between them, repeating GGX filtering.
        std::unordered_map<const ClawRendererDX11 *, SkyEnvironmentCache> g_skyEnvironmentCaches;

        MeshVertex readMeshVertex( const wp_graphics_mesh *mesh, u32 index )
        {
            MeshVertex result;
            const auto data = static_cast<const u8 *>( wp_graphics_mesh_get_vertices( mesh ) );
            const auto stride = wp_graphics_mesh_get_vertex_stride( mesh );
            if( !data || stride == 0 )
            {
                return result;
            }

            const auto vertex = data + static_cast<size_t>( index ) * stride;
            std::memcpy( &result.position, vertex, sizeof( result.position ) );
            switch( wp_graphics_mesh_get_vertex_format( mesh ) )
            {
            case WORKPHONE_VERTEX_FORMAT_PN:
            {
                const auto typed = reinterpret_cast<const wp_graphics_mesh_vertex_pn *>( vertex );
                result.normal = typed->normal;
                result.hasNormal = true;
                break;
            }
            case WORKPHONE_VERTEX_FORMAT_PT:
            {
                const auto typed = reinterpret_cast<const wp_graphics_mesh_vertex_pt *>( vertex );
                result.uv = typed->uv;
                break;
            }
            case WORKPHONE_VERTEX_FORMAT_PNT:
            {
                const auto typed = reinterpret_cast<const wp_graphics_mesh_vertex_pnt *>( vertex );
                result.normal = typed->normal;
                result.uv = typed->uv;
                result.hasNormal = true;
                break;
            }
            case WORKPHONE_VERTEX_FORMAT_PNTC:
            {
                const auto typed = reinterpret_cast<const wp_graphics_mesh_vertex_pntc *>( vertex );
                result.normal = typed->normal;
                result.uv = typed->uv;
                result.colour = typed->color;
                result.hasNormal = true;
                break;
            }
            case WORKPHONE_VERTEX_FORMAT_PC:
                result.colour = reinterpret_cast<const wp_graphics_mesh_vertex_pc *>( vertex )->color;
                break;
            case WORKPHONE_VERTEX_FORMAT_PTC:
            {
                const auto typed = reinterpret_cast<const wp_graphics_mesh_vertex_ptc *>( vertex );
                result.uv = typed->uv;
                result.colour = typed->color;
                break;
            }
            default:
                break;
            }
            return result;
        }

        u32 readMeshIndex( const wp_graphics_mesh *mesh, u32 index )
        {
            const void *indices = wp_graphics_mesh_get_indices( mesh );
            if( !indices )
            {
                return index;
            }
            return wp_graphics_mesh_get_index_format( mesh ) == WORKPHONE_INDEX_FORMAT_UINT32
                       ? static_cast<const u32 *>( indices )[index]
                       : static_cast<u32>( static_cast<const u16 *>( indices )[index] );
        }

        const Array<wp_vertex_pntc> &getMeshVertices( const wp_graphics_mesh *mesh )
        {
            auto &cache = g_meshVertexCaches[mesh];
            const auto source = wp_graphics_mesh_get_vertices( mesh );
            const auto count = wp_graphics_mesh_get_vertex_count( mesh );
            const auto stride = wp_graphics_mesh_get_vertex_stride( mesh );
            const auto format = wp_graphics_mesh_get_vertex_format( mesh );
            if( cache.source == source && cache.count == count && cache.stride == stride &&
                cache.format == format && cache.vertices.size() == count )
            {
                return cache.vertices;
            }

            cache.source = source;
            cache.count = count;
            cache.stride = stride;
            cache.format = format;
            if( cache.geometry )
            {
                wp_renderer_dx11_destroy_geometry( cache.geometry );
                cache.geometry = nullptr;
                cache.renderer = nullptr;
            }
            cache.vertices.resize( count );
            bool hasNormals = true;
            for( u32 i = 0; i < count; ++i )
            {
                const auto vertex = readMeshVertex( mesh, i );
                cache.vertices[i] = { vertex.position, vertex.normal, vertex.uv, vertex.colour };
                hasNormals = hasNormals && vertex.hasNormal;
            }

            // Position-only and position/UV meshes are valid inputs. Generate
            // smooth normals so they still participate in the lit material path.
            const auto topology = wp_graphics_mesh_get_primitive_type( mesh );
            if( !hasNormals && ( topology == WORKPHONE_PRIMITIVE_TRIANGLE_LIST ||
                                 topology == WORKPHONE_PRIMITIVE_TRIANGLE_STRIP ) )
            {
                for( auto &vertex : cache.vertices )
                    vertex.normal = {};

                const auto indexCount = wp_graphics_mesh_get_index_count( mesh );
                const auto elementCount = indexCount > 0 ? indexCount : count;
                const u32 step = topology == WORKPHONE_PRIMITIVE_TRIANGLE_STRIP ? 1u : 3u;
                for( u32 i = 0; i + 2 < elementCount; i += step )
                {
                    u32 ia = readMeshIndex( mesh, i );
                    u32 ib = readMeshIndex( mesh, i + 1 );
                    const u32 ic = readMeshIndex( mesh, i + 2 );
                    if( step == 1u && ( i & 1u ) )
                        std::swap( ia, ib );
                    if( ia >= count || ib >= count || ic >= count )
                        continue;
                    const auto &a = cache.vertices[ia].position;
                    const auto &b = cache.vertices[ib].position;
                    const auto &c = cache.vertices[ic].position;
                    const float abx = b.x - a.x, aby = b.y - a.y, abz = b.z - a.z;
                    const float acx = c.x - a.x, acy = c.y - a.y, acz = c.z - a.z;
                    const wp_vec3f normal = { aby * acz - abz * acy, abz * acx - abx * acz,
                                              abx * acy - aby * acx };
                    for( const auto vertexIndex : { ia, ib, ic } )
                    {
                        auto &n = cache.vertices[vertexIndex].normal;
                        n.x += normal.x;
                        n.y += normal.y;
                        n.z += normal.z;
                    }
                }
                for( auto &vertex : cache.vertices )
                {
                    auto &n = vertex.normal;
                    const float length = std::sqrt( n.x * n.x + n.y * n.y + n.z * n.z );
                    if( length > 1.0e-6f )
                    {
                        n.x /= length;
                        n.y /= length;
                        n.z /= length;
                    }
                    else
                    {
                        n = { 0.0f, 1.0f, 0.0f };
                    }
                }
            }
            return cache.vertices;
        }

        void appendTriangle( Array<wp_vertex_pntc> &vertices, const Array<wp_vertex_pntc> &source, u32 a,
                             u32 b, u32 c )
        {
            const auto vertexCount = source.size();
            if( a >= vertexCount || b >= vertexCount || c >= vertexCount )
            {
                return;
            }

            for( const auto index : { a, b, c } )
            {
                vertices.push_back( source[index] );
            }
        }

        wp_geometry_dx11 *getMeshGeometry( wp_renderer_dx11 *renderer, const wp_graphics_mesh *mesh,
                                           const Array<wp_vertex_pntc> &vertices )
        {
            auto &cache = g_meshVertexCaches[mesh];
            if( cache.geometry && cache.renderer != renderer )
            {
                wp_renderer_dx11_destroy_geometry( cache.geometry );
                cache.geometry = nullptr;
                cache.renderer = nullptr;
            }
            if( !renderer || vertices.empty() ||
                wp_graphics_mesh_get_primitive_type( mesh ) != WORKPHONE_PRIMITIVE_TRIANGLE_LIST )
                return nullptr;

            const void *indices = wp_graphics_mesh_get_indices( mesh );
            const auto count = indices ? wp_graphics_mesh_get_index_count( mesh )
                                       : wp_graphics_mesh_get_vertex_count( mesh );
            if( vertices.size() > static_cast<size_t>( std::numeric_limits<wp_s32>::max() ) ||
                count > static_cast<u32>( std::numeric_limits<wp_s32>::max() ) )
                return nullptr;
            if( !cache.geometry )
            {
                Array<u32> sequentialIndices;
                if( !indices )
                {
                    sequentialIndices.resize( count );
                    for( u32 i = 0; i < count; ++i )
                        sequentialIndices[i] = i;
                    indices = sequentialIndices.data();
                }
                cache.geometry = wp_renderer_dx11_create_indexed_geometry_pntc(
                    renderer, vertices.data(), static_cast<wp_s32>( vertices.size() ), indices,
                    static_cast<wp_s32>( count ),
                    !sequentialIndices.empty() ||
                        wp_graphics_mesh_get_index_format( mesh ) == WORKPHONE_INDEX_FORMAT_UINT32 );
                cache.renderer = cache.geometry ? renderer : nullptr;
            }
            return cache.geometry;
        }

        void drawLitTriangles( wp_renderer_dx11 *renderer, const Array<wp_vertex_pntc> &vertices )
        {
            if( !renderer || vertices.empty() ||
                vertices.size() > static_cast<size_t>( std::numeric_limits<wp_s32>::max() ) )
                return;
            Array<u32> indices( vertices.size() );
            for( size_t i = 0; i < indices.size(); ++i )
                indices[i] = static_cast<u32>( i );
            if( auto geometry = wp_renderer_dx11_create_indexed_geometry_pntc(
                    renderer, vertices.data(), static_cast<wp_s32>( vertices.size() ), indices.data(),
                    static_cast<wp_s32>( indices.size() ), 1 ) )
            {
                wp_renderer_dx11_draw_geometry_pntc( renderer, geometry, 0,
                                                     static_cast<wp_s32>( indices.size() ), 0 );
                wp_renderer_dx11_destroy_geometry( geometry );
            }
        }

        SafeReadPtr<MaterialPassStateData> getPrimaryMaterialPassState(
            const SmartPtr<IMaterial> &material )
        {
            if( !material )
                return {};
            for( const auto &technique : material->getTechniques() )
            {
                if( !technique )
                    continue;
                for( const auto &pass : technique->getPasses() )
                {
                    if( auto context = pass ? pass->getStateContext() : nullptr )
                    {
                        if( auto state =
                                context->getStateDataById<MaterialPassStateData>( pass->getId() ) )
                            return state;
                    }
                }
            }
            return {};
        }

        void applyPrimaryMaterialUvState( const SmartPtr<IMaterial> &material,
                                          const SafeReadPtr<MaterialPassStateData> &state,
                                          wp_material_dx11 &nativeMaterial )
        {
            if( state )
            {
                nativeMaterial.surface.z = state->uvTilingX;
                nativeMaterial.surface.w = state->uvTilingY;
                nativeMaterial.uv_transform.x = state->uvOffsetX;
                nativeMaterial.uv_transform.y = state->uvOffsetY;
                nativeMaterial.uv_transform.w = state->uvRotation;
                nativeMaterial.projection = { static_cast<f32>( material->getUVProjection() ),
                    material->getTriplanarScale(), 0.0f, 0.0f };
                nativeMaterial.controls.x = state->normalStrength;
                nativeMaterial.controls.y = state->aoStrength;
                nativeMaterial.controls.z = state->getFlag( cutoutFlag ) ? state->alphaClip : -1.0f;
                nativeMaterial.controls.w = state->blendMode == 2u ? 1.0f : 0.0f;
                nativeMaterial.texture_sources = { static_cast<f32>( state->metallicSource ),
                    static_cast<f32>( state->roughnessSource ), static_cast<f32>( state->aoSource ),
                    static_cast<f32>( state->opacitySource ) };
                const auto intensity = state->getFlag( emissionEnabledFlag ) ? state->emissionIntensity : 0.0f;
                nativeMaterial.emissive_color.x *= intensity;
                nativeMaterial.emissive_color.y *= intensity;
                nativeMaterial.emissive_color.z *= intensity;
                // UI materials use the same texture and alpha handling without lighting.
                if( state->materialType == MaterialType::UI )
                    nativeMaterial.uv_transform.z = 0.0f;
            }
        }

        bool isMaterialDoubleSided( const SmartPtr<IMaterial> &material )
        {
            if( auto state = getPrimaryMaterialPassState( material ) )
                return state->getFlag( doubleSidedFlag );
            return false;
        }
    }  // namespace

    ClawRendererDX11::ClawRendererDX11() = default;

    ClawRendererDX11::~ClawRendererDX11()
    {
        destroyRenderer();
    }

    void ClawRendererDX11::load( SmartPtr<ISharedObject> data )
    {
        if( m_renderer )
        {
            return;
        }

        setLoadingState( LoadingState::Loading );

        auto window = dynamic_pointer_cast<IGraphicsWindow>( data );
        if( !window )
        {
            if( auto applicationManager = core::IApplicationManager::instancePtr() )
            {
                window = applicationManager->getWindow();
            }
        }

        void *nativeWindow = nullptr;
        if( window )
        {
            const auto size = window->getSize();
            if( size.x > 0 && size.y > 0 )
            {
                m_rtWidth = static_cast<u32>( size.x );
                m_rtHeight = static_cast<u32>( size.y );
            }

            window->getWindowHandle( &nativeWindow );
            m_renderTarget = window;
        }

        WP_LOG_INFO( "WPGraphics/DX11: initializing renderer at " + std::to_string( m_rtWidth ) + "x" +
                     std::to_string( m_rtHeight ) +
                     "; native window=" + String( nativeWindow ? "available" : "missing" ) );
        if( !nativeWindow )
            WP_LOG_WARNING(
                "WPGraphics/DX11: no native window handle; device initialization may fail." );
        m_renderer = wp_renderer_create_dx11( nativeWindow, static_cast<wp_s32>( m_rtWidth ),
                                              static_cast<wp_s32>( m_rtHeight ) );

        if( m_renderer )
        {
            m_windowWidth = m_rtWidth;
            m_windowHeight = m_rtHeight;
            wp_renderer_set_blend_mode( m_renderer, WORKPHONE_BLEND_MODE_ALPHA );
            wp_renderer_set_fill_mode( m_renderer, WORKPHONE_FILL_MODE_SOLID );
            wp_renderer_set_cull_mode( m_renderer, WORKPHONE_CULL_MODE_NONE );
            wp_renderer_set_depth_test_enabled( m_renderer, 1 );
            wp_renderer_set_depth_write_enabled( m_renderer, 1 );
            wp_renderer_set_depth_func( m_renderer, WORKPHONE_DEPTH_FUNC_LESS );

            wp_viewport_i viewport = { 0, 0, static_cast<wp_s32>( m_rtWidth ),
                                       static_cast<wp_s32>( m_rtHeight ) };
            wp_renderer_set_viewport( m_renderer, viewport );
        }

        setLoadingState( m_renderer ? LoadingState::Loaded : LoadingState::Unloaded );
        if( m_renderer )
        {
            WP_LOG_INFO(
                "WPGraphics/DX11: renderer ready; shaders, depth testing and initial viewport "
                "configured." );
        }
        else
        {
            WP_LOG_ERROR( "WPGraphics/DX11: native renderer creation failed at " +
                          std::to_string( m_rtWidth ) + "x" + std::to_string( m_rtHeight ) );
        }
    }

    void ClawRendererDX11::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        destroyRenderer();
        m_camera = nullptr;
        m_renderTarget = nullptr;
        m_viewport = nullptr;
        m_inFrame = false;

        setLoadingState( LoadingState::Unloaded );
    }

    void ClawRendererDX11::destroyRenderer()
    {
        g_skyEnvironmentCaches.erase( this );
        m_environment.reset();
        m_previewEnvironment.reset();
        for( auto &[mesh, cache] : g_meshVertexCaches )
        {
            (void)mesh;
            wp_renderer_dx11_destroy_geometry( cache.geometry );
        }
        g_meshVertexCaches.clear();
        if( m_renderer )
        {
            wp_renderer_destroy( m_renderer );
            m_renderer = nullptr;
        }
    }

    void ClawRendererDX11::beginRender()
    {
        if( !m_renderer || m_inFrame )
        {
            return;
        }

        m_primitiveCount = 0;
        m_hasSkyEnvironment = false;
        if( auto found = g_skyEnvironmentCaches.find( this ); found != g_skyEnvironmentCaches.end() )
        {
            auto &cache = found->second;
            cache.active = nullptr;
            for( auto it = cache.skies.begin(); it != cache.skies.end(); )
                if( !it->second.sky.lock() )
                    it = cache.skies.erase( it );
                else
                    ++it;
        }
        m_inFrame = true;
        wp_renderer_begin_frame( m_renderer );
        if( auto dx11 = wp_renderer_get_dx11( m_renderer ) )
            wp_renderer_dx11_set_environment( dx11, nullptr, 0.0f );
    }

    void ClawRendererDX11::endRender()
    {
        if( !m_renderer || !m_inFrame )
        {
            return;
        }

        wp_renderer_end_frame( m_renderer );

        if( m_renderTarget->isExactly<ClawWindow>() )
        {
            if( auto dx11 = wp_renderer_get_dx11( m_renderer ) )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                auto graphicsSystem = (ClawHammerSystem *)applicationManager->getGraphicsSystemPtr();

                wp_renderer_dx11_present( dx11,  graphicsSystem->getVSync() ? 1 : 0 );
            }
        }

        m_inFrame = false;
    }

    void ClawRendererDX11::flush()
    {
        // The C89 DX11 backend submits through the immediate device context.
    }

    void ClawRendererDX11::clear( const ColourF &colour )
    {
        if( !m_renderer )
        {
            return;
        }

        wp_renderer_set_clear_color( m_renderer, colour.r, colour.g, colour.b, colour.a );
        wp_renderer_set_clear_depth( m_renderer, 1.0f );
        wp_renderer_clear( m_renderer, WORKPHONE_CLEAR_FLAG_ALL );
    }

    void ClawRendererDX11::setRenderTarget( SmartPtr<IRenderTarget> renderTarget )
    {
        m_renderTarget = renderTarget;

        if( !m_renderer )
        {
            return;
        }

        auto dx11 = wp_renderer_get_dx11( m_renderer );
        if( !dx11 )
        {
            return;
        }

        // Unbind any SRV from the preceding compositor pass before a texture can become an RTV.
        wp_renderer_dx11_set_render_texture( dx11, nullptr );

        if( auto clawTarget = dynamic_pointer_cast<ClawRenderTarget>( m_renderTarget.load() ) )
        {
            const auto size = clawTarget->getSize();
            if( size.x > 0 && size.y > 0 )
            {
                m_rtWidth = static_cast<u32>( size.x );
                m_rtHeight = static_cast<u32>( size.y );
            }
            auto nativeTarget = clawTarget->getNativeRenderTexture( m_renderer );
            wp_renderer_dx11_set_render_texture( dx11, nativeTarget );
        }
        else
        {
            // Texture dimensions describe only the active pass. Keep the window's
            // swap-chain size separate so scene/window switches do not resize it.
            if( auto window = dynamic_pointer_cast<IGraphicsWindow>( m_renderTarget.load() ) )
            {
                const auto size = window->getSize();
                if( size.x > 0 && size.y > 0 )
                {
                    if( ( static_cast<u32>( size.x ) != m_windowWidth ||
                          static_cast<u32>( size.y ) != m_windowHeight ) &&
                        wp_renderer_resize( m_renderer, size.x, size.y ) )
                    {
                        m_windowWidth = static_cast<u32>( size.x );
                        m_windowHeight = static_cast<u32>( size.y );
                    }
                    m_rtWidth = static_cast<u32>( size.x );
                    m_rtHeight = static_cast<u32>( size.y );
                }
            }

            // ResizeBuffers unbinds the old output targets. Bind after resizing
            // so a real window resize also leaves a valid colour/depth target.
            void *nativeRenderTarget = nullptr;
            if( m_renderTarget )
            {
                m_renderTarget->_getObject( &nativeRenderTarget );
            }
            wp_renderer_dx11_set_render_target_native( dx11, nativeRenderTarget );
        }
    }

    SmartPtr<IRenderTarget> ClawRendererDX11::getRenderTarget() const
    {
        return m_renderTarget;
    }

    void ClawRendererDX11::setViewport( SmartPtr<IViewport> viewport )
    {
        m_viewport = viewport;
        if( !m_renderer )
        {
            return;
        }

        wp_viewport_i nativeViewport = { 0, 0, static_cast<wp_s32>( m_rtWidth ),
                                         static_cast<wp_s32>( m_rtHeight ) };
        if( m_viewport )
        {
            const auto position = m_viewport->getActualPosition();
            const auto size = m_viewport->getActualSize();
            nativeViewport.x = static_cast<wp_s32>( position.X() );
            nativeViewport.y = static_cast<wp_s32>( position.Y() );
            if( size.X() > 0 && size.Y() > 0 )
            {
                nativeViewport.width = static_cast<wp_s32>( size.X() );
                nativeViewport.height = static_cast<wp_s32>( size.Y() );
            }
        }

        wp_renderer_set_viewport( m_renderer, nativeViewport );
    }

    SmartPtr<IViewport> ClawRendererDX11::getViewport() const
    {
        return m_viewport;
    }

    void ClawRendererDX11::render( const SmartPtr<ISharedObject> &renderData,
                                   const SmartPtr<ITexture> &texture, const Matrix4F &transform,
                                   const ColourF &colour )
    {
        if( !m_renderer )
        {
            return;
        }

        setTransforms( transform );

        void *nativeTexture = nullptr;
        if( texture )
        {
            texture->getTextureFinal( &nativeTexture );
        }
        wp_renderer_set_texture_native( m_renderer, nativeTexture );

        const auto packedColour = packColour( colour );
        const wp_vertex_ptc vertices[] = {
            { { -0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f }, packedColour },
            { { 0.5f, 0.5f, 0.0f }, { 1.0f, 0.0f }, packedColour },
            { { 0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f }, packedColour },
            { { -0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f }, packedColour },
        };

        wp_renderer_draw_indexed_triangles_ptc( m_renderer, vertices, 4, k_quadIndices, 6 );
        m_primitiveCount += 2;
    }

    void ClawRendererDX11::render( const SmartPtr<ISharedObject> &renderData,
                                   const SmartPtr<IMaterial> &material, const Matrix4F &transform,
                                   const ColourF &colour )
    {
        if( !m_renderer )
        {
            return;
        }

        setTransforms( transform );

        const auto packedColour = packColour( colour );
        const wp_vertex_pc vertices[] = {
            { { -0.5f, 0.5f, 0.0f }, packedColour },
            { { 0.5f, 0.5f, 0.0f }, packedColour },
            { { 0.5f, -0.5f, 0.0f }, packedColour },
            { { -0.5f, -0.5f, 0.0f }, packedColour },
        };

        wp_renderer_draw_indexed_triangles_pc( m_renderer, vertices, 4, k_quadIndices, 6 );
        m_primitiveCount += 2;
    }

    void ClawRendererDX11::_getObject( void **ppObject )
    {
        if( ppObject )
        {
            *ppObject = m_renderer ? wp_renderer_get_dx11_device( m_renderer ) : nullptr;
        }
    }

    void ClawRendererDX11::setCamera( SmartPtr<IGraphicsCamera> camera )
    {
        m_camera = camera;
        if( m_camera && m_viewport )
        {
            const auto size = m_viewport->getActualSize();
            if( size.X() > 0 && size.Y() > 0 )
            {
                m_camera->setAspectRatio( size.X() / size.Y() );
            }
        }
    }

    void ClawRendererDX11::forgetMesh( const wp_graphics_mesh *mesh )
    {
        const auto found = g_meshVertexCaches.find( mesh );
        if( found != g_meshVertexCaches.end() )
        {
            wp_renderer_dx11_destroy_geometry( found->second.geometry );
            g_meshVertexCaches.erase( found );
        }
    }

    SmartPtr<IGraphicsCamera> ClawRendererDX11::getCamera() const
    {
        return m_camera;
    }

    void ClawRendererDX11::drawLine( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                                     const ColourF &colour )
    {
        if( !m_renderer )
        {
            return;
        }

        setTransforms( Matrix4F::identity() );

        const auto packedColour = packColour( colour );
        const wp_vertex_pc vertices[] = {
            { { ( start.X() ), ( start.Y() ), ( start.Z() ) }, packedColour },
            { { ( end.X() ), ( end.Y() ), ( end.Z() ) }, packedColour },
        };

        wp_renderer_draw_lines_pc( m_renderer, vertices, 2 );
        ++m_primitiveCount;
    }

    wp_renderer *ClawRendererDX11::getNativeRenderer() const
    {
        return m_renderer;
    }

    void ClawRendererDX11::renderParticles( const Array<wp_particle_sample> &particles,
        const Matrix4F &world, const Vector3F &scale, const SmartPtr<IMaterial> &material )
    {
        if( !m_renderer || particles.empty() ) return;
        const auto view = m_camera ? Matrix4F( m_camera->getViewMatrix().ptr() ) : Matrix4F::identity();
        const auto inverseView = view.inverse();
        const Vector3F right( inverseView[0][0], inverseView[1][0], inverseView[2][0] );
        const Vector3F up( inverseView[0][1], inverseView[1][1], inverseView[2][1] );
        struct DrawParticle { Vector3F center; f32 depth; const wp_particle_sample *particle; };
        Array<DrawParticle> draw;
        draw.reserve( particles.size() );
        for( const auto &particle : particles )
        {
            const auto center = world.transformAffine( Vector3F( particle.position.x * scale.X(),
                particle.position.y * scale.Y(), particle.position.z * scale.Z() ) );
            draw.push_back( { center, view.transformAffine( center ).Z(), &particle } );
        }
        std::stable_sort( draw.begin(), draw.end(), []( const auto &a, const auto &b ) { return a.depth < b.depth; } );
        const auto oldBlend = wp_renderer_get_blend_mode( m_renderer );
        const auto oldCull = wp_renderer_get_cull_mode( m_renderer );
        const auto oldDepthWrite = wp_renderer_get_depth_write_enabled( m_renderer );
        const auto oldDepthTest = wp_renderer_get_depth_test_enabled( m_renderer );
        const auto oldDepthFunc = wp_renderer_get_depth_func( m_renderer );
        const auto oldFill = wp_renderer_get_fill_mode( m_renderer );
        auto blend = material ? ClawUtil::toCBlendMode( material->getBlendMode() ) : WORKPHONE_BLEND_MODE_ALPHA;
        if( blend == WORKPHONE_BLEND_MODE_NONE ) blend = WORKPHONE_BLEND_MODE_ALPHA;
        wp_renderer_set_blend_mode( m_renderer, blend );
        wp_renderer_set_cull_mode( m_renderer, WORKPHONE_CULL_MODE_NONE );
        wp_renderer_set_depth_test_enabled( m_renderer, 1 );
        wp_renderer_set_depth_write_enabled( m_renderer, 0 );
        wp_renderer_set_depth_func( m_renderer, WORKPHONE_DEPTH_FUNC_LEQUAL );
        wp_renderer_set_fill_mode( m_renderer, WORKPHONE_FILL_MODE_SOLID );
        void *texture = nullptr;
        if( material ) if( auto input = material->getTexture( 0 ) ) input->getTextureFinal( &texture );
        wp_renderer_set_texture_native( m_renderer, texture );
        setTransforms( Matrix4F::identity() );
        // Bounded batches avoid overflowing the transient native vertex buffer.
        constexpr size_t batchSize = 1024;
        Array<wp_vertex_ptc> vertices;
        vertices.reserve( batchSize * 6 );
        for( size_t start = 0; start < draw.size(); start += batchSize )
        {
            vertices.clear();
            const auto end = std::min( start + batchSize, draw.size() );
            for( size_t i = start; i < end; ++i )
            {
                const auto &p = *draw[i].particle;
                const auto half = p.size * 0.5f;
                const auto r = right * ( half * std::abs( scale.X() ) );
                const auto u = up * ( half * std::abs( scale.Y() ) );
                const Vector3F corners[] = { draw[i].center-r-u, draw[i].center+r-u,
                    draw[i].center+r+u, draw[i].center-r+u };
                const wp_vec2f uv[] = { {0,1}, {1,1}, {1,0}, {0,0} };
                const auto colour = packColour( ColourF( p.color[0], p.color[1], p.color[2], p.color[3] ) );
                for( auto index : k_quadIndices )
                    vertices.push_back( { { corners[index].X(), corners[index].Y(), corners[index].Z() }, uv[index], colour } );
            }
            wp_renderer_draw_triangles_ptc( m_renderer, vertices.data(), static_cast<wp_s32>( vertices.size() ) );
            m_primitiveCount += static_cast<u32>( vertices.size() / 3 );
        }
        wp_renderer_set_texture_native( m_renderer, nullptr );
        wp_renderer_set_blend_mode( m_renderer, oldBlend );
        wp_renderer_set_cull_mode( m_renderer, oldCull );
        wp_renderer_set_depth_write_enabled( m_renderer, oldDepthWrite );
        wp_renderer_set_depth_test_enabled( m_renderer, oldDepthTest );
        wp_renderer_set_depth_func( m_renderer, oldDepthFunc );
        wp_renderer_set_fill_mode( m_renderer, oldFill );
    }

    void ClawRendererDX11::setSceneLighting( const ColourF &ambient, const Vector3F &direction,
                                             const ColourF &colour, f32 intensity )
    {
        m_ambientLight = ambient;
        m_hasSkyEnvironment = false;
        m_lightDirection = direction;
        m_lightColour = colour;
        m_lightIntensity = std::max( intensity, 0.0f );
    }

    void ClawRendererDX11::applySceneLighting( wp_material_dx11 &material ) const
    {
        Vector3F cameraPosition = Vector3F::zero();
        if( m_camera )
        {
            if( auto cameraOwner = m_camera->getOwner() )
            {
                const auto position = cameraOwner->getWorldPosition();
                cameraPosition = Vector3F( position.X(), position.Y(), position.Z() );
            }
        }
        const float ambient = std::max( { m_ambientLight.r, m_ambientLight.g, m_ambientLight.b, 0.0f } );
        material.light_color = { m_lightColour.r, m_lightColour.g, m_lightColour.b, m_lightIntensity };
        material.light_direction = { m_lightDirection.X(), m_lightDirection.Y(), m_lightDirection.Z(),
                                     0.0f };
        material.camera_position = { cameraPosition.X(), cameraPosition.Y(), cameraPosition.Z(),
                                     ambient };
        material.ambient_color = { m_ambientLight.r, m_ambientLight.g, m_ambientLight.b, 1.0f };
        auto dx11 = wp_renderer_get_dx11( m_renderer );
        if( dx11 && !m_hasSkyEnvironment && !m_previewEnvironment.getView() )
            m_previewEnvironment.update( static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( dx11 ) ),
                static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( dx11 ) ), {} );
        if( dx11 )
        {
            const ClawCubemap *environment = &m_previewEnvironment;
            if( m_hasSkyEnvironment )
                if( auto found = g_skyEnvironmentCaches.find( this ); found != g_skyEnvironmentCaches.end() )
                    if( found->second.active )
                        environment = found->second.active;
            wp_renderer_dx11_set_environment( dx11, environment->getView(), environment->getMaxLod() );
        }
    }

    bool ClawRendererDX11::beginShadowMap( s32 mapSize )
    {
        disableShadows();
        auto camera = getCamera();
        auto dx11 = m_renderer ? wp_renderer_get_dx11( m_renderer ) : nullptr;
        if( !dx11 || !camera || m_lightDirection.dotProduct( m_lightDirection ) < 1e-8f ) return false;
        mapSize = std::clamp( mapSize, 64, 8192 );
        const auto direction = m_lightDirection.normaliseCopy();
        const Vector3F up = std::abs( direction.Y() ) > 0.95f ? Vector3F( 1, 0, 0 ) : Vector3F( 0, 1, 0 );
        // Match the camera's screen-space winding while depth increases along
        // the light direction, so single-sided casters retain their front faces.
        const auto right = direction.crossProduct( up ).normaliseCopy();
        const auto vertical = right.crossProduct( direction );
        auto lightView = Matrix4F::identity();
        for( int i = 0; i < 3; ++i )
        {
            lightView[0][i] = right[i]; lightView[1][i] = vertical[i]; lightView[2][i] = direction[i];
        }
        const auto inverse = ( Matrix4F( camera->getProjectionMatrix().ptr() ) *
                               Matrix4F( camera->getViewMatrix().ptr() ) ).inverse();
        const auto transformPoint = []( const Matrix4F &matrix, const Vector3F &point ) {
            const float w = matrix[3][0] * point[0] + matrix[3][1] * point[1] +
                            matrix[3][2] * point[2] + matrix[3][3];
            Vector3F result;
            for( int i = 0; i < 3; ++i ) result[i] = ( matrix[i][0] * point[0] +
                matrix[i][1] * point[1] + matrix[i][2] * point[2] + matrix[i][3] ) / w;
            return result;
        };
        Vector3F minimum( 1e30f, 1e30f, 1e30f ), maximum( -1e30f, -1e30f, -1e30f );
        const float nearClip = camera->getNearClipDistance();
        const float farClip = camera->getFarClipDistance();
        const float fraction = std::clamp( ( std::min( farClip, 200.0f ) - nearClip ) /
                                          std::max( farClip - nearClip, 0.001f ), 0.001f, 1.0f );
        for( int y = -1; y <= 1; y += 2 ) for( int x = -1; x <= 1; x += 2 )
        {
            const auto nearPoint = transformPoint( inverse, Vector3F( static_cast<float>( x ), static_cast<float>( y ), -1 ) );
            const auto farPoint = transformPoint( inverse, Vector3F( static_cast<float>( x ), static_cast<float>( y ), 1 ) );
            for( const auto &point : { nearPoint, nearPoint + ( farPoint - nearPoint ) * fraction } )
            {
                const auto p = transformPoint( lightView, point );
                for( int i = 0; i < 3; ++i )
                {
                    minimum[i] = std::min( minimum[i], p[i] ); maximum[i] = std::max( maximum[i], p[i] );
                }
            }
        }
        // Leave depth room for casters outside the camera frustum toward the light.
        minimum[2] -= 500.0f; maximum[2] += 50.0f;
        auto projection = Matrix4F::identity();
        for( int i = 0; i < 3; ++i )
        {
            const float extent = std::max( maximum[i] - minimum[i], 1.0f ) + 2.0f;
            float center = ( minimum[i] + maximum[i] ) * 0.5f;
            if( i < 2 ) center = std::floor( center / ( extent / static_cast<float>( mapSize ) ) ) * ( extent / static_cast<float>( mapSize ) );
            projection[i][i] = 2.0f / extent; projection[i][3] = -2.0f * center / extent;
        }
        m_shadowMatrix = projection * lightView;
        const auto matrix = toCMatrix( m_shadowMatrix );
        m_shadowPass = wp_renderer_dx11_begin_shadow_map( dx11, &matrix, mapSize ) != 0;
        return m_shadowPass;
    }

    void ClawRendererDX11::endShadowMap()
    {
        if( !m_shadowPass ) return;
        wp_renderer_dx11_end_shadow_map( wp_renderer_get_dx11( m_renderer ) );
        m_shadowPass = false;
        m_shadowsEnabled = true;
        setTransforms( Matrix4F::identity() );
    }

    void ClawRendererDX11::disableShadows()
    {
        m_shadowsEnabled = false;
        if( m_renderer ) wp_renderer_dx11_enable_shadow_receiving( wp_renderer_get_dx11( m_renderer ), 0 );
    }

    void ClawRendererDX11::renderMesh( ClawMesh *mesh, const Matrix4F &transform )
    {
        auto nativeMesh = mesh ? mesh->getNativeMesh() : nullptr;
        if( !m_renderer || !mesh || !mesh->isVisible() || !nativeMesh )
        {
            return;
        }

        const auto vertexCount = wp_graphics_mesh_get_vertex_count( nativeMesh );
        const auto indexCount = wp_graphics_mesh_get_index_count( nativeMesh );
        const auto elementCount = indexCount > 0 ? indexCount : vertexCount;
        if( vertexCount == 0 || elementCount == 0 )
        {
            return;
        }

        setTransforms( transform );
        const auto topology = wp_graphics_mesh_get_primitive_type( nativeMesh );
        const auto submeshCount = wp_graphics_mesh_get_submesh_count( nativeMesh );
        const auto drawCount = std::max<s32>( submeshCount, 1 );
        const auto &meshVertices = getMeshVertices( nativeMesh );
        if( meshVertices.empty() ||
            meshVertices.size() > static_cast<size_t>( std::numeric_limits<wp_s32>::max() ) )
        {
            return;
        }

        const auto oldBlend = wp_renderer_get_blend_mode( m_renderer );
        const auto oldCull = wp_renderer_get_cull_mode( m_renderer );
        const auto oldDepthWrite = wp_renderer_get_depth_write_enabled( m_renderer );
        const auto oldDepthTest = wp_renderer_get_depth_test_enabled( m_renderer );
        const auto oldDepthFunc = wp_renderer_get_depth_func( m_renderer );
        const auto oldFill = wp_renderer_get_fill_mode( m_renderer );
        wp_renderer_set_blend_mode( m_renderer, WORKPHONE_BLEND_MODE_NONE );

        auto dx11 = wp_renderer_get_dx11( m_renderer );
        const auto geometry = getMeshGeometry( dx11, nativeMesh, meshVertices );

        for( s32 submeshIndex = 0; submeshIndex < drawCount; ++submeshIndex )
        {
            u32 start = 0;
            u32 count = elementCount;
            if( submeshCount > 0 )
            {
                if( const auto submesh = wp_graphics_mesh_get_submesh( nativeMesh, submeshIndex ) )
                {
                    start = std::min( submesh->index_start, elementCount );
                    count = std::min( submesh->index_count, elementCount - start );
                }
            }

            auto material = mesh->getMaterial( submeshIndex );
            if( !material )
            {
                material = mesh->getMaterial();
            }
            if( !material )
            {
                auto materialName = mesh->getMaterialName( submeshIndex );
                if( StringUtil::isNullOrEmpty( materialName ) )
                    materialName = mesh->getMaterialName();
                if( !StringUtil::isNullOrEmpty( materialName ) )
                {
                    auto applicationManager = core::IApplicationManager::instancePtr();
                    auto graphicsSystem =
                        applicationManager ? applicationManager->getGraphicsSystem() : nullptr;
                    auto materialManager =
                        graphicsSystem ? graphicsSystem->getMaterialManager() : nullptr;
                    if( materialManager )
                    {
                        material = dynamic_pointer_cast<IMaterial>(
                            materialManager->getByName( materialName ) );
                    }
                }
            }

            void *nativeTexture = nullptr;
            if( material )
            {
                if( auto texture = material->getTexture( 0 ) )
                {
                    texture->getTextureFinal( &nativeTexture );
                }
            }

            wp_renderer_set_texture_native( m_renderer, nativeTexture );

            // These are Workphone/Ogre PBS slots, not the low-level C material slots.
            const auto state = getPrimaryMaterialPassState( material );
            if( m_shadowPass && state && !state->getFlag( castShadowsFlag ) ) continue;
            wp_renderer_dx11_enable_shadow_receiving( dx11, m_shadowsEnabled && mesh->getReceiveShadows() &&
                ( !state || state->getFlag( receiveShadowsFlag ) ) );
            u32 textureSlots[] = { 1u, 2u, 3u, 13u, 22u, 24u };
            if( state )
            {
                if( state->metallicSource == 3u ) textureSlots[1] = 25u;
                if( state->roughnessSource == 2u || state->roughnessSource == 4u ) textureSlots[2] = 2u;
                if( state->roughnessSource == 3u ) textureSlots[2] = 25u;
                if( state->aoSource == 1u ) textureSlots[4] = 25u;
                if( state->opacitySource == 2u ) textureSlots[5] = 25u;
            }
            void *textureViews[6] = {};
            if( material )
                for( size_t i = 0; i < 6; ++i )
                    if( auto texture = material->getTexture( textureSlots[i] ) )
                        texture->getTextureFinal( &textureViews[i] );
            wp_renderer_dx11_set_material_textures( dx11, textureViews );
            wp_renderer_dx11_set_material_sampler( dx11, state ? state->uvWrapU : 0u,
                state ? state->uvWrapV : 0u, state ? state->uvFilter : 1u,
                state ? static_cast<u32>( Math<f32>::clamp( state->uvAniso, 1.0f, 16.0f ) ) : 1u );

            wp_material_dx11 nativeMaterial{};
            const auto diffuse = material ? material->getDiffuse() : ColourF::White;
            const auto specular =
                material ? material->getSpecular() : ColourF( 0.04f, 0.04f, 0.04f, 1.0f );
            const auto emissive = material ? material->getEmissive() : ColourF::Black;
            nativeMaterial.base_color = { diffuse.r, diffuse.g, diffuse.b, diffuse.a };
            nativeMaterial.specular_color = { specular.r, specular.g, specular.b, specular.a };
            nativeMaterial.emissive_color = { emissive.r, emissive.g, emissive.b, emissive.a };
            applySceneLighting( nativeMaterial );
            // A material's reflection probe overrides the sky environment for this draw.
            if( material )
                if( auto reflection = material->getTexture( static_cast<u32>( PbsTextureTypes::PBSM_REFLECTION ) ) )
                {
                    void *view = nullptr;
                    reflection->getTextureFinal( &view );
                    if( view )
                    {
                        D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
                        static_cast<ID3D11ShaderResourceView *>( view )->GetDesc( &desc );
                        if( desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURECUBE )
                        {
                            wp_renderer_dx11_set_environment( dx11, view, static_cast<float>( desc.TextureCube.MipLevels - 1 ) );
                            nativeMaterial.environment.z = 1.0f;
                        }
                    }
                }
            nativeMaterial.surface = { material ? material->getMetalness() : 0.0f,
                                       material ? material->getRoughness() : 0.5f, 1.0f, 1.0f };
            nativeMaterial.uv_transform = { 0.0f, 0.0f, 1.0f, 0.0f };
            nativeMaterial.controls = { 1.0f, 1.0f, -1.0f, 0.0f };
            nativeMaterial.map_flags = { textureViews[0] ? 1.0f : 0.0f,
                textureViews[1] ? 1.0f : 0.0f, textureViews[2] ? 1.0f : 0.0f,
                textureViews[3] ? 1.0f : 0.0f };
            nativeMaterial.extra_map_flags = { textureViews[4] ? 1.0f : 0.0f,
                textureViews[5] ? 1.0f : 0.0f, 0.0f, 0.0f };
            applyPrimaryMaterialUvState( material, state, nativeMaterial );
            if( dx11 )
                wp_renderer_dx11_set_material( dx11, &nativeMaterial );
            auto blend = material ? ClawUtil::toCBlendMode( material->getBlendMode() ) : WORKPHONE_BLEND_MODE_NONE;
            if( material && material->isTransparent() && blend == WORKPHONE_BLEND_MODE_NONE )
                blend = WORKPHONE_BLEND_MODE_ALPHA;
            wp_renderer_set_blend_mode( m_renderer, blend );
            wp_renderer_set_depth_write_enabled( m_renderer, m_shadowPass || !material || material->getDepthWrite() );
            // The editor's comparison list differs from the C renderer enum.
            constexpr wp_depth_func depthFunctions[] = { WORKPHONE_DEPTH_FUNC_NEVER,
                WORKPHONE_DEPTH_FUNC_LESS, WORKPHONE_DEPTH_FUNC_LEQUAL, WORKPHONE_DEPTH_FUNC_EQUAL,
                WORKPHONE_DEPTH_FUNC_GEQUAL, WORKPHONE_DEPTH_FUNC_GREATER, WORKPHONE_DEPTH_FUNC_ALWAYS };
            const auto depthTest = material ? material->getDepthTest() : 2u;
            wp_renderer_set_depth_test_enabled( m_renderer, 1 );
            wp_renderer_set_depth_func( m_renderer, m_shadowPass ? WORKPHONE_DEPTH_FUNC_LESS : depthFunctions[std::min( depthTest, 6u )] );
            wp_renderer_set_fill_mode( m_renderer, !m_shadowPass && state && state->getFlag( showWireframeFlag )
                ? WORKPHONE_FILL_MODE_WIREFRAME : WORKPHONE_FILL_MODE_SOLID );
            wp_renderer_set_cull_mode( m_renderer, isMaterialDoubleSided( material )
                                                       ? WORKPHONE_CULL_MODE_NONE
                                                       : material ? ClawUtil::toCCullMode( material->getCullMode() )
                                                                  : WORKPHONE_CULL_MODE_BACK );

            if( topology == WORKPHONE_PRIMITIVE_TRIANGLE_STRIP && count >= 3 )
            {
                Array<wp_vertex_pntc> vertices;
                vertices.reserve( static_cast<size_t>( count - 2u ) * 3u );
                for( u32 i = 0; i + 2u < count; ++i )
                {
                    auto a = readMeshIndex( nativeMesh, start + i );
                    auto b = readMeshIndex( nativeMesh, start + i + 1u );
                    auto c = readMeshIndex( nativeMesh, start + i + 2u );
                    if( i & 1u )
                    {
                        std::swap( a, b );
                    }
                    appendTriangle( vertices, meshVertices, a, b, c );
                }

                if( !vertices.empty() &&
                    vertices.size() <= static_cast<size_t>( std::numeric_limits<wp_s32>::max() ) )
                {
                    drawLitTriangles( dx11, vertices );
                    m_primitiveCount += static_cast<u32>( vertices.size() / 3u );
                }
            }
            else if( topology == WORKPHONE_PRIMITIVE_TRIANGLE_LIST )
            {
                count -= count % 3u;
                if( count == 0 )
                {
                    continue;
                }

                if( geometry )
                {
                    wp_renderer_dx11_draw_geometry_pntc( dx11, geometry, static_cast<wp_s32>( start ),
                                                         static_cast<wp_s32>( count ), 0 );
                }
                else
                {
                    // Retry with transient geometry while retaining normals and PBR shading.
                    Array<wp_vertex_pntc> fallback;
                    fallback.reserve( count );
                    for( u32 i = 0; i + 2 < count; i += 3 )
                    {
                        appendTriangle( fallback, meshVertices, readMeshIndex( nativeMesh, start + i ),
                                        readMeshIndex( nativeMesh, start + i + 1 ),
                                        readMeshIndex( nativeMesh, start + i + 2 ) );
                    }
                    drawLitTriangles( dx11, fallback );
                }

                m_primitiveCount += count / 3u;
            }
        }

        wp_renderer_set_texture_native( m_renderer, nullptr );
        // Retain resolved texture/sampler bindings for the next mesh. Target
        // and external UI boundaries invalidate the native binding cache.
        wp_renderer_set_blend_mode( m_renderer, oldBlend );
        wp_renderer_set_cull_mode( m_renderer, oldCull );
        wp_renderer_set_depth_write_enabled( m_renderer, oldDepthWrite );
        wp_renderer_set_depth_test_enabled( m_renderer, oldDepthTest );
        wp_renderer_set_depth_func( m_renderer, oldDepthFunc );
        wp_renderer_set_fill_mode( m_renderer, oldFill );
    }

    void ClawRendererDX11::renderTerrain( const SmartPtr<ClawTerrain> &terrain )
    {
        if( !terrain || !terrain->isVisible() ) return;
        auto nativeMesh = terrain->getNativeRenderMesh();
        auto dx11 = m_renderer ? wp_renderer_get_dx11( m_renderer ) : nullptr;
        if( !nativeMesh || !dx11 )
            return;

        const auto &vertices = getMeshVertices( nativeMesh );
        const auto indices = wp_graphics_mesh_get_indices( nativeMesh );
        const auto indexCount = wp_graphics_mesh_get_index_count( nativeMesh );
        if( vertices.empty() || !indices || indexCount == 0 ||
            vertices.size() > static_cast<size_t>( std::numeric_limits<wp_s32>::max() ) ||
            indexCount > static_cast<u32>( std::numeric_limits<wp_s32>::max() ) )
            return;

        const auto geometry = getMeshGeometry( dx11, nativeMesh, vertices );
        if( !geometry )
            return;

        const Matrix4F transform( terrain->getWorldTransform().getTransformationMatrix().ptr() );
        setTransforms( transform );
        void *nativeTexture = nullptr;
        if( auto texture = terrain->getTexture( 0 ) )
            texture->getTextureFinal( &nativeTexture );
        wp_renderer_set_texture_native( m_renderer, nativeTexture );

        wp_renderer_dx11_enable_shadow_receiving( dx11, m_shadowsEnabled );
        wp_renderer_dx11_set_material_textures( dx11, nullptr );
        wp_material_dx11 material{};
        material.base_color = { 1.0f, 1.0f, 1.0f, 1.0f };
        material.specular_color = { 0.04f, 0.04f, 0.04f, 1.0f };
        applySceneLighting( material );
        material.surface = { 0.0f, 0.85f, 8.0f, 8.0f };
        material.uv_transform = { 0.0f, 0.0f, 1.0f, 0.0f };
        wp_renderer_dx11_set_material( dx11, &material );

        const auto oldCull = wp_renderer_get_cull_mode( m_renderer );
        const auto oldDepthTest = wp_renderer_get_depth_test_enabled( m_renderer );
        const auto oldDepthWrite = wp_renderer_get_depth_write_enabled( m_renderer );
        const auto oldDepthFunc = wp_renderer_get_depth_func( m_renderer );
        const auto oldFill = wp_renderer_get_fill_mode( m_renderer );
        wp_renderer_set_cull_mode( m_renderer, WORKPHONE_CULL_MODE_NONE );
        if( m_shadowPass )
        {
            wp_renderer_set_depth_test_enabled( m_renderer, 1 );
            wp_renderer_set_depth_write_enabled( m_renderer, 1 );
            wp_renderer_set_depth_func( m_renderer, WORKPHONE_DEPTH_FUNC_LESS );
            wp_renderer_set_fill_mode( m_renderer, WORKPHONE_FILL_MODE_SOLID );
        }
        wp_renderer_dx11_draw_geometry_pntc( dx11, geometry, 0, static_cast<wp_s32>( indexCount ), 0 );
        wp_renderer_set_cull_mode( m_renderer, oldCull );
        wp_renderer_set_depth_test_enabled( m_renderer, oldDepthTest );
        wp_renderer_set_depth_write_enabled( m_renderer, oldDepthWrite );
        wp_renderer_set_depth_func( m_renderer, oldDepthFunc );
        wp_renderer_set_fill_mode( m_renderer, oldFill );
        wp_renderer_set_texture_native( m_renderer, nullptr );
        m_primitiveCount += indexCount / 3;
    }

    void ClawRendererDX11::renderSky( const SmartPtr<ISky> &sky )
    {
        auto camera = getCamera();

        if( !m_renderer || !camera || !sky || !sky->isVisible() )
        {
            return;
        }

        auto textures = sky->getTextures();
        if( textures.empty() )
        {
            if( auto material = sky->getMaterial() )
            {
                textures = material->getCubicTextures();
            }
        }
        if( textures.empty() )
        {
            return;
        }

        if( auto dx11 = wp_renderer_get_dx11( m_renderer ) )
        {
            std::array<ID3D11ShaderResourceView *, 6> views{};
            for( size_t face = 0; face < std::min( textures.size(), views.size() ); ++face )
                if( textures[face] )
                {
                    void *view = nullptr;
                    textures[face]->getTextureFinal( &view );
                    views[face] = static_cast<ID3D11ShaderResourceView *>( view );
                }
            auto &cache = g_skyEnvironmentCaches[this];
            auto &entry = cache.skies[sky.get()];
            if( entry.sky.lock() != sky )
            {
                entry.cubemap.reset();
                entry.sky = sky;
            }
            m_hasSkyEnvironment = entry.cubemap.update( static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( dx11 ) ),
                static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( dx11 ) ), views );
            cache.active = m_hasSkyEnvironment ? &entry.cubemap : nullptr;
        }

        const auto farClip = std::max( camera->getFarClipDistance(), 10.0f );
        const auto requestedDistance = std::max( sky->getDistance(), 1.0f );
        const auto size = std::min( requestedDistance, farClip * 0.1f );
        Matrix4F world;
        world.makeTransform( Vector3F::zero(), Vector3F( size, size, size ), QuaternionF::identity() );
        setTransforms( world );

        // Keep the sky centered on the camera while retaining its current rotation.
        // The node's C++ position cache may lag behind the native view matrix.
        auto skyView = Matrix4F( camera->getViewMatrix().ptr() );
        skyView[0][3] = 0.0f;
        skyView[1][3] = 0.0f;
        skyView[2][3] = 0.0f;
        const auto nativeSkyView = toCMatrix( skyView );
        wp_renderer_set_view_matrix( m_renderer, &nativeSkyView );

        const auto oldCull = wp_renderer_get_cull_mode( m_renderer );
        const auto oldDepthTest = wp_renderer_get_depth_test_enabled( m_renderer );
        const auto oldDepthWrite = wp_renderer_get_depth_write_enabled( m_renderer );
        wp_renderer_set_cull_mode( m_renderer, WORKPHONE_CULL_MODE_NONE );
        wp_renderer_set_depth_test_enabled( m_renderer, 0 );
        wp_renderer_set_depth_write_enabled( m_renderer, 0 );

        constexpr u32 white = 0xFFFFFFFFu;
        const wp_vec2f uv00 = { 0.0f, 0.0f };
        const wp_vec2f uv10 = { 1.0f, 0.0f };
        const wp_vec2f uv11 = { 1.0f, 1.0f };
        const wp_vec2f uv01 = { 0.0f, 1.0f };
        const wp_vec3f corners[8] = {
            { -1.0f, -1.0f, -1.0f }, { 1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, -1.0f },
            { -1.0f, 1.0f, -1.0f },  { -1.0f, -1.0f, 1.0f }, { 1.0f, -1.0f, 1.0f },
            { 1.0f, 1.0f, 1.0f },    { -1.0f, 1.0f, 1.0f },
        };
        const u8 faces[6][4] = {
            { 0, 1, 2, 3 },  // front (-Z)
            { 5, 4, 7, 6 },  // back (+Z)
            { 4, 0, 3, 7 },  // left (-X)
            { 1, 5, 6, 2 },  // right (+X)
            { 3, 2, 6, 7 },  // up (+Y)
            { 4, 5, 1, 0 },  // down (-Y)
        };

        for( u32 face = 0; face < 6; ++face )
        {
            if( face >= textures.size() || !textures[face] )
            {
                continue;
            }

            void *nativeTexture = nullptr;
            textures[face]->getTextureFinal( &nativeTexture );
            if( !nativeTexture )
            {
                continue;
            }
            wp_renderer_set_texture_native( m_renderer, nativeTexture );

            const auto &indices = faces[face];
            const wp_vertex_ptc vertices[6] = {
                { corners[indices[0]], uv01, white }, { corners[indices[1]], uv11, white },
                { corners[indices[2]], uv10, white }, { corners[indices[0]], uv01, white },
                { corners[indices[2]], uv10, white }, { corners[indices[3]], uv00, white },
            };
            wp_renderer_draw_triangles_ptc( m_renderer, vertices, 6 );
            m_primitiveCount += 2;
        }

        wp_renderer_set_texture_native( m_renderer, nullptr );
        wp_renderer_set_cull_mode( m_renderer, oldCull );
        wp_renderer_set_depth_test_enabled( m_renderer, oldDepthTest );
        wp_renderer_set_depth_write_enabled( m_renderer, oldDepthWrite );
    }

    void ClawRendererDX11::setTransforms( const Matrix4F &world )
    {
        auto camera = getCamera();
        const auto nativeWorld = toCMatrix( world );
        const auto view = !m_shadowPass && camera ? Matrix4F( camera->getViewMatrix().ptr() ) : Matrix4F::identity();
        const auto projection = m_shadowPass ? m_shadowMatrix :
            camera ? Matrix4F( camera->getProjectionMatrix().ptr() ) : Matrix4F::identity();
        const auto nativeView = toCMatrix( view );
        const auto nativeProjection = toCMatrix( projection );

        wp_renderer_set_world_matrix( m_renderer, &nativeWorld );
        wp_renderer_set_view_matrix( m_renderer, &nativeView );
        wp_renderer_set_projection_matrix( m_renderer, &nativeProjection );
    }

    wp_mat4f ClawRendererDX11::toCMatrix( const Matrix4F &matrix )
    {
        wp_mat4f result;
        std::memcpy( result.m, matrix.ptr(), sizeof( result.m ) );
        return result;
    }

    u32 ClawRendererDX11::packColour( const ColourF &colour )
    {
        const auto toByte = []( f32 value ) {
            return static_cast<u32>( std::clamp( value, 0.0f, 1.0f ) * 255.0f + 0.5f );
        };

        return ( toByte( colour.r ) << 24 ) | ( toByte( colour.g ) << 16 ) |
               ( toByte( colour.b ) << 8 ) | toByte( colour.a );
    }
}  // namespace workphone::render
