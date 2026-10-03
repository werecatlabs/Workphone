#ifndef __WP_Vector4_H_
#define __WP_Vector4_H_

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Math/Math.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <cstring>

namespace workphone
{

    /**
     * @class Vector4
     * @brief Represents a 4-dimensional mathematical vector.
     *
     * This class provides a generic 4D vector implementation with common arithmetic operations,
     * element access, and utility functions. It is templated to support various numeric types.
     *
     * @tparam T The type of the vector elements (e.g., float, double, int).
     */
    template <class T>
    class WPCore_API Vector4
    {
    public:
        /**
         * @brief Default constructor. Initializes all components to zero.
         */
        Vector4();

        /**
         * @brief Constructs a vector with the given components.
         * @param x The x-component.
         * @param y The y-component.
         * @param z The z-component.
         * @param w The w-component.
         */
        Vector4( T x, T y, T z, T w );

        /**
         * @brief Copy constructor.
         * @param other The vector to copy from.
         */
        Vector4( const Vector4<T> &other );

        /**
         * @brief Constructs a Vector4 from a Vector3 and an additional value.
         * @param vec The 3D vector for x, y, z.
         * @param value The w-component.
         */
        Vector4( const Vector3<T> &vec, T value );

        /**
         * @brief Constructs a Vector4 from a pointer to an array of 4 values.
         * @param ptr Pointer to an array of 4 elements.
         */
        explicit Vector4( const T *ptr );

        T dotProduct( const Vector4<T> &other ) const;

        // operators
        /**
         * @brief Unary negation operator.
         * @return A vector with all components negated.
         */
        Vector4<T> operator-() const;

        /**
         * @brief Assignment operator.
         * @param other The vector to assign from.
         * @return Reference to this vector.
         */
        Vector4<T> &operator=( const Vector4<T> &other );

        /**
         * @brief Vector addition.
         * @param other The vector to add.
         * @return The sum of this and the other vector.
         */
        Vector4<T> operator+( const Vector4<T> &other ) const;

        /**
         * @brief In-place vector addition.
         * @param other The vector to add.
         * @return Reference to this vector after addition.
         */
        Vector4<T> &operator+=( const Vector4<T> &other );

        /**
         * @brief Vector subtraction.
         * @param other The vector to subtract.
         * @return The difference between this and the other vector.
         */
        Vector4<T> operator-( const Vector4<T> &other ) const;

        /**
         * @brief In-place vector subtraction.
         * @param other The vector to subtract.
         * @return Reference to this vector after subtraction.
         */
        Vector4<T> &operator-=( const Vector4<T> &other );

        /**
         * @brief Component-wise vector multiplication.
         * @param other The vector to multiply with.
         * @return The component-wise product.
         */
        Vector4<T> operator*( const Vector4<T> &other ) const;

        /**
         * @brief In-place component-wise vector multiplication.
         * @param other The vector to multiply with.
         * @return Reference to this vector after multiplication.
         */
        Vector4<T> &operator*=( const Vector4<T> &other );

        /**
         * @brief Scalar multiplication.
         * @param v The scalar value.
         * @return The result of multiplying all components by the scalar.
         */
        Vector4<T> operator*( T v ) const;

        /**
         * @brief In-place scalar multiplication.
         * @param v The scalar value.
         * @return Reference to this vector after multiplication.
         */
        Vector4<T> &operator*=( T v );

        /**
         * @brief Component-wise vector division.
         * @param other The vector to divide by.
         * @return The component-wise quotient.
         */
        Vector4<T> operator/( const Vector4<T> &other ) const;

        /**
         * @brief In-place component-wise vector division.
         * @param other The vector to divide by.
         * @return Reference to this vector after division.
         */
        Vector4<T> &operator/=( const Vector4<T> &other );

        /**
         * @brief Scalar division.
         * @param v The scalar value.
         * @return The result of dividing all components by the scalar.
         */
        Vector4<T> operator/( T v ) const;

        /**
         * @brief In-place scalar division.
         * @param v The scalar value.
         * @return Reference to this vector after division.
         */
        Vector4<T> &operator/=( T v );

        /**
         * @brief Less-than-or-equal comparison operator.
         * @param other The vector to compare with.
         * @return True if this vector is less than or equal to the other.
         */
        bool operator<=( const Vector4<T> &other ) const;

        /**
         * @brief Greater-than-or-equal comparison operator.
         * @param other The vector to compare with.
         * @return True if this vector is greater than or equal to the other.
         */
        bool operator>=( const Vector4<T> &other ) const;

        /**
         * @brief Less-than comparison operator.
         * @param other The vector to compare with.
         * @return True if this vector is less than the other.
         */
        bool operator<( const Vector4<T> &other ) const;

        /**
         * @brief Greater-than comparison operator.
         * @param other The vector to compare with.
         * @return True if this vector is greater than the other.
         */
        bool operator>( const Vector4<T> &other ) const;

        /**
         * @brief Equality comparison operator.
         * @param other The vector to compare with.
         * @return True if all components are equal.
         */
        bool operator==( const Vector4<T> &other ) const;

