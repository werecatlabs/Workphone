#ifndef __WP_Vector3_H_
#define __WP_Vector3_H_

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Math/Math.hpp>
#include <algorithm>
#include <cstring>

namespace workphone
{
    /**
     * @class Vector3
     * @brief Represents a 3-dimensional mathematical vector.
     * @tparam T The type of the vector components (e.g., float, double, int).
     *
     * This class provides a comprehensive set of operations for 3D vector mathematics,
     * including arithmetic, normalization, dot/cross products, interpolation, and more.
     * It is suitable for use in graphics, physics, and general computational geometry.
     */
    template <class T>
    class WPCore_API Vector3
    {
    public:
        /**
         * @brief Default constructor. Initializes all components to zero.
         */
        Vector3();

        /**
         * @brief Constructs a vector with the given components.
         * @param x The x-coordinate.
         * @param y The y-coordinate.
         * @param z The z-coordinate.
         */
        Vector3( T x, T y, T z );

        /**
         * @brief Copy constructor.
         * @param other The vector to copy.
         */
        Vector3( const Vector3<T> &other );

        /**
         * @brief Constructs a vector from an array of 3 values.
         * @param ptr Pointer to an array of 3 coordinates.
         */
        explicit Vector3( const T *ptr );

        // operators
        /**
         * @brief Unary minus operator. Returns the negation of this vector.
         * @return The negated vector.
         */
        Vector3<T> operator-() const;

        /**
         * @brief Assignment operator.
         * @param other The vector to assign from.
         * @return Reference to this vector.
         */
        Vector3<T> &operator=( const Vector3<T> &other );

        /**
         * @brief Vector addition.
         * @param other The vector to add.
         * @return The sum of the two vectors.
         */
        Vector3<T> operator+( const Vector3<T> &other ) const;

        /**
         * @brief Adds another vector to this vector.
         * @param other The vector to add.
         * @return Reference to this vector.
         */
        Vector3<T> &operator+=( const Vector3<T> &other );

        /**
         * @brief Vector subtraction.
         * @param other The vector to subtract.
         * @return The difference of the two vectors.
         */
        Vector3<T> operator-( const Vector3<T> &other ) const;
        /**
         * @brief Subtracts another vector from this vector.
         * @param other The vector to subtract.
         * @return Reference to this vector.
         */
        Vector3<T> &operator-=( const Vector3<T> &other );

        /**
         * @brief Component-wise vector multiplication.
         * @param other The vector to multiply with.
         * @return The component-wise product.
         */
        Vector3<T> operator*( const Vector3<T> &other ) const;
        /**
         * @brief Component-wise vector multiplication and assignment.
         * @param other The vector to multiply with.
         * @return Reference to this vector.
         */
        Vector3<T> &operator*=( const Vector3<T> &other );
        /**
         * @brief Scalar multiplication.
         * @param v The scalar value.
         * @return The scaled vector.
         */
        Vector3<T> operator*( T v ) const;
        /**
         * @brief Scalar multiplication and assignment.
         * @param v The scalar value.
         * @return Reference to this vector.
         */
        Vector3<T> &operator*=( T v );

        /**
         * @brief Component-wise vector division.
         * @param other The vector to divide by.
         * @return The component-wise quotient.
         */
        Vector3<T> operator/( const Vector3<T> &other ) const;
        /**
         * @brief Component-wise vector division and assignment.
         * @param other The vector to divide by.
         * @return Reference to this vector.
         */
        Vector3<T> &operator/=( const Vector3<T> &other );
        /**
         * @brief Scalar division.
         * @param v The scalar value.
         * @return The scaled vector.
         */
        Vector3<T> operator/( T v ) const;
        /**
         * @brief Scalar division and assignment.
         * @param v The scalar value.
         * @return Reference to this vector.
         */
        Vector3<T> &operator/=( T v );

        /**
         * @brief Less-than-or-equal comparison (lexicographical).
         * @param other The vector to compare with.
         * @return True if this vector is less than or equal to other.
         */
        bool operator<=( const Vector3<T> &other ) const;
        /**
         * @brief Greater-than-or-equal comparison (lexicographical).
         * @param other The vector to compare with.
         * @return True if this vector is greater than or equal to other.
         */
        bool operator>=( const Vector3<T> &other ) const;
        /**
         * @brief Less-than comparison (lexicographical).
         * @param other The vector to compare with.
         * @return True if this vector is less than other.
         */
        bool operator<( const Vector3<T> &other ) const;
        /**
         * @brief Greater-than comparison (lexicographical).
         * @param other The vector to compare with.
         * @return True if this vector is greater than other.
         */
        bool operator>( const Vector3<T> &other ) const;

