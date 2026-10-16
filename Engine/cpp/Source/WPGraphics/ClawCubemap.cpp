#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawCubemap.hpp>
#include <workphone_graphics_renderer_dx11.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <limits>

namespace workphone::render
{
    namespace
    {
        struct V
        {
            float x, y, z;

            V operator+( V b ) const
            {
                return { x + b.x, y + b.y, z + b.z };
            }

            V operator*( float s ) const
            {
                return { x * s, y * s, z * s };
            }
        };

        float dot( V a, V b )
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        V cross( V a, V b )
        {
            return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
        }

        V unit( V v )
        {
            return v * ( 1.0f / std::sqrt( std::max( dot( v, v ), 1e-12f ) ) );
        }

        struct Face
        {
            unsigned width = 0, height = 0;
            std::vector<V> pixels;
        };

        // D3D cube face order: +X, -X, +Y, -Y, +Z, -Z.
        V direction( unsigned face, float u, float v )
        {
            const V axes[] = { { 1, -v, -u }, { -1, -v, u }, { u, 1, v },
                               { u, -1, -v }, { u, -v, 1 },  { -u, -v, -1 } };
            return unit( axes[face] );
        }

        V sample( const std::array<Face, 6> &faces, V d )
        {
            if( faces[0].pixels.empty() )
            {
                // A broad sky/ground gradient and two softboxes make polished metal readable.
                const float sky = std::clamp( d.y * 0.5f + 0.5f, 0.0f, 1.0f );
                const float key =
                    std::pow( std::max( dot( d, unit( { -0.5f, 0.7f, -0.5f } ) ), 0.0f ), 48.0f );
                const float fill =
                    std::pow( std::max( dot( d, unit( { 0.8f, 0.3f, 0.4f } ) ), 0.0f ), 24.0f );
                return V{ 0.08f, 0.07f, 0.06f } * ( 1 - sky ) + V{ 0.55f, 0.65f, 0.8f } * sky +
                       V{ 6, 5.5f, 4.8f } * key + V{ 1.5f, 1.7f, 2 } * fill;
            }
            // Match renderSky's UVs, including the horizontal flip on the four side faces.
            unsigned f;
            float u, v;
            const float ax = std::abs( d.x ), ay = std::abs( d.y ), az = std::abs( d.z );
            if( ax >= ay && ax >= az )
            {
                f = d.x > 0 ? 3 : 2;
                u = ( d.x > 0 ? d.z : -d.z ) / ax;
                v = -d.y / ax;
            }
            else if( ay >= az )
            {
                f = d.y > 0 ? 4 : 5;
                u = d.x / ay;
                v = ( d.y > 0 ? d.z : -d.z ) / ay;
            }
            else
            {
                f = d.z > 0 ? 1 : 0;
                u = ( d.z > 0 ? -d.x : d.x ) / az;
                v = -d.y / az;
            }
            const auto &face = faces[f];
            const float x =
                std::clamp( ( u * 0.5f + 0.5f ) * face.width - 0.5f, 0.0f, float( face.width - 1 ) );
            const float y =
                std::clamp( ( v * 0.5f + 0.5f ) * face.height - 0.5f, 0.0f, float( face.height - 1 ) );
            const unsigned ix = unsigned( x ), iy = unsigned( y ),
                           jx = std::min( ix + 1, face.width - 1 ),
                           jy = std::min( iy + 1, face.height - 1 );
            const float fx = x - ix, fy = y - iy;
            return ( face.pixels[iy * face.width + ix] * ( 1 - fx ) +
                     face.pixels[iy * face.width + jx] * fx ) *
                       ( 1 - fy ) +
                   ( face.pixels[jy * face.width + ix] * ( 1 - fx ) +
                     face.pixels[jy * face.width + jx] * fx ) *
                       fy;
        }

