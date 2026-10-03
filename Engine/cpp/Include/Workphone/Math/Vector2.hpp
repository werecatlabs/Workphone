#ifndef Vector2_h__
#define Vector2_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Math/Math.hpp>
#include <cstring>

namespace workphone
{

    /**
     * @class Vector2
     * @brief A 2D vector class for mathematical operations in two-dimensional space.
     *
     * Provides a comprehensive set of vector operations, including arithmetic, normalization,
     * interpolation, and geometric utilities. Supports both scalar and component-wise operations.
     *
     * @tparam T The numeric type of the vector components (e.g., float, double, int).
     */
    template <class T>
    class WPCore_API Vector2
    {
    public:
        /**
         * @brief Default constructor. Initializes both x and y to zero.
         */
        Vector2();

        /**
         * @brief Constructs a Vector2 from its x and y components.
         * @param x The x component of the Vector2.
         * @param y The y component of the Vector2.
         */
        Vector2( T x, T y );

        /**
         * @brief Copy constructor.
         * @param other The other Vector2 to construct from.
         */
        Vector2( const Vector2<T> &other );

        // operators
        /**
         * @brief Unary minus operator. Returns the negation of this vector.
         * @return A new Vector2 with both components negated.
         */
        Vector2<T> operator-() const;

        /**
         * @brief Assignment operator.
         * @param other The vector to assign from.
         * @return Reference to this vector after assignment.
         */
        Vector2<T> &operator=( const Vector2<T> &other );

        /**
         * @brief Vector addition.
         * @param other The vector to add.
         * @return The sum of this vector and the other.
         */
        Vector2<T> operator+( const Vector2<T> &other ) const;
        /**
         * @brief In-place vector addition.
         * @param other The vector to add.
         * @return Reference to this vector after addition.
         */
        Vector2<T> &operator+=( const Vector2<T> &other );

        /**
         * @brief Vector subtraction.
         * @param other The vector to subtract.
         * @return The difference between this vector and the other.
         */
        Vector2<T> operator-( const Vector2<T> &other ) const;
        /**
         * @brief In-place vector subtraction.
         * @param other The vector to subtract.
         * @return Reference to this vector after subtraction.
         */
        Vector2<T> &operator-=( const Vector2<T> &other );

        /**
         * @brief Component-wise vector multiplication.
         * @param other The vector to multiply with.
         * @return The component-wise product of this vector and the other.
         */
        Vector2<T> operator*( const Vector2<T> &other ) const;
        /**
         * @brief In-place component-wise vector multiplication.
         * @param other The vector to multiply with.
         * @return Reference to this vector after multiplication.
         */
        Vector2<T> &operator*=( const Vector2<T> &other );
        /**
         * @brief Scalar multiplication.
         * @param v The scalar value to multiply with.
         * @return The product of this vector and the scalar.
         */
        Vector2<T> operator*( T v ) const;
        /**
         * @brief In-place scalar multiplication.
         * @param v The scalar value to multiply with.
         * @return Reference to this vector after multiplication.
         */
        Vector2<T> &operator*=( T v );

        /**
         * @brief Component-wise vector division.
         * @param other The vector to divide by.
         * @return The component-wise quotient of this vector and the other.
         */
        Vector2<T> operator/( const Vector2<T> &other ) const;
        /**
         * @brief In-place component-wise vector division.
         * @param other The vector to divide by.
         * @return Reference to this vector after division.
         */
        Vector2<T> &operator/=( const Vector2<T> &other );
        /**
         * @brief Scalar division.
         * @param v The scalar value to divide by.
         * @return The quotient of this vector and the scalar.
         */
        Vector2<T> operator/( T v ) const;

        /**
         * @brief In-place scalar division.
         * @param v The scalar value to divide by.
         * @return Reference to this vector after division.
         */
        Vector2<T> &operator/=( T v );

        /**
         * @brief Less-than-or-equal comparison operator.
         * @param other The vector to compare with.
         * @return True if this vector is less than or equal to the other.
         */
        bool operator<=( const Vector2<T> &other ) const;

        /**
         * @brief Greater-than-or-equal comparison operator.
         * @param other The vector to compare with.
         * @return True if this vector is greater than or equal to the other.
         */
        bool operator>=( const Vector2<T> &other ) const;