        /**
         * @brief Equality comparison.
         * @param other The vector to compare with.
         * @return True if the vectors are equal.
         */
        bool operator==( const Vector3<T> &other ) const;
        /**
         * @brief Inequality comparison.
         * @param other The vector to compare with.
         * @return True if the vectors are not equal.
         */
        bool operator!=( const Vector3<T> &other ) const;

        /**
         * @brief Checks if the squared length is less than a value.
         * @param value The value to compare with.
         * @return True if squared length is less than value.
         */
        bool operator<( T value ) const;
        /**
         * @brief Checks if the squared length is greater than a value.
         * @param value The value to compare with.
         * @return True if squared length is greater than value.
         */
        bool operator>( T value ) const;

        /**
         * @brief Conversion to const pointer to the underlying data.
         * @return Pointer to the first component.
         */
        explicit operator const T *() const;
        /**
         * @brief Conversion to pointer to the underlying data.
         * @return Pointer to the first component.
         */
        explicit operator T *();
        /**
         * @brief Accesses a component by index (const).
         * @param i The index (0=x, 1=y, 2=z).
         * @return The value of the component.
         */
        T operator[]( s32 i ) const;
        /**
         * @brief Accesses a component by index.
         * @param i The index (0=x, 1=y, 2=z).
         * @return Reference to the component.
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
         * @brief Gets the z component (const).
         * @return The z component.
         */
        T Z() const;
        /**
         * @brief Gets the z component (mutable).
         * @return Reference to the z component.
         */
        T &Z();

        /**
         * @brief Checks if this vector equals another, within a tolerance.
         * @param other The vector to compare to.
         * @param tolerance The error tolerance.
         * @return True if the vectors are equal within the given tolerance.
         */
        bool equals( const Vector3<T> &other, T tolerance = T( 0.0000001 ) ) const;

        /**
         * @brief Sets the vector components.
         * @param nx The x-coordinate.
         * @param ny The y-coordinate.
         * @param nz The z-coordinate.
         */
        void set( T nx, T ny, T nz );

        /**
         * @brief Sets the vector components to those of another vector.
         * @param p The vector to copy.
         */
        void set( const Vector3<T> &p );

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
         * @brief Computes the dot product with another vector.
         * @param other The other vector.
         * @return The dot product (scalar value).
         */
        T dotProduct( const Vector3<T> &other ) const;

        /**
         * @brief Computes the absolute value of the dot product with another vector.
         * @param vec The other vector.
         * @return The absolute dot product.
         */
        T dotProductABS( const Vector3<T> &vec ) const;

        /**
         * @brief Computes the Euclidean distance from another point.
         * @param other The other point.
         * @return The distance between this point and the other point.
         */
        T getDistanceFrom( const Vector3<T> &other ) const;

        /**
         * @brief Computes the squared Euclidean distance from another point.
         * @param other The other point.
         * @return The squared distance between this point and the other point.
         */
        T getDistanceFromSQ( const Vector3<T> &other ) const;

        /**
         * @brief Computes the cross product with another vector.
         * @param p The other vector.
         * @return The cross product vector.
         */
        Vector3<T> crossProduct( const Vector3<T> &p ) const;

        /**
         * @brief Checks if this point is between two other points (on the line segment).
         * @param begin The starting point.
         * @param end The ending point.
         * @return True if this point lies on the line segment.
         */
        bool isBetweenPoints( const Vector3<T> &begin, const Vector3<T> &end ) const;

        /**
         * @brief Returns a normalized copy of this vector.
         * @return A normalized vector (length 1).
         */
        Vector3<T> normaliseCopy() const;

        /**
         * @brief Normalizes this vector in place using a fast approximation.
         * @return Reference to this vector.
         */
        Vector3<T> &normaliseFast();

        /**
         * @brief Normalizes this vector and returns its original length.
         * @return The original length of the vector.
         */
        T normaliseLength();

        /**
         * @brief Normalizes this vector in place.
         */
        void normalise();

        /**
         * @brief Sets the length of this vector to a new value.
         * @param newlength The new length.
         */
        void setLength( T newlength );

        /**
         * @brief Inverts the direction of this vector.
         */
        void invert();

