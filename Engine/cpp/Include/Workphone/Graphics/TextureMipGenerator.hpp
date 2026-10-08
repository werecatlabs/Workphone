#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace workphone::render
{
    enum class TextureMipFilter
    {
        None,
        Colour,
        Data,
        Normal,
        Cutout,
        Roughness
    };
    struct TextureMipSettings
    {
        TextureMipFilter filter = TextureMipFilter::None;
        std::uint32_t atlasColumns = 1;
        float alphaCutoff = .5f;
    };
    struct TextureMipLevel
    {
        std::uint32_t width = 0, height = 0;
        std::vector<std::uint8_t> bgra;
    };

    // BGRA output retains the shader's existing colour decode. Atlas tiles never mix.
    inline std::vector<TextureMipLevel> generateTextureMips( const std::uint8_t *pixels,
                                                             std::uint32_t width, std::uint32_t height,
                                                             const TextureMipSettings &settings )
    {
        if( !pixels || !width || !height || width > 16384 || height > 16384 || !settings.atlasColumns ||
            width % settings.atlasColumns || !std::isfinite( settings.alphaCutoff ) ||
            settings.alphaCutoff <= 0 || settings.alphaCutoff >= 1 )
            throw std::invalid_argument( "Invalid texture mip input." );
        std::vector<TextureMipLevel> levels;
        levels.push_back( { width, height, { pixels, pixels + size_t( width ) * height * 4 } } );
        if( settings.filter == TextureMipFilter::None )
            return levels;
        const bool colour =
            settings.filter == TextureMipFilter::Colour || settings.filter == TextureMipFilter::Cutout;
        const auto byte = []( float v ) {
            return std::uint8_t( std::lround( std::clamp( v, 0.f, 1.f ) * 255 ) );
        };
        while( levels.back().width > 1 || levels.back().height > 1 )
        {
            const auto &src = levels.back();
            const auto w = std::max( src.width / 2, 1u ), h = std::max( src.height / 2, 1u );
            // D3D halves whole-image dimensions: truncate the chain before merging views.
            if( w % settings.atlasColumns || w < settings.atlasColumns )
                break;
            TextureMipLevel dst{ w, h, std::vector<std::uint8_t>( size_t( w ) * h * 4 ) };
            const auto sw = src.width / settings.atlasColumns, dw = w / settings.atlasColumns;
            for( std::uint32_t y = 0; y < h; ++y )
                for( std::uint32_t x = 0; x < w; ++x )
                {
                    const auto tile = x / dw, local = x % dw;
                    const auto x0 = tile * sw + local * sw / dw,
                               x1 = tile * sw + ( local + 1 ) * sw / dw;
                    const auto y0 = y * src.height / h, y1 = ( y + 1 ) * src.height / h;
                    float sum[3]{}, alpha = 0, weight = 0;
                    const float count = float( ( x1 - x0 ) * ( y1 - y0 ) );
                    for( auto sy = y0; sy < y1; ++sy )
                        for( auto sx = x0; sx < x1; ++sx )
                        {
                            const auto *p = src.bgra.data() + ( size_t( sy ) * src.width + sx ) * 4;
                            const float a = p[3] / 255.f, wt = colour ? a : 1.f;
                            alpha += a;
                            weight += wt;
                            for( int c = 0; c < 3; ++c )
                            {
                                const float v = p[c] / 255.f;
                                sum[c] +=
                                    wt * ( colour ? std::pow( v, 2.2f )
                                           : settings.filter == TextureMipFilter::Normal ? v * 2 - 1
                                           : settings.filter == TextureMipFilter::Roughness
                                               ? v * v * v * v
                                               : v );
                            }
                        }
                    for( auto &v : sum )
                        v /= std::max( weight, 1e-8f );
                    if( settings.filter == TextureMipFilter::Normal )
                    {
                        const float len =
                            std::sqrt( sum[0] * sum[0] + sum[1] * sum[1] + sum[2] * sum[2] );
                        if( len < 1e-6f )
                        {
                            sum[0] = 1;
                            sum[1] = sum[2] = 0;
                        }
                        else
                            for( auto &v : sum )
                                v /= len;
                    }
                    auto *p = dst.bgra.data() + ( size_t( y ) * w + x ) * 4;
                    for( int c = 0; c < 3; ++c )
                        p[c] = byte( colour ? std::pow( sum[c], 1.f / 2.2f )
                                     : settings.filter == TextureMipFilter::Normal ? sum[c] * .5f + .5f
                                     : settings.filter == TextureMipFilter::Roughness
                                         ? std::pow( sum[c], .25f )
                                         : sum[c] );
                    p[3] = byte( alpha / count );
                }
            if( settings.filter == TextureMipFilter::Cutout )
            {
                for( std::uint32_t tile = 0; tile < settings.atlasColumns; ++tile )
                {
                    size_t covered = 0;
                    for( std::uint32_t y = 0; y < src.height; ++y )
                        for( std::uint32_t x = 0; x < sw; ++x )
                            covered +=
                                src.bgra[( size_t( y ) * src.width + tile * sw + x ) * 4 + 3] / 255.f >=
                                settings.alphaCutoff;
                    const float desired = float( covered ) / float( size_t( sw ) * src.height );
                    float low = 0, high = 8, best = 1, error = 2;
                    for( int it = 0; it < 16; ++it )
                    {
                        const float scale = it == 0 ? 1.f : ( low + high ) * .5f;
                        size_t actual = 0;
                        for( std::uint32_t y = 0; y < h; ++y )
                            for( std::uint32_t x = 0; x < dw; ++x )
                                actual += byte( dst.bgra[( size_t( y ) * w + tile * dw + x ) * 4 + 3] /
                                                255.f * scale ) /
                                              255.f >=
                                          settings.alphaCutoff;
                        const float coverage = float( actual ) / float( size_t( dw ) * h );
                        if( std::abs( coverage - desired ) < error )
                        {
                            error = std::abs( coverage - desired );
                            best = scale;
                        }
                        if( coverage < desired )
                            low = scale;
                        else
                            high = scale;
                    }
                    for( std::uint32_t y = 0; y < h; ++y )
                        for( std::uint32_t x = 0; x < dw; ++x )
                        {
                            auto &a = dst.bgra[( size_t( y ) * w + tile * dw + x ) * 4 + 3];
                            a = byte( a / 255.f * best );
                        }
                }
            }
            levels.push_back( std::move( dst ) );
        }
        return levels;
    }
}  // namespace workphone::render
