#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UIRenderer.hpp>
#include "Workphone/Workphone.hpp"
#include <Ogre.h>
#include <OgreRenderSystem.h>
#include <OgreTextureGpuManager.h>
#include <OgreStagingTexture.h>
#include <OgreTextureBox.h>
#include <OgrePixelFormatGpuUtils.h>
#include <Vao/OgreVaoManager.h>
#include <Vao/OgreVertexArrayObject.h>
#include <Vao/OgreIndirectBufferPacked.h>
#include <CommandBuffer/OgreCbDrawCall.h>
#include <OgrePsoCacheHelper.h>
#include <OgreUnifiedHighLevelGpuProgram.h>
#include <OgreHlmsManager.h>
#include <cstring>

extern "C" {
#include "workphone.h"
}

namespace workphone
{
    namespace render
    {

        UIRenderer *UIRenderer::s_instance = nullptr;

        UIRenderer::UIRenderer()
        {
            s_instance = this;
        }

        UIRenderer::~UIRenderer()
        {
            s_instance = nullptr;
        }

        void UIRenderer::load( SmartPtr<ISharedObject> data )
        {
            setLoadingState( LoadingState::Loading );

            m_ctx = (::wp_context *)malloc( sizeof( ::wp_context ) );
            if( m_ctx )
            {
                memset( m_ctx, 0, sizeof( ::wp_context ) );
                wp_init_default( m_ctx, nullptr );
            }

            m_atlas = (wp_font_atlas *)malloc( sizeof( wp_font_atlas ) );
            m_texNull = (wp_draw_null_texture *)malloc( sizeof( wp_draw_null_texture ) );
            m_cmds = (wp_buffer *)malloc( sizeof( wp_buffer ) );

            memset( m_atlas, 0, sizeof( wp_font_atlas ) );
            memset( m_texNull, 0, sizeof( wp_draw_null_texture ) );
            memset( m_cmds, 0, sizeof( wp_buffer ) );

            wp_font_atlas_init_default( m_atlas );
            wp_font_atlas_begin( m_atlas );

            m_atlas->default_font = wp_font_atlas_add_default( m_atlas, 13.0f, nullptr );

            wp_buffer_init_default( m_cmds );
            wp_style_load_all_cursors( m_ctx, m_atlas->cursors );

            createFontTexture();
            createMaterial();
            setupBuffers( 100000, 100000 );

            setLoadingState( LoadingState::Loaded );
        }

        void UIRenderer::unload( SmartPtr<ISharedObject> data )
        {
            setLoadingState( LoadingState::Unloading );

            auto root = Ogre::Root::getSingletonPtr();
            if( !root )
            {
                // If Root is already destroyed we cannot safely release GPU resources,
                // but we must still free CPU-side allocations to avoid leaking them.
                WP_LOG( "Ogre::Root already destroyed. Skipping GPU resource cleanup in UIRenderer" );
            }

            Ogre::VaoManager *vaoManager = nullptr;
            Ogre::RenderSystem *renderSystem = nullptr;
            if( root )
            {
                renderSystem = root->getRenderSystem();
                if( renderSystem )
                    vaoManager = renderSystem->getVaoManager();
            }

            if( m_vao && vaoManager )
            {
                const Ogre::VertexBufferPackedVec &vertexBuffers = m_vao->getVertexBuffers();
                for( auto *vb : vertexBuffers )
                {
                    if( vb->getMappingState() != Ogre::MS_UNMAPPED )
                        vb->unmap( Ogre::UO_UNMAP_ALL );

                    vaoManager->destroyVertexBuffer( vb );
                }

                auto indexBuffer = m_vao->getIndexBuffer();
                if( indexBuffer )
                    vaoManager->destroyIndexBuffer( indexBuffer );

                vaoManager->destroyVertexArrayObject( m_vao );

                m_vao = nullptr;
                m_vbo = nullptr;
                m_ibo = nullptr;
            }

            if( m_indirectBuffer && vaoManager )
            {
                vaoManager->destroyIndirectBuffer( m_indirectBuffer );
                m_indirectBuffer = nullptr;
            }

            delete m_psoCache;
            m_psoCache = nullptr;

            if( m_samplerblock || m_descSetSampler )
            {
                Ogre::HlmsManager *hlmsManager = root ? root->getHlmsManager() : nullptr;
                if( hlmsManager )
                {
                    if( m_descSetSampler )
                        hlmsManager->destroyDescriptorSetSampler( m_descSetSampler );

                    if( m_samplerblock )
                        hlmsManager->destroySamplerblock( m_samplerblock );
                }

                m_descSetSampler = nullptr;
                m_samplerblock = nullptr;
            }

            if( m_pass )
            {
                auto materialManager = Ogre::MaterialManager::getSingletonPtr();
                auto technique = m_pass->getParent();
                auto material = technique->getParent();
                materialManager->remove( material->getHandle() );
                m_pass = nullptr;
            }

            // Remove GPU programs created by createMaterial(). The unified wrappers must be
            // removed last because they hold delegate references to the backend programs.
            if( root )
            {
                Ogre::HighLevelGpuProgramManager &gpuMgr =
                    Ogre::HighLevelGpuProgramManager::getSingleton();
                const Ogre::String group = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

                auto removeGpuProgram = [&gpuMgr, &group]( Ogre::HighLevelGpuProgramPtr &program ) {
                    if( program )
                    {
                        const auto name = program->getName();
                        program.reset();

                        if( !gpuMgr.getByName( name, group ).isNull() )
                        {
                            gpuMgr.remove( name );
                        }
                    }
                };

                removeGpuProgram( m_vertexShaderD3D11 );
                removeGpuProgram( m_pixelShaderD3D11 );
                removeGpuProgram( m_vertexShaderMetal );
                removeGpuProgram( m_pixelShaderMetal );
                removeGpuProgram( m_vertexShaderGL );
                removeGpuProgram( m_pixelShaderGL );
                removeGpuProgram( m_vertexShaderUnified );
                removeGpuProgram( m_pixelShaderUnified );
            }
            else
            {
                m_vertexShaderD3D11.reset();
                m_pixelShaderD3D11.reset();
                m_vertexShaderMetal.reset();
                m_pixelShaderMetal.reset();
                m_vertexShaderGL.reset();
                m_pixelShaderGL.reset();
                m_vertexShaderUnified.reset();
                m_pixelShaderUnified.reset();
            }

            if( m_fontTexture )
            {
                if( renderSystem )
                {
                    auto textureManager = renderSystem->getTextureGpuManager();
                    if( textureManager )
                        textureManager->destroyTexture( m_fontTexture );
                }

                m_fontTexture = nullptr;
            }

            // Always free CPU-side allocations regardless of GPU state.
            if( m_atlas )
            {
                wp_font_atlas_cleanup( m_atlas );
                free( m_atlas );
                m_atlas = nullptr;
            }

            if( m_texNull )
            {
                free( m_texNull );
                m_texNull = nullptr;
            }

            if( m_cmds )
            {
                wp_buffer_free( m_cmds );
                free( m_cmds );
                m_cmds = nullptr;
            }

            if( m_ctx )
            {
                wp_clear( m_ctx );
                free( m_ctx );
                m_ctx = nullptr;
            }

            setLoadingState( LoadingState::Unloaded );
        }