        //! Rotates the vector by a specified number of degrees around the Y axis and the specified
        //! center.
        /** \param degrees: Number of degrees to rotate around the Y axis.
        \param center: The center of the rotation. */
        void rotateXZBy( T degrees, const Vector3<T> &center );

        //! Rotates the vector by a specified number of degrees around the Z axis and the specified
        //! center.
        /** \param degrees: Number of degrees to rotate around the Z axis.
        \param center: The center of the rotation. */
        void rotateXYBy( T degrees, const Vector3<T> &center );

        //! Rotates the vector by a specified number of degrees around the X axis and the specified
        //! center.
        /** \param degrees: Number of degrees to rotate around the X axis.
        \param center: The center of the rotation. */
        void rotateYZBy( T degrees, const Vector3<T> &center );

        /**
         * @brief Returns an interpolated vector between this and another vector.
         * @param other The other vector.
         * @param d The interpolation value between 0.0 and 1.0.
         * @return The interpolated vector.
         */
        Vector3<T> getInterpolated( const Vector3<T> &other, T d ) const;

        /**
         * @brief Returns a quadratic interpolated vector between this and two other vectors.
         * @param other0 The first vector.
         * @param other1 The second vector.
         * @param factor The interpolation value between 0.0 and 1.0.
         * @return The quadratic interpolated vector.
         */
        Vector3<T> getInterpolated_quadratic( const Vector3<T> &other0, const Vector3<T> &other1,
                                              T factor ) const;

        /**
         * @brief Gets the Y and Z rotations of this vector.
         * @return A vector representing the rotation in degrees (Z component is always 0).
         */
        Vector3<T> getHorizontalAngle();

        /**
         * @brief Fills an array of 4 values with the vector data.
         * @param array Pointer to an array of 4 values.
         */
        void getAs4Values( T *array ) const;

        /**
         * @brief Checks if this vector has a length of 0.
         * @return True if the length is 0, false otherwise.
         */
        bool isZeroLength() const;

        /**
         * @brief Sets all components to their absolute values.
         */
        void makeAbs();

        /**
         * @brief Sets this vector's components to the minimum of its own and another vector's
         * components.
         * @param cmp The vector to compare with.
         */
        void makeFloor( const Vector3 &cmp );

        /**
         * @brief Sets this vector's components to the maximum of its own and another vector's
         * components.
         * @param cmp The vector to compare with.
         */
        void makeCeil( const Vector3 &cmp );

        /**
         * @brief Returns a vector perpendicular to this vector.
         * @return A perpendicular vector.
         */
        Vector3 perpendicular( void ) const;

        /**
         * @brief Returns the midpoint between this and another vector.
         * @param vec The other vector.
         * @return The midpoint vector.
         */
        Vector3 midPoint( const Vector3<T> &vec ) const;

        /**
         * @brief Returns a pointer to the underlying data.
         * @return Pointer to the first component.
         */
        T *ptr();

        /**
         * @brief Returns a const pointer to the underlying data.
         * @return Const pointer to the first component.
         */
        const T *ptr() const;

        /**
         * @brief Returns a zero vector (0,0,0).
         * @return A zero vector.
         */
        static const Vector3 &zero();

        /**
         * @brief Returns a unit vector (1,1,1).
         * @return A unit vector.
         */
        static const Vector3 &unit();

        /**
         * @brief Returns a unit vector along the positive X axis (1,0,0).
         * @return A unit X vector.
         */
        static const Vector3 &unitX();

        /**
         * @brief Returns a unit vector along the positive Y axis (0,1,0).
         * @return A unit Y vector.
         */
        static const Vector3 &unitY();

        /**
         * @brief Returns a unit vector along the positive Z axis (0,0,1).
         * @return A unit Z vector.
         */
        static const Vector3 &unitZ();

        /**
         * @brief Returns a positive vector along the x axis (1,0,0).
         * @return A positive X vector.
         */
        static const Vector3 &positiveX();

        /**
         * @brief Returns a negative vector along the x axis (-1,0,0).
         * @return A negative X vector.
         */
        static const Vector3 &negativeX();

        /**
         * @brief Returns a positive vector along the y axis (0,1,0).
         * @return A positive Y vector.
         */
        static const Vector3 &positiveY();

        /**
         * @brief Returns a negative vector along the y axis (0,-1,0).
         * @return A negative Y vector.
         */
        static const Vector3 &negativeY();

        /**
         * @brief Returns a positive vector along the z axis (0,0,1).
         * @return A positive Z vector.
         */
        static const Vector3 &positiveZ();

