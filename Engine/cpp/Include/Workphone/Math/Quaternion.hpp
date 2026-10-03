#ifndef __WP_QUATERNION_H_
#define __WP_QUATERNION_H_

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Matrix3.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <cmath>
#include <complex>

namespace workphone
{

    /**
     * @brief Templated class for representing 3D rotations using quaternions.
     *
     * @tparam T The numeric type for quaternion components (typically float or double).
     *
     * A quaternion is a mathematical structure that extends complex numbers and provides
     * an efficient way to represent 3D rotations without suffering from gimbal lock.
     * It consists of four components: a scalar part (w) and a vector part (x, y, z).
     *
     * Quaternions offer several advantages over other rotation representations:
     * - No gimbal lock issues that occur with Euler angles
     * - Smooth interpolation between orientations
     * - Compact representation (4 components vs 9 for rotation matrices)
     * - Efficient composition of rotations
     *
     * Mathematical representation: q = w + xi + yj + zk
     * where i, j, k are the fundamental quaternion units.
     *
     * @note For unit quaternions (normalized), the quaternion represents a rotation
     *       where w = cos(θ/2) and (x,y,z) = sin(θ/2) * axis, where θ is the
     *       rotation angle and axis is the unit rotation axis.
     *
     * @see Vector3, Matrix3
     */
    template <typename T>
    class WPCore_API Quaternion
    {
    public:
        /**
         * @brief Default constructor - creates an identity quaternion.
         *
         * Initializes the quaternion to represent no rotation:
         * w = 1, x = 0, y = 0, z = 0
         */
        Quaternion();

        /**
         * @brief Constructs a quaternion from individual components.
         *
         * @param w The scalar (real) part of the quaternion
         * @param x The x-component of the vector (imaginary) part
         * @param y The y-component of the vector (imaginary) part
         * @param z The z-component of the vector (imaginary) part
         *
         * @note The constructor parameters are ordered as (w, x, y, z) following
         *       the mathematical convention where w is the scalar part.
         */
        Quaternion( T w, T x, T y, T z );

        /**
         * @brief Constructs a quaternion from Euler angles.
         *
         * Creates a quaternion representing the rotation specified by the given
         * Euler angles in radians. The rotation order is XYZ.
         *
         * @param x Rotation around the x-axis in radians
         * @param y Rotation around the y-axis in radians
         * @param z Rotation around the z-axis in radians
         *
         * @note The resulting quaternion is automatically normalized.
         */
        Quaternion( T x, T y, T z );

        /**
         * @brief Constructs a quaternion from a rotation matrix.
         *
         * Converts a 3x3 rotation matrix to its quaternion representation using
         * Shoemake's algorithm for numerical stability.
         *
         * @param mat The rotation matrix to convert (must be orthogonal)
         *
         * @pre The input matrix should be a valid rotation matrix (orthogonal with determinant 1)
         */
        explicit Quaternion( const Matrix3<T> &mat );

        /**
         * @brief Constructs a quaternion from an array of components.
         *
         * @param ptr Pointer to array containing [x, y, z, w] components
         *
         * @pre ptr must point to at least 4 valid T values
         * @note Array order is [x, y, z, w], different from constructor parameter order
         */
        explicit Quaternion( const T *ptr );

        /**
         * @brief Copy constructor.
         *
         * @param other The quaternion to copy
         */
        Quaternion( const Quaternion &other );

        /**
         * @brief Constructs a quaternion from three orthogonal axes.
         *
         * Creates a quaternion representing the rotation defined by the given
         * coordinate system axes.
         *
         * @param xaxis The x-axis direction vector
         * @param yaxis The y-axis direction vector
         * @param zaxis The z-axis direction vector
         *
         * @pre The input vectors should be orthonormal
         */
        Quaternion( const Vector3<T> &xaxis, const Vector3<T> &yaxis, const Vector3<T> &zaxis );