        bool readFace( ID3D11Device *device, ID3D11DeviceContext *context,
                       ID3D11ShaderResourceView *view, Face &face )
        {
            wp_texture_face_readback_dx11 readback{};
            if( !wp_dx11_read_texture_face_native( device, context, view, &readback ) )
                return false;
            struct ReadbackGuard
            {
                wp_texture_face_readback_dx11 &data;
                ~ReadbackGuard() { wp_dx11_release_texture_face_readback_native( &data ); }
            } guard{ readback };
            face.width = readback.width;
            face.height = readback.height;
            face.pixels.resize( size_t( readback.width ) * readback.height );
            const bool bgra = readback.format == WP_DX11_FACE_BGRA8;
            const bool floats = readback.format == WP_DX11_FACE_RGBA32F;
            const bool halves = readback.format == WP_DX11_FACE_RGBA16F;
            const auto decode = []( float c ) { return c <= .04045f ? c / 12.92f : std::pow( ( c + .055f ) / 1.055f, 2.4f ); };
            const auto half = []( unsigned short bits ) {
                const unsigned exponent = ( bits >> 10 ) & 31, fraction = bits & 1023;
                const float magnitude = exponent == 0 ? std::ldexp( float( fraction ), -24 )
                    : exponent == 31 ? ( fraction ? std::numeric_limits<float>::quiet_NaN() : std::numeric_limits<float>::infinity() )
                    : std::ldexp( 1.f + fraction / 1024.f, int( exponent ) - 15 );
                return bits & 32768 ? -magnitude : magnitude;
            };
            bool valid = true;
            for( unsigned y = 0; y < readback.height; ++y )
            {
                for( unsigned x = 0; x < readback.width; ++x )
                {
                    const auto row = static_cast<const unsigned char *>( readback.pixels ) +
                                     y * readback.row_pitch;
                    V value;
                    if( floats )
                    {
                        const auto p = reinterpret_cast<const float *>( row ) + x * 4;
                        value = { p[0], p[1], p[2] };
                    }
                    else if( halves )
                    {
                        const auto p = reinterpret_cast<const unsigned short *>( row ) + x * 4;
                        value = { half( p[0] ), half( p[1] ), half( p[2] ) };
                    }
                    else
                    {
                        const auto p = row + x * 4;
                        value = { decode( p[bgra ? 2 : 0] / 255.f ), decode( p[1] / 255.f ), decode( p[bgra ? 0 : 2] / 255.f ) };
                    }
                    valid = valid && std::isfinite( value.x ) && std::isfinite( value.y ) && std::isfinite( value.z ) &&
                        value.x >= 0 && value.y >= 0 && value.z >= 0;
                    face.pixels[y * readback.width + x] = value;
                }
            }
            return valid;
        }
    }  // namespace

    ClawCubemap::~ClawCubemap()
    {
        reset();
    }

    void ClawCubemap::reset()
    {
        wp_dx11_cubemap_owner_destroy( m_owner );
        m_owner = nullptr;
    }

    ID3D11ShaderResourceView *ClawCubemap::getView() const
    {
        return static_cast<ID3D11ShaderResourceView *>(
            wp_dx11_cubemap_owner_get_view( m_owner ) );
    }

    ID3D11ShaderResourceView *ClawCubemap::getViewForDevice( ID3D11Device *device ) const
    {
        auto *view = getView();
        return wp_dx11_texture_view_matches_device( view, device ) ? view : nullptr;
    }

