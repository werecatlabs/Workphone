#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIWorkphoneRenderer.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <Workphone/Workphone.hpp>
#include <workphone.h>
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_renderer_dx11.h>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace
{
    struct ClawUIDrawVertex
    {
        wp_f32 position[2];
        wp_u8 colour[4];
        wp_f32 uv[2];
    };
}  // namespace

namespace workphone::ui
{
    ClawUIWorkphoneRenderer::ClawUIWorkphoneRenderer()
    {
        memset( &m_commandBuffer, 0, sizeof( m_commandBuffer ) );
        memset( &m_vertexBuffer, 0, sizeof( m_vertexBuffer ) );
        memset( &m_indexBuffer, 0, sizeof( m_indexBuffer ) );
        memset( &m_config, 0, sizeof( m_config ) );

        // Describe a vertex with position (float2), colour (RGBA32), uv (float2).
        static const struct wp_draw_vertex_layout_element vertexLayout[] = {
            { WORKPHONE_VERTEX_POSITION, WORKPHONE_FORMAT_FLOAT,
              offsetof( ClawUIDrawVertex, position ) },
            { WORKPHONE_VERTEX_COLOR, WORKPHONE_FORMAT_R8G8B8A8, offsetof( ClawUIDrawVertex, colour ) },
            { WORKPHONE_VERTEX_TEXCOORD, WORKPHONE_FORMAT_FLOAT, offsetof( ClawUIDrawVertex, uv ) },
            WORKPHONE_VERTEX_LAYOUT_END
        };

        m_config.vertex_layout = vertexLayout;
        m_config.vertex_size = sizeof( ClawUIDrawVertex );
        m_config.vertex_alignment = WORKPHONE_ALIGNOF( wp_f32 );
        m_config.global_alpha = 1.0f;
        m_config.line_AA = WORKPHONE_ANTI_ALIASING_ON;
        m_config.shape_AA = WORKPHONE_ANTI_ALIASING_ON;
        m_config.circle_segment_count = 22;
        m_config.arc_segment_count = 22;
        m_config.curve_segment_count = 22;
        m_config.tex_null.texture.id = 0;
        m_config.tex_null.uv.x = 0.0f;
        m_config.tex_null.uv.y = 0.0f;
    }

    ClawUIWorkphoneRenderer::~ClawUIWorkphoneRenderer()
    {
        releaseFontTextureNative();

        // wp_buffer_free() does not release fixed buffers; we own the backing memory.
        delete[] static_cast<wp_byte *>( m_commandBuffer.memory.ptr );
        delete[] static_cast<wp_byte *>( m_vertexBuffer.memory.ptr );
        delete[] static_cast<wp_byte *>( m_indexBuffer.memory.ptr );
    }

    void ClawUIWorkphoneRenderer::clearCachedFontTexture()
    {
        releaseFontTextureNative();
    }

    void ClawUIWorkphoneRenderer::releaseFontTextureNative()
    {
        if( m_fontTextureNative && !m_fontTextureIsPixelPointer &&
            m_fontTextureRenderer )
        {
            if( auto *dx11 = wp_renderer_get_dx11( m_fontTextureRenderer ) )
            {
                wp_renderer_dx11_destroy_texture_native( m_fontTextureNative );
            }
        }
        m_fontTextureNative = nullptr;
        m_fontTextureRenderer = nullptr;
        m_fontTextureCachedWidth = 0;
        m_fontTextureCachedHeight = 0;
        m_fontTextureIsPixelPointer = false;
    }

