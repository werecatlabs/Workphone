#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/ShadowMaskBaker.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <numeric>

namespace workphone
{
    namespace
    {
        constexpr float kRayEpsilon = 1e-6f;

        float getAxis( const Vector3F &v, int axis )
        {
            if( axis == 0 )
                return v.x;
            if( axis == 1 )
                return v.y;
            return v.z;
        }

        Vector3F minVec( const Vector3F &a, const Vector3F &b )
        {
            return Vector3F( std::min( a.x, b.x ), std::min( a.y, b.y ), std::min( a.z, b.z ) );
        }

        Vector3F maxVec( const Vector3F &a, const Vector3F &b )
        {
            return Vector3F( std::max( a.x, b.x ), std::max( a.y, b.y ), std::max( a.z, b.z ) );
        }
    }  // namespace

    ShadowMaskBaker::TriangleBvh::TriangleBvh( const std::vector<BakeTriangle> &triangles ) :
        m_triangles( triangles )
    {
        m_indices.resize( m_triangles.size() );
        std::iota( m_indices.begin(), m_indices.end(), 0 );

        if( !m_triangles.empty() )
            buildRecursive( 0, static_cast<int>( m_indices.size() ) );
    }

    ShadowMaskBaker::Aabb ShadowMaskBaker::TriangleBvh::makeTriangleBounds( const BakeTriangle &tri )
    {
        Aabb b;
        b.min = minVec( tri.v0.position, minVec( tri.v1.position, tri.v2.position ) );
        b.max = maxVec( tri.v0.position, maxVec( tri.v1.position, tri.v2.position ) );

        const Vector3F pad( 0.0001f, 0.0001f, 0.0001f );
        b.min -= pad;
        b.max += pad;
        return b;
    }

    ShadowMaskBaker::Aabb ShadowMaskBaker::TriangleBvh::merge( const Aabb &a, const Aabb &b )
    {
        return { minVec( a.min, b.min ), maxVec( a.max, b.max ) };
    }

    Vector3F ShadowMaskBaker::TriangleBvh::centroid( const BakeTriangle &tri )
    {
        return ( tri.v0.position + tri.v1.position + tri.v2.position ) * ( 1.0f / 3.0f );
    }

    int ShadowMaskBaker::TriangleBvh::buildRecursive( int first, int count )
    {
        BvhNode node;

        Aabb bounds = makeTriangleBounds( m_triangles[m_indices[first]] );
        for( int i = 1; i < count; ++i )
            bounds = merge( bounds, makeTriangleBounds( m_triangles[m_indices[first + i]] ) );

        node.bounds = bounds;

        const int nodeIndex = static_cast<int>( m_nodes.size() );
        m_nodes.push_back( node );

        constexpr int maxLeafSize = 8;

        if( count <= maxLeafSize )
        {
            m_nodes[nodeIndex].firstTri = first;
            m_nodes[nodeIndex].triCount = count;
            return nodeIndex;
        }

        const Vector3F extent = bounds.max - bounds.min;

        int axis = 0;
        if( extent.y > extent.x && extent.y > extent.z )
            axis = 1;
        else if( extent.z > extent.x && extent.z > extent.y )
            axis = 2;

        const int mid = first + count / 2;

        std::nth_element( m_indices.begin() + first, m_indices.begin() + mid,
                          m_indices.begin() + first + count, [&]( int a, int b ) {
                              return getAxis( centroid( m_triangles[a] ), axis ) <
                                     getAxis( centroid( m_triangles[b] ), axis );
                          } );

        m_nodes[nodeIndex].left = buildRecursive( first, count / 2 );
        m_nodes[nodeIndex].right = buildRecursive( mid, count - count / 2 );

        return nodeIndex;
    }

    bool ShadowMaskBaker::TriangleBvh::occluded( const Ray &ray ) const
    {
        if( m_nodes.empty() )
            return false;

        return occludedRecursive( 0, ray );
    }