    bool ClawCubemap::update( ID3D11Device *device, ID3D11DeviceContext *context,
                              const std::array<ID3D11ShaderResourceView *, 6> &views )
    {
        if( !wp_dx11_device_matches_context( device, context ) )
        {
            return false;
        }
        std::array<void *, 6> nativeViews{};
        for( size_t i = 0; i < views.size(); ++i ) nativeViews[i] = views[i];
        if( wp_dx11_cubemap_owner_matches( m_owner, device, nativeViews.data() ) ) return true;
        std::array<Face, 6> faces;
        const bool hasFaces =
            std::all_of( views.begin(), views.end(), []( auto p ) { return p != nullptr; } );
        if( hasFaces )
        {
            for( unsigned i = 0; i < 6; ++i )
            {
                if( !readFace( device, context, views[i], faces[i] ) )
                {
                    return false;
                }
            }
        }
        if( !hasFaces &&
            std::any_of( views.begin(), views.end(), []( auto p ) { return p != nullptr; } ) )
        {
            return false;
        }
        std::array<std::vector<float>, 48> pixels;
        std::array<wp_texture_cube_mip_dx11, 48> data{};
        // GGX samples depend on roughness, not face or texel. Compute the
        // expensive trigonometry once per mip instead of millions of times.
        std::array<std::array<V, 64>, 8> halfVectors{};
        for( unsigned mip = 1; mip < 8; ++mip )
        {
            const float roughness = mip / 7.0f, a = roughness * roughness;
            for( unsigned s = 0; s < 64; ++s )
            {
                unsigned bits = s;
                float radical = 0, fraction = 0.5f;
                while( bits )
                {
                    radical += ( bits & 1 ) * fraction;
                    fraction *= 0.5f;
                    bits >>= 1;
                }
                const float phi = 6.2831853f * s / 64,
                            c = std::sqrt( ( 1 - radical ) / ( 1 + ( a * a - 1 ) * radical ) );
                const float sine = std::sqrt( std::max( 1 - c * c, 0.0f ) );
                halfVectors[mip][s] = { sine * std::cos( phi ), sine * std::sin( phi ), c };
            }
        }
        for( unsigned f = 0; f < 6; ++f )
        {
            for( unsigned mip = 0; mip < 8; ++mip )
            {
                const unsigned size = 128u >> mip, index = f * 8 + mip;
                auto &output = pixels[index];
                output.resize( size * size * 4 );
                for( unsigned y = 0; y < size; ++y )
                {
                    for( unsigned x = 0; x < size; ++x )
                    {
                        const V n =
                            direction( f, 2 * ( x + 0.5f ) / size - 1, 2 * ( y + 0.5f ) / size - 1 );
                        V sum = sample( faces, n );
                        float weight = 1;
                        if( mip > 0 )
                        {
                            sum = { 0, 0, 0 };
                            weight = 0;
                            const V t = unit( cross(
                                        std::abs( n.y ) < 0.99f ? V{ 0, 1, 0 } : V{ 1, 0, 0 }, n ) ),
                                    b = cross( n, t );
                            // Hammersley samples of GGX, weighted by N.L. Sampling the original
                            // six faces for every level allows the blur to cross cube edges.
                            for( unsigned s = 0; s < 64; ++s )
                            {
                                const auto &sampleVector = halfVectors[mip][s];
                                const V h = t * sampleVector.x + b * sampleVector.y + n * sampleVector.z;
                                const V l = h * ( 2 * dot( n, h ) ) + n * ( -1 );
                                const float w = std::max( dot( n, l ), 0.0f );
                                sum = sum + sample( faces, l ) * w;
                                weight += w;
                            }
                        }
                        const unsigned offset = ( y * size + x ) * 4;
                        output[offset] = sum.x / weight;
                        output[offset + 1] = sum.y / weight;
                        output[offset + 2] = sum.z / weight;
                        output[offset + 3] = 1;
                        if( !std::isfinite( output[offset] ) || !std::isfinite( output[offset + 1] ) ||
                            !std::isfinite( output[offset + 2] ) ) return false;
                    }
                }
                data[index] = { output.data(), size, size, output.size() };
            }
        }
        auto *nextOwner = wp_dx11_cubemap_owner_create(
            device, data.data(), static_cast<wp_u32>( data.size() ), nativeViews.data() );
        if( !nextOwner )
        {
            return false;
        }
        reset();
        m_owner = nextOwner;
        return true;
    }
}  // namespace workphone::render
