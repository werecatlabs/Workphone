#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawDepthReadback.hpp>
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_renderer_software.h>
#include <algorithm>
#include <cmath>
#if defined WP_PLATFORM_WIN32 && WP_BUILD_RENDERER_DX11
#    include <WorkphonePlatformWin32/workphone_graphics_depth_readback_dx11.h>
#endif

namespace workphone::render
{
    struct ClawDepthReadback::Native
    {
#if defined WP_PLATFORM_WIN32 && WP_BUILD_RENDERER_DX11
        wp_depth_readback_dx11 *handle = wp_depth_readback_dx11_create();

        ~Native()
        {
            wp_depth_readback_dx11_destroy( handle );
        }
#endif
    };

    ClawDepthReadback::ClawDepthReadback() = default;
    ClawDepthReadback::~ClawDepthReadback() = default;

    ClawDepthReadback::Surface ClawDepthReadback::surface( wp_renderer *renderer ) const
    {
        if( !renderer )
        {
            return {};
        }
        if( auto software = wp_renderer_get_software( renderer ) )
        {
            const auto w = wp_renderer_software_get_width( software ),
                       h = wp_renderer_software_get_height( software );
            const auto viewport = wp_renderer_get_viewport( renderer );
            if( viewport.x < 0 || viewport.y < 0 || viewport.width <= 0 || viewport.height <= 0 ||
                viewport.x + viewport.width > w || viewport.y + viewport.height > h )
            {
                return {};
            }
            return { unsigned( w ),
                     unsigned( h ),
                     unsigned( viewport.x ),
                     unsigned( viewport.y ),
                     unsigned( viewport.width ),
                     unsigned( viewport.height ),
                     1 };
        }
#if defined WP_PLATFORM_WIN32 && WP_BUILD_RENDERER_DX11
        if( auto dx11 = wp_renderer_get_dx11( renderer ) )
        {
            wp_u32 width, height, samples;
            if( wp_renderer_dx11_depth_surface( dx11, &width, &height, &samples ) )
            {
                return { width, height, 0, 0, width, height, samples };
            }
        }
#endif
        return {};
    }

    bool ClawDepthReadback::read( wp_renderer *renderer, std::vector<float> &depth, unsigned &width,
                                  unsigned &height )
    {
        const auto dimensions = surface( renderer );
        if( !dimensions[4] || !dimensions[5] )
        {
            return false;
        }
        width = std::min( dimensions[4], 256u );
        height = std::min( dimensions[5], 256u );
        depth.assign( size_t( width ) * height, 1.0f );
        if( auto software = wp_renderer_get_software( renderer ) )
        {
            const auto source = wp_renderer_software_get_depth_buffer( software );
            if( !source )
            {
                return false;
            }
            for( unsigned y = 0; y < height; ++y )
            {
                for( unsigned x = 0; x < width; ++x )
                {
                    float farthest = 0;
                    const auto yEnd = ( ( y + 1 ) * dimensions[5] + height - 1 ) / height;
                    const auto xEnd = ( ( x + 1 ) * dimensions[4] + width - 1 ) / width;
                    for( auto sy = y * dimensions[5] / height; sy < yEnd; ++sy )
                    {
                        for( auto sx = x * dimensions[4] / width; sx < xEnd; ++sx )
                        {
                            const auto sample = source[size_t( sy + dimensions[3] ) * dimensions[0] +
                                                       sx + dimensions[2]];
                            farthest = std::max(
                                farthest,
                                std::isfinite( sample ) && sample >= 0 && sample <= 1 ? sample : 1.0f );
                        }
                    }
                    depth[size_t( y ) * width + x] = farthest;
                }
            }
            return true;
        }
#if defined WP_PLATFORM_WIN32 && WP_BUILD_RENDERER_DX11
        if( auto dx11 = wp_renderer_get_dx11( renderer ) )
        {
            if( !m_native || !m_native->handle )
            {
                m_native = std::make_unique<Native>();
            }
            return wp_renderer_dx11_read_depth( m_native->handle, dx11, depth.data(), width, height,
                                               static_cast<wp_u32>( depth.size() ) ) != 0;
        }
#endif
        return false;
    }
}  // namespace workphone::render