        /**
         * @brief Assignment operator.
         *
         * @param other The quaternion to assign from
         * @return Reference to this quaternion after assignment
         */
        Quaternion &operator=( const Quaternion &other );

        /**
         * @brief Equality comparison operator.
         *
         * Compares two quaternions for equality within floating-point tolerance.
         *
         * @param other The quaternion to compare with
         * @return true if quaternions are approximately equal, false otherwise
         */
        bool operator==( const Quaternion &other ) const;

        /**
         * @brief Inequality comparison operator.
         *
         * @param other The quaternion to compare with
         * @return true if quaternions are not approximately equal, false otherwise
         */
        bool operator!=( const Quaternion &other ) const;

        /**
         * @brief Quaternion addition operator.
         *
         * Performs component-wise addition of two quaternions.
         *
         * @param other The quaternion to add
         * @return The sum of the two quaternions
         *
         * @note Addition is commutative: q1 + q2 = q2 + q1
         */
        Quaternion operator+( const Quaternion &other ) const;

        /**
         * @brief Quaternion subtraction operator.
         *
         * Performs component-wise subtraction of two quaternions.
         *
         * @param other The quaternion to subtract
         * @return The difference of the two quaternions
         */
        Quaternion operator-( const Quaternion &other ) const;

        /**
         * @brief Unary negation operator.
         *
         * Returns the additive inverse of the quaternion (negates all components).
         *
         * @return The negated quaternion
         *
         * @note For unit quaternions, -q represents the same rotation as q
         */
        Quaternion operator-() const;

        /**
         * @brief Quaternion multiplication operator (Hamilton product).
         *
         * Multiplies this quaternion by another quaternion using the Hamilton product.
         * This operation combines rotations, with the right operand applied first.
         *
         * @param other The quaternion to multiply by
         * @return The product quaternion representing the combined rotation
         *
         * @warning Quaternion multiplication is NOT commutative: p * q ≠ q * p in general
         *
         * @note For unit quaternions representing rotations, multiplication
         *       corresponds to composition of rotations.
         */
        Quaternion operator*( const Quaternion &other ) const;

        /**
         * @brief Scalar multiplication operator.
         *
         * Multiplies all quaternion components by a scalar value.
         *
         * @param s The scalar to multiply by
         * @return The scaled quaternion
         *
         * @note Scalar multiplication is commutative: q * s = s * q
         */
        Quaternion operator*( T s ) const;

        /**
         * @brief Friend function for scalar-quaternion multiplication.
         *
         * Enables multiplication with scalar as left operand: scalar * quaternion
         *
         * @param scalar The scalar multiplier
         * @param rkQ The quaternion to multiply
         * @return The scaled quaternion
         */
        friend Quaternion<T> operator*( T fScalar, const Quaternion<T> &rkQ )
        {
            return Quaternion( fScalar * rkQ.w, fScalar * rkQ.x, fScalar * rkQ.y, fScalar * rkQ.z );
        }

        /**
         * @brief In-place scalar multiplication operator.
         *
         * Multiplies this quaternion by a scalar value in place.
         *
         * @param s The scalar to multiply by
         * @return Reference to this quaternion after scaling
         */
        Quaternion &operator*=( T s );

        /**
         * @brief Vector rotation operator.
         *
         * Rotates a 3D vector by this quaternion using the formula: q * v * q^(-1)
         * This is equivalent to applying the rotation represented by the quaternion
         * to the vector.
         *
         * @param v The vector to rotate
         * @return The rotated vector
         *
         * @note This implementation uses an optimized form that avoids explicit
         *       quaternion-vector-quaternion multiplication for better performance.
         *
         * @pre The quaternion should be normalized for correct rotation
         */
        Vector3<T> operator*( const Vector3<T> &v ) const;

        /**
         * @brief In-place quaternion multiplication operator.
         *
         * Multiplies this quaternion by another quaternion in place.
         *
         * @param other The quaternion to multiply by
         * @return Reference to this quaternion after multiplication
         */
        Quaternion &operator*=( const Quaternion &other );