    void *ClawUIWorkphoneRenderer::ensureFontTextureNative( struct wp_renderer *renderer,
                                                         const void *fontPixels,
                                                         wp_s32 fontWidth,
                                                         wp_s32 fontHeight )
    {
        // No pixels or zero-sized atlas: nothing to cache.
        if( !renderer || !fontPixels || fontWidth <= 0 || fontHeight <= 0 )
        {
            releaseFontTextureNative();
            return nullptr;
        }

        // For the software backend wp_renderer_set_texture just stores the pixel pointer,
        // so the cached "native" value is just the pixel pointer itself.
        if( wp_renderer_get_type( renderer ) != WORKPHONE_RENDERER_TYPE_DX11 )
        {
            if( m_fontTextureNative == fontPixels &&
                m_fontTextureRenderer == renderer && m_fontTextureCachedWidth == fontWidth &&
                m_fontTextureCachedHeight == fontHeight && m_fontTextureIsPixelPointer )
            {
                return m_fontTextureNative;
            }
            releaseFontTextureNative();
            m_fontTextureNative = const_cast< void * >( fontPixels );
            m_fontTextureRenderer = renderer;
            m_fontTextureCachedWidth = fontWidth;
            m_fontTextureCachedHeight = fontHeight;
            m_fontTextureIsPixelPointer = true;
            return m_fontTextureNative;
        }

        // DX11 backend: upload pixels into a native SRV that we own.
        if( m_fontTextureNative && m_fontTextureRenderer == renderer &&
            m_fontTextureCachedWidth == fontWidth && m_fontTextureCachedHeight == fontHeight &&
            !m_fontTextureIsPixelPointer )
        {
            return m_fontTextureNative;
        }

        releaseFontTextureNative();
        auto *dx11 = wp_renderer_get_dx11( renderer );
        if( !dx11 )
        {
            return nullptr;
        }

        m_fontTextureNative = wp_renderer_dx11_create_texture_native(
            dx11, fontPixels, fontWidth, fontHeight, WORKPHONE_PIXEL_FORMAT_RGBA8 );
        m_fontTextureRenderer = renderer;
        m_fontTextureCachedWidth = fontWidth;
        m_fontTextureCachedHeight = fontHeight;
        m_fontTextureIsPixelPointer = false;
        return m_fontTextureNative;
    }

    bool ClawUIWorkphoneRenderer::convert( struct wp_context *ctx )
    {
        if( !ctx )
        {
            return false;
        }

        // Count commands so we can size buffers.  WorkphoneCore stores UI commands in a
        // singly-linked list accessible via wp__begin/wp__next.
        m_commandCount = 0;
        for( const struct wp_command *cmd = wp__begin( ctx ); cmd != nullptr;
             cmd = wp__next( ctx, cmd ) )
        {
            ++m_commandCount;
        }

        auto commandBytes =
            std::max<size_t>( m_commandCount * sizeof( struct wp_draw_command ) * 2, 16 * 1024 );
        auto vertexBytes = std::max<size_t>( m_commandCount * 256 * m_config.vertex_size, 256 * 1024 );
        auto indexBytes = std::max<size_t>( m_commandCount * 512 * sizeof( wp_draw_index ), 128 * 1024 );

        wp_flags result = WORKPHONE_CONVERT_SUCCESS;
        for( u32 attempt = 0; attempt < 3; ++attempt )
        {
            resizeBuffers( commandBytes, vertexBytes, indexBytes );
            result = wp_convert( ctx, &m_commandBuffer, &m_vertexBuffer, &m_indexBuffer, &m_config );
            if( result == WORKPHONE_CONVERT_SUCCESS )
            {
                break;
            }

            commandBytes = std::max<size_t>( commandBytes * 2, m_commandBuffer.needed * 2 );
            vertexBytes = std::max<size_t>( vertexBytes * 2, m_vertexBuffer.needed * 2 );
            indexBytes = std::max<size_t>( indexBytes * 2, m_indexBuffer.needed * 2 );
        }

        if( result != WORKPHONE_CONVERT_SUCCESS )
        {
            WP_LOG_ERROR( "WorkphoneCore UI conversion failed after growing its draw buffers" );
            m_commandCount = 0;
            m_vertexCount = 0;
            m_indexCount = 0;
            return false;
        }

        m_vertexCount = m_vertexBuffer.allocated / m_config.vertex_size;
        m_indexCount = m_indexBuffer.allocated / sizeof( wp_draw_index );
        m_commandCount = 0;
        const struct wp_draw_command *drawCommand = nullptr;
        wp_draw_foreach( drawCommand, ctx, &m_commandBuffer )
        {
            if( drawCommand->elem_count > 0 )
            {
                ++m_commandCount;
            }
        }
        return true;
    }

