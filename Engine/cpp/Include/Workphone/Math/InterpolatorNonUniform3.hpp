#ifndef InterpolatorNonUniform3_h__
#define InterpolatorNonUniform3_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Pair.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{

    /**
     * @brief A non-uniform interpolator for 3D vectors.
     *
     * This class allows for interpolation of 3D vectors based on non-uniformly spaced values.
     * It is templated to support different numeric types (e.g., float, double).
     *
     * @tparam T The type of the vector components (e.g., f32, f64).
     */
    template <class T>
    class WPCore_API InterpolatorNonUniform3
    {
    public:
        InterpolatorNonUniform3();
        ~InterpolatorNonUniform3();

        void setValues( const Array<Pair<T, Vector3<T>>> &values );

        const Array<Pair<T, Vector3<T>>> &getValues() const;

        Vector3<T> interpolate( const T &t );

    protected:
        Array<Pair<T, Vector3<T>>> m_values;
    };

    using InterpolatorNonUniform4F = InterpolatorNonUniform3<f32>;
    using InterpolatorNonUniform4D = InterpolatorNonUniform3<f64>;
}  // namespace workphone

#endif  // InterpolatorNonUniform3_h__