        /**
         * @brief Returns a negative vector along the z axis (0,0,-1).
         * @return A negative Z vector.
         */
        static const Vector3 &negativeZ();

        /**
         * @brief Returns a unit vector pointing up (0,1,0).
         * @return An up vector.
         */
        static const Vector3 &up();

        /**
         * @brief Returns a unit vector pointing down (0,-1,0).
         * @return A down vector.
         */
        static const Vector3 &down();

        /**
         * @brief Returns a unit vector pointing right (1,0,0).
         * @return A right vector.
         */
        static const Vector3 &right();

        /**
         * @brief Returns a unit vector pointing left (-1,0,0).
         * @return A left vector.
         */
        static const Vector3 &left();

        /**
         * @brief Returns a unit vector pointing backward (0,0,1).
         * @return A back vector.
         */
        static const Vector3 &back();

        /**
         * @brief Returns a unit vector pointing forward (0,0,-1).
         * @return A forward vector.
         */
        static const Vector3 &forward();

        /**
         * @brief Creates a Vector3 from spherical coordinates.
         * @param radius The distance from the origin to the point.
         * @param latitude The angle between the point and the xz-plane, in radians.
         * @param longitude The angle between the point and the positive z-axis, in radians.
         * @return The resulting Vector3.
         */
        static Vector3 fromCoords( T radius, T latitude, T longitude );

        /**
         * @brief Checks whether the vector's elements are valid numbers (not NaN or infinity).
         * @return True if the vector is valid, false otherwise.
         */
        bool isValid() const;

        /**
         * @brief Checks whether the vector's elements are finite numbers (not NaN or infinity).
         * @return True if the vector is finite, false otherwise.
         */
        bool isFinite() const;

        /**
         * @brief Generates a complement basis for a given vector w.
         * @param u On output, the first basis vector.
         * @param v On output, the second basis vector.
         * @param w The original vector to generate the basis from.
         */
        static void generateComplementBasis( Vector3<T> &u, Vector3<T> &v, const Vector3<T> &w );

        /**
         * @brief Computes the distance between two vectors.
         * @param v1 The first vector.
         * @param v2 The second vector.
         * @return The distance between v1 and v2.
         */
        static T distance( const Vector3<T> &v1, const Vector3<T> &v2 );

        /**
         * @brief The zero vector (0,0,0).
         */
        static const Vector3 ZERO;

        /**
         * @brief The x-axis unit vector (1,0,0).
         */
        static const Vector3 UNIT_X;

        /**
         * @brief The y-axis unit vector (0,1,0).
         */
        static const Vector3 UNIT_Y;

        /**
         * @brief The z-axis unit vector (0,0,1).
         */
        static const Vector3 UNIT_Z;

        /**
         * @brief The unit vector (1,1,1).
         */
        static const Vector3 UNIT;

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

    private:
        /**
         * @brief Compares this vector with another vector (lexicographical order).
         * @param other The vector to compare with.
         * @return 0 if equal, negative if less, positive if greater.
         */
        s32 compare( const Vector3 &other ) const;
    };

    /**
     * @brief Scalar multiplication of a vector with a scalar value.
     * @tparam S The type of the scalar value.
     * @tparam T The type of the vector elements.
     * @param scalar The scalar value to multiply with.
     * @param vector The vector to multiply.
     * @return The result of scalar multiplication.
     */
    template <class S, class T>
    Vector3<T> operator*( S scalar, const Vector3<T> &vector )
    {
        return vector * scalar;
    }

    /** @brief Scalar division of a vector by a scalar value.
     * @tparam S The type of the scalar value.
     * @tparam T The type of the vector elements.
     * @param scalar The scalar value to divide by.
     * @param vector The vector to divide.
     * @return The result of scalar division.
     */
    template <class S, class T>
    Vector3<T> operator/( S scalar, const Vector3<T> &vector )
    {
        return { static_cast<T>( scalar / vector.x ), static_cast<T>( scalar / vector.y ),
                 static_cast<T>( scalar / vector.z ) };
    }

    /**
     * @brief A typedef for a 3-dimensional vector with integer elements.
     */
    using Vector3I = Vector3<s32>;

    /**
     * @brief A typedef for a 3-dimensional vector with single-precision floating-point elements.
     */
    using Vector3F = Vector3<f32>;

    /**
     * @brief A typedef for a 3-dimensional vector with double-precision floating-point elements.
     */
    using Vector3D = Vector3<f64>;
}  // namespace workphone

#endif
