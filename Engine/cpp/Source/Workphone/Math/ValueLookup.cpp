#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/ValueLookup.hpp>

namespace workphone
{
    template <class T>
    ValueLookup<T>::ValueLookup()
    {
        m_values.resize( 2 );
        m_values[0] = T( 0 );
        m_values[1] = T( 0 );
    }

    template <class T>
    ValueLookup<T>::~ValueLookup() = default;

    template <class T>
    u32 ValueLookup<T>::getType() const
    {
        return m_type;
    }

    template <class T>
    void ValueLookup<T>::setType( u32 type )
    {
        m_type = type;
    }

    template <class T>
    void ValueLookup<T>::setMin( const T &minValue )
    {
        m_values[0] = minValue;
    }

    template <class T>
    void ValueLookup<T>::setMax( const T &maxValue )
    {
        m_values[1] = maxValue;
    }

    template <class T>
    u32 ValueLookup<T>::getSeed() const
    {
        return m_seed;
    }

    template <class T>
    void ValueLookup<T>::setSeed( u32 seed )
    {
        m_seed = seed;
    }

    template <class T>
    ValueLookupNumber<T>::ValueLookupNumber()
    {
        m_values.resize( 2 );
        m_values[0] = T( 0 );
        m_values[1] = T( 0 );
    }

    template <class T>
    ValueLookupNumber<T>::~ValueLookupNumber() = default;

    template <class T>
    T ValueLookupNumber<T>::getValue()
    {
        switch( m_type )
        {
        case ValueLookupTypes::VALUE_TYPE_FIXED:
            return m_values[0];
        case ValueLookupTypes::VALUE_TYPE_RANDOM:
            return Math<T>::RangedRandom( m_values[0], m_values[1], m_seed++ );
        default:
            return T( 0 );
        }
    }

    template <class T>
    u32 ValueLookupNumber<T>::getType() const
    {
        return m_type;
    }

    template <class T>
    void ValueLookupNumber<T>::setType( u32 type )
    {
        m_type = type;
    }

    template <class T>
    void ValueLookupNumber<T>::setMin( const T &minValue )
    {
        m_values[0] = minValue;
    }

    template <class T>
    void ValueLookupNumber<T>::setMax( const T &maxValue )
    {
        m_values[1] = maxValue;
    }

    template <class T>
    u32 ValueLookupNumber<T>::getSeed() const
    {
        return m_seed;
    }

    template <class T>
    void ValueLookupNumber<T>::setSeed( u32 seed )
    {
        m_seed = seed;
    }

    template <class T>
    ValueLookupVector3<T>::ValueLookupVector3()
    {
        m_values.resize( 2 );
        m_values[0] = Vector3<T>::zero();
        m_values[1] = Vector3<T>::zero();
    }

    template <class T>
    ValueLookupVector3<T>::~ValueLookupVector3() = default;

    template <class T>
    Vector3<T> ValueLookupVector3<T>::getValue()
    {
        switch( m_type )
        {
        case ValueLookupTypes::VALUE_TYPE_FIXED:
            return m_values[0];
        case ValueLookupTypes::VALUE_TYPE_RANDOM:
        {
            const auto &minValue = m_values[0];
            const auto &maxValue = m_values[1];
            return Vector3<T>( Math<T>::RangedRandom( minValue.X(), maxValue.X(), m_seed++ ),
                               Math<T>::RangedRandom( minValue.Y(), maxValue.Y(), m_seed++ ),
                               Math<T>::RangedRandom( minValue.Z(), maxValue.Z(), m_seed++ ) );
        }
        default:
            return Vector3<T>::zero();
        }
    }

    template <class T>
    u32 ValueLookupVector3<T>::getType() const
    {
        return m_type;
    }

    template <class T>
    void ValueLookupVector3<T>::setType( u32 type )
    {
        m_type = type;
    }

    template <class T>
    void ValueLookupVector3<T>::setMin( const Vector3<T> &minValue )
    {
        m_values[0] = minValue;
    }

    template <class T>
    void ValueLookupVector3<T>::setMax( const Vector3<T> &maxValue )
    {
        m_values[1] = maxValue;
    }

    template <class T>
    u32 ValueLookupVector3<T>::getSeed() const
    {
        return m_seed;
    }

    template <class T>
    void ValueLookupVector3<T>::setSeed( u32 seed )
    {
        m_seed = seed;
    }

    // explicit instantiation
    template class ValueLookup<s32>;
    template class ValueLookup<f32>;
    template class ValueLookup<f64>;
    template class ValueLookupNumber<s32>;
    template class ValueLookupNumber<f32>;
    template class ValueLookupNumber<f64>;
    template class ValueLookupVector3<s32>;
    template class ValueLookupVector3<f32>;
    template class ValueLookupVector3<f64>;
}  // namespace workphone