        /**
         * @brief Const cast operator to array.
         *
         * Provides direct access to quaternion components as a const array.
         *
         * @return Const pointer to the first component (w)
         *
         * @note Array layout is [w, x, y, z]
         */
        explicit operator const T *() const;

        /**
         * @brief Cast operator to array.
         *
         * Provides direct access to quaternion components as an array.
         *
         * @return Pointer to the first component (w)
         *
         * @note Array layout is [w, x, y, z]
         */
        explicit operator T *();

        /**
         * @brief Const subscript operator.
         *
         * Provides array-style access to quaternion components.
         *
         * @param i Component index: 0=w, 1=x, 2=y, 3=z
         * @return The component value at the specified index
         *
         * @pre i must be in range [0, 3]
         */
        T operator[]( int i ) const;

        /**
         * @brief Subscript operator.
         *
         * Provides array-style access to quaternion components.
         *
         * @param i Component index: 0=w, 1=x, 2=y, 3=z
         * @return Reference to the component at the specified index
         *
         * @pre i must be in range [0, 3]
         */
        T &operator[]( int i );

        /**
         * @brief Gets the scalar (w) component.
         *
         * @return The w component value
         */
        T W() const;

        /**
         * @brief Gets a reference to the scalar (w) component.
         *
         * @return Reference to the w component
         */
        T &W();

        /**
         * @brief Gets the x component of the vector part.
         *
         * @return The x component value
         */
        T X() const;

        /**
         * @brief Gets a reference to the x component of the vector part.
         *
         * @return Reference to the x component
         */
        T &X();

        /**
         * @brief Gets the y component of the vector part.
         *
         * @return The y component value
         */
        T Y() const;

        /**
         * @brief Gets a reference to the y component of the vector part.
         *
         * @return Reference to the y component
         */
        T &Y();

        /**
         * @brief Gets the z component of the vector part.
         *
         * @return The z component value
         */
        T Z() const;

        /**
         * @brief Gets a reference to the z component of the vector part.
         *
         * @return Reference to the z component
         */
        T &Z();

        /**
         * @brief Calculates the dot product with another quaternion.
         *
         * Computes the four-dimensional dot product: w₁w₂ + x₁x₂ + y₁y₂ + z₁z₂
         *
         * @param other The other quaternion
         * @return The dot product value
         *
         * @note For unit quaternions, the dot product represents the cosine of
         *       half the angle between the rotations.
         */
        T dotProduct( const Quaternion &other ) const;

        /**
         * @brief Normalizes this quaternion in place.
         *
         * Scales the quaternion to unit length, making it suitable for representing
         * rotations. If the quaternion is already normalized, no operation is performed.
         *
         * @note After normalization, magnitude() will return 1.0 (within floating-point precision)
         * @warning If the quaternion has zero length, the result is undefined
         */
        void normalise();

        /**
         * @brief Returns a normalized copy of this quaternion.
         *
         * Creates a new quaternion that is the normalized version of this one,
         * without modifying the original.
         *
         * @return A normalized copy of this quaternion
         */
        Quaternion normaliseCopy() const;

        /**
         * @brief Sets all quaternion components.
         *
         * @param x The x component value
         * @param y The y component value
         * @param z The z component value
         * @param w The w component value
         */
        void set( T x, T y, T z, T w );

        /**
         * @brief Calculates the multiplicative inverse of this quaternion.
         *
         * For a unit quaternion, the inverse equals the conjugate. For non-unit
         * quaternions, the inverse is conjugate divided by the squared magnitude.
         *
         * @return The inverse quaternion
         *
         * @note If q * q.inverse() should equal the identity quaternion
         * @warning Returns identity quaternion if this quaternion has zero magnitude
         */
        Quaternion inverse() const;

        /**
         * @brief Calculates the quaternion exponential.
         *
         * Computes e^q using the quaternion exponential formula.
         * For pure quaternions (w=0), this gives cos(|v|) + sin(|v|) * v/|v|
         *
         * @return The exponential of this quaternion
         */
        Quaternion exp() const;

