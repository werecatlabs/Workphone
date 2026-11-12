#include <FreeImage.h>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace
{
    using Bytes = std::vector<BYTE>;
    using Bitmap = std::unique_ptr<FIBITMAP, decltype( &FreeImage_Unload )>;
    using Memory = std::unique_ptr<FIMEMORY, decltype( &FreeImage_CloseMemory )>;

    void require( bool value, const char *message )
    {
        if( !value ) throw std::runtime_error( message );
    }

    Bytes makeWebP( unsigned bpp, int flags )
    {
        Bitmap bitmap( FreeImage_Allocate( 3, 2, bpp ), &FreeImage_Unload );
        require( bool( bitmap ), "Could not allocate test bitmap" );
        for( unsigned y = 0; y < 2; ++y )
        {
            auto row = FreeImage_GetScanLine( bitmap.get(), y );
            for( unsigned x = 0; x < 3; ++x )
            {
                auto pixel = row + x * ( bpp / 8 );
                pixel[FI_RGBA_RED] = static_cast<BYTE>( 30 + x * 60 );
                pixel[FI_RGBA_GREEN] = static_cast<BYTE>( 40 + y * 80 );
                pixel[FI_RGBA_BLUE] = static_cast<BYTE>( 200 - x * 30 );
                if( bpp == 32 ) pixel[FI_RGBA_ALPHA] = static_cast<BYTE>( 80 + x * 60 );
            }
        }
        Memory memory( FreeImage_OpenMemory(), &FreeImage_CloseMemory );
        require( bool( memory ) &&
                     FreeImage_SaveToMemory( FIF_WEBP, bitmap.get(), memory.get(), flags ),
                 "Could not encode test WebP" );
        BYTE *data = nullptr;
        DWORD size = 0;
        require( FreeImage_AcquireMemory( memory.get(), &data, &size ) && size > 20,
                 "Missing encoded WebP bytes" );
        return Bytes( data, data + size );
    }

    Bytes decode( Bytes &encoded )
    {
        Memory memory( FreeImage_OpenMemory( encoded.data(), static_cast<DWORD>( encoded.size() ) ),
                       &FreeImage_CloseMemory );
        require( bool( memory ), "Could not open test memory" );
        require( FreeImage_GetFileTypeFromMemory( memory.get() ) == FIF_WEBP,
                 "WebP signature detection failed" );
        require( FreeImage_SeekMemory( memory.get(), 0, SEEK_SET ), "Header seek failed" );
        Bitmap header( FreeImage_LoadFromMemory( FIF_WEBP, memory.get(), FIF_LOAD_NOPIXELS ),
                       &FreeImage_Unload );
        require( bool( header ), "WebP header rejected valid bytes with trailing container padding" );
        require( !FreeImage_HasPixels( header.get() ) && FreeImage_GetWidth( header.get() ) == 3 &&
                     FreeImage_GetHeight( header.get() ) == 2 && FreeImage_GetPitch( header.get() ) == 12,
                 "WebP header dimensions or bounded header-only behavior changed" );
        const auto bpp = FreeImage_GetBPP( header.get() );
        require( FreeImage_SeekMemory( memory.get(), 0, SEEK_SET ), "Decode seek failed" );
        Bitmap bitmap( FreeImage_LoadFromMemory( FIF_WEBP, memory.get() ), &FreeImage_Unload );
        require( bool( bitmap ) && FreeImage_HasPixels( bitmap.get() ) &&
                     FreeImage_GetWidth( bitmap.get() ) == 3 && FreeImage_GetHeight( bitmap.get() ) == 2 &&
                     FreeImage_GetPitch( bitmap.get() ) == 12 && FreeImage_GetBPP( bitmap.get() ) == bpp,
                 "Decoded WebP disagrees with its admitted header" );
        Bitmap rgba( FreeImage_ConvertTo32Bits( bitmap.get() ), &FreeImage_Unload );
        require( bool( rgba ), "WebP RGBA conversion failed" );
        Bytes pixels;
        for( unsigned y = 0; y < 2; ++y )
        {
            const auto row = FreeImage_GetScanLine( rgba.get(), y );
            pixels.insert( pixels.end(), row, row + 3 * 4 );
        }
        return pixels;
    }

    void requireRejected( Bytes encoded )
    {
        Memory memory( FreeImage_OpenMemory( encoded.data(), static_cast<DWORD>( encoded.size() ) ),
                       &FreeImage_CloseMemory );
        require( bool( memory ), "Could not open malformed test memory" );
        Bitmap header( FreeImage_LoadFromMemory( FIF_WEBP, memory.get(), FIF_LOAD_NOPIXELS ),
                       &FreeImage_Unload );
        require( !header, "Truncated WebP unexpectedly passed header preflight" );
        require( FreeImage_SeekMemory( memory.get(), 0, SEEK_SET ), "Malformed seek failed" );
        Bitmap bitmap( FreeImage_LoadFromMemory( FIF_WEBP, memory.get() ), &FreeImage_Unload );
        require( !bitmap, "Truncated WebP unexpectedly decoded" );
    }

    void checkContainerPadding( unsigned bpp, int flags )
    {
        auto encoded = makeWebP( bpp, flags );
        const auto pixels = decode( encoded );
        for( const auto padding : { 1u, 2u, 3u, 8u, 9u, 16u } )
        {
            auto padded = encoded;
            padded.insert( padded.end(), padding, BYTE( 0xA5 ) );
            require( decode( padded ) == pixels, "Trailing padding changed decoded WebP pixels" );
        }
        auto truncated = encoded;
        truncated.pop_back();
        requireRejected( truncated );
        requireRejected( Bytes( encoded.begin(), encoded.begin() + 12 ) );
        auto oversized = encoded;
        std::fill( oversized.begin() + 4, oversized.begin() + 8, BYTE( 0xFF ) );
        requireRejected( oversized );
    }
}

int main()
{
    FreeImage_Initialise( TRUE );
    try
    {
        require( FreeImage_FIFSupportsNoPixels( FIF_WEBP ), "WebP requires bounded header preflight" );
        checkContainerPadding( 24, WEBP_DEFAULT );
        checkContainerPadding( 32, WEBP_DEFAULT );
        checkContainerPadding( 32, WEBP_LOSSLESS );
        FreeImage_DeInitialise();
        std::cout << "WebP container padding, bounded headers, pixels and truncation contracts passed\n";
        return 0;
    }
    catch( const std::exception &error )
    {
        FreeImage_DeInitialise();
        std::cerr << error.what() << '\n';
        return 1;
    }
}