    bool ShadowMaskBaker::TriangleBvh::occludedRecursive( int nodeIndex, const Ray &ray ) const
    {
        const BvhNode &node = m_nodes[nodeIndex];

        if( !intersectRayAabb( ray, node.bounds ) )
            return false;

        if( node.isLeaf() )
        {
            float t = 0.0f;

            for( int i = 0; i < node.triCount; ++i )
            {
                const BakeTriangle &tri = m_triangles[m_indices[node.firstTri + i]];

                if( intersectRayTriangle( ray, tri.v0.position, tri.v1.position, tri.v2.position, t ) )
                    return true;
            }

            return false;
        }

        return occludedRecursive( node.left, ray ) || occludedRecursive( node.right, ray );
    }

    bool ShadowMaskBaker::intersectRayTriangle( const Ray &ray, const Vector3F &a, const Vector3F &b,
                                                const Vector3F &c, float &outT )
    {
        const Vector3F ab = b - a;
        const Vector3F ac = c - a;
        const Vector3F p = ray.dir.crossProduct( ac );
        const float det = ab.dotProduct( p );

        if( std::fabs( det ) < kRayEpsilon )
            return false;

        const float invDet = 1.0f / det;
        const Vector3F tvec = ray.origin - a;
        const float u = tvec.dotProduct( p ) * invDet;

        if( u < 0.0f || u > 1.0f )
            return false;

        const Vector3F q = tvec.crossProduct( ab );
        const float v = ray.dir.dotProduct( q ) * invDet;

        if( v < 0.0f || u + v > 1.0f )
            return false;

        const float t = ac.dotProduct( q ) * invDet;

        if( t <= kRayEpsilon || t >= ray.maxT )
            return false;

        outT = t;
        return true;
    }

    bool ShadowMaskBaker::intersectRayAabb( const Ray &ray, const Aabb &aabb )
    {
        float tMin = 0.0f;
        float tMax = ray.maxT;

        for( int axis = 0; axis < 3; ++axis )
        {
            const float origin = getAxis( ray.origin, axis );
            const float dir = getAxis( ray.dir, axis );
            const float minB = getAxis( aabb.min, axis );
            const float maxB = getAxis( aabb.max, axis );

            if( std::fabs( dir ) < kRayEpsilon )
            {
                if( origin < minB || origin > maxB )
                    return false;
                continue;
            }

            const float invD = 1.0f / dir;
            float t0 = ( minB - origin ) * invD;
            float t1 = ( maxB - origin ) * invD;

            if( t0 > t1 )
                std::swap( t0, t1 );

            tMin = std::max( tMin, t0 );
            tMax = std::min( tMax, t1 );

            if( tMin > tMax )
                return false;
        }

        return true;
    }

    bool ShadowMaskBaker::pointInUvTriangle( const Vector2F &p, const Vector2F &a, const Vector2F &b,
                                             const Vector2F &c, float &w0, float &w1, float &w2 )
    {
        const float v0x = b.x - a.x;
        const float v0y = b.y - a.y;
        const float v1x = c.x - a.x;
        const float v1y = c.y - a.y;
        const float v2x = p.x - a.x;
        const float v2y = p.y - a.y;

        const float den = v0x * v1y - v1x * v0y;

        if( std::fabs( den ) < 1e-8f )
            return false;

        w1 = ( v2x * v1y - v1x * v2y ) / den;
        w2 = ( v0x * v2y - v2x * v0y ) / den;
        w0 = 1.0f - w1 - w2;

        constexpr float tolerance = -0.0001f;
        return w0 >= tolerance && w1 >= tolerance && w2 >= tolerance;
    }

    Vector3F ShadowMaskBaker::baryPosition( const BakeTriangle &tri, float w0, float w1, float w2 )
    {
        return tri.v0.position * w0 + tri.v1.position * w1 + tri.v2.position * w2;
    }

    Vector3F ShadowMaskBaker::baryNormal( const BakeTriangle &tri, float w0, float w1, float w2 )
    {
        Vector3F n = tri.v0.normal * w0 + tri.v1.normal * w1 + tri.v2.normal * w2;

        if( n.lengthSquared() <= 1e-8f )
        {
            const Vector3F ab = tri.v1.position - tri.v0.position;
            const Vector3F ac = tri.v2.position - tri.v0.position;
            n = ab.crossProduct( ac );
        }

        return n.normaliseCopy();
    }

