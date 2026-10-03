#ifndef __WP_MATH_H_INCLUDED__
#define __WP_MATH_H_INCLUDED__

#include <Workphone/WorkphoneTypes.hpp>

#ifdef min
#    undef min
#endif

#ifdef max
#    undef max
#endif

namespace workphone
{

    /**
     * @brief The Math class provides a collection of
     * static methods for performing common mathematical operations.
     */
    template <class T>
    class Math
    {
    public:
        //! Returns the value of PI.
        static T WPCore_API pi();

        //! Returns the value of PI divided by 2.
        static T WPCore_API half_pi();

        //! Returns the value of PI multiplied by 2.
        static T WPCore_API two_pi();

        //! Converts degrees to radians.
        static T WPCore_API deg_to_rad();

        //! Converts radians to degrees.
        static T WPCore_API rad_to_deg();

        //! Rounds a value to the nearest integer.
        static s32 WPCore_API Round( T value );

        //! Returns the absolute value of a number.
        static T WPCore_API Abs( T value );

        //! Computes the arctangent of a number.
        static T WPCore_API Atan( T x );

        //! Computes the arctangent of two numbers.
        static T WPCore_API ATan2( const T &y, const T &x );

        //! Computes the square of a number.
        static T WPCore_API Sqr( T value );

        //! Computes the square root of a number.
        static T WPCore_API Sqrt( T value );

        //! Computes the inverse square root of a number.
        static T WPCore_API SqrtInv( T value );

        //! Computes the sine of an angle.
        static T WPCore_API Sin( T value );

        //! Computes the cosine of an angle.
        static T WPCore_API Cos( T value );

        //! Computes the tangent of an angle.
        static T WPCore_API Tan( T value );

        //! Computes the arccosine of a number.
        static T WPCore_API ACos( T value );

        //! Computes the arcsine of a number.
        static T WPCore_API ASin( T fValue );

        //! Converts degrees to radians.
        static T WPCore_API DegToRad( const T &deg );

        //! Converts radians to degrees.
        static T WPCore_API RadToDeg( const T &rad );

        //! Converts radians to full degrees.
        static T WPCore_API RadToFullDegrees( const T &rad );

        //! Sets the random seed used by RangedRandom.
        static void WPCore_API SetRandomSeed( s32 seed );

        //! Generates a random number between min and max.
        static T WPCore_API RangedRandom( T min, T max, s32 seed = 0 );

        //! Swaps the values of two variables.
        static void WPCore_API swap( T &firstValue, T &secondValue );

        //! Swaps the values of two variables if the second value is greater than the first.
        static void WPCore_API swapIfGreater( T &firstValue, T &secondValue );

        //! Swaps the values of two variables if the second value is less than the first.
        static void WPCore_API swapIfLessThan( T &firstValue, T &secondValue );

        //! Returns the nearest multiple of a given number.
        static T WPCore_API makeMultipleOf( T value, T multiple );

        //! Tests whether a number is finite.
        static bool WPCore_API isFinite( T value );

        //! Tests whether a number is NaN (not a number).
        static bool WPCore_API isNaN( T value );

        //! Tests whether two numbers are equal, within a given tolerance.
        static bool WPCore_API equals( T af, T bf, T maxDiff = 5 );

        //! Normalizes a value between a minimum and maximum.
        static T WPCore_API normalize( const T &value, const T &min, const T &max );

        //! Returns the minimum of two values.
        static T WPCore_API min( T a, T b );

        //! Returns the maximum of two values.
        static T WPCore_API max( T a, T b );

        //! Linearly interpolates between two values.
        template <class B>
        static B lerp( const B a, const B b, const T t )
        {
            return a + t * ( b - a );
        }

        //! Calculates the inverse linear interpolation of a value.
        static T WPCore_API inverseLerp( T a, T b, T t );

        //! Clamps a value between a minimum and maximum value.
        static const T WPCore_API clamp( T value, T low, T high );

        /**
         * Clamps a value between 0 and 1.
         *
         * @param value The value to be clamped.
         * @return The value after being clamped between 0 and 1.
         */
        static T WPCore_API clamp01( T value );

        /**
         * Determines if a float is approximately zero, taking floating point rounding errors into
         * account.
         *
         * @param a The float to be checked for near-zero.
         * @param tolerance The amount of tolerance in the near-zero check.
         * @return True if a is approximately zero, false otherwise.
         */
        static bool WPCore_API isZero( T a, T tolerance );

