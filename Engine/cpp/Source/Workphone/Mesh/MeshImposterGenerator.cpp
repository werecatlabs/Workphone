#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/MeshImposterGenerator.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace workphone
{

    MeshImposterAtlas generateMeshImposters( const Array<MeshImposterTriangle> &triangles,
                                             const Vector3F &lo, const Vector3F &hi, u32 tileWidth,
                                             u32 tileHeight )
    {
        if( triangles.empty() || tileWidth < 8 || tileHeight < 8 || tileWidth > 1024 ||
            tileHeight > 1024 || !MathUtil<f32>::isFinite( lo ) || !MathUtil<f32>::isFinite( hi ) ||
            hi.x <= lo.x || hi.y <= lo.y || hi.z <= lo.z )
            throw std::invalid_argument( "Invalid mesh imposter bake dimensions or bounds." );
        
        MeshImposterAtlas result;
        result.width = tileWidth * 3;
        result.height = tileHeight;
        result.tileWidth = tileWidth;
        result.rgba.resize( size_t( result.width ) * result.height * 4, 0 );
        result.views = { { { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, {}, {} },
                           { { 0, 0, 1 }, { 0, 1, 0 }, { -1, 0, 0 }, {}, {} },
                           { { 1, 0, 0 }, { 0, 0, -1 }, { 0, 1, 0 }, {}, {} } } };
        
        auto light = Vector3F( .4f, .75f, .5f );
        light.normalise();
        
        for( size_t viewIndex = 0; viewIndex < result.views.size(); ++viewIndex )
        {
            auto &view = result.views[viewIndex];
            view.minimum = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
            view.maximum = -view.minimum;
            for( int corner = 0; corner < 8; ++corner )
            {
                Vector3F p( corner & 1 ? hi.x : lo.x, corner & 2 ? hi.y : lo.y,
                            corner & 4 ? hi.z : lo.z );
                const auto x = p.dotProduct( view.right ), y = p.dotProduct( view.up );
                view.minimum.x = std::min( view.minimum.x, x );
                view.minimum.y = std::min( view.minimum.y, y );
                view.maximum.x = std::max( view.maximum.x, x );
                view.maximum.y = std::max( view.maximum.y, y );
            }
            
            Array<float> depth( size_t( tileWidth ) * tileHeight,
                                -std::numeric_limits<float>::infinity() );
            
            for( const auto &triangle : triangles )
            {
                std::array<Vector3F, 3> projected;
                for( size_t i = 0; i < 3; ++i )
                {
                    const auto &p = triangle.positions[i];
                    if( !MathUtil<f32>::isFinite( p ) )
                        throw std::invalid_argument( "Non-finite imposter vertex." );
                    projected[i] = { 1 + ( p.dotProduct( view.right ) - view.minimum.x ) /
                                             ( view.maximum.x - view.minimum.x ) * ( tileWidth - 3 ),
                                     1 + ( view.maximum.y - p.dotProduct( view.up ) ) /
                                             ( view.maximum.y - view.minimum.y ) * ( tileHeight - 3 ),
                                     p.dotProduct( view.towardCamera ) };
                }
                const auto &a = projected[0], &b = projected[1], &c = projected[2];
                const float det = ( b.y - c.y ) * ( a.x - c.x ) + ( c.x - b.x ) * ( a.y - c.y );
                if( std::abs( det ) < 1e-8f )
                    continue;
                auto normal = ( triangle.positions[1] - triangle.positions[0] )
                                  .crossProduct( triangle.positions[2] - triangle.positions[0] );
                normal.normalise();
                const auto illumination = .45f + .65f * std::max( 0.f, normal.dotProduct( light ) );
                const auto toSRGB = [&]( float v ) {
                    return u8( std::round(
                        255 * std::pow( std::clamp( v * illumination, 0.f, 1.f ), 1.f / 2.2f ) ) );
                };
                const int xmin = std::max( 0, int( std::floor( std::min( { a.x, b.x, c.x } ) ) ) );
                const int xmax =
                    std::min( int( tileWidth ) - 1, int( std::ceil( std::max( { a.x, b.x, c.x } ) ) ) );
                const int ymin = std::max( 0, int( std::floor( std::min( { a.y, b.y, c.y } ) ) ) );
                const int ymax =
                    std::min( int( tileHeight ) - 1, int( std::ceil( std::max( { a.y, b.y, c.y } ) ) ) );
                for( int y = ymin; y <= ymax; ++y )
                {
                    for( int x = xmin; x <= xmax; ++x )
                    {
                        const float u =
                            ( ( b.y - c.y ) * ( x + .5f - c.x ) + ( c.x - b.x ) * ( y + .5f - c.y ) ) /
                            det;
                        const float v =
                            ( ( c.y - a.y ) * ( x + .5f - c.x ) + ( a.x - c.x ) * ( y + .5f - c.y ) ) /
                            det;
                        const float w = 1 - u - v;
                        if( u < 0 || v < 0 || w < 0 )
                            continue;
                        const float z = a.z * u + b.z * v + c.z * w;
                        const auto index = size_t( y ) * tileWidth + x;
                        if( z <= depth[index] )
                            continue;
                        depth[index] = z;
                        auto pixel = result.rgba.data() +
                                     ( size_t( y ) * result.width + viewIndex * tileWidth + x ) * 4;
                        pixel[0] = toSRGB( triangle.colour.r );
                        pixel[1] = toSRGB( triangle.colour.g );
                        pixel[2] = toSRGB( triangle.colour.b );
                        pixel[3] = 255;
                    }
                }
            }
        }
        return result;
    }

}  // namespace workphone