        /**
         * @brief Inequality comparison operator.
         * @param other The vector to compare with.
         * @return True if any component is not equal.
         */
        bool operator!=( const Vector4<T> &other ) const;

        // member access
        /**
         * @brief Conversion to const pointer to the underlying data.
         * @return Pointer to the first element.
         */
        explicit operator const T *() const;

        /**
         * @brief Conversion to pointer to the underlying data.
         * @return Pointer to the first element.
         */
        explicit operator T *();

        /**
         * @brief Array subscript operator (const).
         * @param i Index (0=x, 1=y, 2=z, 3=w).
         * @return Value at the given index.
         */
        T operator[]( s32 i ) const;

        /**
         * @brief Array subscript operator.
         * @param i Index (0=x, 1=y, 2=z, 3=w).
         * @return Reference to the value at the given index.
         */
        T &operator[]( s32 i );

        /**
         * @brief Gets the x-component (const).
         * @return The x-component.
         */
        T X() const;

        /**
         * @brief Gets the x-component (mutable).
         * @return Reference to the x-component.
         */
        T &X();

        /**
         * @brief Gets the y-component (const).
         * @return The y-component.
         */
        T Y() const;

        /**
         * @brief Gets the y-component (mutable).
         * @return Reference to the y-component.
         */
        T &Y();

        /**
         * @brief Gets the z-component (const).
         * @return The z-component.
         */
        T Z() const;

        /**
         * @brief Gets the z-component (mutable).
         * @return Reference to the z-component.
         */
        T &Z();

        /**
         * @brief Gets the w-component (const).
         * @return The w-component.
         */
        T W() const;

        /**
         * @brief Gets the w-component (mutable).
         * @return Reference to the w-component.
         */
        T &W();

        /**
         * @brief Checks if this vector is equal to another within a given tolerance.
         * @param other The vector to compare with.
         * @param tolerance The tolerance for comparison (default: Math<T>::epsilon()).
         * @return True if all components are equal within the given tolerance.
         */
        bool equals( const Vector4<T> &other, f32 tolerance = Math<T>::epsilon() ) const;

        /**
         * @brief Returns a zero vector (all components are zero).
         * @return A zero vector.
         */
        static Vector4<T> zero();

        /**
         * @brief Returns a pointer to the underlying data.
         * @return Pointer to the first element.
         */
        T *ptr();

        /**
         * @brief Returns a const pointer to the underlying data.
         * @return Const pointer to the first element.
         */
        const T *ptr() const;

        /**
         * @brief Checks if all components are valid (finite).
         * @return True if all components are valid.
         */
        bool isValid() const;

        /**
         * @brief Checks if all components are finite numbers.
         * @return True if all components are finite.
         */
        bool isFinite() const;

        /**
         * @brief Returns a Vector3 with the x, y, and z components of this vector.
         * @return A Vector3 containing x, y, z.
         */
        Vector3<T> xyz() const;

        /**
         * @brief Constant representing the zero vector (all components are zero).
         */
        static const Vector4 ZERO;

        /**
         * @brief Constant representing the unit vector along the x-axis.
         */
        static const Vector4 UNIT_X;

        /**
         * @brief Constant representing the unit vector along the y-axis.
         */
        static const Vector4 UNIT_Y;

        /**
         * @brief Constant representing the unit vector along the z-axis.
         */
        static const Vector4 UNIT_Z;

        /**
         * @brief Constant representing the unit vector (all components are one).
         */
        static const Vector4 UNIT;

        /**
         * @brief The x-component of the vector.
         */
        T x;

        /**
         * @brief The y-component of the vector.
         */
        T y;

        /**
         * @brief The z-component of the vector.
         */
        T z;

        /**
         * @brief The w-component of the vector.
         */
        T w;

    private:
        /**
         * @brief Compares this vector to another using memory comparison.
         * @param other The vector to compare with.
         * @return 0 if equal, <0 if less, >0 if greater.
         */
        s32 compare( const Vector4 &other ) const;
    };

    /**
     * @brief Scalar multiplication operator for Vector4 class.
     *
     * This overload provides the ability to multiply a Vector4 object by a scalar value,
     * by invoking the Vector4::operator* method.
     *
     * @tparam S The scalar type.
     * @tparam T The element type of the Vector4.
     * @param scalar The scalar value to multiply.
     * @param vector The Vector4 to be multiplied.
     * @return The resulting Vector4.
     */
    template <class S, class T>
    Vector4<T> operator*( const S scalar, const Vector4<T> &vector )
    {
        return vector * scalar;
    }

    /**
     * @typedef Vector4I
     * @brief 4D vector of signed 32-bit integers.
     */
    using Vector4I = Vector4<s32>;

    /**
     * @typedef Vector4F
     * @brief 4D vector of 32-bit floating point values.
     */
    using Vector4F = Vector4<f32>;

    /**
     * @typedef Vector4D
     * @brief 4D vector of 64-bit floating point values.
     */
    using Vector4D = Vector4<f64>;

}  // namespace workphone

#endif