        /**
         * @brief Calculates the quaternion natural logarithm.
         *
         * Computes the natural logarithm of this quaternion.
         * For unit quaternions, this gives the "rotation vector" representation.
         *
         * @return The natural logarithm of this quaternion
         */
        Quaternion log() const;

        /**
         * @brief Spherical linear interpolation between two quaternions.
         *
         * Performs smooth interpolation between two quaternions along the shortest
         * path on the quaternion sphere, maintaining constant angular velocity.
         *
         * @param t Interpolation parameter in [0,1] (0=q1, 1=q2)
         * @param q1 Starting quaternion
         * @param q2 Ending quaternion
         * @param shortestPath If true, choose the shorter rotation path
         * @return Interpolated quaternion
         *
         * @note SLERP preserves the unit length property if input quaternions are unit
         */
        static Quaternion slerp( T t, const Quaternion &q1, const Quaternion &q2,
                                 bool shortestPath = false );

        /**
         * @brief Spherical quadratic interpolation using four quaternions.
         *
         * Performs smooth interpolation through four quaternions using spherical
         * quadratic interpolation, providing C1 continuity for animation.
         *
         * @param fT Interpolation parameter in [0,1]
         * @param rkP First control quaternion
         * @param rkA Second control quaternion
         * @param rkB Third control quaternion
         * @param rkQ Fourth control quaternion
         * @param shortestPath If true, use shortest rotation paths
         * @return Interpolated quaternion
         */
        static Quaternion squad( T fT, const Quaternion &rkP, const Quaternion &rkA,
                                 const Quaternion &rkB, const Quaternion &rkQ,
                                 bool shortestPath = false );

        /**
         * @brief Sets this quaternion from angle-axis representation.
         *
         * Constructs a quaternion representing rotation by the given angle
         * around the specified axis.
         *
         * @param angle Rotation angle in radians
         * @param axis Unit vector representing the rotation axis
         *
         * @pre The axis vector should be normalized
         */
        void fromAngleAxis( T angle, const Vector3<T> &axis );

        /**
         * @brief Sets this quaternion from Euler angles in degrees.
         *
         * @param degrees Vector containing Euler angles in degrees (x, y, z)
         */
        void fromDegrees( const Vector3<T> &degrees );

        /**
         * @brief Sets this quaternion from Euler angles in radians.
         *
         * @param radians Vector containing Euler angles in radians (x, y, z)
         */
        void fromRadians( const Vector3<T> &radians );

        /**
         * @brief Sets this quaternion to the identity.
         *
         * Resets the quaternion to represent no rotation: w=1, x=0, y=0, z=0
         */
        void makeIdentity();

        /**
         * @brief Sets this quaternion from a rotation matrix.
         *
         * Converts a 3x3 rotation matrix to quaternion representation using
         * Shoemake's numerically stable algorithm.
         *
         * @param kRot The rotation matrix to convert
         *
         * @pre The input matrix should be a valid rotation matrix
         */
        void fromRotationMatrix( const Matrix3<T> &kRot );

        /**
         * @brief Converts this quaternion to a rotation matrix.
         *
         * Generates the equivalent 3x3 rotation matrix representation.
         *
         * @param kRot Reference to matrix where result will be stored
         *
         * @note The quaternion should be normalized for correct results
         */
        void toRotationMatrix( Matrix3<T> &kRot ) const;

        /**
         * @brief Sets this quaternion from three orthogonal axes.
         *
         * Constructs the quaternion from the given coordinate frame axes.
         *
         * @param xAxis The x-axis direction
         * @param yAxis The y-axis direction
         * @param zAxis The z-axis direction
         *
         * @pre The axes should form an orthonormal basis
         */
        void fromAxes( const Vector3<T> &xAxis, const Vector3<T> &yAxis, const Vector3<T> &zAxis );