        ::wp_context *UIRenderer::getContext()
        {
            return m_ctx;
        }

        void UIRenderer::beginFrame()
        {
            // Input events are queued before this render pass; clearing here drops click edges
            // before widgets such as wp_button_label can consume them.
        }

        void UIRenderer::endFrame()
        {
            if( m_ctx )
            {
                wp_input_end( m_ctx );
                // Clear transient click/key edges after UI widgets have consumed this frame.
                wp_input_begin( m_ctx );
            }
        }

        namespace
        {
            struct UIVertex
            {
                float position[2];
                float uv[2];
                wp_byte col[4];
            };
        }  // namespace

        void UIRenderer::render()
        {
            if( !m_ctx )
                return;

            /*
            // absolute position test: two buttons will overlap, one will be on top of the other, and a third button will be below them
            if( wp_begin( m_ctx, (wp_c8 *)"My Window", wp_make_rect( 50, 50, 400, 400 ),
                          WORKPHONE_WINDOW_BORDER | WORKPHONE_WINDOW_MOVABLE ) )
            {
                // Start free layout (no constraints). Last arg is the number of widgets to push.
                wp_layout_space_begin( m_ctx, WORKPHONE_STATIC, 400, 3 );

                // Button 1
                wp_layout_space_push( m_ctx, wp_make_rect( 50, 50, 120, 30 ) );
                if( wp_button_label( m_ctx, "Button 1" ) )
                {
                }

                // Button 2 (similar position intentionally)
                wp_layout_space_push( m_ctx, wp_make_rect( 100, 120, 120, 30 ) );
                if( wp_button_label( m_ctx, "Button 2" ) )
                {
                }

                // Button 3 (different position)
                wp_layout_space_push( m_ctx, wp_make_rect( 100, 150, 120, 30 ) );
                if( wp_button_label( m_ctx, "Button 3" ) )
                {
                }

                wp_layout_space_end( m_ctx );
            }
            wp_end( m_ctx );
            */

            //if( wp_begin(
            //        m_ctx, reinterpret_cast<const wp_c8 *>( "Demo" ), wp_make_rect( 50, 50, 230, 250 ),
            //        WORKPHONE_WINDOW_BORDER | WORKPHONE_WINDOW_MOVABLE | WORKPHONE_WINDOW_SCALABLE |
            //            WORKPHONE_WINDOW_MINIMIZABLE | WORKPHONE_WINDOW_TITLE ) )
            //{
            //    wp_layout_row_static( m_ctx, 30, 80, 1 );
            //    if( wp_button_label( m_ctx, "button" ) )
            //        fprintf( stdout, "button pressed\n" );
            //}
            //wp_end( m_ctx );

            if( !m_vbo || !m_ibo || !m_vao )
            {
                wp_clear( m_ctx );
                return;
            }

            // Map vertex and index buffers
            void *mappedVertices = m_vbo->map( 0, m_vbo->getNumElements() );
            void *mappedIndices = m_ibo->map( 0, m_ibo->getNumElements() );

            if( !mappedVertices || !mappedIndices )
            {
                if( mappedVertices )
                    m_vbo->unmap( Ogre::UO_KEEP_PERSISTENT );
                if( mappedIndices )
                    m_ibo->unmap( Ogre::UO_KEEP_PERSISTENT );
                wp_clear( m_ctx );
                return;
            }

            // Set up convert configuration matching our vertex layout
            static const struct wp_draw_vertex_layout_element vertexLayout[] = {
                { WORKPHONE_VERTEX_POSITION, WORKPHONE_FORMAT_FLOAT,
                  WORKPHONE_OFFSETOF( struct UIVertex, position ) },
                { WORKPHONE_VERTEX_TEXCOORD, WORKPHONE_FORMAT_FLOAT,
                  WORKPHONE_OFFSETOF( struct UIVertex, uv ) },
                { WORKPHONE_VERTEX_COLOR, WORKPHONE_FORMAT_R8G8B8A8,
                  WORKPHONE_OFFSETOF( struct UIVertex, col ) },
                { WORKPHONE_VERTEX_LAYOUT_END }
            };

            struct wp_convert_config config;
            memset( &config, 0, sizeof( config ) );
            config.vertex_layout = vertexLayout;
            config.vertex_size = sizeof( struct UIVertex );
            config.vertex_alignment = WORKPHONE_ALIGNOF( struct UIVertex );
            config.global_alpha = 1.0f;
            config.shape_AA = WORKPHONE_ANTI_ALIASING_ON;
            config.line_AA = WORKPHONE_ANTI_ALIASING_ON;
            config.circle_segment_count = 22;
            config.curve_segment_count = 22;
            config.arc_segment_count = 22;
            config.tex_null = *m_texNull;

            // Convert UI commands into vertex/index data
            struct wp_buffer vbuf, ibuf;
            wp_buffer_init_fixed( &vbuf, mappedVertices,
                                  static_cast<wp_u32>( m_maxVertices * sizeof( struct UIVertex ) ) );
            wp_buffer_init_fixed( &ibuf, mappedIndices,
                                  static_cast<wp_u32>( m_maxIndices * m_ibo->getBytesPerElement() ) );
            wp_convert( m_ctx, m_cmds, &vbuf, &ibuf, &config );

            // Unmap buffers
            m_vbo->unmap( Ogre::UO_KEEP_PERSISTENT );
            m_ibo->unmap( Ogre::UO_KEEP_PERSISTENT );

            // Issue draw commands
            draw( m_cmds );

            // Clear state for next frame
            wp_clear( m_ctx );
            wp_buffer_clear( m_cmds );
        }

