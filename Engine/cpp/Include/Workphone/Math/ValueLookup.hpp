#ifndef FBMath_ValueLookup_h__
#define FBMath_ValueLookup_h__

#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    class ValueLookupTypes
    {
    public:
        enum
        {
            VALUE_TYPE_FIXED,
            VALUE_TYPE_RANDOM,
        };
    };

    template <class T>
    class WPCore_API ValueLookup
    {
    public:
        ValueLookup();
        virtual ~ValueLookup();

        virtual T getValue() = 0;

        u32 getType() const;
        void setType( u32 type );
        void setMin( const T &minValue );
        void setMax( const T &maxValue );
        u32 getSeed() const;
        void setSeed( u32 seed );

    protected:
        u32 m_type = ValueLookupTypes::VALUE_TYPE_FIXED;
        u32 m_seed = 0;
        Array<T> m_values;
    };

    template <class T>
    class WPCore_API ValueLookupNumber
    {
    public:
        ValueLookupNumber();
        ~ValueLookupNumber();

        T getValue();
        u32 getType() const;
        void setType( u32 type );
        void setMin( const T &minValue );
        void setMax( const T &maxValue );
        u32 getSeed() const;
        void setSeed( u32 seed );

    protected:
        u32 m_type = ValueLookupTypes::VALUE_TYPE_FIXED;
        u32 m_seed = 0;
        Array<T> m_values;
    };

    using ValueLookupNumberI = ValueLookupNumber<s32>;
    using ValueLookupNumberF = ValueLookupNumber<f32>;
    using ValueLookupNumberD = ValueLookupNumber<f64>;

    template <class T>
    class WPCore_API ValueLookupVector3
    {
    public:
        ValueLookupVector3();
        ~ValueLookupVector3();

        Vector3<T> getValue();
        u32 getType() const;
        void setType( u32 type );
        void setMin( const Vector3<T> &minValue );
        void setMax( const Vector3<T> &maxValue );
        u32 getSeed() const;
        void setSeed( u32 seed );

    protected:
        u32 m_type = ValueLookupTypes::VALUE_TYPE_FIXED;
        u32 m_seed = 0;
        Array<Vector3<T>> m_values;
    };

    using ValueLookupVector3I = ValueLookupVector3<s32>;
    using ValueLookupVector3F = ValueLookupVector3<f32>;
    using ValueLookupVector3D = ValueLookupVector3<f64>;
}  // namespace workphone

#endif  // FBMath_ValueLookup_h__
