#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/FalloffGenerator.hpp>

namespace workphone
{
    template <class T>
    FalloffGenerator<T>::FalloffGenerator() = default;

    template <class T>
    FalloffGenerator<T>::~FalloffGenerator() = default;

    template <class T>
    auto FalloffGenerator<T>::generate() -> Array<Array<T>>
    {
        Array<Array<T>> map;
        map.resize( m_size );

        for( s32 i = 0; i < m_size; i++ )
        {
            map[i].resize( m_size );
        }

        for( s32 i = 0; i < m_size; i++ )
        {
            for( s32 j = 0; j < m_size; j++ )
            {
                // Convert array indices to normalized coordinates [-1, 1]
                auto x = i / static_cast<T>( m_size ) * 2 - 1;
                auto y = j / static_cast<T>( m_size ) * 2 - 1;

                // Calculate distance from center using Chebyshev distance (max of x, y)
                auto value = Math<T>::max( Math<T>::Abs( x ), Math<T>::Abs( y ) );
                map[i][j] = evaluate( value );
            }
        }

        return map;
    }

    template <class T>
    auto FalloffGenerator<T>::evaluate( T value ) -> T
    {
        return Math<T>::Pow( value, m_falloffDirection ) /
               ( Math<T>::Pow( value, m_falloffDirection ) +
                 Math<T>::Pow( m_falloffRange - m_falloffRange * value, m_falloffDirection ) );
    }

    template <class T>
    auto FalloffGenerator<T>::getFalloffDirection() const -> T
    {
        return m_falloffDirection;
    }

    template <class T>
    auto FalloffGenerator<T>::getFalloffRange() const -> T
    {
        return m_falloffRange;
    }

    template <class T>
    auto FalloffGenerator<T>::getSize() const -> s32
    {
        return m_size;
    }

    template <class T>
    void FalloffGenerator<T>::setFalloffDirection( T falloffDirection )
    {
        m_falloffDirection = falloffDirection;
    }

    template <class T>
    void FalloffGenerator<T>::setFalloffRange( T falloffRange )
    {
        m_falloffRange = falloffRange;
    }

    template <class T>
    void FalloffGenerator<T>::setSize( s32 size )
    {
        m_size = size;
    }

    // explicit instantiation
    template class FalloffGenerator<f32>;
    template class FalloffGenerator<f64>;
}  // namespace workphone