    bool ClawUIWorkphoneRenderer::submit( struct wp_context *ctx, wp_renderer *renderer,
                                          const void *fontPixels, wp_s32 fontWidth, wp_s32 fontHeight,
                                          wp_handle fontTexture )
    {
        if( !ctx || !renderer || m_vertexCount == 0 || m_indexCount == 0 ||
            sizeof( wp_draw_index ) != sizeof( uint16_t ) )
        {
            return false;
        }

        const auto viewport = wp_renderer_get_viewport( renderer );
        const auto width = viewport.width;
        const auto height = viewport.height;
        if( width <= 0 || height <= 0 )
        {
            return false;
        }

        const auto *sourceVertices = static_cast<const ClawUIDrawVertex *>( getVertices() );
        std::vector<wp_vertex_ptc> vertices( m_vertexCount );
        for( size_t i = 0; i < m_vertexCount; ++i )
        {
            const auto &source = sourceVertices[i];
            auto &target = vertices[i];
            target.position.x = ( 2.0f * source.position[0] / static_cast<wp_f32>( width ) ) - 1.0f;
            target.position.y = 1.0f - ( 2.0f * source.position[1] / static_cast<wp_f32>( height ) );
            target.position.z = 0.0f;
            target.uv.x = source.uv[0];
            target.uv.y = source.uv[1];
            target.color = ( static_cast<wp_u32>( source.colour[0] ) << 24 ) |
                           ( static_cast<wp_u32>( source.colour[1] ) << 16 ) |
                           ( static_cast<wp_u32>( source.colour[2] ) << 8 ) |
                           static_cast<wp_u32>( source.colour[3] );
        }

        const auto oldBlend = wp_renderer_get_blend_mode( renderer );
        const auto oldCull = wp_renderer_get_cull_mode( renderer );
        const auto oldDepthTest = wp_renderer_get_depth_test_enabled( renderer );
        const auto oldDepthWrite = wp_renderer_get_depth_write_enabled( renderer );

        wp_mat4f identity;
        wp_mat4f_identity( &identity );
        wp_renderer_set_world_matrix( renderer, &identity );
        wp_renderer_set_view_matrix( renderer, &identity );
        wp_renderer_set_projection_matrix( renderer, &identity );
        wp_renderer_set_blend_mode( renderer, WORKPHONE_BLEND_MODE_ALPHA );
        wp_renderer_set_cull_mode( renderer, WORKPHONE_CULL_MODE_NONE );
        wp_renderer_set_depth_test_enabled( renderer, wp_false );
        wp_renderer_set_depth_write_enabled( renderer, wp_false );
        wp_renderer_set_scissor_enabled( renderer, wp_true );

        // Resolve the font texture once so we never re-upload the atlas from CPU pixels
        // per text draw and so every texture bind in the loop below goes through the
        // same native API path.  The render backend looks up the bound handle via
        // ptc_external_texture_view, so this matches the way ClawImguiManager binds
        // its font atlas (avoids the stale ptc_texture_view bug described there).
        void *fontNative = nullptr;
        if( fontPixels && fontWidth > 0 && fontHeight > 0 )
        {
            fontNative = ensureFontTextureNative( renderer, fontPixels, fontWidth, fontHeight );
        }

        // If we still have no native font handle (e.g. fontPixels was null because the
        // atlas bake failed), fall back to the fontTexture wp_handle pointer.  WorkphoneCore
        // stores the font atlas SRV as wp_handle_ptr(fontPixels.data()) on the context, so
        // the font texture's `.ptr` is already the GPU-side handle for our purposes.  This
        // keeps every draw binding a real (non-null) handle, avoiding the same stale
        // ptc_external_texture_view leak the previous "fall through to nullptr" path had.
        if( !fontNative )
        {
            fontNative = const_cast< void * >( fontTexture.ptr );
        }

        size_t indexOffset = 0;
        const struct wp_draw_command *command = nullptr;
        wp_draw_foreach( command, ctx, &m_commandBuffer )
        {
            if( command->elem_count == 0 )
            {
                continue;
            }

            wp_viewport_i scissor = { viewport.x + static_cast<wp_s32>(
                                                      std::max( 0.0f, command->clip_rect.x ) ),
                                      viewport.y + static_cast<wp_s32>(
                                                       std::max( 0.0f, command->clip_rect.y ) ),
                                      static_cast<wp_s32>( std::max( 0.0f, command->clip_rect.w ) ),
                                      static_cast<wp_s32>( std::max( 0.0f, command->clip_rect.h ) ) };
            wp_renderer_set_scissor_rect( renderer, scissor );

            // Pick a native handle for every command.  All three branches go through
            // wp_renderer_set_texture_native so the backend's ptc_external_texture_view is
            // always overwritten (the old mixed path let stale ptc_texture_view survive
            // across draws).
            void *textureView = nullptr;
            if( command->texture.ptr == fontTexture.ptr && fontPixels )
            {
                textureView = fontNative;
            }
            else if( command->texture.ptr )
            {
                auto *texture = static_cast<render::ITexture *>( command->texture.ptr );
                void *nativeTexture = nullptr;
                texture->getTextureFinal( &nativeTexture );
                textureView = nativeTexture ? nativeTexture : fontNative;
            }
            else
            {
                textureView = fontNative;
            }

            wp_renderer_set_texture_native( renderer, textureView );

            wp_renderer_draw_indexed_triangles_ptc(
                renderer, vertices.data(), static_cast<wp_s32>( vertices.size() ),
                getIndices() + indexOffset, static_cast<wp_s32>( command->elem_count ) );
            indexOffset += command->elem_count;
        }

        // Restore renderer state; leave the native texture bound on the white fallback so
        // a subsequent scene draw doesn't see a dangling SRV pointer.
        wp_renderer_set_texture_native( renderer, nullptr );
        wp_renderer_set_scissor_enabled( renderer, wp_false );
        wp_renderer_set_blend_mode( renderer, oldBlend );
        wp_renderer_set_cull_mode( renderer, oldCull );
        wp_renderer_set_depth_test_enabled( renderer, oldDepthTest );
        wp_renderer_set_depth_write_enabled( renderer, oldDepthWrite );
        return true;
    }