        /**
         * Calculates the reciprocal (1/x) of a given float.
         *
         * @param f The float to take the reciprocal of.
         * @return The reciprocal of f.
         */
        static T WPCore_API reciprocal( const T &f );

        /**
         * Rounds a given float up to the nearest power of 2.
         *
         * @param x The float to be rounded up.
         * @return The nearest power of 2 above x.
         */
        static T WPCore_API nextPow2( T x );

        /**
         * Wraps a given angle in radians between -pi and pi.
         *
         * @param r The angle to be wrapped in radians.
         * @return The wrapped angle in radians.
         */
        static T WPCore_API wrapRadians( T r );

        /**
         * Wraps a given angle in degrees between 0 and range.
         *
         * @param degrees The angle to be wrapped in degrees.
         * @param range The range to wrap the angle in degrees.
         * @return The wrapped angle in degrees.
         */
        static T WPCore_API wrapDegrees( T degrees, T range );

        /**
         * Wraps a given angle in gradians between 0 and 400.
         *
         * @param r The angle to be wrapped in gradians.
         * @return The wrapped angle in gradians.
         */
        static T WPCore_API wrapGradians( T r );

        /**
         * Wraps a given value between a lower and upper bound.
         *
         * @param value The value to be wrapped.
         * @param lowerBound The lower bound to wrap the value between.
         * @param upperBound The upper bound to wrap the value between.
         * @return The wrapped value.
         */
        static T WPCore_API wrap( T value, T lowerBound, T upperBound );

        /**
         * Calculates the exponential function of a given float.
         *
         * @param value The float to calculate the exponential function of.
         * @return The exponential function of value.
         */
        static T WPCore_API Exp( T value );

        /**
         * Determines the sign of a given float.
         *
         * @param value The float to determine the sign of.
         * @return 1 if value is positive, -1 if value is negative, and 0 if value is zero.
         */
        static int WPCore_API Sign( T value );

        /**
         * Rounds a given float down to the nearest integer.
         *
         * @param value The float to round down.
         * @return The integer rounded down from value.
         */
        static T WPCore_API Floor( T value );

        /**
         * Rounds a given float down to the nearest integer, and returns it as a signed 32-bit integer.
         *
         * @param value The float to round down.
         * @return The integer rounded down from value as a signed 32-bit integer.
         */
        static s32 WPCore_API FloorToInt( T value );

        /**
         * @brief Rounds the given value to the nearest integer.
         * @param value The value to round.
         * @return The rounded value.
         */
        static T WPCore_API round( T value );

        /**
         * @brief Rounds the given value to the specified number of decimal places.
         * @param value The value to round.
         * @param prec The number of decimal places to round to.
         * @return The rounded value.
         */
        static T WPCore_API round( T value, u32 prec );

        /**
         * @brief Rounds the given value to the nearest integer and returns the result as an integer.
         * @param value The value to round.
         * @return The rounded value as an integer.
         */
        static s32 WPCore_API roundToInt( T value );

        /**
         * @brief Rounds the given value to the nearest float.
         * @param value The value to round.
         * @return The rounded value as a float.
         */
        static f32 WPCore_API roundToFloat( T value );

        /**
         * @brief Calculates the natural logarithm of the given value.
         * @param value The value to calculate the logarithm of.
         * @return The natural logarithm of the given value.
         */
        static T WPCore_API Ln( T value );

        /**
         * @brief Truncates the given value to the nearest integer.
         * @param value The value to truncate.
         * @return The truncated value.
         */
        static T WPCore_API trunc( T value );

        /**
         * @brief Calculates the modulo of two values.
         * @param x The dividend.
         * @param y The divisor.
         * @return The remainder of the division x / y.
         */
        static T WPCore_API Mod( T x, T y );

        /**
         * @brief Calculates the power of the given value to the specified power.
         * @param value The value to raise to the power.
         * @param powVal The power to raise the value to.
         * @return The value raised to the specified power.
         */
        static T WPCore_API Pow( T value, T powVal );

        /**
         * @brief Limits the given value to be no greater than the specified limit.
         * @param v The value to limit.
         * @param value The limit to apply.
         */
        static void WPCore_API Limit( T &v, T value );

        /**
         * @brief Returns the value that is considered "close enough" to zero given the type's epsilon.
         * @return The value that is considered "close enough" to zero.
         */
        static T WPCore_API epsilon();

