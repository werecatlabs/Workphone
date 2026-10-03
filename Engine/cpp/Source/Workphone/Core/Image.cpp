#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/Image.hpp>

namespace workphone
{

    Image::Image() = default;

    Image::~Image() = default;

    const ColourF &Image::at( s32 x, s32 y ) const
    {
        return pixels[static_cast<std::size_t>( y ) * width + x];
    }

    ColourF &Image::at( s32 x, s32 y )
    {
        return pixels[static_cast<std::size_t>( y ) * width + x];
    }

}  // namespace workphone