        /**
         * @brief Extracts three orthogonal axes from this quaternion.
         *
         * Computes the coordinate frame axes represented by this quaternion.
         *
         * @param xAxis Reference to store the x-axis direction
         * @param yAxis Reference to store the y-axis direction
         * @param zAxis Reference to store the z-axis direction
         */
        void toAxes( Vector3<T> &xAxis, Vector3<T> &yAxis, Vector3<T> &zAxis ) const;

        /**
         * @brief Creates a quaternion from angle-axis representation.
         *
         * Static factory method that creates a quaternion representing rotation
         * by the given angle around the specified axis.
         *
         * @param rfAngle Rotation angle in radians
         * @param rkAxis Unit vector representing the rotation axis
         * @return The constructed quaternion
         *
         * @pre The axis vector should be normalized
         */
        static Quaternion<T> angleAxis( const T &rfAngle, const Vector3<T> &rkAxis );

        /**
         * @brief Creates a quaternion from Euler angles in radians.
         *
         * Static factory method that creates a quaternion from XYZ Euler angles.
         *
         * @param x Rotation around x-axis in radians
         * @param y Rotation around y-axis in radians
         * @param z Rotation around z-axis in radians
         * @return The constructed quaternion
         */
        static Quaternion<T> euler( T x, T y, T z );

        /**
         * @brief Creates a quaternion from Euler angles in degrees.
         *
         * Static factory method that creates a quaternion from XYZ Euler angles.
         *
         * @param x Rotation around x-axis in degrees
         * @param y Rotation around y-axis in degrees
         * @param z Rotation around z-axis in degrees
         * @return The constructed quaternion
         */
        static Quaternion<T> eulerDegrees( T x, T y, T z );

        /**
         * @brief Returns the identity quaternion.
         *
         * Static method that returns a quaternion representing no rotation.
         *
         * @return Identity quaternion (1, 0, 0, 0)
         */
        static const Quaternion identity();

        /**
         * @brief Gets a pointer to the quaternion data.
         *
         * Provides direct access to the internal component array.
         *
         * @return Pointer to the first component
         *
         * @note Array layout is [w, x, y, z] for this implementation
         */
        T *ptr();

        /**
         * @brief Gets a const pointer to the quaternion data.
         *
         * Provides direct read-only access to the internal component array.
         *
         * @return Const pointer to the first component
         *
         * @note Array layout is [w, x, y, z] for this implementation
         */
        const T *ptr() const;

        /**
         * @brief Sets this quaternion from Euler angles in radians.
         *
         * Alternative interface for setting quaternion from XYZ Euler angles.
         *
         * @param x Rotation around x-axis in radians
         * @param y Rotation around y-axis in radians
         * @param z Rotation around z-axis in radians
         */
        void set( T x, T y, T z );

        /**
         * @brief Extracts the yaw angle from this quaternion.
         *
         * Computes the yaw (rotation around Y-axis) component of the rotation
         * represented by this quaternion.
         *
         * @param reprojectAxis Whether to reproject the quaternion onto the unit sphere
         * @return The yaw angle in radians
         */
        T getYaw( bool reprojectAxis = true ) const;

        /**
         * @brief Calculates the squared magnitude (norm) of this quaternion.
         *
         * Computes w² + x² + y² + z² without taking the square root.
         *
         * @return The squared magnitude
         *
         * @note This is more efficient than magnitude() when only relative
         *       magnitudes are needed or for checking if quaternion is unit length.
         */
        T norm() const;

        /**
         * @brief Calculates the squared magnitude of this quaternion.
         *
         * Computes w² + x² + y² + z² (same as norm()).
         *
         * @return The squared magnitude
         */
        T magnitudeSquared() const;

        /**
         * @brief Calculates the magnitude (length) of this quaternion.
         *
         * Computes √(w² + x² + y² + z²).
         *
         * @return The magnitude
         *
         * @note For unit quaternions (representing rotations), this should equal 1.0
         */
        T magnitude() const;