        /**
         * @brief Calculates the 2D Perlin noise at the specified point.
         * @param x The X coordinate of the point.
         * @param y The Y coordinate of the point.
         * @return The value of the 2D Perlin noise at the specified point.
         */
        static T WPCore_API PerlinNoise( T x, T y );

        /**
         * @brief Calculates the fade function for Perlin noise calculations.
         * @param t The input to the fade function.
         * @return The output of the fade function.
         */
        static T WPCore_API Fade( T t );

        /**
         * @brief Performs a linear interpolation between two values based on a ratio.
         * @param t The ratio between the two values (0.0-1.0).
         * @param a The first value to interpolate between.
         * @param b The second value to interpolate between.
         * @return The interpolated value.
         */
        static T WPCore_API Lerp( T t, T a, T b );

        /**
         * Returns the gradient for a 1D Perlin noise function given the integer hash and input x
         * coordinate.
         *
         * @param hash The integer hash used to calculate the gradient.
         * @param x The input x coordinate used to calculate the gradient.
         *
         * @return The gradient for the given parameters.
         */
        static T WPCore_API Grad( int hash, T x );

        /**
         * Returns the gradient for a 2D Perlin noise function given the integer hash and input x and y
         * coordinates.
         *
         * @param hash The integer hash used to calculate the gradient.
         * @param x The input x coordinate used to calculate the gradient.
         * @param y The input y coordinate used to calculate the gradient.
         *
         * @return The gradient for the given parameters.
         */
        static T WPCore_API Grad( int hash, T x, T y );

        /**
         * Returns the gradient for a 3D Perlin noise function given the integer hash and input x, y, and
         * z coordinates.
         *
         * @param hash The integer hash used to calculate the gradient.
         * @param x The input x coordinate used to calculate the gradient.
         * @param y The input y coordinate used to calculate the gradient.
         * @param z The input z coordinate used to calculate the gradient.
         *
         * @return The gradient for the given parameters.
         */
        static T WPCore_API Grad( int hash, T x, T y, T z );

        /**
         * Restricts a value to lie within two boundaries.
         *
         * @param value The value to restrict.
         * @param boundary1 The first boundary.
         * @param boundary2 The second boundary.
         *
         * @return The restricted value.
         */
        static T WPCore_API bound( T value, T boundary1, T boundary2 );

        /**
         * @brief Smoothly interpolate between the current angle and the target angle while applying
         * damping.
         *
         * This function uses a smooth damping algorithm to interpolate between the current angle and the
         * target angle, considering the current angular velocity and a specified time step. The angular
         * velocity is updated to achieve a smooth transition from the current angle to the target angle
         * over the specified time period.
         *
         * @param current     The current angle (in degrees).
         * @param target      The target angle (in degrees).
         * @param vel         The current angular velocity (in degrees per second).
         * @param time        The time step for the interpolation (in seconds).
         * @param smoothTime  The desired time to reach the target angle (in seconds).
         * @return The new angle after interpolation.
         */
        static T WPCore_API smoothDampAngle( T current, T target, T vel, T time, T smoothTime );

        /**
         * @brief Smoothly interpolate between the current angle and the target angle while applying
         * damping.
         *
         * This function uses a smooth damping algorithm to interpolate between the current angle and the
         * target angle, considering the current angular velocity and a specified time step. The angular
         * velocity is updated to achieve a smooth transition from the current angle to the target angle
         * over the specified time period.
         *
         * @param current     The current angle (in degrees).
         * @param target      The target angle (in degrees).
         * @param velocity    Reference to the current angular velocity (in degrees per second).
         * @param time        The time step for the interpolation (in seconds).
         * @return The new angle after interpolation.
         */
        static T WPCore_API smoothDampAngle( T current, T target, T &velocity, T time );

        /**
         * Smoothly interpolates a value towards a target using a critically-damped spring
         * approximation. `velocity` is updated and should be preserved between calls.
         * @param current Current value.
         * @param target  Target value.
         * @param velocity Reference to current velocity; updated by the function.
         * @param time    Delta time (seconds).
         * @param smoothTime Approximate time to reach the target.
         */
        static T WPCore_API smoothDamp( T current, T target, T &velocity, T time, T smoothTime );

    private:
        /**
         * The internal seed used for the Math class's random number generator.
         */
        static s32 m_randomSeed;
    };

    using MathI = Math<s32>;
    using MathF = Math<f32>;
    using MathD = Math<f64>;

}  // namespace workphone

#endif
