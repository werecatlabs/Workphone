#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace workphone::scene::citySurfaceScalar
{
    // The masked scalar slot samples red only. A constant always evaluates
    // identically from one texel, including every valid sampler and mip level.
    inline procedural::TextureBuffer split( const procedural::TextureBuffer &source, unsigned channel )
    {
        const auto bytes = source.pixels.size();
        if( channel >= 3 || !source.width || !source.height || bytes % 4 != 0 ||
            std::uint64_t( source.width ) * source.height != bytes / 4 )
        {
            throw std::invalid_argument(
                "City scalar map requires a complete RGBA source and ORM channel." );
        }

        const auto first = source.pixels[channel];
        bool constant = true;
        for( std::size_t pixel = 4; pixel < bytes; pixel += 4 )
        {
            if( source.pixels[pixel + channel] != first )
            {
                constant = false;
                break;
            }
        }

        procedural::TextureBuffer result( constant ? 1u : source.width, constant ? 1u : source.height );
        for( std::size_t pixel = 0; pixel < result.pixels.size(); pixel += 4 )
        {
            result.pixels[pixel] = result.pixels[pixel + 1] = result.pixels[pixel + 2] =
                source.pixels[pixel + channel];
            result.pixels[pixel + 3] = 255;
        }
        return result;
    }
}  // namespace workphone::scene::citySurfaceScalar