        void UIRenderer::setDisplaySize( float width, float height )
        {
            m_width = width;
            m_height = height;
        }

        void UIRenderer::updateProjectionMatrix( float width, float height )
        {
            if( !m_pass )
                return;

            // Orthographic projection: maps screen coordinates to clip space [-1,1]
            Ogre::Matrix4 projMatrix( 2.0f / width, 0.0f, 0.0f, -1.0f, 0.0f, -2.0f / height, 0.0f, 1.0f,
                                      0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f );

            m_pass->getVertexProgramParameters()->setNamedConstant( "ProjectionMatrix", projMatrix );
        }

        void UIRenderer::createFontTexture()
        {
            wp_s32 w = 0;
            wp_s32 h = 0;

            const void *image = wp_font_atlas_bake( m_atlas, &w, &h, WORKPHONE_FONT_ATLAS_RGBA32 );
            if( !image || w <= 0 || h <= 0 )
                return;

            auto root = Ogre::Root::getSingletonPtr();
            if( !root )
                return;

            auto renderSystem = root->getRenderSystem();
            if( !renderSystem )
                return;

            auto textureManager = renderSystem->getTextureGpuManager();
            if( !textureManager )
                return;

            if( m_fontTexture )
            {
                textureManager->destroyTexture( m_fontTexture );
                m_fontTexture = nullptr;
            }

            m_fontTexture = textureManager->createTexture(
                "UIRendererFontAtlas", Ogre::GpuPageOutStrategy::Discard,
                Ogre::TextureFlags::ManualTexture, Ogre::TextureTypes::Type2D );
            m_fontTexture->setResolution( static_cast<Ogre::uint32>( w ),
                                          static_cast<Ogre::uint32>( h ) );
            m_fontTexture->setPixelFormat( Ogre::PFG_RGBA8_UNORM );
            m_fontTexture->scheduleTransitionTo( Ogre::GpuResidency::Resident );

            Ogre::StagingTexture *stagingTexture = textureManager->getStagingTexture(
                static_cast<Ogre::uint32>( w ), static_cast<Ogre::uint32>( h ), 1u, 1u,
                Ogre::PFG_RGBA8_UNORM );
            stagingTexture->startMapRegion();
            Ogre::TextureBox texBox = stagingTexture->mapRegion( static_cast<Ogre::uint32>( w ),
                                                                 static_cast<Ogre::uint32>( h ), 1u, 1u,
                                                                 Ogre::PFG_RGBA8_UNORM );

            const size_t srcBytesPerRow = static_cast<size_t>( w ) * 4u;
            const Ogre::uint8 *srcData = static_cast<const Ogre::uint8 *>( image );
            Ogre::uint8 *dstData = static_cast<Ogre::uint8 *>( texBox.data );
            for( wp_s32 y = 0; y < h; ++y )
            {
                memcpy( dstData, srcData, srcBytesPerRow );
                srcData += srcBytesPerRow;
                dstData += texBox.bytesPerRow;
            }

            stagingTexture->stopMapRegion();
            stagingTexture->upload( texBox, m_fontTexture, 0, 0, 0 );
            textureManager->removeStagingTexture( stagingTexture );

            wp_font_atlas_end( m_atlas, wp_handle_ptr( m_fontTexture ), m_texNull );

            if( m_ctx && m_atlas->default_font )
            {
                wp_style_set_font( m_ctx, &m_atlas->default_font->handle );
            }
        }