    void ClawUIWorkphoneRenderer::setNullTexture( const struct wp_draw_null_texture &nullTexture )
    {
        m_config.tex_null = nullTexture;
    }

    size_t ClawUIWorkphoneRenderer::getCommandCount() const
    {
        return m_commandCount;
    }

    const struct wp_draw_command *ClawUIWorkphoneRenderer::getCommands() const
    {
        return static_cast<const struct wp_draw_command *>( m_commandBuffer.memory.ptr );
    }

    const void *ClawUIWorkphoneRenderer::getVertices() const
    {
        return m_vertexBuffer.memory.ptr;
    }

    size_t ClawUIWorkphoneRenderer::getVertexCount() const
    {
        return m_vertexCount;
    }

    const wp_draw_index *ClawUIWorkphoneRenderer::getIndices() const
    {
        return static_cast<const wp_draw_index *>( m_indexBuffer.memory.ptr );
    }

    size_t ClawUIWorkphoneRenderer::getIndexCount() const
    {
        return m_indexCount;
    }

    const struct wp_draw_vertex_layout_element *ClawUIWorkphoneRenderer::getVertexLayout()
    {
        static const struct wp_draw_vertex_layout_element layout[] = {
            { WORKPHONE_VERTEX_POSITION, WORKPHONE_FORMAT_FLOAT,
              offsetof( ClawUIDrawVertex, position ) },
            { WORKPHONE_VERTEX_COLOR, WORKPHONE_FORMAT_R8G8B8A8, offsetof( ClawUIDrawVertex, colour ) },
            { WORKPHONE_VERTEX_TEXCOORD, WORKPHONE_FORMAT_FLOAT, offsetof( ClawUIDrawVertex, uv ) },
            WORKPHONE_VERTEX_LAYOUT_END
        };
        return layout;
    }

    void ClawUIWorkphoneRenderer::resizeBuffers( size_t commandBytes, size_t vertexBytes,
                                                 size_t indexBytes )
    {
        if( m_commandBuffer.memory.ptr == nullptr || m_commandBuffer.memory.size < commandBytes )
        {
            delete[] static_cast<wp_byte *>( m_commandBuffer.memory.ptr );
            void *memory = new wp_byte[commandBytes];
            wp_buffer_init_fixed( &m_commandBuffer, memory, commandBytes );
        }
        wp_buffer_clear( &m_commandBuffer );

        if( m_vertexBuffer.memory.ptr == nullptr || m_vertexBuffer.memory.size < vertexBytes )
        {
            delete[] static_cast<wp_byte *>( m_vertexBuffer.memory.ptr );
            void *memory = new wp_byte[vertexBytes];
            wp_buffer_init_fixed( &m_vertexBuffer, memory, vertexBytes );
        }
        wp_buffer_clear( &m_vertexBuffer );

        if( m_indexBuffer.memory.ptr == nullptr || m_indexBuffer.memory.size < indexBytes )
        {
            delete[] static_cast<wp_byte *>( m_indexBuffer.memory.ptr );
            void *memory = new wp_byte[indexBytes];
            wp_buffer_init_fixed( &m_indexBuffer, memory, indexBytes );
        }
        wp_buffer_clear( &m_indexBuffer );
    }
}  // namespace workphone::ui
