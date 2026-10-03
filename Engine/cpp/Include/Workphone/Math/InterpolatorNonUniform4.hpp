#ifndef InterpolatorNonUniform4_h__
#define InterpolatorNonUniform4_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Pair.hpp>
#include <Workphone/Math/Vector4.hpp>

namespace workphone
{

    /**
     * @brief A non-uniform interpolator for 4D vectors.
     *
     * This class allows for interpolation of 4D vectors based on non-uniformly spaced values.
     * It uses a list of pairs where each pair consists of a scalar value and a corresponding 4D vector.
     *
     * @tparam T The numeric type used for the scalar and vector components (e.g., float, double).
     */
    template <class T>
    class WPCore_API InterpolatorNonUniform4
    {
    public:
        InterpolatorNonUniform4();
        ~InterpolatorNonUniform4();

        void setValues( const Array<Pair<T, Vector4<T>>> &values );

        const Array<Pair<T, Vector4<T>>> &getValues() const;

        Vector4<T> interpolate( const T &t );

    protected:
        Array<Pair<T, Vector4<T>>> m_values;
    };

    using InterpolatorNonUniform4F = InterpolatorNonUniform4<f32>;
    using InterpolatorNonUniform4D = InterpolatorNonUniform4<f64>;

}  // namespace workphone

#endif  // InterpolatorNonUniform4_h__
