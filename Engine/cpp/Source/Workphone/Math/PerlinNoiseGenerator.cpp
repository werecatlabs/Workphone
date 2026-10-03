#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/PerlinNoiseGenerator.hpp>

namespace workphone
{
    template <class T>
    PerlinNoiseGenerator<T>::PerlinNoiseGenerator() = default;

    template <class T>
    PerlinNoiseGenerator<T>::~PerlinNoiseGenerator() = default;

    template <class T>
    auto PerlinNoiseGenerator<T>::generate() -> Array<Array<T>>
    {
        T maxLocalNoiseHeight = T( 0.0 );
        T minLocalNoiseHeight = T( 0.0 );
        return generateNoise( maxLocalNoiseHeight, minLocalNoiseHeight );
    }

    template <class T>
    auto PerlinNoiseGenerator<T>::generate( T &maxLocalNoiseHeight, T &minLocalNoiseHeight )
        -> Array<Array<T>>
    {
        return generateNoise( maxLocalNoiseHeight, minLocalNoiseHeight );
    }

    template <class T>
    auto PerlinNoiseGenerator<T>::generateNoise( T &maxLocalNoiseHeight, T &minLocalNoiseHeight )
        -> Array<Array<T>>
    {
        Array<Array<T>> noiseMap;
        noiseMap.resize( m_size );

        for( auto &a : noiseMap )
        {
            a.resize( m_size );
        }

        Array<Vector2<T>> octaveOffsets;
        octaveOffsets.resize( m_octaves );

        T maxPossibleHeight = 0;
        T amplitude = 1;
        T frequency = 1;

        // Calculate octave offsets and maximum possible height
        for( s32 i = 0; i < m_octaves; i++ )
        {
            octaveOffsets[i] = Vector2<T>( m_offset, m_offset );

            maxPossibleHeight += amplitude;
            amplitude *= m_persistance;
        }

        // Prevent division by zero
        if( m_scale <= T( 0.0 ) )
        {
            m_scale = T( 0.0001 );
        }

        maxLocalNoiseHeight = std::numeric_limits<T>::min();
        minLocalNoiseHeight = std::numeric_limits<T>::max();

        T halfSize = m_size / T( 2.0 );

        // Generate noise for each point in the map
        for( s32 y = 0; y < m_size; y++ )
        {
            for( s32 x = 0; x < m_size; x++ )
            {
                amplitude = T( 1.0 );
                frequency = T( 1.0 );
                T noiseHeight = T( 0.0 );

                // Combine multiple octaves of noise
                for( s32 i = 0; i < m_octaves; i++ )
                {
                    T sampleX = ( x - halfSize + octaveOffsets[i].X() ) / m_scale * frequency;
                    T sampleY = ( y - halfSize + octaveOffsets[i].Y() ) / m_scale * frequency;

                    T perlinValue = Math<T>::PerlinNoise( sampleX, sampleY ) * T( 2.0 ) - T( 1.0 );
                    noiseHeight += perlinValue * amplitude;

                    amplitude *= m_persistance;
                    frequency *= m_lacunarity;
                }

                // Track min/max values for normalization
                if( noiseHeight > maxLocalNoiseHeight )
                {
                    maxLocalNoiseHeight = noiseHeight;
                }
                else if( noiseHeight < minLocalNoiseHeight )
                {
                    minLocalNoiseHeight = noiseHeight;
                }

                noiseMap[x][y] = noiseHeight;
            }
        }

        return noiseMap;
    }

    template <class T>
    auto PerlinNoiseGenerator<T>::getSize() const -> s32
    {
        return m_size;
    }

    template <class T>
    void PerlinNoiseGenerator<T>::setSize( s32 size )
    {
        m_size = size;
    }

    template <class T>
    auto PerlinNoiseGenerator<T>::getOctaves() const -> s32
    {
        return m_octaves;
    }

    template <class T>
    void PerlinNoiseGenerator<T>::setOctaves( s32 octaves )
    {
        m_octaves = octaves;
    }

    template <class T>
    auto PerlinNoiseGenerator<T>::getScale() const -> T
    {
        return m_scale;
    }

    template <class T>
    void PerlinNoiseGenerator<T>::setScale( T scale )
    {
        m_scale = scale;
    }

    template <class T>
    auto PerlinNoiseGenerator<T>::getOffset() const -> T
    {
        return m_offset;
    }

    template <class T>
    void PerlinNoiseGenerator<T>::setOffset( T offset )
    {
        m_offset = offset;
    }

    template <class T>
    auto PerlinNoiseGenerator<T>::getPersistance() const -> T
    {
        return m_persistance;
    }

    template <class T>
    void PerlinNoiseGenerator<T>::setPersistance( T persistance )
    {
        m_persistance = persistance;
    }

    template <class T>
    auto PerlinNoiseGenerator<T>::getLacunarity() const -> T
    {
        return m_lacunarity;
    }

    template <class T>
    void PerlinNoiseGenerator<T>::setLacunarity( T lacunarity )
    {
        m_lacunarity = lacunarity;
    }

    // explicit instantiation
    template class PerlinNoiseGenerator<f32>;
    template class PerlinNoiseGenerator<f64>;
}  // namespace workphone
