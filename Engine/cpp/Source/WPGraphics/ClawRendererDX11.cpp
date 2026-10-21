#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawMaterialRenderCache.hpp>
#include <WPGraphics/ClawHammerSystem.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/ClawTerrain.hpp>
#include <WPGraphics/ClawTerrainMaterial.hpp>
#include <WPGraphics/ClawWindow.hpp>
#include <WPGraphics/ClawUtil.hpp>
#include <Workphone/Graphics/FoliageData.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include "ClawMeshMaterial.hpp"
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
    bool ClawRendererDX11::setSplitMaterialUploads( bool enabled )
    {
        return m_renderer && wp_renderer_dx11_set_material_upload_mode(
            wp_renderer_get_dx11( m_renderer ), enabled ? WP_DX11_MATERIAL_UPLOAD_SPLIT : WP_DX11_MATERIAL_UPLOAD_COMBINED );
    }
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawRendererDX11, IRenderer3 );

    namespace
    {
        constexpr u16 k_quadIndices[] = { 0, 1, 2, 0, 2, 3 };

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

        MaterialRenderState::Ptr getPrimaryMaterialPassState( const SmartPtr<IMaterial> &material )
        {
            if( auto concrete = dynamic_pointer_cast<Material>( material ) )
                return concrete->getRenderState();
            // Third-party material implementations retain the interface fallback.
            if( material ) for( const auto &technique : material->getTechniques() )
                if( technique ) for( const auto &pass : technique->getPasses() )
                    if( auto context = pass ? pass->getStateContext() : nullptr )
                        if( auto state = context->getStateDataById<MaterialPassStateData>( pass->getId() ) )
                            return state->getRenderState();
            return {};
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
        {
            WP_LOG_WARNING(
                "WPGraphics/DX11: no native window handle; device initialization may fail." );
        }
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
        m_materialRenderCache.reset();
        m_directMaterialRecords.reset();
        g_skyEnvironmentCaches.erase( this );
        m_environment.reset();
        m_previewEnvironment.reset();
        if( m_renderer )
        {
            wp_renderer_destroy( m_renderer );
            m_renderer = nullptr;
        }
        m_foliageStreamBudget.reset();
    }

    void ClawRendererDX11::beginRender()
    {
        if( !m_renderer || m_inFrame )
        {
            return;
        }

        m_primitiveCount = 0;
        if( m_materialRenderCache ) m_materialRenderCache->maintain();
        m_hasSkyEnvironment = false;
        if( auto found = g_skyEnvironmentCaches.find( this ); found != g_skyEnvironmentCaches.end() )
        {
            auto &cache = found->second;
            cache.active = nullptr;
            for( auto it = cache.skies.begin(); it != cache.skies.end(); )
            {
                if( !it->second.sky.lock() )
                {
                    it = cache.skies.erase( it );
                }
                else
                {
                    ++it;
                }
            }
        }
        m_inFrame = true;
        ++m_terrainFrame;
        wp_renderer_begin_frame( m_renderer );
        if( auto dx11 = wp_renderer_get_dx11( m_renderer ) )
        {
            wp_renderer_dx11_set_environment( dx11, nullptr, 0.0f );
        }
    }

    void ClawRendererDX11::endRender()
    {
        if( !m_renderer || !m_inFrame )
        {
            return;
        }

        // The generic end_frame also Presents with VSync forced on. The window
        // adapter owns presentation policy, so finish only the DX11 pass here.
        wp_renderer_dx11_end_frame( wp_renderer_get_dx11( m_renderer ) );

        if( auto window = dynamic_pointer_cast<IGraphicsWindow>( m_renderTarget.load() ) )
        {
            if( auto dx11 = wp_renderer_get_dx11( m_renderer ) )
            {
                auto app = core::IApplicationManager::instancePtr();
                auto graphics = app ? dynamic_cast<ClawHammerSystem *>(
                                          app->getGraphicsSystemPtr() ) : nullptr;
                void *handle = nullptr;
                window->getWindowHandle( &handle );
                const auto result = wp_renderer_dx11_try_present(
                    dx11, handle, graphics && graphics->getVSync() ? 1 : 0 );
                if( result < 0 )
                {
                    wp_render_statistics_dx11 statistics{};
                    wp_renderer_dx11_get_statistics( dx11, &statistics );
                    WP_LOG_ERROR( "WPGraphics/DX11: Present failed; HRESULT=" +
                                  std::to_string( statistics.last_present_hresult ) );
                }
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
                    // A rejected resize retains the old swap-chain buffers.
                    // Publish their actual extent rather than the requested size.
                    m_windowWidth = static_cast<u32>( wp_renderer_dx11_get_width( dx11 ) );
                    m_windowHeight = static_cast<u32>( wp_renderer_dx11_get_height( dx11 ) );
                    m_rtWidth = m_windowWidth;
                    m_rtHeight = m_windowHeight;
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
        wp_renderer_dx11_forget_mesh( mesh );
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
                                            const Matrix4F &world, const Vector3F &scale,
                                            const SmartPtr<IMaterial> &material )
    {
        if( !m_renderer || particles.empty() )
        {
            return;
        }
        const auto view = m_camera ? Matrix4F( m_camera->getViewMatrix().ptr() ) : Matrix4F::identity();
        const auto inverseView = view.inverse();
        const Vector3F right( inverseView[0][0], inverseView[1][0], inverseView[2][0] );
        const Vector3F up( inverseView[0][1], inverseView[1][1], inverseView[2][1] );

        struct DrawParticle
        {
            Vector3F center;
            f32 depth;
            const wp_particle_sample *particle;
        };

        Array<DrawParticle> draw;
        draw.reserve( particles.size() );
        for( const auto &particle : particles )
        {
            const auto center = world.transformAffine( Vector3F( particle.position.x * scale.X(),
                                                                 particle.position.y * scale.Y(),
                                                                 particle.position.z * scale.Z() ) );
            // Native camera inversion can round the affine bottom-right element.
            // Depth only needs the view's third row, without an exact-affine assertion.
            const auto depth =
                view[2][0] * center.X() + view[2][1] * center.Y() + view[2][2] * center.Z() + view[2][3];
            draw.push_back( { center, depth, &particle } );
        }
        std::stable_sort( draw.begin(), draw.end(),
                          []( const auto &a, const auto &b ) { return a.depth < b.depth; } );
        const auto oldBlend = wp_renderer_get_blend_mode( m_renderer );
        const auto oldCull = wp_renderer_get_cull_mode( m_renderer );
        const auto oldDepthWrite = wp_renderer_get_depth_write_enabled( m_renderer );
        const auto oldDepthTest = wp_renderer_get_depth_test_enabled( m_renderer );
        const auto oldDepthFunc = wp_renderer_get_depth_func( m_renderer );
        const auto oldFill = wp_renderer_get_fill_mode( m_renderer );
        // Particle geometry uses its own unlit vertex-colour shader. Borrow the
        // same immutable surface/resource version without applying PBR constants.
        const auto particleState = getPrimaryMaterialPassState(material);
        auto blend = material ? ClawUtil::toCBlendMode(particleState ? particleState->blendMode : material->getBlendMode())
                              : WORKPHONE_BLEND_MODE_ALPHA;
        if( blend == WORKPHONE_BLEND_MODE_NONE )
        {
            blend = WORKPHONE_BLEND_MODE_ALPHA;
        }
        wp_renderer_set_blend_mode( m_renderer, blend );
        wp_renderer_set_cull_mode( m_renderer, WORKPHONE_CULL_MODE_NONE );
        wp_renderer_set_depth_test_enabled( m_renderer, 1 );
        wp_renderer_set_depth_write_enabled( m_renderer, 0 );
        wp_renderer_set_depth_func( m_renderer, WORKPHONE_DEPTH_FUNC_LEQUAL );
        wp_renderer_set_fill_mode( m_renderer, WORKPHONE_FILL_MODE_SOLID );
        void *texture = nullptr;
        if( material )
        {
            SmartPtr<ITexture> fallback;
            auto input = particleState ? particleState->textures[0].get() :
                         (fallback = material->getTexture(0)).get();
            if( input )
            {
                input->getTextureFinal( &texture );
            }
        }
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
                const Vector3F corners[] = { draw[i].center - r - u, draw[i].center + r - u,
                                             draw[i].center + r + u, draw[i].center - r + u };
                const wp_vec2f uv[] = { { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 } };
                const auto colour =
                    packColour( ColourF( p.color[0], p.color[1], p.color[2], p.color[3] ) );
                for( auto index : k_quadIndices )
                {
                    vertices.push_back( { { corners[index].X(), corners[index].Y(), corners[index].Z() },
                                          uv[index],
                                          colour } );
                }
            }
            wp_renderer_draw_triangles_ptc( m_renderer, vertices.data(),
                                            static_cast<wp_s32>( vertices.size() ) );
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

    void ClawRendererDX11::setSceneFog( u32 mode, const ColourF &colour, f32 density, f32 start,
                                        f32 end )
    {
        m_fogColour = { colour.r, colour.g, colour.b, 1.f };
        m_fogParams = { static_cast<f32>( std::min( mode, 3u ) ), std::max( density, 0.f ),
                        std::max( start, 0.f ), std::max( end, start + .001f ) };
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
        material.fog_color = m_fogColour;
        material.fog_params = m_shadowPass ? wp_vec4f{} : m_fogParams;
        auto dx11 = wp_renderer_get_dx11( m_renderer );
        if( dx11 && !m_hasSkyEnvironment && !m_previewEnvironment.getView() )
        {
            m_previewEnvironment.update(
                static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( dx11 ) ),
                static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( dx11 ) ), {} );
        }
        if( dx11 )
        {
            const ClawCubemap *environment = &m_previewEnvironment;
            if( m_hasSkyEnvironment )
            {
                if( auto found = g_skyEnvironmentCaches.find( this );
                    found != g_skyEnvironmentCaches.end() )
                {
                    if( found->second.active )
                    {
                        environment = found->second.active;
                    }
                }
            }
            wp_renderer_dx11_set_environment( dx11, environment->getView(), environment->getMaxLod() );
        }
    }

    bool ClawRendererDX11::beginShadowMap( s32 mapSize )
    {
        disableShadows();
        auto camera = getCamera();
        auto dx11 = m_renderer ? wp_renderer_get_dx11( m_renderer ) : nullptr;
        if( !dx11 || !camera || m_lightDirection.dotProduct( m_lightDirection ) < 1e-8f )
        {
            return false;
        }
        mapSize = std::clamp( mapSize, 64, 8192 );
        const auto direction = m_lightDirection.normaliseCopy();
        const Vector3F up =
            std::abs( direction.Y() ) > 0.95f ? Vector3F( 1, 0, 0 ) : Vector3F( 0, 1, 0 );
        // Match the camera's screen-space winding while depth increases along
        // the light direction, so single-sided casters retain their front faces.
        const auto right = direction.crossProduct( up ).normaliseCopy();
        const auto vertical = right.crossProduct( direction );
        auto lightView = Matrix4F::identity();
        for( int i = 0; i < 3; ++i )
        {
            lightView[0][i] = right[i];
            lightView[1][i] = vertical[i];
            lightView[2][i] = direction[i];
        }
        const auto inverse = ( Matrix4F( camera->getProjectionMatrix().ptr() ) *
                               Matrix4F( camera->getViewMatrix().ptr() ) )
                                 .inverse();
        const auto transformPoint = []( const Matrix4F &matrix, const Vector3F &point ) {
            const float w = matrix[3][0] * point[0] + matrix[3][1] * point[1] + matrix[3][2] * point[2] +
                            matrix[3][3];
            Vector3F result;
            for( int i = 0; i < 3; ++i )
            {
                result[i] = ( matrix[i][0] * point[0] + matrix[i][1] * point[1] +
                              matrix[i][2] * point[2] + matrix[i][3] ) /
                            w;
            }
            return result;
        };
        Vector3F minimum( 1e30f, 1e30f, 1e30f ), maximum( -1e30f, -1e30f, -1e30f );
        const float nearClip = camera->getNearClipDistance();
        const float farClip = camera->getFarClipDistance();
        const float fraction = std::clamp(
            ( std::min( farClip, 200.0f ) - nearClip ) / std::max( farClip - nearClip, 0.001f ), 0.001f,
            1.0f );
        for( int y = -1; y <= 1; y += 2 )
        {
            for( int x = -1; x <= 1; x += 2 )
            {
                const auto nearPoint = transformPoint(
                    inverse, Vector3F( static_cast<float>( x ), static_cast<float>( y ), -1 ) );
                const auto farPoint = transformPoint(
                    inverse, Vector3F( static_cast<float>( x ), static_cast<float>( y ), 1 ) );
                for( const auto &point : { nearPoint, nearPoint + ( farPoint - nearPoint ) * fraction } )
                {
                    const auto p = transformPoint( lightView, point );
                    for( int i = 0; i < 3; ++i )
                    {
                        minimum[i] = std::min( minimum[i], p[i] );
                        maximum[i] = std::max( maximum[i], p[i] );
                    }
                }
            }
        }
        // Leave depth room for casters outside the camera frustum toward the light.
        minimum[2] -= 500.0f;
        maximum[2] += 50.0f;
        auto projection = Matrix4F::identity();
        for( int i = 0; i < 3; ++i )
        {
            const float extent = std::max( maximum[i] - minimum[i], 1.0f ) + 2.0f;
            float center = ( minimum[i] + maximum[i] ) * 0.5f;
            if( i < 2 )
            {
                center = std::floor( center / ( extent / static_cast<float>( mapSize ) ) ) *
                         ( extent / static_cast<float>( mapSize ) );
            }
            projection[i][i] = 2.0f / extent;
            projection[i][3] = -2.0f * center / extent;
        }
        m_shadowMatrix = projection * lightView;
        const auto matrix = toCMatrix( m_shadowMatrix );
        m_shadowPass = wp_renderer_dx11_begin_shadow_map( dx11, &matrix, mapSize ) != 0;
        return m_shadowPass;
    }

    void ClawRendererDX11::endShadowMap()
    {
        if( !m_shadowPass )
        {
            return;
        }
        wp_renderer_dx11_end_shadow_map( wp_renderer_get_dx11( m_renderer ) );
        m_shadowPass = false;
        m_shadowsEnabled = true;
        setTransforms( Matrix4F::identity() );
    }

    void ClawRendererDX11::disableShadows()
    {
        m_shadowsEnabled = false;
        if( m_renderer )
        {
            wp_renderer_dx11_enable_shadow_receiving( wp_renderer_get_dx11( m_renderer ), 0 );
        }
    }

    void ClawRendererDX11::renderMeshSnapshot( ClawMesh *mesh, const Matrix4F &world,
                                               const wp_mat4f &view, const wp_mat4f &projection )
    {
        renderMeshBatch( mesh, world, nullptr, 0, &view, &projection );
    }

    void ClawRendererDX11::renderMesh( ClawMesh *mesh, const Matrix4F &transform )
    {
        renderMeshBatch( mesh, transform, nullptr, 0 );
    }

    Matrix4F ClawRendererDX11::foliageViewProjection( bool shadow ) const
    {
        if( shadow && m_shadowPass )
        {
            return m_shadowMatrix;
        }
        const auto camera = getCamera();
        return camera ? Matrix4F( camera->getProjectionMatrix().ptr() ) *
                            Matrix4F( camera->getViewMatrix().ptr() )
                      : Matrix4F::identity();
    }

    u32 ClawRendererDX11::renderFoliageGeometry( const wp_geometry_dx11 *geometry, u32 first,
                                                 u32 indices, const wp_foliage_instance_dx11 *instances,
                                                 u32 count, const wp_foliage_parameters_dx11 &parameters,
                                                 const FoliageMaterial &source, void *const *views,
                                                 bool unlit, bool atlas )
    {
        auto native = m_renderer ? wp_renderer_get_dx11( m_renderer ) : nullptr;
        if( !native || !geometry || !count || count > foliageMaximumInstances || !views ||
            !wp_renderer_dx11_validate_foliage_instances( instances, s32( count ), &parameters ) )
        {
            return 0;
        }
        if( !m_foliageStreamBudget )
        {
            String error;
            m_foliageStreamBudget = WorldStreamingScheduler::shared()->reserve(
                StreamMemoryClass::Gpu, WP_DX11_INSTANCE_CAPACITY * 160ull + sizeof( parameters ),
                error );
            if( !m_foliageStreamBudget )
            {
                return 0;
            }
        }
        setTransforms( Matrix4F::identity() );
        wp_renderer_dx11_set_terrain_material( native, nullptr, nullptr );
        wp_renderer_set_texture_native( m_renderer, views[0] );
        void *maps[6] = { views[1], views[2], views[2], nullptr, views[2], nullptr };
        wp_renderer_dx11_set_material_textures( native, maps );
        wp_renderer_dx11_set_material_sampler( native, atlas ? 1 : 0, atlas ? 1 : 0, 2, 1 );
        wp_material_dx11 material{};
        material.base_color = { source.tint.r, source.tint.g, source.tint.b, source.tint.a };
        material.specular_color = { .04f, .04f, .04f, 0 };
        material.surface = { source.metalness, source.roughness, 1, 1 };
        material.uv_transform = { 0, 0, unlit ? 0.0f : 1.0f, 0 };
        material.controls = { 1, 1, source.alphaCutoff, 0 };
        material.map_flags = { views[1] ? 1.0f : 0.0f, views[2] ? 1.0f : 0.0f, views[2] ? 1.0f : 0.0f,
                               0 };
        material.extra_map_flags = { views[2] ? 1.0f : 0.0f, 0, 0, 0 };
        material.texture_sources = { 4, 3, 0, 0 };
        applySceneLighting( material );
        wp_renderer_dx11_set_material( native, &material );
        wp_renderer_dx11_enable_shadow_receiving( native, m_shadowsEnabled );
        wp_renderer_set_blend_mode( m_renderer, WORKPHONE_BLEND_MODE_NONE );
        wp_renderer_set_depth_write_enabled( m_renderer, 1 );
        wp_renderer_set_depth_test_enabled( m_renderer, 1 );
        wp_renderer_set_depth_func(
            m_renderer, m_shadowPass ? WORKPHONE_DEPTH_FUNC_LESS : WORKPHONE_DEPTH_FUNC_LEQUAL );
        wp_renderer_set_cull_mode(
            m_renderer, source.twoSided ? WORKPHONE_CULL_MODE_NONE : WORKPHONE_CULL_MODE_BACK );
        wp_renderer_set_fill_mode( m_renderer, WORKPHONE_FILL_MODE_SOLID );
        const auto submitted = wp_renderer_dx11_draw_foliage_instances(
            native, geometry, s32( first ), s32( indices ), instances, s32( count ), &parameters );
        m_primitiveCount =
            u32( std::min<u64>( std::numeric_limits<u32>::max(),
                                u64( m_primitiveCount ) + u64( indices / 3 ) * submitted ) );
        wp_renderer_set_texture_native( m_renderer, nullptr );
        wp_renderer_dx11_set_material_textures( native, nullptr );
        auto context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( native ) );
        ID3D11Buffer *empty = nullptr;
        UINT stride = 0, offset = 0;
        ID3D11ShaderResourceView *emptyView = nullptr;
        context->PSSetShaderResources( 0, 1, &emptyView );
        context->IASetVertexBuffers( 0, 1, &empty, &stride, &offset );
        context->IASetIndexBuffer( nullptr, DXGI_FORMAT_R32_UINT, 0 );
        return u32( submitted );
    }

    u64 ClawRendererDX11::renderMeshInstances( ClawMesh *mesh, const wp_instance_pntc_dx11 *instances,
                                               u32 count )
    {
        if( !mesh || !count || count > 65536 ||
            !wp_renderer_dx11_validate_instances_pntc( instances, static_cast<wp_s32>( count ) ) )
        {
            return 0;
        }
        const auto native = mesh->getNativeMesh();
        if( !native ||
            wp_graphics_mesh_get_primitive_type( native ) != WORKPHONE_PRIMITIVE_TRIANGLE_LIST ||
            !wp_graphics_mesh_get_index_count( native ) )
        {
            return 0;
        }
        // Snapshot-based transparency validation shares the captured draw records.
        return renderMeshBatch( mesh, Matrix4F::identity(), instances, count );
    }

    void ClawRendererDX11::prepareMaterialRecords( ClawMaterialDrawRecords &records )
    {
        if( records.getRecordCount() && !m_materialRenderCache )
            m_materialRenderCache = std::make_unique<ClawMaterialRenderCache>();
        records.seal( m_materialRenderCache.get() );
    }

    ClawRendererDX11::MaterialRecordStatistics ClawRendererDX11::getMaterialRecordStatistics() const
    {
        MaterialRecordStatistics result;
        if(m_materialRenderCache)
        {
            result.preparations = m_materialRenderCache->getPreparationCount();
            result.storageBytes = m_materialRenderCache->getStorageBytes();
            result.storageAllocations = m_materialRenderCache->getStorageAllocationCount();
            result.slots = m_materialRenderCache->getSlotCount();
            result.resident = m_materialRenderCache->getResidentCount();
        }
        if(m_directMaterialRecords) result.directStorageAllocations = m_directMaterialRecords->getStorageAllocationCount();
        return result;
    }

    bool ClawRendererDX11::renderMeshRecords( ClawMesh *mesh, const Matrix4F &transform,
        const ClawMaterialDrawRecords &records, const ClawMaterialDrawRecords::Binding &binding,
        const wp_mat4f *view, const wp_mat4f *projection )
    {
        const auto native = mesh ? mesh->getNativeMesh() : nullptr;
        if( !native || !records.valid( binding, mesh,
                static_cast<u32>( std::max<s32>( 1, wp_graphics_mesh_get_submesh_count( native ) ) ) ) )
            return false;
        renderMeshBatch( mesh, transform, nullptr, 0, view, projection, &records, &binding );
        return true;
    }

    u64 ClawRendererDX11::renderMeshBatch( ClawMesh *mesh, const Matrix4F &transform,
                                           const wp_instance_pntc_dx11 *instances, u32 instanceCount,
                                           const wp_mat4f *snapshotView,
                                           const wp_mat4f *snapshotProjection,
                                           const ClawMaterialDrawRecords *records,
                                           const ClawMaterialDrawRecords::Binding *binding )
    {
        auto nativeMesh = mesh ? mesh->getNativeMesh() : nullptr;
        if( !m_renderer || !mesh || !mesh->isVisible() || !nativeMesh )
        {
            return 0;
        }

        const auto vertexCount = wp_graphics_mesh_get_vertex_count( nativeMesh );
        const auto indexCount = wp_graphics_mesh_get_index_count( nativeMesh );
        const auto elementCount = indexCount > 0 ? indexCount : vertexCount;
        if( vertexCount == 0 || elementCount == 0 )
        {
            return 0;
        }

        // Preview/UI/direct/instanced meshes use the same publication and persistent
        // records as scene jobs. Keep one idle workspace; a reentrant draw borrows
        // another without modifying the outer draw's immutable publication.
        auto directRecords = records ? nullptr : std::move(m_directMaterialRecords);
        struct RetireDirect
        {
            std::unique_ptr<ClawMaterialDrawRecords> &active, &idle;
            ~RetireDirect()
            {
                if(active)
                {
                    try { active->clear(); if(!idle) idle = std::move(active); }
                    catch(...) {} // Do not recycle a publication whose clear failed.
                }
            }
        } retireDirect{directRecords, m_directMaterialRecords};
        if(!records)
        {
            if(!directRecords) directRecords = std::make_unique<ClawMaterialDrawRecords>();
            directRecords->capture(mesh);
            prepareMaterialRecords(*directRecords);
            records = directRecords.get(); binding = records->find(mesh);
            if(!binding) return 0;
        }
        if(instances)
            for(u32 section = 0; section < binding->count; ++section)
            {
                const auto record = records->get(*binding, section);
                if(!record) return 0;
                if(record->state)
                {
                    if(record->state->getFlag(transparentFlag) ||
                       ClawUtil::toCBlendMode(record->state->blendMode) != WORKPHONE_BLEND_MODE_NONE) return 0;
                }
                else if(record->material && (record->material->isTransparent() ||
                        ClawUtil::toCBlendMode(record->material->getBlendMode()) != WORKPHONE_BLEND_MODE_NONE)) return 0;
            }

        u64 submitted = 0;
        setTransforms( transform, snapshotView, snapshotProjection );
        const auto topology = wp_graphics_mesh_get_primitive_type( nativeMesh );
        const auto submeshCount = wp_graphics_mesh_get_submesh_count( nativeMesh );
        const auto drawCount = std::max<s32>( submeshCount, 1 );
        const auto oldBlend = wp_renderer_get_blend_mode( m_renderer );
        const auto oldCull = wp_renderer_get_cull_mode( m_renderer );
        const auto oldDepthWrite = wp_renderer_get_depth_write_enabled( m_renderer );
        const auto oldDepthTest = wp_renderer_get_depth_test_enabled( m_renderer );
        const auto oldDepthFunc = wp_renderer_get_depth_func( m_renderer );
        const auto oldFill = wp_renderer_get_fill_mode( m_renderer );
        wp_renderer_set_blend_mode( m_renderer, WORKPHONE_BLEND_MODE_NONE );

        auto dx11 = wp_renderer_get_dx11( m_renderer );
        const auto geometry = topology == WORKPHONE_PRIMITIVE_TRIANGLE_LIST
            ? wp_renderer_dx11_prepare_mesh_geometry( dx11, nativeMesh, 0, elementCount ) : nullptr;

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

            const auto record = records->get(*binding, submeshIndex);
            if(!record) continue;
            auto material = record->material;
            const auto state = record->state;
            // The snapshot retains all texture owners for this draw. Only the
            // third-party fallback needs a temporary owning reference.
            SmartPtr<ITexture> fallbackTexture;
            const auto getTexture = [&]( u32 slot ) -> ITexture * {
                if( state )
                    return slot < state->textures.size() ? state->textures[slot].get() : nullptr;
                fallbackTexture = material ? material->getTexture( slot ) : nullptr;
                return fallbackTexture.get();
            };

            if( m_shadowPass && state && !state->getFlag( castShadowsFlag ) )
                continue;

            const auto prepared = record->prepared;

            void *nativeTexture = nullptr;
            if( material )
            {
                if( auto texture = getTexture( 0 ) )
                {
                    texture->getTextureFinal( &nativeTexture );
                }
            }

            wp_renderer_set_texture_native( m_renderer, nativeTexture );

            // These are Workphone/Ogre PBS slots, not the low-level C material slots.

            wp_renderer_dx11_enable_shadow_receiving(
                dx11, m_shadowsEnabled && mesh->getReceiveShadows() &&
                          ( !state || state->getFlag( receiveShadowsFlag ) ) );

            constexpr u32 defaultTextureSlots[] = { 1u, 2u, 3u, 13u, 22u, 24u };
            void *textureViews[6] = {};
            if( material )
            {
                for( size_t i = 0; i < 6; ++i )
                {
                    if( auto texture = getTexture( prepared ? prepared->textureSlots[i] : defaultTextureSlots[i] ) )
                    {
                        texture->getTextureFinal( &textureViews[i] );
                    }
                }
            }

            wp_renderer_dx11_set_material_textures( dx11, textureViews );
            wp_renderer_dx11_set_material_sampler(
                dx11, prepared ? prepared->wrapU : 0u, prepared ? prepared->wrapV : 0u,
                prepared ? prepared->filter : 1u, prepared ? prepared->anisotropy : 1u );

            wp_material_dx11 nativeMaterial = prepared ? prepared->compose() : wp_material_dx11{};
            if( !prepared )
            {
                const auto diffuse = material ? material->getDiffuse() : ColourF::White;
                const auto specular = material ? material->getSpecular() : ColourF( 0.04f, 0.04f, 0.04f, 1.0f );
                const auto emissive = material ? material->getEmissive() : ColourF::Black;
                nativeMaterial.base_color = { diffuse.r, diffuse.g, diffuse.b, diffuse.a };
                nativeMaterial.specular_color = { specular.r, specular.g, specular.b, specular.a };
                nativeMaterial.emissive_color = { emissive.r, emissive.g, emissive.b, emissive.a };
                nativeMaterial.surface = { material ? material->getMetalness() : 0.0f,
                                           material ? material->getRoughness() : 0.5f, 1.0f, 1.0f };
                nativeMaterial.uv_transform = { 0.0f, 0.0f, 1.0f, 0.0f };
                nativeMaterial.controls = { 1.0f, 1.0f, -1.0f, 0.0f };
            }
            applySceneLighting( nativeMaterial );

            // A material's reflection probe overrides the sky environment for this draw.
            if( material )
            {
                if( auto reflection =
                        getTexture( static_cast<u32>( PbsTextureTypes::PBSM_REFLECTION ) ) )
                {
                    void *view = nullptr;
                    reflection->getTextureFinal( &view );
                    if( view )
                    {
                        D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
                        static_cast<ID3D11ShaderResourceView *>( view )->GetDesc( &desc );
                        if( desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURECUBE )
                        {
                            wp_renderer_dx11_set_environment(
                                dx11, view, static_cast<float>( desc.TextureCube.MipLevels - 1 ) );
                            nativeMaterial.environment.z = 1.0f;
                        }
                    }
                }
            }

            nativeMaterial.map_flags = { textureViews[0] ? 1.0f : 0.0f, textureViews[1] ? 1.0f : 0.0f,
                                         textureViews[2] ? 1.0f : 0.0f, textureViews[3] ? 1.0f : 0.0f };
            nativeMaterial.extra_map_flags = { textureViews[4] ? 1.0f : 0.0f,
                                               textureViews[5] ? 1.0f : 0.0f, 0.0f, 0.0f };

            if( dx11 )
            {
                wp_renderer_dx11_set_material( dx11, &nativeMaterial );
            }

            auto blend = material ? ClawUtil::toCBlendMode( ( state ? state->blendMode : material->getBlendMode() ) )
                                  : WORKPHONE_BLEND_MODE_NONE;

            if( material && ( state ? state->getFlag( transparentFlag ) : material->isTransparent() ) && blend == WORKPHONE_BLEND_MODE_NONE )
            {
                blend = WORKPHONE_BLEND_MODE_ALPHA;
            }

            wp_renderer_set_blend_mode( m_renderer, blend );
            wp_renderer_set_depth_write_enabled(
                m_renderer, m_shadowPass || !material || ( state ? state->getFlag( depthWriteFlag ) : material->getDepthWrite() ) );
            // The editor's comparison list differs from the C renderer enum.
            constexpr wp_depth_func depthFunctions[] = {
                WORKPHONE_DEPTH_FUNC_NEVER, WORKPHONE_DEPTH_FUNC_LESS,   WORKPHONE_DEPTH_FUNC_LEQUAL,
                WORKPHONE_DEPTH_FUNC_EQUAL, WORKPHONE_DEPTH_FUNC_GEQUAL, WORKPHONE_DEPTH_FUNC_GREATER,
                WORKPHONE_DEPTH_FUNC_ALWAYS
            };
            const auto depthTest = material ? ( state ? state->depthTest : material->getDepthTest() ) : 2u;
            wp_renderer_set_depth_test_enabled( m_renderer, 1 );
            wp_renderer_set_depth_func( m_renderer, m_shadowPass
                                                        ? WORKPHONE_DEPTH_FUNC_LESS
                                                        : depthFunctions[std::min( depthTest, 6u )] );
            wp_renderer_set_fill_mode( m_renderer,
                                       !m_shadowPass && state && state->getFlag( showWireframeFlag )
                                           ? WORKPHONE_FILL_MODE_WIREFRAME
                                           : WORKPHONE_FILL_MODE_SOLID );
            wp_renderer_set_cull_mode( m_renderer,
                                       ( state && state->getFlag( doubleSidedFlag ) ) ? WORKPHONE_CULL_MODE_NONE
                                       : material ? ClawUtil::toCCullMode( ( state ? state->cullMode : material->getCullMode() ) )
                                                  : WORKPHONE_CULL_MODE_BACK );

            if( instances )
            {
                if( geometry && count > 0 && count % 3u == 0 )
                {
                    const auto drawn = wp_renderer_dx11_draw_geometry_pntc_instanced(
                        dx11, geometry, static_cast<wp_s32>( start ), static_cast<wp_s32>( count ), 0,
                        instances, static_cast<wp_s32>( instanceCount ) );
                    if( drawn > 0 )
                    {
                        submitted += drawn;
                        m_primitiveCount = static_cast<u32>(
                            std::min<u64>( std::numeric_limits<u32>::max(),
                                           u64( m_primitiveCount ) + u64( count / 3u ) * drawn ) );
                    }
                }
            }
            else if( topology == WORKPHONE_PRIMITIVE_TRIANGLE_STRIP && count >= 3 )
            {
                if( auto stripGeometry = wp_renderer_dx11_prepare_mesh_geometry( dx11, nativeMesh, start, count ) )
                {
                    wp_renderer_dx11_draw_geometry_pntc( dx11, stripGeometry, 0,
                        static_cast<wp_s32>( ( count - 2u ) * 3u ), 0 );
                    m_primitiveCount += count - 2u;
                }
            }
            else if( topology == WORKPHONE_PRIMITIVE_TRIANGLE_LIST && geometry )
            {
                count -= count % 3u;
                if( count > 0 )
                {
                    wp_renderer_dx11_draw_geometry_pntc( dx11, geometry, static_cast<wp_s32>( start ),
                        static_cast<wp_s32>( count ), 0 );
                    m_primitiveCount += count / 3u;
                }
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
        return submitted;
    }

    bool ClawRendererDX11::prepareTerrainGeometry( const wp_graphics_mesh *mesh )
    {
        auto dx11 = m_renderer ? wp_renderer_get_dx11( m_renderer ) : nullptr;
        return mesh && dx11 && wp_renderer_dx11_prepare_mesh_geometry( dx11, const_cast<wp_graphics_mesh *>( mesh ),
            0, wp_graphics_mesh_get_index_count( mesh ) );
    }

    void ClawRendererDX11::prepareTerrain( SmartPtr<ClawTerrain> terrain,
                                           const Array<SmartPtr<IGraphicsCamera>> &cameras )
    {
        if( !terrain || !terrain->isVisible() )
        {
            return;
        }
        auto dx11 = m_renderer ? wp_renderer_get_dx11( m_renderer ) : nullptr;
        if( !dx11 )
        {
            return;
        }
        const Matrix4F requestedTransform(
            terrain->getRequestedTerrainWorldTransform().getTransformationMatrix().ptr() );
        Array<TerrainTileView> views;
        TerrainTileView view;
        view.cull = false;
        const auto scale = terrain->getRequestedTerrainWorldTransform().getScale();
        view.verticalScale = f32( scale.y );
        view.minimumWorldScale = f32( std::min( { scale.x, scale.y, scale.z } ) );
        view.prefetchMargin = 0.15f;
        const auto addCamera = [&]( const SmartPtr<IGraphicsCamera> &camera ) {
            auto cameraTileView = view;
            if( camera )
            {
                const Matrix4F cameraView( camera->getViewMatrix().ptr() );
                const Matrix4F projection( camera->getProjectionMatrix().ptr() );
                cameraTileView.localToClip = projection * cameraView * requestedTransform;
                cameraTileView.localCamera =
                    ( cameraView * requestedTransform ).inverse().transformAffine( Vector3F::zero() );
                const auto target = getRenderTarget();
                const auto height = target ? std::max( 1, target->getSize().y ) : 1080;
                cameraTileView.pixelsPerMetreAtUnitDistance =
                    std::abs( projection[1][1] ) * height * 0.5f;
                cameraTileView.cull = true;
            }
            views.push_back( cameraTileView );
        };

        auto c = getCamera();
        addCamera( c );

        // Resident demand is the union of scene cameras. Changing which viewport
        // draws first must not cancel another camera's in-flight tile generation.
        for( const auto &camera : cameras )
        {
            if( camera && camera != getCamera() )
            {
                if( views.size() >= 15 )
                {
                    break;
                }
                addCamera( camera );
            }
        }
        view = views.front();
        if( m_shadowPass || m_shadowsEnabled )
        {
            auto shadowView = view;
            shadowView.localToClip = m_shadowMatrix * requestedTransform;
            shadowView.maximumPixelError = 1;
            views.push_back( shadowView );
        }
        String terrainError;
        terrain->prepareRenderTiles( *this, views, terrainError );
    }

    void ClawRendererDX11::renderTerrain( SmartPtr<ClawTerrain> terrain, bool prepare )
    {
        if( !terrain || !terrain->isVisible() )
        {
            return;
        }
        auto dx11 = m_renderer ? wp_renderer_get_dx11( m_renderer ) : nullptr;
        if( !dx11 )
        {
            return;
        }
        if( prepare )
        {
            prepareTerrain( terrain );
        }
        String terrainError;
        const auto tiles = terrain->getNativeRenderTiles();
        if( tiles.empty() )
        {
            return;
        }
        const Matrix4F transform( terrain->getWorldTransform().getTransformationMatrix().ptr() );
        setTransforms( transform );
        void *nativeTexture = nullptr;
        if( auto texture = terrain->getTexture( 0 ) )
        {
            texture->getTextureFinal( &nativeTexture );
        }
        wp_renderer_set_texture_native( m_renderer, nativeTexture );

        wp_renderer_dx11_enable_shadow_receiving( dx11, m_shadowsEnabled );
        wp_renderer_dx11_set_material_textures( dx11, nullptr );
        wp_material_dx11 material{};
        material.base_color = { 1.0f, 1.0f, 1.0f, 1.0f };
        material.specular_color = { 0.04f, 0.04f, 0.04f, 1.0f };
        applySceneLighting( material );
        material.surface = { 0.0f, 0.85f, 8.0f, 8.0f };
        material.controls = { 1.0f, 1.0f, -1.0f, 0.0f };
        material.uv_transform = { 0.0f, 0.0f, 1.0f, 0.0f };
        wp_renderer_dx11_set_material( dx11, &material );
        if( !terrain->terrainMaterial().bind( *this, terrainError ) )
        {
            wp_renderer_set_texture_native( m_renderer, nullptr );
            ClawTerrainMaterial::unbind( *this );
            return;
        }

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
        for( auto nativeMesh : tiles )
        {
            const auto indexCount = wp_graphics_mesh_get_index_count( nativeMesh );
            const auto geometry = wp_renderer_dx11_prepare_mesh_geometry( dx11, nativeMesh, 0, indexCount );
            if( !geometry )
            {
                continue;
            }
            wp_renderer_dx11_draw_geometry_pntc( dx11, geometry, 0, static_cast<wp_s32>( indexCount ),
                                                 0 );
            m_primitiveCount += indexCount / 3;
        }
        wp_renderer_set_cull_mode( m_renderer, oldCull );
        wp_renderer_set_depth_test_enabled( m_renderer, oldDepthTest );
        wp_renderer_set_depth_write_enabled( m_renderer, oldDepthWrite );
        wp_renderer_set_depth_func( m_renderer, oldDepthFunc );
        wp_renderer_set_fill_mode( m_renderer, oldFill );
        wp_renderer_set_texture_native( m_renderer, nullptr );
        ClawTerrainMaterial::unbind( *this );
        // The context retains bound buffers. Clear the final tile bindings so
        // terrain retirement also retires their GPU allocation reservations.
        // Material unbind invalidates the native cached bindings for the next draw.
        auto *context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( dx11 ) );
        ID3D11Buffer *emptyBuffer = nullptr;
        const UINT emptyStride = 0, emptyOffset = 0;
        context->IASetVertexBuffers( 0, 1, &emptyBuffer, &emptyStride, &emptyOffset );
        context->IASetIndexBuffer( nullptr, DXGI_FORMAT_UNKNOWN, 0 );
    }

    void ClawRendererDX11::renderSky( const SmartPtr<ISky> &sky )
    {
        auto camera = getCamera();

        if( !m_renderer || !camera || !sky || !sky->isVisible() )
        {
            return;
        }

        auto ownedTextures = sky->getTextures();
        Material::CubicTextureSnapshot materialTextures;
        if( ownedTextures.empty() )
        {
            if( auto material = sky->getMaterial() )
            {
                if( auto concrete = dynamic_pointer_cast<Material>( material ) )
                    materialTextures = concrete->getCubicTextureSnapshot();
                else
                    ownedTextures = material->getCubicTextures();
            }
        }
        const auto &textures = materialTextures ? *materialTextures : ownedTextures;
        if( textures.empty() )
        {
            return;
        }

        if( auto dx11 = wp_renderer_get_dx11( m_renderer ) )
        {
            std::array<ID3D11ShaderResourceView *, 6> views{};
            for( size_t face = 0; face < std::min( textures.size(), views.size() ); ++face )
            {
                if( textures[face] )
                {
                    void *view = nullptr;
                    textures[face]->getTextureFinal( &view );
                    views[face] = static_cast<ID3D11ShaderResourceView *>( view );
                }
            }
            auto &cache = g_skyEnvironmentCaches[this];
            auto &entry = cache.skies[sky.get()];
            if( entry.sky.lock() != sky )
            {
                entry.cubemap.reset();
                entry.sky = sky;
            }
            m_hasSkyEnvironment = entry.cubemap.update(
                static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( dx11 ) ),
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

    void ClawRendererDX11::setTransforms( const Matrix4F &world, const wp_mat4f *snapshotView,
                                          const wp_mat4f *snapshotProjection )
    {
        const auto nativeWorld = toCMatrix( world );
        if( !m_shadowPass && snapshotView && snapshotProjection )
        {
            wp_renderer_set_world_matrix( m_renderer, &nativeWorld );
            wp_renderer_set_view_matrix( m_renderer, snapshotView );
            wp_renderer_set_projection_matrix( m_renderer, snapshotProjection );
            return;
        }
        auto camera = getCamera();
        const auto view =
            !m_shadowPass && camera ? Matrix4F( camera->getViewMatrix().ptr() ) : Matrix4F::identity();
        const auto projection = m_shadowPass ? m_shadowMatrix
                                : camera     ? Matrix4F( camera->getProjectionMatrix().ptr() )
                                             : Matrix4F::identity();
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