    float ShadowMaskBaker::bakeLightVisibility( const Vector3F &position, const Vector3F &normal,
                                                const BakeLight &light, const TriangleBvh &bvh,
                                                const ShadowMaskBakeSettings &settings )
    {
        Ray ray;
        ray.origin = position + normal * settings.normalBias;

        if( light.type == BakeLightType::Directional )
        {
            ray.dir = ( light.direction * -1.0f ).normaliseCopy();
            ray.maxT = settings.directionalRayDistance;
        }
        else
        {
            const Vector3F toLight = light.position - ray.origin;
            const float dist = toLight.length();

            if( dist <= settings.normalBias || dist > light.range )
                return 0.0f;

            ray.dir = toLight.normaliseCopy();
            ray.maxT = dist - settings.normalBias;
        }

        if( settings.rejectBackfaces && normal.dotProduct( ray.dir ) <= 0.0f )
            return 0.0f;

        return bvh.occluded( ray ) ? 0.0f : 1.0f;
    }

    void ShadowMaskBaker::dilate( int width, int height, std::vector<std::array<uint8_t, 4>> &pixels,
                                  std::vector<uint8_t> &coverage, int iterations )
    {
        for( int iter = 0; iter < iterations; ++iter )
        {
            auto nextPixels = pixels;
            auto nextCoverage = coverage;

            for( int y = 0; y < height; ++y )
            {
                for( int x = 0; x < width; ++x )
                {
                    const size_t index = static_cast<size_t>( y * width + x );

                    if( coverage[index] )
                        continue;

                    int found = 0;
                    std::array<int, 4> accum = { 0, 0, 0, 0 };

                    for( int oy = -1; oy <= 1; ++oy )
                    {
                        for( int ox = -1; ox <= 1; ++ox )
                        {
                            if( ox == 0 && oy == 0 )
                                continue;

                            const int nx = x + ox;
                            const int ny = y + oy;

                            if( nx < 0 || ny < 0 || nx >= width || ny >= height )
                                continue;

                            const size_t nIndex = static_cast<size_t>( ny * width + nx );

                            if( !coverage[nIndex] )
                                continue;

                            for( int c = 0; c < 4; ++c )
                                accum[c] += pixels[nIndex][c];

                            ++found;
                        }
                    }

                    if( found > 0 )
                    {
                        for( int c = 0; c < 4; ++c )
                            nextPixels[index][c] = static_cast<uint8_t>( accum[c] / found );

                        nextCoverage[index] = 1;
                    }
                }
            }

            pixels.swap( nextPixels );
            coverage.swap( nextCoverage );
        }
    }

    bool ShadowMaskBaker::writeRGBA8Tga( const std::string &path, int width, int height,
                                         const std::vector<std::array<uint8_t, 4>> &pixels )
    {
        std::ofstream file( path, std::ios::binary );

        if( !file )
            return false;

        uint8_t header[18] = {};
        header[2] = 2;
        header[12] = static_cast<uint8_t>( width & 0xFF );
        header[13] = static_cast<uint8_t>( ( width >> 8 ) & 0xFF );
        header[14] = static_cast<uint8_t>( height & 0xFF );
        header[15] = static_cast<uint8_t>( ( height >> 8 ) & 0xFF );
        header[16] = 32;
        header[17] = 0x20 | 8;

        file.write( reinterpret_cast<const char *>( header ), sizeof( header ) );

        for( const auto &p : pixels )
        {
            const uint8_t bgra[4] = { p[2], p[1], p[0], p[3] };
            file.write( reinterpret_cast<const char *>( bgra ), 4 );
        }

        return static_cast<bool>( file );
    }