        void UIRenderer::createMaterial()
        {
            auto root = Ogre::Root::getSingletonPtr();
            if( !root )
                return;

            auto renderSystem = root->getRenderSystem();
            if( !renderSystem )
                return;

            const Ogre::String renderSystemName = renderSystem->getName();
            const bool isD3D11 = renderSystemName == "Direct3D11 Rendering Subsystem";
            const bool isMetal = renderSystemName == "Metal Rendering Subsystem";
            const bool isOpenGL = renderSystemName == "OpenGL 3+ Rendering Subsystem";

            if( !isD3D11 && !isMetal && !isOpenGL )
            {
                WP_LOG( "UIRenderer::createMaterial unsupported render system: " + renderSystemName );
                return;
            }

            // ---- Shader source strings ----

            static const char *vertexShaderSrcD3D11 = {
                "uniform float4x4 ProjectionMatrix;\n"
                "struct VS_INPUT\n"
                "{\n"
                "float2 pos : POSITION;\n"
                "float4 col : COLOR0;\n"
                "float2 uv  : TEXCOORD0;\n"
                "};\n"
                "struct PS_INPUT\n"
                "{\n"
                "float4 pos : SV_POSITION;\n"
                "float4 col : COLOR0;\n"
                "float2 uv  : TEXCOORD0;\n"
                "};\n"
                "PS_INPUT main(VS_INPUT input)\n"
                "{\n"
                "PS_INPUT output;\n"
                "output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\n"
                "output.col = input.col;\n"
                "output.uv  = input.uv;\n"
                "return output;\n"
                "}"
            };

            static const char *pixelShaderSrcD3D11 = {
                "struct PS_INPUT\n"
                "{\n"
                "float4 pos : SV_POSITION;\n"
                "float4 col : COLOR0;\n"
                "float2 uv  : TEXCOORD0;\n"
                "};\n"
                "sampler sampler0: register(s0);\n"
                "Texture2D texture0: register(t0);\n"
                "\n"
                "float4 main(PS_INPUT input) : SV_Target\n"
                "{\n"
                "float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \n"
                "return out_col; \n"
                "}"
            };

            static const char *vertexShaderSrcGLSL = {
                "#version 150\n"
                "uniform mat4 ProjectionMatrix; \n"
                "in vec2 vertex;\n"
                "in vec2 uv0;\n"
                "in vec4 colour;\n"
                "out vec2 Texcoord;\n"
                "out vec4 col;\n"
                "void main()\n"
                "{\n"
                "gl_Position = ProjectionMatrix* vec4(vertex.xy, 0.f, 1.f);\n"
                "Texcoord  = uv0;\n"
                "col = colour;\n"
                "}"
            };

            static const char *pixelShaderSrcGLSL = {
                "#version 150\n"
                "in vec2 Texcoord;\n"
                "in vec4 col;\n"
                "uniform sampler2D sampler0;\n"
                "out vec4 out_col;\n"
                "void main()\n"
                "{\n"
                "out_col = col * texture(sampler0, Texcoord);\n"
                "}"
            };

            static const char *vertexShaderSrcMetal = {
                "#include <metal_stdlib>\n"
                "using namespace metal;\n"
                "\n"
                "struct Constant {\n"
                "    float4x4 ProjectionMatrix;\n"
                "};\n"
                "\n"
                "struct VertexIn {\n"
                "    float2 position  [[attribute(VES_POSITION)]];\n"
                "    float2 texCoords [[attribute(VES_TEXTURE_COORDINATES0)]];\n"
                "    float4 colour     [[attribute(VES_DIFFUSE)]];\n"
                "};\n"
                "\n"
                "struct VertexOut {\n"
                "    float4 position [[position]];\n"
                "    float2 texCoords;\n"
                "    float4 colour;\n"
                "};\n"
                "\n"
                "vertex VertexOut vertex_main(VertexIn in                 [[stage_in]],\n"
                "                             constant Constant &uniforms [[buffer(PARAMETER_SLOT)]]) "
                "{\n"
                "    VertexOut out;\n"
                "    out.position = uniforms.ProjectionMatrix * float4(in.position, 0, 1);\n"
                "    out.texCoords = in.texCoords;\n"
                "    out.colour = in.colour;\n"
                "    return out;\n"
                "}\n"
            };

            static const char *fragmentShaderSrcMetal = {
                "#include <metal_stdlib>\n"
                "using namespace metal;\n"
                "\n"
                "struct VertexOut {\n"
                "    float4 position [[position]];\n"
                "    float2 texCoords;\n"
                "    float4 colour;\n"
                "};\n"
                "\n"
                "fragment float4 main_metal(VertexOut in [[stage_in]],\n"
                "                             texture2d<float> texture [[texture(0)]]) {\n"
                "    constexpr sampler linearSampler(coord::normalized, min_filter::linear, "
                "mag_filter::linear, mip_filter::linear);\n"
                "    float4 texColour = texture.sample(linearSampler, in.texCoords);\n"
                "    return in.colour * texColour;\n"
                "}\n"
            };

            // ---- Create GPU programs via UnifiedHighLevelGpuProgram ----

            Ogre::HighLevelGpuProgramManager &mgr = Ogre::HighLevelGpuProgramManager::getSingleton();

            m_vertexShaderUnified = mgr.getByName( "wpui/VP" );
            m_pixelShaderUnified = mgr.getByName( "wpui/FP" );

            if( m_vertexShaderUnified.isNull() )
            {
                m_vertexShaderUnified = mgr.createProgram(
                    "wpui/VP", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, "unified",
                    Ogre::GPT_VERTEX_PROGRAM );
            }

            if( m_pixelShaderUnified.isNull() )
            {
                m_pixelShaderUnified = mgr.createProgram(
                    "wpui/FP", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, "unified",
                    Ogre::GPT_FRAGMENT_PROGRAM );
            }

            auto *vertexShaderPtr =
                static_cast<Ogre::UnifiedHighLevelGpuProgram *>( m_vertexShaderUnified.get() );
            auto *pixelShaderPtr =
                static_cast<Ogre::UnifiedHighLevelGpuProgram *>( m_pixelShaderUnified.get() );

            if( isD3D11 )
            {
                m_vertexShaderD3D11 = mgr.getByName( "wpui/VP/D3D11" );
                m_pixelShaderD3D11 = mgr.getByName( "wpui/FP/D3D11" );

                if( m_vertexShaderD3D11.isNull() )
                {
                    m_vertexShaderD3D11 = mgr.createProgram(
                        "wpui/VP/D3D11", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, "hlsl",
                        Ogre::GPT_VERTEX_PROGRAM );
                    m_vertexShaderD3D11->setParameter( "target", "vs_4_0" );
                    m_vertexShaderD3D11->setParameter( "entry_point", "main" );
                    m_vertexShaderD3D11->setSource( vertexShaderSrcD3D11 );
                    m_vertexShaderD3D11->load();
                    vertexShaderPtr->addDelegateProgram( m_vertexShaderD3D11->getName() );
                }

                if( m_pixelShaderD3D11.isNull() )
                {
                    m_pixelShaderD3D11 = mgr.createProgram(
                        "wpui/FP/D3D11", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, "hlsl",
                        Ogre::GPT_FRAGMENT_PROGRAM );
                    m_pixelShaderD3D11->setParameter( "target", "ps_4_0" );
                    m_pixelShaderD3D11->setParameter( "entry_point", "main" );
                    m_pixelShaderD3D11->setSource( pixelShaderSrcD3D11 );
                    m_pixelShaderD3D11->load();
                    pixelShaderPtr->addDelegateProgram( m_pixelShaderD3D11->getName() );
                }
            }

            if( isMetal )
            {
                m_vertexShaderMetal = mgr.getByName( "wpui/VP/Metal" );
                m_pixelShaderMetal = mgr.getByName( "wpui/FP/Metal" );

                if( m_vertexShaderMetal.isNull() )
                {
                    m_vertexShaderMetal = mgr.createProgram(
                        "wpui/VP/Metal", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                        "metal", Ogre::GPT_VERTEX_PROGRAM );
                    m_vertexShaderMetal->setParameter( "entry_point", "vertex_main" );
                    m_vertexShaderMetal->setSource( vertexShaderSrcMetal );
                    m_vertexShaderMetal->load();
                    vertexShaderPtr->addDelegateProgram( m_vertexShaderMetal->getName() );
                }

                if( m_pixelShaderMetal.isNull() )
                {
                    m_pixelShaderMetal = mgr.createProgram(
                        "wpui/FP/Metal", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                        "metal", Ogre::GPT_FRAGMENT_PROGRAM );
                    m_pixelShaderMetal->setParameter( "entry_point", "main_metal" );
                    m_pixelShaderMetal->setSource( fragmentShaderSrcMetal );
                    m_pixelShaderMetal->load();
                    pixelShaderPtr->addDelegateProgram( m_pixelShaderMetal->getName() );
                }
            }

            if( isOpenGL )
            {
                m_vertexShaderGL = mgr.getByName( "wpui/VP/GL150" );
                m_pixelShaderGL = mgr.getByName( "wpui/FP/GL150" );

                if( m_vertexShaderGL.isNull() )
                {
                    m_vertexShaderGL = mgr.createProgram(
                        "wpui/VP/GL150", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, "glsl",
                        Ogre::GPT_VERTEX_PROGRAM );
                    m_vertexShaderGL->setSource( vertexShaderSrcGLSL );
                    m_vertexShaderGL->load();
                    vertexShaderPtr->addDelegateProgram( m_vertexShaderGL->getName() );
                }

                if( m_pixelShaderGL.isNull() )
                {
                    m_pixelShaderGL = mgr.createProgram(
                        "wpui/FP/GL150", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, "glsl",
                        Ogre::GPT_FRAGMENT_PROGRAM );
                    m_pixelShaderGL->setSource( pixelShaderSrcGLSL );
                    m_pixelShaderGL->load();
                    m_pixelShaderGL->setParameter( "sampler0", "int 0" );
                    pixelShaderPtr->addDelegateProgram( m_pixelShaderGL->getName() );
                }
            }

            // ---- Create v1 Material + Pass ----

            Ogre::MaterialPtr uiMaterial = Ogre::MaterialManager::getSingleton().getByName(
                "wpui/material", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
            if( uiMaterial.isNull() )
            {
                uiMaterial = Ogre::MaterialManager::getSingleton().create(
                    "wpui/material", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
            }

            m_pass = uiMaterial->getTechnique( 0 )->getPass( 0 );
            m_pass->setVertexProgram( "wpui/VP" );
            m_pass->setFragmentProgram( "wpui/FP" );

            // Blendblock: standard alpha blending for UI
            Ogre::HlmsBlendblock blendblock( *m_pass->getBlendblock() );
            blendblock.mSourceBlendFactor = Ogre::SBF_SOURCE_ALPHA;
            blendblock.mDestBlendFactor = Ogre::SBF_ONE_MINUS_SOURCE_ALPHA;

            //blendblock.mSourceBlendFactorAlpha = Ogre::SBF_ONE_MINUS_SOURCE_ALPHA;
            //blendblock.mDestBlendFactorAlpha = Ogre::SBF_ZERO;
            blendblock.mSourceBlendFactorAlpha = Ogre::SBF_ONE;
            blendblock.mDestBlendFactorAlpha = Ogre::SBF_ONE_MINUS_SOURCE_ALPHA;

            blendblock.mBlendOperation = Ogre::SBO_ADD;
            blendblock.mBlendOperationAlpha = Ogre::SBO_ADD;
            blendblock.mSeparateBlend = true;
            blendblock.mIsTransparent = true;

            // Macroblock: no culling, no depth, scissor test enabled
            Ogre::HlmsMacroblock macroblock( *m_pass->getMacroblock() );
            macroblock.mCullMode = Ogre::CULL_NONE;
            macroblock.mDepthFunc = Ogre::CMPF_ALWAYS_PASS;
            macroblock.mDepthCheck = false;
            macroblock.mDepthWrite = false;
            macroblock.mScissorTestEnabled = true;

            m_pass->setBlendblock( blendblock );
            m_pass->setMacroblock( macroblock );

            // OpenGL needs sampler0 set explicitly
            if( isOpenGL )
            {
                auto fragmentProgramParameters = m_pass->getFragmentProgramParameters();
                fragmentProgramParameters->setNamedConstant( "sampler0", 0 );
            }

            auto textureUnitState = m_pass->createTextureUnitState();
            textureUnitState->setTextureName( "UIRendererFontAtlas" );

            // ---- Create PsoCacheHelper ----
            if( !m_psoCache )
            {
                m_psoCache = new Ogre::PsoCacheHelper( renderSystem );
            }

            // ---- Create sampler block (linear filter, clamp) ----
            {
                Ogre::HlmsManager *hlmsManager = root->getHlmsManager();
                Ogre::HlmsSamplerblock samplerParams;
                samplerParams.mMinFilter = Ogre::FO_LINEAR;
                samplerParams.mMagFilter = Ogre::FO_LINEAR;
                samplerParams.mMipFilter = Ogre::FO_NONE;
                samplerParams.mU = Ogre::TAM_CLAMP;
                samplerParams.mV = Ogre::TAM_CLAMP;
                samplerParams.mW = Ogre::TAM_CLAMP;
                m_samplerblock = hlmsManager->getSamplerblock( samplerParams );

                // Build a persistent DescriptorSetSampler so mRsData is populated by the
                // render system and _setSamplers() works correctly for all render targets.
                if( m_descSetSampler )
                {
                    hlmsManager->destroyDescriptorSetSampler( m_descSetSampler );
                    m_descSetSampler = nullptr;
                }

                Ogre::DescriptorSetSampler descSetSamplerDef;
                descSetSamplerDef.mSamplers.push_back( m_samplerblock );
                descSetSamplerDef.mShaderTypeSamplerCount[Ogre::PixelShader] = 1u;
                m_descSetSampler = hlmsManager->getDescriptorSetSampler( descSetSamplerDef );
            }

            // Set initial projection matrix
            updateProjectionMatrix( m_width, m_height );
        }

        void UIRenderer::setupBuffers( size_t maxVertices, size_t maxIndices )
        {
            auto root = Ogre::Root::getSingletonPtr();
            if( !root )
                return;

            auto renderSystem = root->getRenderSystem();
            if( !renderSystem )
                return;

            auto vaoManager = renderSystem->getVaoManager();
            if( !vaoManager )
                return;

            if( m_vao )
            {
                const Ogre::VertexBufferPackedVec &vertexBuffers = m_vao->getVertexBuffers();
                for( auto *vb : vertexBuffers )
                {
                    if( vb->getMappingState() != Ogre::MS_UNMAPPED )
                        vb->unmap( Ogre::UO_UNMAP_ALL );
                    vaoManager->destroyVertexBuffer( vb );
                }

                if( m_vao->getIndexBuffer() )
                    vaoManager->destroyIndexBuffer( m_vao->getIndexBuffer() );

                vaoManager->destroyVertexArrayObject( m_vao );
                m_vao = nullptr;
                m_vbo = nullptr;
                m_ibo = nullptr;
            }

            m_maxVertices = maxVertices;
            m_maxIndices = maxIndices;

            Ogre::VertexElement2Vec vertexElements;
            vertexElements.reserve( 3 );
            vertexElements.push_back( Ogre::VertexElement2( Ogre::VET_FLOAT2, Ogre::VES_POSITION ) );
            vertexElements.push_back(
                Ogre::VertexElement2( Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES ) );
            vertexElements.push_back( Ogre::VertexElement2( Ogre::VET_UBYTE4_NORM, Ogre::VES_DIFFUSE ) );

            m_vbo =
                vaoManager->createVertexBuffer( vertexElements, static_cast<Ogre::uint32>( maxVertices ),
                                                Ogre::BT_DYNAMIC_PERSISTENT, nullptr, false );

#ifdef WORKPHONE_UINT_DRAW_INDEX
            auto indexType = Ogre::IndexBufferPacked::IT_32BIT;
#else
            auto indexType = Ogre::IndexBufferPacked::IT_16BIT;
#endif
            m_ibo = vaoManager->createIndexBuffer( indexType, static_cast<Ogre::uint32>( maxIndices ),
                                                   Ogre::BT_DYNAMIC_PERSISTENT, nullptr, false );

            Ogre::VertexBufferPackedVec vertexBuffers;
            vertexBuffers.push_back( m_vbo );
            m_vao = vaoManager->createVertexArrayObject( vertexBuffers, m_ibo, Ogre::OT_TRIANGLE_LIST );

            if( m_indirectBuffer )
            {
                vaoManager->destroyIndirectBuffer( m_indirectBuffer );
                m_indirectBuffer = nullptr;
            }

            m_indirectBuffer = vaoManager->createIndirectBuffer(
                sizeof( Ogre::CbDrawIndexed ) * 256u, Ogre::BT_DYNAMIC_PERSISTENT, nullptr, false );
        }

        void UIRenderer::uploadBuffers( struct wp_buffer *vbuf, struct wp_buffer *ibuf )
        {
            if( !vbuf || !ibuf || !m_vbo || !m_ibo )
                return;

            const void *vertexData = wp_buffer_memory_const( vbuf );
            const wp_size vertexBytes = wp_buffer_total( vbuf );

            const void *indexData = wp_buffer_memory_const( ibuf );
            const wp_size indexBytes = wp_buffer_total( ibuf );

            if( !vertexData || !vertexBytes || !indexData || !indexBytes )
                return;

            // Map vertex buffer and copy data
            {
                const size_t vertexStride = m_vbo->getBytesPerElement();
                const size_t vertexCount = std::min<size_t>( vertexBytes / vertexStride, m_maxVertices );

                void *dstVerts = m_vbo->map( 0, m_vbo->getNumElements() );
                memcpy( dstVerts, vertexData, vertexCount * vertexStride );
                m_vbo->unmap( Ogre::UO_KEEP_PERSISTENT );
            }

            // Map index buffer and copy data
            {
                const size_t indexStride = m_ibo->getBytesPerElement();
                const size_t indexCount = std::min<size_t>( indexBytes / indexStride, m_maxIndices );

                void *dstIndices = m_ibo->map( 0, m_ibo->getNumElements() );
                memcpy( dstIndices, indexData, indexCount * indexStride );
                m_ibo->unmap( Ogre::UO_KEEP_PERSISTENT );
            }
        }

        void UIRenderer::draw( struct wp_buffer *cmds )
        {
            if( !cmds || !m_ctx || !m_vao || !m_indirectBuffer || !m_psoCache || !m_pass )
                return;

            auto root = Ogre::Root::getSingletonPtr();
            if( !root )
                return;

            auto renderSystem = root->getRenderSystem();
            if( !renderSystem )
                return;

            // ---- Set up PSO via PsoCacheHelper ----

            m_psoCache->clearState();

            // Must call setRenderTarget before getPso() to initialize the pass hash
            Ogre::RenderPassDescriptor *passDesc = renderSystem->getCurrentPassDescriptor();
            if( !passDesc )
                return;

            m_psoCache->setRenderTarget( passDesc );

            const Ogre::HlmsBlendblock *blendblock = m_pass->getBlendblock();
            const Ogre::HlmsMacroblock *macroblock = m_pass->getMacroblock();

            m_psoCache->setMacroblock( macroblock );
            m_psoCache->setBlendblock( blendblock );
            m_psoCache->setVertexShader(
                const_cast<Ogre::GpuProgramPtr &>( m_pass->getVertexProgram() ) );
            m_psoCache->setPixelShader(
                const_cast<Ogre::GpuProgramPtr &>( m_pass->getFragmentProgram() ) );

            // Build vertex format description matching our VAO
            Ogre::VertexElement2VecVec vertexElements;
            {
                Ogre::VertexElement2Vec singleSource;
                singleSource.push_back( Ogre::VertexElement2( Ogre::VET_FLOAT2, Ogre::VES_POSITION ) );
                singleSource.push_back(
                    Ogre::VertexElement2( Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES ) );
                singleSource.push_back(
                    Ogre::VertexElement2( Ogre::VET_UBYTE4_NORM, Ogre::VES_DIFFUSE ) );
                vertexElements.push_back( singleSource );
            }

            m_psoCache->setVertexFormat( vertexElements, Ogre::OT_TRIANGLE_LIST, false );

            Ogre::HlmsPso *pso = m_psoCache->getPso();
            renderSystem->_setPipelineStateObject( pso );

            // Update projection matrix to match the actual render target dimensions.
            // This handles rendering to a target texture whose size differs from m_width/m_height.
            {
                Ogre::Viewport &vp = renderSystem->_getCurrentRenderViewport();
                const float vpW = static_cast<float>( vp.getActualWidth() );
                const float vpH = static_cast<float>( vp.getActualHeight() );

                if( !MathF::equals( vpW, m_width ) || !MathF::equals( vpH, m_height ) )
                    updateProjectionMatrix( vpW, vpH );
            }

            // Bind vertex program parameters (projection matrix) — must come after updateProjectionMatrix.
            renderSystem->bindGpuProgramParameters(
                Ogre::GPT_VERTEX_PROGRAM, m_pass->getVertexProgramParameters(), Ogre::GPV_ALL );

            // ---- Issue draw commands ----

            renderSystem->_setVertexArrayObject( m_vao );
            renderSystem->_setIndirectBuffer( m_indirectBuffer );

            // Bind the sampler to slot 0 for the pixel shader
            if( m_descSetSampler )
                renderSystem->_setSamplers( 0u, m_descSetSampler );

            Ogre::uint32 indexOffset = 0;
            size_t drawParamOffset = 0;
            const struct wp_draw_command *cmd;
            wp_draw_foreach( cmd, m_ctx, cmds )
            {
                if( !cmd->elem_count )
                    continue;

                // Scissor rect: clamp to and normalize against the actual render target dimensions.
                {
                    Ogre::Viewport &vp = renderSystem->_getCurrentRenderViewport();
                    const float vpW = static_cast<float>( vp.getActualWidth() );
                    const float vpH = static_cast<float>( vp.getActualHeight() );
                    const float l = std::max( cmd->clip_rect.x, 0.0f );
                    const float t = std::max( cmd->clip_rect.y, 0.0f );
                    const float r = std::min( cmd->clip_rect.x + cmd->clip_rect.w, vpW );
                    const float b = std::min( cmd->clip_rect.y + cmd->clip_rect.h, vpH );
                    vp.setScissors( l / vpW, t / vpH, ( r - l ) / vpW, ( b - t ) / vpH );
                }

                // Bind the texture referenced by this draw command.
                auto *texture = static_cast<Ogre::TextureGpu *>( cmd->texture.ptr );
                if( texture )
                    renderSystem->_setTexture( 0, texture, false );

                // Write draw parameters at the current byte offset into the sw indirect buffer
                Ogre::CbDrawIndexed *drawParam = reinterpret_cast<Ogre::CbDrawIndexed *>(
                    static_cast<Ogre::uint8 *>( m_indirectBuffer->getSwBufferPtr() ) + drawParamOffset );
                drawParam->primCount = cmd->elem_count;
                drawParam->instanceCount = 1u;
                drawParam->firstVertexIndex = indexOffset;
                drawParam->baseVertex = 0u;
                drawParam->baseInstance = 0u;

                Ogre::CbDrawCallIndexed drawCall( 0, m_vao, (void *)drawParamOffset );
                drawCall.numDraws = 1u;
                renderSystem->_renderEmulated( &drawCall );

                indexOffset += cmd->elem_count;
                drawParamOffset += sizeof( Ogre::CbDrawIndexed );
            }

            // Reset scissors to full viewport
            {
                Ogre::Viewport &vp = renderSystem->_getCurrentRenderViewport();
                vp.setScissors( 0.0f, 0.0f, 1.0f, 1.0f );
            }
        }

        auto UIRenderer::getSingletonPtr() -> UIRenderer *
        {
            return s_instance;
        }

        auto UIRenderer::getSingleton() -> UIRenderer &
        {
            return ( *s_instance );
        }

        void UIRenderer::setCamera( SmartPtr<IGraphicsCamera> camera )
        {
        }

        workphone::SmartPtr<workphone::render::IGraphicsCamera> UIRenderer::getCamera() const
        {
            return nullptr;
        }

    }  // namespace render
}  // namespace workphone