        /**
         * @brief Rotates a vector by this quaternion.
         *
         * Applies the rotation represented by this quaternion to the given vector
         * using an optimized algorithm.
         *
         * @param v The vector to rotate
         * @return The rotated vector
         *
         * @pre The quaternion should be normalized for correct rotation
         */
        Vector3<T> rotate( const Vector3<T> &v ) const;

        /**
         * @brief Applies the inverse rotation to a vector.
         *
         * Rotates the vector by the inverse of this quaternion's rotation.
         * Equivalent to using the conjugate quaternion for unit quaternions.
         *
         * @param v The vector to rotate
         * @return The inversely rotated vector
         *
         * @pre The quaternion should be normalized for correct rotation
         */
        Vector3<T> rotateInv( const Vector3<T> &v ) const;

        /**
         * @brief Gets the rotated x-axis (first basis vector).
         *
         * Computes the direction of the x-axis after applying this quaternion's rotation.
         *
         * @return The rotated x-axis vector
         */
        Vector3<T> getBasisVector0() const;

        /**
         * @brief Gets the rotated y-axis (second basis vector).
         *
         * Computes the direction of the y-axis after applying this quaternion's rotation.
         *
         * @return The rotated y-axis vector
         */
        Vector3<T> getBasisVector1() const;

        /**
         * @brief Gets the rotated z-axis (third basis vector).
         *
         * Computes the direction of the z-axis after applying this quaternion's rotation.
         *
         * @return The rotated z-axis vector
         */
        Vector3<T> getBasisVector2() const;

        /**
         * @brief Returns the quaternion conjugate.
         *
         * Computes the conjugate by negating the vector part: (w, -x, -y, -z).
         * For unit quaternions, the conjugate equals the inverse.
         *
         * @return The conjugate quaternion
         */
        Quaternion<T> getConjugate() const;

        /**
         * @brief Checks if the quaternion has valid component values.
         *
         * Verifies that all components are finite (not NaN or infinite).
         *
         * @return true if all components are finite, false otherwise
         */
        bool isValid() const;

        /**
         * @brief Checks if all quaternion components are finite.
         *
         * Tests whether all components are finite numbers (not NaN or infinite).
         *
         * @return true if all components are finite, false otherwise
         */
        bool isFinite() const;

        /**
         * @brief Checks if the quaternion is normalized (unit length).
         *
         * Tests whether the quaternion magnitude is approximately 1.0 within tolerance.
         *
         * @return true if quaternion is approximately unit length, false otherwise
         */
        bool isUnit() const;

        /**
         * @brief Checks if the quaternion has reasonable component values.
         *
         * Performs sanity checks to ensure the quaternion is in a reasonable state
         * for representing rotations.
         *
         * @return true if quaternion appears sane, false otherwise
         */
        bool isSane() const;

        /**
         * @brief Computes the shortest rotation between two vectors.
         *
         * Calculates the quaternion representing the shortest rotation that would
         * align the source vector with the destination vector.
         *
         * @param src The source vector (starting direction)
         * @param dest The destination vector (target direction)
         * @param fallbackAxis Axis to use if vectors are nearly opposite (default: auto-generated)
         * @return Quaternion representing the shortest rotation from src to dest
         *
         * @note If src and dest are nearly opposite, uses fallbackAxis to avoid singularities
         */
        static Quaternion<T> getRotationTo( const Vector3<T> &src, const Vector3<T> &dest,
                                            const Vector3<T> &fallbackAxis = Vector3<T>::zero() );

        T w; /**< The scalar (real) component of the quaternion */
        T x; /**< The x component of the vector (imaginary) part */
        T y; /**< The y component of the vector (imaginary) part */
        T z; /**< The z component of the vector (imaginary) part */
    };

    /// A 3D quaternion with single-precision floating-point components.
    using QuaternionF = Quaternion<f32>;

    /// A 3D quaternion with double-precision floating-point components.
    using QuaternionD = Quaternion<f64>;

}  // namespace workphone

#endif
