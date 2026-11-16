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
        {
            throw std::invalid_argument( "Invalid mesh imposter bake dimensions or bounds." );
        }

        // Validate the borrowed input before admitting output or raster scratch.
        for( const auto &triangle : triangles )
        {
            for( const auto &position : triangle.positions )
            {
                if( !MathUtil<f32>::isFinite( position ) )
                {
                    throw std::invalid_argument( "Non-finite imposter vertex." );
                }
            }
        }

        MeshImposterAtlas result;
        result.width = tileWidth * 3;
        result.height = tileHeight;
        result.tileWidth = tileWidth;
        const auto imageBytes = size_t( result.width ) * result.height * 4;
        result.rgba.reserve( imageBytes );
        result.rgba.resize( imageBytes, 0 );
        result.views = { { { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, {}, {} },
                           { { 0, 0, 1 }, { 0, 1, 0 }, { -1, 0, 0 }, {}, {} },
                           { { 1, 0, 0 }, { 0, 0, -1 }, { 0, 1, 0 }, {}, {} } } };

        auto light = Vector3F( .4f, .75f, .5f );
        light.normalise();

        // One admitted depth workspace serves all three views. Its charge
        // overlaps the complete retained atlas until rasterization finishes.
        auto depth = makeSourceBudgetSnapshot<float>();
        depth.reserve( size_t( tileWidth ) * tileHeight );
        depth.resize( size_t( tileWidth ) * tileHeight );

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

            std::fill( depth.begin(), depth.end(), -std::numeric_limits<float>::infinity() );

            for( const auto &triangle : triangles )
            {
                std::array<Vector3F, 3> projected;
                for( size_t i = 0; i < 3; ++i )
                {
                    const auto &p = triangle.positions[i];
                    projected[i] = { 1 + ( p.dotProduct( view.right ) - view.minimum.x ) /
                                             ( view.maximum.x - view.minimum.x ) * ( tileWidth - 3 ),
                                     1 + ( view.maximum.y - p.dotProduct( view.up ) ) /
                                             ( view.maximum.y - view.minimum.y ) * ( tileHeight - 3 ),
                                     p.dotProduct( view.towardCamera ) };
                }
                const auto &a = projected[0], &b = projected[1], &c = projected[2];
                const float det = ( b.y - c.y ) * ( a.x - c.x ) + ( c.x - b.x ) * ( a.y - c.y );
                if( std::abs( det ) < 1e-8f )
                {
                    continue;
                }
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
                        {
                            continue;
                        }
                        const float z = a.z * u + b.z * v + c.z * w;
                        const auto index = size_t( y ) * tileWidth + x;
                        if( z <= depth[index] )
                        {
                            continue;
                        }
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

    MeshImposterSpriteSheet bakeMeshImposterSpriteSheet(
        const Array<MeshImposterTriangle> &triangles, const Vector3F &lo, const Vector3F &hi,
        const MeshImposterSpriteSheetOptions &options )
    {
        const auto finite = []( const Vector3F &v ) { return MathUtil<f32>::isFinite( v ); };
        const auto extent = hi - lo;
        if( triangles.empty() || triangles.size() > 32768 || !finite( lo ) || !finite( hi ) ||
            !finite( extent ) || std::min( { extent.x, extent.y, extent.z } ) <= 0 ||
            options.tileWidth < 8 || options.tileHeight < 8 || options.tileWidth > 1024 ||
            options.tileHeight > 1024 || options.azimuthViews < 4 || options.azimuthViews > 32 ||
            options.elevationRings < 1 || options.elevationRings > 4 || options.gutter < 1 ||
            options.gutter > 16 || options.gutter * 2 + 4 > options.tileWidth ||
            options.gutter * 2 + 4 > options.tileHeight || options.supersampling < 1 ||
            options.supersampling > 4 || !options.maximumRasterSamples ||
            options.maximumRasterSamples > 1024ull * 1024 * 1024 )
        {
            throw std::invalid_argument( "Invalid directional mesh imposter bake options or bounds." );
        }
        const auto viewCount = options.azimuthViews * options.elevationRings + 1;
        if( !options.columns || options.columns > viewCount )
            throw std::invalid_argument( "Invalid directional mesh imposter atlas columns." );
        const auto rows = ( viewCount + options.columns - 1 ) / options.columns;
        const auto width = options.columns * options.tileWidth, height = rows * options.tileHeight;
        const auto imageBytes = u64( width ) * height * 4;
        if( width > 16384 || height > 16384 || imageBytes > 128ull * 1024 * 1024 )
            throw std::invalid_argument( "Directional mesh imposter atlas exceeds its image bound." );

        // Validate borrowed inputs before admitting any retained or temporary storage.
        for( const auto &triangle : triangles )
        {
            const auto &colour = triangle.colour;
            if( !std::isfinite( colour.r ) || !std::isfinite( colour.g ) ||
                !std::isfinite( colour.b ) || !std::isfinite( colour.a ) )
                throw std::invalid_argument( "Non-finite directional imposter colour." );
            for( const auto &p : triangle.positions )
                if( !finite( p ) || p.x < lo.x || p.y < lo.y || p.z < lo.z ||
                    p.x > hi.x || p.y > hi.y || p.z > hi.z )
                    throw std::invalid_argument( "Directional imposter vertex is outside finite bounds." );
        }

        MeshImposterSpriteSheet result;
        result.width = width; result.height = height;
        result.tileWidth = options.tileWidth; result.tileHeight = options.tileHeight;
        result.columns = options.columns; result.rows = rows; result.gutter = options.gutter;
        result.rgba.reserve( size_t( imageBytes ) );
        result.rgba.resize( size_t( imageBytes ), 0 );
        result.views.reserve( viewCount );
        constexpr double pi = 3.14159265358979323846;
        for( u32 index = 0; index < viewCount; ++index )
        {
            MeshImposterView view;
            if( index + 1 == viewCount )
            {
                view.towardCamera = { 0, 1, 0 };
                view.right = { 1, 0, 0 };
                view.up = { 0, 0, -1 };
            }
            else
            {
                const auto elevation = ( index / options.azimuthViews ) * ( pi * .5 / options.elevationRings );
                const auto azimuth = ( index % options.azimuthViews ) * ( 2 * pi / options.azimuthViews );
                view.towardCamera = { f32( std::sin( azimuth ) * std::cos( elevation ) ),
                    f32( std::sin( elevation ) ), f32( std::cos( azimuth ) * std::cos( elevation ) ) };
                view.right = { f32( std::cos( azimuth ) ), 0, f32( -std::sin( azimuth ) ) };
                view.up = view.towardCamera.crossProduct( view.right );
            }
            view.tileX = ( index % options.columns ) * options.tileWidth;
            view.tileY = ( index / options.columns ) * options.tileHeight;
            view.uvMinimum = { f32( view.tileX + options.gutter ) / width,
                               f32( view.tileY + options.gutter ) / height };
            view.uvMaximum = { f32( view.tileX + options.tileWidth - options.gutter ) / width,
                               f32( view.tileY + options.tileHeight - options.gutter ) / height };
            view.minimum = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
            view.maximum = -view.minimum;
            view.maximumDepth = -std::numeric_limits<float>::max();
            for( u32 corner = 0; corner < 8; ++corner )
            {
                const Vector3F p( corner & 1 ? hi.x : lo.x, corner & 2 ? hi.y : lo.y,
                                  corner & 4 ? hi.z : lo.z );
                const auto x = p.dotProduct( view.right ), y = p.dotProduct( view.up );
                view.minimum.x = std::min( view.minimum.x, x );
                view.minimum.y = std::min( view.minimum.y, y );
                view.maximum.x = std::max( view.maximum.x, x );
                view.maximum.y = std::max( view.maximum.y, y );
                view.maximumDepth = std::max( view.maximumDepth, p.dotProduct( view.towardCamera ) );
            }
            const auto span = view.maximum - view.minimum;
            if( !std::isfinite( span.x ) || !std::isfinite( span.y ) || !std::isfinite( view.maximumDepth ) ||
                span.x <= 0 || span.y <= 0 )
                throw std::invalid_argument( "Directional imposter projection bounds overflow." );
            result.views.push_back( view );
        }

        const auto sampling = options.supersampling;
        const auto rasterWidth = options.tileWidth * sampling, rasterHeight = options.tileHeight * sampling;
        const auto rasterPixels = size_t( rasterWidth ) * rasterHeight;
        auto depth = makeSourceBudgetSnapshot<float>();
        depth.reserve( rasterPixels ); depth.resize( rasterPixels );
        auto colours = makeSourceBudgetSnapshot<u8>();
        colours.reserve( rasterPixels * 4 ); colours.resize( rasterPixels * 4 );
        auto light = Vector3F( .4f, .75f, .5f );
        light.normalise();
        const auto gutter = options.gutter * sampling;
        const auto rasterSpanX = f32( rasterWidth - 2 * gutter );
        const auto rasterSpanY = f32( rasterHeight - 2 * gutter );
        for( const auto &view : result.views )
        {
            std::fill( depth.begin(), depth.end(), -std::numeric_limits<float>::infinity() );
            std::fill( colours.begin(), colours.end(), 0 );
            const auto span = view.maximum - view.minimum;
            for( const auto &triangle : triangles )
            {
                std::array<Vector3F, 3> projected;
                for( size_t i = 0; i < 3; ++i )
                {
                    const auto &p = triangle.positions[i];
                    projected[i] = {
                        gutter + ( p.dotProduct( view.right ) - view.minimum.x ) / span.x * rasterSpanX,
                        gutter + ( view.maximum.y - p.dotProduct( view.up ) ) / span.y * rasterSpanY,
                        p.dotProduct( view.towardCamera ) };
                }
                const auto &a = projected[0], &b = projected[1], &c = projected[2];
                const auto det = ( b.y - c.y ) * ( a.x - c.x ) + ( c.x - b.x ) * ( a.y - c.y );
                if( std::abs( det ) < 1e-8f ) continue;
                const auto xmin = std::max( int( gutter ), int( std::floor( std::min( { a.x, b.x, c.x } ) ) ) );
                const auto xmax = std::min( int( rasterWidth - gutter - 1 ),
                    int( std::ceil( std::max( { a.x, b.x, c.x } ) ) ) );
                const auto ymin = std::max( int( gutter ), int( std::floor( std::min( { a.y, b.y, c.y } ) ) ) );
                const auto ymax = std::min( int( rasterHeight - gutter - 1 ),
                    int( std::ceil( std::max( { a.y, b.y, c.y } ) ) ) );
                if( xmax < xmin || ymax < ymin ) continue;
                const auto samples = u64( xmax - xmin + 1 ) * u64( ymax - ymin + 1 );
                if( samples > options.maximumRasterSamples - result.rasterSamples )
                    throw std::runtime_error( "Directional mesh imposter raster-work budget exhausted." );
                result.rasterSamples += samples;

                // Double intermediates keep shading finite even for very large valid meshes.
                const auto ab = triangle.positions[1] - triangle.positions[0];
                const auto ac = triangle.positions[2] - triangle.positions[0];
                const double nx = double( ab.y ) * ac.z - double( ab.z ) * ac.y;
                const double ny = double( ab.z ) * ac.x - double( ab.x ) * ac.z;
                const double nz = double( ab.x ) * ac.y - double( ab.y ) * ac.x;
                const auto length = std::sqrt( nx * nx + ny * ny + nz * nz );
                const auto lit = length > 0 ? std::max( 0., ( nx * light.x + ny * light.y + nz * light.z ) / length ) : 0.;
                const auto illumination = .45 + .65 * lit;
                const auto toSRGB = [&]( float value ) {
                    return u8( std::round( 255 * std::pow( std::clamp( value * illumination, 0., 1. ), 1. / 2.2 ) ) );
                };
                const std::array<u8, 3> colour{ toSRGB( triangle.colour.r ), toSRGB( triangle.colour.g ), toSRGB( triangle.colour.b ) };
                for( int y = ymin; y <= ymax; ++y )
                    for( int x = xmin; x <= xmax; ++x )
                    {
                        const float u = ( ( b.y - c.y ) * ( x + .5f - c.x ) +
                            ( c.x - b.x ) * ( y + .5f - c.y ) ) / det;
                        const float v = ( ( c.y - a.y ) * ( x + .5f - c.x ) +
                            ( a.x - c.x ) * ( y + .5f - c.y ) ) / det;
                        const float w = 1 - u - v;
                        if( u < 0 || v < 0 || w < 0 ) continue;
                        const auto z = a.z * u + b.z * v + c.z * w;
                        const auto index = size_t( y ) * rasterWidth + x;
                        if( z <= depth[index] ) continue;
                        depth[index] = z;
                        auto pixel = colours.data() + index * 4;
                        pixel[0] = colour[0]; pixel[1] = colour[1]; pixel[2] = colour[2]; pixel[3] = 255;
                    }
            }
            // Resolve each tile independently; no sample can cross its transparent gutter.
            for( u32 y = options.gutter; y < options.tileHeight - options.gutter; ++y )
                for( u32 x = options.gutter; x < options.tileWidth - options.gutter; ++x )
                {
                    u32 count = 0;
                    std::array<u32, 3> sum{};
                    for( u32 sy = 0; sy < sampling; ++sy )
                        for( u32 sx = 0; sx < sampling; ++sx )
                        {
                            const auto pixel = colours.data() +
                                ( size_t( y * sampling + sy ) * rasterWidth + x * sampling + sx ) * 4;
                            if( !pixel[3] ) continue;
                            ++count;
                            for( size_t c = 0; c < sum.size(); ++c ) sum[c] += pixel[c];
                        }
                    if( !count ) continue;
                    auto output = result.rgba.data() +
                        ( size_t( view.tileY + y ) * result.width + view.tileX + x ) * 4;
                    for( size_t c = 0; c < sum.size(); ++c ) output[c] = u8( ( sum[c] + count / 2 ) / count );
                    output[3] = u8( ( count * 255 + sampling * sampling / 2 ) / ( sampling * sampling ) );
                }
        }
        return result;
    }

    s32 selectMeshImposterView( const Array<MeshImposterView> &views, const Vector3F &towardCamera,
                                s32 previous, f32 hysteresis )
    {
        const bool previousValid = previous >= 0 && size_t( previous ) < views.size();
        const auto retained = previousValid ? previous : -1;
        if( views.empty() || !MathUtil<f32>::isFinite( towardCamera ) ) return retained;
        // Scaling first avoids overflow and allows very small finite directions.
        const auto magnitude = std::max( { std::abs( towardCamera.x ), std::abs( towardCamera.y ),
                                          std::abs( towardCamera.z ) } );
        if( magnitude <= 0 ) return retained;
        auto direction = towardCamera / magnitude;
        direction.normalise();
        f32 bestScore = -std::numeric_limits<f32>::infinity();
        s32 selected = -1;
        for( size_t i = 0; i < views.size(); ++i )
        {
            const auto score = direction.dotProduct( views[i].towardCamera );
            if( std::isfinite( score ) && score > bestScore )
            {
                bestScore = score;
                selected = s32( i );
            }
        }
        if( selected < 0 ) return retained;
        const auto margin = std::isfinite( hysteresis ) ? std::clamp( hysteresis, 0.f, .25f ) : 0.f;
        if( previousValid && std::isfinite( direction.dotProduct( views[previous].towardCamera ) ) &&
            bestScore - direction.dotProduct( views[previous].towardCamera ) <= margin )
            return previous;
        return selected;
    }

}  // namespace workphone