        /**
         * @brief Less-than comparison operator.
         * @param other The vector to compare with.
         * @return True if this vector is less than the other.
         */
        bool operator<( const Vector2<T> &other ) const;

        /**
         * @brief Greater-than comparison operator.
         * @param other The vector to compare with.
         * @return True if this vector is greater than the other.
         */
        bool operator>( const Vector2<T> &other ) const;

        /**
         * @brief Equality comparison operator.
         * @param other The vector to compare with.
         * @return True if both components are equal.
         */
        bool operator==( const Vector2<T> &other ) const;

        /**
         * @brief Inequality comparison operator.
         * @param other The vector to compare with.
         * @return True if any component is not equal.
         */
        bool operator!=( const Vector2<T> &other ) const;

        // member access
        /**
         * @brief Conversion to const pointer to the underlying data.
         * @return Pointer to the first component (x).
         */
        explicit operator const T *() const;

        /**
         * @brief Conversion to pointer to the underlying data.
         * @return Pointer to the first component (x).
         */
        explicit operator T *();

        /**
         * @brief Array subscript operator (const).
         * @param i Index (0 for x, 1 for y).
         * @return The value of the component at index i.
         */
        T operator[]( s32 i ) const;

        /**
         * @brief Array subscript operator.
         * @param i Index (0 for x, 1 for y).
         * @return Reference to the component at index i.
         */
        T &operator[]( s32 i );

        /**
         * @brief Gets the x component (const).
         * @return The x component.
         */
        T X() const;

        /**
         * @brief Gets the x component (mutable).
         * @return Reference to the x component.
         */
        T &X();

        /**
         * @brief Gets the y component (const).
         * @return The y component.
         */
        T Y() const;

        /**
         * @brief Gets the y component (mutable).
         * @return Reference to the y component.
         */
        T &Y();

        /**
         * @brief Checks if this vector is approximately equal to another vector.
         * @param other The other vector to compare to.
         * @return True if the vectors are approximately equal within a small margin of error.
         */
        bool equals( const Vector2<T> &other ) const;

        /**
         * @brief Sets the x and y components of this vector.
         * @param nx The new x component.
         * @param ny The new y component.
         */
        void set( T nx, T ny );

        /**
         * @brief Sets this vector to be the same as another vector.
         * @param p The vector to copy from.
         */
        void set( const Vector2<T> &p );

        /**
         * @brief Returns the length (magnitude) of the vector.
         * @return The length of the vector.
         */
        T length() const;

        /**
         * @brief Returns the squared length of the vector.
         * @return The squared length of the vector.
         */
        T lengthSquared() const;

        /**
         * @brief Returns the dot product of this vector with another vector.
         * @param other The other vector.
         * @return The dot product of the two vectors.
         */
        T dotProduct( const Vector2<T> &other ) const;

        /**
         * @brief Returns the distance from another point (interpreting the vector as a point in 2D
         * space).
         * @param other The other point.
         * @return The Euclidean distance between the two points.
         */
        T getDistanceFrom( const Vector2<T> &other ) const;

        /**
         * @brief Returns the squared distance from another point (interpreting the vector as a point in
         * 2D space).
         * @param other The other point.
         * @return The squared Euclidean distance between the two points.
         */
        T getDistanceFromSQ( const Vector2<T> &other ) const;

        /**
         * @brief Rotates the point around a center by an amount of degrees.
         * @param degrees The angle in degrees to rotate by.
         * @param center The center point to rotate around.
         */
        void rotateBy( T degrees, const Vector2<T> &center );

        /**
         * @brief Normalizes the vector to unit length (in-place).
         */
        void normalise();

        /**
         * @brief Normalizes the vector and returns the original length.
         * @return The original length of the vector before normalization.
         */
        T normaliseLength();

        /**
         * @brief Returns a normalized copy of the vector.
         * @return A new normalized vector.
         */
        Vector2<T> normaliseCopy() const;

        /**
         * @brief Sets the length of the vector to a new value (preserves direction).
         * @param newlength The new length for the vector.
         */
        void setLength( T newlength );

        /**
         * @brief Returns a perpendicular vector (y, -x).
         * @return The perpendicular vector.
         */
        Vector2<T> perp() const;

        /**
         * @brief Returns a normalized perpendicular vector (y, -x) / |(x, y)|.
         * @return The unit perpendicular vector.
         */
        Vector2<T> unitPerp() const;

