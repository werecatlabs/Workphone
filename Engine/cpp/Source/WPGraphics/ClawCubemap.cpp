#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawCubemap.hpp>
#include <d3d11.h>
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
            ID3D11Resource *resource = nullptr;
            ID3D11Texture2D *source = nullptr, *staging = nullptr;
            view->GetResource( &resource );
            const auto hr = resource->QueryInterface( __uuidof( ID3D11Texture2D ),
                                                      reinterpret_cast<void **>( &source ) );
            resource->Release();
            if( FAILED( hr ) )
            {
                return false;
            }
            D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
            view->GetDesc( &viewDesc );
            if( viewDesc.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE2D )
            {
                source->Release();
                return false;
            }
            D3D11_TEXTURE2D_DESC desc;
            source->GetDesc( &desc );
            ID3D11Device *sourceDevice = nullptr;
            source->GetDevice( &sourceDevice );
            const bool sameDevice = sourceDevice == device;
            if( sourceDevice ) sourceDevice->Release();
            if( !sameDevice ) { source->Release(); return false; }
            const auto format = viewDesc.Format;
            const bool bgra = format == DXGI_FORMAT_B8G8R8A8_UNORM || format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
            const bool rgba = format == DXGI_FORMAT_R8G8B8A8_UNORM || format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            const bool floats = format == DXGI_FORMAT_R32G32B32A32_FLOAT;
            const bool halves = format == DXGI_FORMAT_R16G16B16A16_FLOAT;
            const auto mip = viewDesc.Texture2D.MostDetailedMip;
            if( (!bgra && !rgba && !floats && !halves) || desc.ArraySize != 1 ||
                desc.SampleDesc.Count != 1 || mip >= desc.MipLevels )
            {
                source->Release();
                return false;
            }
            desc.Width = std::max( 1u, desc.Width >> mip );
            desc.Height = std::max( 1u, desc.Height >> mip );
            if( desc.Width > 2048 || desc.Height > 2048 ) { source->Release(); return false; }
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.BindFlags = 0;
            desc.MiscFlags = 0;
            desc.Usage = D3D11_USAGE_STAGING;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            if( FAILED( device->CreateTexture2D( &desc, nullptr, &staging ) ) )
            {
                source->Release();
                return false;
            }
            context->CopySubresourceRegion( staging, 0, 0, 0, 0, source, mip, nullptr );
            source->Release();
            D3D11_MAPPED_SUBRESOURCE mapped;
            if( FAILED( context->Map( staging, 0, D3D11_MAP_READ, 0, &mapped ) ) )
            {
                staging->Release();
                return false;
            }
            face.width = desc.Width;
            face.height = desc.Height;
            face.pixels.resize( size_t( desc.Width ) * desc.Height );
            const auto decode = []( float c ) { return c <= .04045f ? c / 12.92f : std::pow( ( c + .055f ) / 1.055f, 2.4f ); };
            const auto half = []( unsigned short bits ) {
                const unsigned exponent = ( bits >> 10 ) & 31, fraction = bits & 1023;
                const float magnitude = exponent == 0 ? std::ldexp( float( fraction ), -24 )
                    : exponent == 31 ? ( fraction ? std::numeric_limits<float>::quiet_NaN() : std::numeric_limits<float>::infinity() )
                    : std::ldexp( 1.f + fraction / 1024.f, int( exponent ) - 15 );
                return bits & 32768 ? -magnitude : magnitude;
            };
            bool valid = true;
            for( unsigned y = 0; y < desc.Height; ++y )
            {
                for( unsigned x = 0; x < desc.Width; ++x )
                {
                    const auto row = static_cast<const unsigned char *>( mapped.pData ) + y * mapped.RowPitch;
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
                    face.pixels[y * desc.Width + x] = value;
                }
            }
            context->Unmap( staging, 0 );
            staging->Release();
            return valid;
        }
    }  // namespace

    ClawCubemap::~ClawCubemap()
    {
        reset();
    }

    void ClawCubemap::reset()
    {
        if( m_view )
        {
            m_view->Release();
        }
        m_view = nullptr;
        for( auto &face : m_faces )
        {
            if( face )
            {
                face->Release();
            }
            face = nullptr;
        }
    }

    bool ClawCubemap::update( ID3D11Device *device, ID3D11DeviceContext *context,
                              const std::array<ID3D11ShaderResourceView *, 6> &views )
    {
        if( !device || !context )
        {
            return false;
        }
        ID3D11Device *contextDevice = nullptr;
        context->GetDevice( &contextDevice );
        const bool sameDevice = contextDevice == device;
        if( contextDevice ) contextDevice->Release();
        if( !sameDevice ) return false;
        if( m_view && views == m_faces )
        {
            ID3D11Device *cubeDevice = nullptr;
            m_view->GetDevice( &cubeDevice );
            const bool current = cubeDevice == device;
            if( cubeDevice ) cubeDevice->Release();
            return current;
        }
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
        std::array<D3D11_SUBRESOURCE_DATA, 48> data{};
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
                data[index].pSysMem = output.data();
                data[index].SysMemPitch = size * 16;
            }
        }
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = desc.Height = 128;
        desc.MipLevels = 8;
        desc.ArraySize = 6;
        desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
        ID3D11Texture2D *texture = nullptr;
        ID3D11ShaderResourceView *view = nullptr;
        if( FAILED( device->CreateTexture2D( &desc, data.data(), &texture ) ) )
        {
            return false;
        }
        const auto hr = device->CreateShaderResourceView( texture, nullptr, &view );
        texture->Release();
        if( FAILED( hr ) )
        {
            return false;
        }
        reset();
        m_view = view;
        m_faces = views;
        for( auto face : m_faces )
        {
            if( face )
            {
                face->AddRef();
            }
        }
        return true;
    }
}  // namespace workphone::render