    bool ShadowMaskBaker::bake( const std::string &outputTga, const ShadowMaskBakeSettings &settings,
                                const std::vector<BakeTriangle> &meshTriangles,
                                const std::vector<BakeTriangle> &sceneTriangles,
                                const std::vector<BakeLight> &lights )
    {
        if( settings.width <= 0 || settings.height <= 0 )
            return false;

        if( meshTriangles.empty() || sceneTriangles.empty() )
            return false;

        const int samplesPerAxis = std::max( 1, settings.samplesPerAxis );
        const int sampleCount = samplesPerAxis * samplesPerAxis;

        std::vector<std::array<uint8_t, 4>> pixels;
        pixels.resize( static_cast<size_t>( settings.width * settings.height ), { 255, 255, 255, 255 } );

        std::vector<uint8_t> coverage;
        coverage.resize( static_cast<size_t>( settings.width * settings.height ), 0 );

        TriangleBvh bvh( sceneTriangles );

        for( const BakeTriangle &tri : meshTriangles )
        {
            const Vector2F uv0 = tri.v0.uv1;
            const Vector2F uv1 = tri.v1.uv1;
            const Vector2F uv2 = tri.v2.uv1;

            const float minU = std::min( uv0.x, std::min( uv1.x, uv2.x ) );
            const float maxU = std::max( uv0.x, std::max( uv1.x, uv2.x ) );
            const float minV = std::min( uv0.y, std::min( uv1.y, uv2.y ) );
            const float maxV = std::max( uv0.y, std::max( uv1.y, uv2.y ) );

            const int x0 = std::max( 0, static_cast<int>( std::floor( minU * settings.width ) ) - 1 );
            const int x1 = std::min( settings.width - 1,
                                     static_cast<int>( std::ceil( maxU * settings.width ) ) + 1 );
            const int y0 = std::max( 0, static_cast<int>( std::floor( minV * settings.height ) ) - 1 );
            const int y1 = std::min( settings.height - 1,
                                     static_cast<int>( std::ceil( maxV * settings.height ) ) + 1 );

            for( int y = y0; y <= y1; ++y )
            {
                for( int x = x0; x <= x1; ++x )
                {
                    std::array<float, 4> visibilityAccum = { 0.0f, 0.0f, 0.0f, 0.0f };
                    std::array<int, 4> visibilitySamples = { 0, 0, 0, 0 };

                    int coveredSamples = 0;

                    for( int sy = 0; sy < samplesPerAxis; ++sy )
                    {
                        for( int sx = 0; sx < samplesPerAxis; ++sx )
                        {
                            const float fx = ( static_cast<float>( sx ) + 0.5f ) /
                                             static_cast<float>( samplesPerAxis );
                            const float fy = ( static_cast<float>( sy ) + 0.5f ) /
                                             static_cast<float>( samplesPerAxis );

                            const Vector2F uv(
                                ( static_cast<float>( x ) + fx ) / static_cast<float>( settings.width ),
                                ( static_cast<float>( y ) + fy ) /
                                    static_cast<float>( settings.height ) );

                            float w0 = 0.0f;
                            float w1 = 0.0f;
                            float w2 = 0.0f;

                            if( !pointInUvTriangle( uv, uv0, uv1, uv2, w0, w1, w2 ) )
                                continue;

                            ++coveredSamples;

                            const Vector3F pos = baryPosition( tri, w0, w1, w2 );
                            const Vector3F nrm = baryNormal( tri, w0, w1, w2 );

                            for( const BakeLight &light : lights )
                            {
                                if( light.channel < 0 || light.channel > 3 )
                                    continue;

                                const float visibility =
                                    bakeLightVisibility( pos, nrm, light, bvh, settings );

                                visibilityAccum[light.channel] += visibility;
                                visibilitySamples[light.channel] += 1;
                            }
                        }
                    }

                    if( coveredSamples == 0 )
                        continue;

                    const size_t pixelIndex = static_cast<size_t>( y * settings.width + x );
                    auto &out = pixels[pixelIndex];

                    for( int channel = 0; channel < 4; ++channel )
                    {
                        if( visibilitySamples[channel] <= 0 )
                            continue;

                        const float value =
                            visibilityAccum[channel] / static_cast<float>( visibilitySamples[channel] );

                        const float clamped = std::max( 0.0f, std::min( 1.0f, value ) );
                        out[channel] = static_cast<uint8_t>( std::round( clamped * 255.0f ) );
                    }

                    coverage[pixelIndex] = 1;
                }
            }
        }

        dilate( settings.width, settings.height, pixels, coverage, settings.dilationIterations );

        return writeRGBA8Tga( outputTga, settings.width, settings.height, pixels );
    }
}  // namespace workphone