        /**
         * @brief Returns the perp-dot product (x * other.y - y * other.x).
         * @param other The other vector.
         * @return The perp-dot product.
         */
        T dotPerp( const Vector2<T> &other ) const;

        /**
         * @brief Calculates the angle of this vector in degrees (trigonometric sense).
         * @return The angle in degrees, between 0 and 360.
         */
        T getAngleTrig() const;

        /**
         * @brief Calculates the angle of this vector in degrees (counter-trigonometric sense).
         * @return The angle in degrees, between 0 and 360.
         */
        T getAngle() const;

        /**
         * @brief Calculates the angle between this vector and another in degrees.
         * @param b The other vector.
         * @return The angle in degrees, between 0 and 90.
         */
        T getAngleWith( const Vector2<T> &b ) const;

        /**
         * @brief Checks if this vector (as a point) is between two other points.
         * @param begin The beginning vector.
         * @param end The ending vector.
         * @return True if this vector is between begin and end.
         */
        bool isBetweenPoints( const Vector2<T> &begin, const Vector2<T> &end ) const;

        /**
         * @brief Returns an interpolated vector between this and another vector.
         * @param other The other vector.
         * @param d Interpolation factor (0.0f to 1.0f).
         * @return The interpolated vector.
         */
        Vector2<T> getInterpolated( const Vector2<T> &other, T d ) const;

        /**
         * @brief Returns a quadratic interpolated vector between this and two other vectors.
         * @param v2 The second vector.
         * @param v3 The third vector.
         * @param d Interpolation factor (0.0f to 1.0f).
         * @return The quadratic interpolated vector.
         */
        Vector2<T> getInterpolated_quadratic( const Vector2<T> &v2, const Vector2<T> &v3, T d ) const;

        /**
         * @brief Sets this vector to the interpolated vector between a and b.
         * @param a The first vector.
         * @param b The second vector.
         * @param t Interpolation factor.
         */
        void interpolate( const Vector2<T> &a, const Vector2<T> &b, T t );

        /**
         * @brief Returns a pointer to the underlying data.
         * @return Pointer to the first component (x).
         */
        T *ptr();

        /**
         * @brief Returns a const pointer to the underlying data.
         * @return Const pointer to the first component (x).
         */
        const T *ptr() const;

        /**
         * @brief Returns a zero vector (0, 0).
         * @return A zero vector.
         */
        static Vector2 zero();

        /**
         * @brief Returns a unit vector (1, 1).
         * @return A unit vector.
         */
        static Vector2 unit();

        /**
         * @brief Checks if the vector's elements have valid values (finite).
         * @return True if both components are finite.
         */
        bool isValid() const;

        /**
         * @brief Checks if the vector's elements are finite.
         * @return True if both components are finite.
         */
        bool isFinite() const;

        /**
         * @brief Constant representing the zero vector (0, 0).
         */
        static const Vector2 ZERO;
        /**
         * @brief Constant representing the unit vector along the x-axis (1, 0).
         */
        static const Vector2 UNIT_X;
        /**
         * @brief Constant representing the unit vector along the y-axis (0, 1).
         */
        static const Vector2 UNIT_Y;
        /**
         * @brief Constant representing the unit vector (1, 1).
         */
        static const Vector2 UNIT;

        /**
         * @brief The x-component of the vector.
         */
        T x;

        /**
         * @brief The y-component of the vector.
         */
        T y;

    private:
        /**
         * @brief Compares this vector with another for ordering.
         * @param other The other vector to compare with.
         * @return Negative if less, zero if equal, positive if greater.
         */
        s32 compare( const Vector2 &other ) const;
    };

    template <class S, class T>
    Vector2<T> operator*( const S scalar, const Vector2<T> &vector )
    {
        return vector * scalar;
    }

    /**
     * @typedef Vector2I
     * @brief Typedef for a 2D vector with integer components (s32).
     */
    using Vector2I = Vector2<s32>;

    /**
     * @typedef Vector2F
     * @brief Typedef for a 2D vector with float components (f32).
     */
    using Vector2F = Vector2<f32>;

    /**
     * @typedef Vector2D
     * @brief Typedef for a 2D vector with double components (f64).
     */
    using Vector2D = Vector2<f64>;

}  // namespace workphone

#endif  // Vector2_h__
