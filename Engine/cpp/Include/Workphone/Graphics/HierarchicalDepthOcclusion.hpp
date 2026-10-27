#ifndef WORKPHONE_HIERARCHICAL_DEPTH_OCCLUSION_HPP
#define WORKPHONE_HIERARCHICAL_DEPTH_OCCLUSION_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace workphone::render
{
    /** Conservative conventional-Z hierarchy. Every parent stores the farthest
     * depth, including clear pixels. Unknown samples, near-plane crossings and
     * camera changes remain visible. Matrices are row-major, column-vector. */
    class HierarchicalDepthOcclusion
    {
    public:
        using Matrix = std::array<float, 16>;
        void clear() { m_levels.clear(); }
        bool publish( const float *depth, unsigned width, unsigned height, const Matrix &viewProjection )
        {
            clear();
            if( !depth || !width || !height || width > 16384 || height > 16384 ) return false;
            for( float value : viewProjection ) if( !std::isfinite( value ) ) return false;
            m_matrix = viewProjection;
            Level base{ width, height, std::vector<float>( size_t( width ) * height ) };
            for( size_t i = 0; i < base.values.size(); ++i )
                base.values[i] = std::isfinite( depth[i] ) && depth[i] >= 0 && depth[i] <= 1 ? depth[i] : 1.0f;
            m_levels.push_back( std::move( base ) );
            while( m_levels.back().width > 1 || m_levels.back().height > 1 )
            {
                const auto &previous = m_levels.back();
                Level next{ ( previous.width + 1 ) / 2, ( previous.height + 1 ) / 2, {} };
                next.values.resize( size_t( next.width ) * next.height, 1.0f );
                for( unsigned y = 0; y < next.height; ++y ) for( unsigned x = 0; x < next.width; ++x )
                {
                    float farthest = 0;
                    for( unsigned dy = 0; dy < 2; ++dy ) for( unsigned dx = 0; dx < 2; ++dx )
                    {
                        const auto sx = x * 2 + dx, sy = y * 2 + dy;
                        farthest = std::max( farthest, sx < previous.width && sy < previous.height ?
                            previous.values[size_t( sy ) * previous.width + sx] : 1.0f );
                    }
                    next.values[size_t( y ) * next.width + x] = farthest;
                }
                m_levels.push_back( std::move( next ) );
            }
            return true;
        }

        bool occluded( const float minimum[3], const float maximum[3], const Matrix &world,
                       const Matrix &currentViewProjection ) const
        {
            if( m_levels.empty() || currentViewProjection != m_matrix ) return false;
            float left = 1, right = -1, bottom = 1, top = -1, nearest = 1;
            for( unsigned axis = 0; axis < 3; ++axis )
                if( !std::isfinite( minimum[axis] ) || !std::isfinite( maximum[axis] ) ||
                    minimum[axis] > maximum[axis] ) return false;
            for( unsigned corner = 0; corner < 8; ++corner )
            {
                const float local[] = { corner & 1 ? maximum[0] : minimum[0],
                    corner & 2 ? maximum[1] : minimum[1], corner & 4 ? maximum[2] : minimum[2], 1 };
                float position[4] = {}, clip[4] = {};
                for( unsigned row = 0; row < 4; ++row ) for( unsigned column = 0; column < 4; ++column )
                    position[row] += world[row * 4 + column] * local[column];
                for( unsigned row = 0; row < 4; ++row ) for( unsigned column = 0; column < 4; ++column )
                    clip[row] += m_matrix[row * 4 + column] * position[column];
                for( float value : clip ) if( !std::isfinite( value ) ) return false;
                if( clip[3] <= 0.0001f || clip[2] <= -clip[3] ) return false;
                const auto x = clip[0] / clip[3], y = clip[1] / clip[3];
                nearest = std::min( nearest, ( clip[2] / clip[3] + 1.0f ) * 0.5f );
                left = std::min( left, x ); right = std::max( right, x );
                bottom = std::min( bottom, y ); top = std::max( top, y );
            }
            if( left < -1 || right > 1 || bottom < -1 || top > 1 || nearest <= 0 || nearest > 1 ) return false;
            const auto &base = m_levels.front();
            float x0 = ( left + 1 ) * 0.5f * base.width - 2;
            float x1 = ( right + 1 ) * 0.5f * base.width + 2;
            float y0 = ( 1 - top ) * 0.5f * base.height - 2;
            float y1 = ( 1 - bottom ) * 0.5f * base.height + 2;
            size_t lod = 0;
            while( lod + 1 < m_levels.size() && ( x1 - x0 > 4 || y1 - y0 > 4 ) )
            { x0 *= 0.5f; x1 *= 0.5f; y0 *= 0.5f; y1 *= 0.5f; ++lod; }
            const auto &level = m_levels[lod];
            const auto firstX = std::max( 0, static_cast<int>( std::floor( x0 ) ) );
            const auto firstY = std::max( 0, static_cast<int>( std::floor( y0 ) ) );
            const auto lastX = std::min( int( level.width ) - 1, static_cast<int>( std::ceil( x1 ) ) );
            const auto lastY = std::min( int( level.height ) - 1, static_cast<int>( std::ceil( y1 ) ) );
            if( firstX > lastX || firstY > lastY ) return false;
            for( int y = firstY; y <= lastY; ++y ) for( int x = firstX; x <= lastX; ++x )
                if( level.values[size_t( y ) * level.width + x] + 0.002f >= nearest ) return false;
            return true;
        }

    private:
        struct Level { unsigned width, height; std::vector<float> values; };
        Matrix m_matrix{};
        std::vector<Level> m_levels;
    };
}

#endif
