#ifndef MathUtil_h__
#define MathUtil_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Math/Math.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Math/Polygon2.hpp>
#include <Workphone/Math/Polygon3.hpp>
#include <Workphone/Math/Ray3.hpp>
#include <Workphone/Math/Sphere3.hpp>
#include <Workphone/Math/Cylinder3.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Deque.hpp>
#include <Workphone/Core/Pair.hpp>
#include <Workphone/Core/Set.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{

    /**
     * @file MathUtil.hpp
     * @brief Utility helpers for common mathematical operations used across the engine.
     *
     * This header provides a templated MathUtil class containing a set of
     * small, commonly used mathematical utilities such as equality comparisons
     * with tolerance, coordinate conversions, simple geometry helpers,
     * interval tests and polygon utilities.
     *
     * Most functions are neutral with respect to numeric type and are templated
     * on T (typically f32 or f64). Implementations that require a fixed
     * numeric type use typedefs at the bottom of this file.
     */

    /**
     * @brief A utility class for performing various mathematical operations.
     *
     * The class contains only static functions and does not hold state.
     * It is intended for lightweight operations used by other engine systems.
     *
     * @tparam T Numeric type for calculations (e.g., f32, f64).
     */
    template <class T>
    class WPCore_API MathUtil
    {
    public:
        /**
         * @brief Compare two scalar values using a default tolerance.
         *
         * Uses an engine-wide epsilon value (Math<T>::epsilon()) to test if
         * the difference between f1 and f2 is negligible.
         *
         * @param f1 First value.
         * @param f2 Second value.
         * @return true if values are approximately equal, false otherwise.
         */
        static bool equals( T f1, T f2 );

        /**
         * @brief Compare two scalar values using an explicit tolerance.
         *
         * @param f1 First value.
         * @param f2 Second value.
         * @param tolerance Maximum allowed absolute difference.
         * @return true if |f1 - f2| <= tolerance, false otherwise.
         */
        static bool equals( T f1, T f2, T tolerance );

        /**
         * @brief Compare two 2D vectors for approximate equality.
         *
         * Component-wise comparison using the engine epsilon tolerance.
         *
         * @param a First vector.
         * @param b Second vector.
         * @return true if vectors are approximately equal component-wise.
         */
        static bool equals( const Vector2<T> &a, const Vector2<T> &b );

        /**
         * @brief Compare two 3D vectors for approximate equality.
         *
         * Component-wise comparison using the engine epsilon tolerance.
         *
         * @param a First vector.
         * @param b Second vector.
         * @return true if vectors are approximately equal component-wise.
         */
        static bool equals( const Vector3<T> &a, const Vector3<T> &b );

        /**
         * @brief Compare two quaternions for approximate equality.
         *
         * Takes numerical error into account; does not attempt to account for
         * quaternion double-cover (q and -q represent same rotation) � callers
         * should normalize quaternions before comparison if necessary.
         *
         * @param a First quaternion.
         * @param b Second quaternion.
         * @return true if quaternions are approximately equal component-wise.
         */
        static bool equals( const Quaternion<T> &a, const Quaternion<T> &b );

        /**
         * @brief Compare two 3D vectors using an explicit tolerance.
         *
         * @param a First vector.
         * @param b Second vector.
         * @param tolerance Maximum allowed component-wise difference.
         * @return true if vectors are approximately equal within tolerance.
         */
        static bool equals( const Vector3<T> &a, const Vector3<T> &b, T tolerance );

        /**
         * @brief Compare two quaternions using an explicit tolerance.
         *
         * @param a First quaternion.
         * @param b Second quaternion.
         * @param tolerance Maximum allowed component-wise difference.
         * @return true if quaternions are approximately equal within tolerance.
         */
        static bool equals( const Quaternion<T> &a, const Quaternion<T> &b, T tolerance );

        /**
         * @brief Check that all components of a 3D vector are finite.
         *
         * @param value Vector to check.
         * @return true if all components are finite (not NaN or Inf).
         */
        static bool isFinite( const Vector3<T> &value );

        /**
         * @brief Check that all components of a quaternion are finite.
         *
         * @param value Quaternion to check.
         * @return true if all components are finite (not NaN or Inf).
         */
        static bool isFinite( const Quaternion<T> &value );

        /**
         * @brief Compute the shortest-arc quaternion rotating one vector to another.
         *
         * Computes a quaternion that rotates the normalized vector `src` to the
         * normalized vector `dest`. If src and dest are nearly opposite, any
         * perpendicular axis is a valid rotation axis; in that case `fallbackAxis`
         * will be used if non-zero, otherwise an axis will be generated.
         *
         * Both src and dest are treated as directions (their magnitudes are ignored).
         *
         * @param src Source direction vector (should be normalized for best results).
         * @param dest Destination direction vector (should be normalized for best results).
         * @param fallbackAxis Axis to use when src and dest are nearly opposite. Use Vector3<T>::ZERO to
         * auto-generate.
         * @return Quaternion representing the rotation from src to dest.
         * @remarks The resulting quaternion is suitable for orientation operations.
         */
        static Quaternion<T> getRotationTo( const Vector3<T> &src, const Vector3<T> &dest,
                                            const Vector3<T> &fallbackAxis = Vector3<T>::ZERO );

        /**
         * @brief Create an orientation quaternion that faces in the given direction.
         *
         * Builds a rotation where the forward vector maps to `vec`. The returned
         * quaternion represents an orientation whose forward axis aligns with `vec`.
         *
         * @param vec Direction vector to face (does not need to be normalized).
         * @return Orientation quaternion that faces `vec`.
         */
        static Quaternion<T> getOrientationFromDirection( const Vector3<T> &vec );

        /**
         * @brief Create an orientation quaternion that faces in the given direction with options.
         *
         * This variant allows specifying a local direction vector (the local forward)
         * and optionally fixing yaw around a provided axis.
         *
         * @param vec World-space target direction.
         * @param localDirectionVector Local forward direction to align with `vec`.
         * @param bYawFixed If true, yaw is held fixed around `yawFixedAxis`.
         * @param yawFixedAxis Axis to use when fixing yaw.
         * @return Orientation quaternion that satisfies the provided constraints.
         */
        static Quaternion<T> getOrientationFromDirection( const Vector3<T> &vec,
                                                          const Vector3<T> &localDirectionVector,
                                                          bool bYawFixed,
                                                          const Vector3<T> &yawFixedAxis );

        /**
         * @brief Reflect a vector about a plane normal.
         *
         * Computes the mirrored vector of `vec` reflected across the plane with
         * normal `normal`. Both vectors do not need to be normalized; normal is
         * assumed to be normalized for correct magnitude of reflection.
         *
         * @param vec Incident vector.
         * @param normal Surface normal (should be normalized).
         * @return Reflected vector.
         */
        static Vector3<T> reflect( const Vector3<T> &vec, const Vector3<T> &normal );

        /**
         * @brief Convert a cartesian coordinate to spherical coordinates.
         *
         * Output format: (radius, polar/inclination, azimuth) � angles are in radians.
         * Polar/inclination is the angle from the positive Z axis, and azimuth is the
         * angle around the Z axis. Callers should verify the engine convention before use.
         *
         * @param cartesian 3D point in cartesian coordinates.
         * @return Vector3 containing spherical coordinates [r, inclination, azimuth].
         */
        static Vector3<T> toSpherical( const Vector3<T> &cartesian );

        /**
         * @brief Convert spherical coordinates back to cartesian coordinates.
         *
         * Expects the same component ordering used by toSpherical: (radius, polar/inclination, azimuth).
         *
         * @param spherical Vector3 containing [r, inclination, azimuth] in radians.
         * @return 3D point in cartesian coordinates.
         */
        static Vector3<T> toCartesian( const Vector3<T> &spherical );

        /**
         * @brief Compute a world-space position from a rotation (spherical-like conversion helper).
         *
         * Given a rotation vector, and axis basis vectors (`up`, `pitchVec`, `yawVec`),
         * return the corresponding position on the unit sphere or directional vector.
         *
         * @param rotation Rotation vector or spherical-like rotation.
         * @param up Up vector used as reference.
         * @param pitchVec Pitch axis basis vector (output or cached basis).
         * @param yawVec Yaw axis basis vector (output or cached basis).
         * @return Position/direction corresponding to the provided rotation.
         */
        static Vector3<T> toPosition( const Vector3<T> &rotation, Vector3<T> &up, Vector3<T> &pitchVec,
                                      Vector3<T> &yawVec );

        /**
         * @brief Compute a rotation from a cartesian direction given basis vectors.
         *
         * Performs the inverse of toPosition given basis vectors for pitch and yaw.
         *
         * @param cartesian Direction or position vector.
         * @param pitchVec Pitch axis basis vector.
         * @param yawVec Yaw axis basis vector.
         * @return Rotation vector (spherical-like) corresponding to `cartesian`.
         */
        static Vector3<T> toRotation( const Vector3<T> &cartesian, const Vector3<T> &pitchVec,
                                      const Vector3<T> &yawVec );

        /**
         * @brief Compute the arithmetic mean of the values in an Array.
         *
         * Behavior for empty containers is implementation-defined; caller should
         * ensure the container is non-empty when calling.
         *
         * @param v Container of numeric values.
         * @return Arithmetic mean of values.
         */
        static T average( const Array<T> &v );

        /**
         * @brief Compute the arithmetic mean of the values in a Deque.
         *
         * @param v Container of numeric values.
         * @return Arithmetic mean of values.
         */
        static T average( const Deque<T> &v );

        /**
         * @brief Compute an approximate great-circle distance between two lat/long points.
         *
         * Uses a haversine-like method to compute the surface distance between two
         * geographic coordinates. Coordinates are expected as (latitude, longitude)
         * in degrees. The result is returned in meters using Earth's mean radius
         * (approx. 6,371,000 m).
         *
         * @param lat_long_1 First point as (latitude, longitude) in degrees.
         * @param lat_long_2 Second point as (latitude, longitude) in degrees.
         * @return Great-circle distance between the two points, in meters.
         */
        static T latlong_distance( const Vector2<T> &lat_long_1, const Vector2<T> &lat_long_2 );

        /**
         * @brief Swap two 3D vectors.
         *
         * Utility wrapper around std::swap for Vector3<T>.
         *
         * @param a First vector (will be swapped).
         * @param b Second vector (will be swapped).
         */
        static void swap( Vector3<T> &a, Vector3<T> &b );

        /**
         * @brief Round the components of a 3D vector to a number of decimal places.
         *
         * @param v Vector to round.
         * @param decimals Number of decimals to keep (default = 10).
         * @return Rounded vector.
         */
        static Vector3<T> round( const Vector3<T> &v, s32 decimals = 10 );

        /**
         * @brief Ensure that Min and Max form a valid AABB (swap components if necessary).
         *
         * After calling, it is guaranteed that Min <= Max component-wise.
         *
         * @param Min Minimum corner (may be mutated).
         * @param Max Maximum corner (may be mutated).
         */
        static void repair( Vector3<T> &Min, Vector3<T> &Max );

        /**
         * @brief Find the lowest value in a range.
         *
         * Generic iterator-based search that writes the lowest found value into `value`.
         *
         * @tparam IT_TYPE Input iterator type.
         * @param begin Begin iterator.
         * @param end End iterator.
         * @param value Output parameter set to the lowest element in the range.
         */
        template <class IT_TYPE>
        static void lowest( IT_TYPE begin, IT_TYPE end, T &value );

        /**
         * @brief Find the highest value in a range.
         *
         * Generic iterator-based search that writes the highest found value into `value`.
         *
         * @tparam IT_TYPE Input iterator type.
         * @param begin Begin iterator.
         * @param end End iterator.
         * @param value Output parameter set to the highest element in the range.
         */
        template <class IT_TYPE>
        static void highest( IT_TYPE begin, IT_TYPE end, T &value );

        /**
         * @brief Build a transform matrix from a plane equation.
         *
         * Converts a plane (Plane3<T>) to a Transform3<T> placing the local origin
         * on the plane and aligning the local Z axis with the plane normal.
         *
         * @param plane Plane equation to convert.
         * @return Transform3 representing the plane's pose.
         */
        static Transform3<T> transformFromPlaneEquation( Plane3<T> plane );

        /**
         * @brief Test intersection between a ray and an axis-aligned bounding box (AABB).
         *
         * @param ray Ray to test.
         * @param box Axis-aligned bounding box to test.
         * @return Pair where first indicates hit, and second is distance along ray to intersection.
         */
        static Pair<bool, T> intersects( const Ray3<T> &ray, const AABB3<T> &box );

        /**
         * @brief Test intersection between a ray and a sphere.
         *
         * @param ray Ray to test.
         * @param sphere Sphere to test.
         * @param discardInside If true, ignore intersections where the ray origin is inside the sphere.
         * @return Pair where first indicates hit, and second is distance along ray to nearest
         * intersection.
         */
        static Pair<bool, T> intersects( const Ray3<T> &ray, const Sphere3<T> &sphere,
                                         bool discardInside = true );

        /**
         * @brief Test intersection between a ray and a cylinder.
         *
         * @param ray Ray to test.
         * @param box Cylinder to test.
         * @return Pair where first indicates hit, and second is distance along ray to intersection.
         */
        static Pair<bool, T> intersects( const Ray3<T> &ray, const Cylinder3<T> &box );

        /**
         * @brief Convert polygon data to a human readable string.
         *
         * Intended for debugging � lists the number of points and each point on a new line.
         *
         * @param polygon Polygon to stringify.
         * @return String describing polygon contents.
         */
        static String printPolygonData( const Polygon2F &polygon );

        /**
         * @brief Weld together points that are close to one another.
         *
         * Removes or merges duplicate vertices within the specified tolerance.
         * Note: parameter name `tollerance` mirrors the existing API (typo preserved).
         *
         * @param points Set of points to weld (may be mutated).
         * @param tollerance Distance threshold below which two points are considered the same.
         */
        static void weldPoints( Set<Vector2<T>> &points, T tollerance );

        /**
         * @brief Create a polygon from an unordered set of points.
         *
         * The returned polygon will contain the same points in insertion order of the Set.
         *
         * @param points Input set of points.
         * @return Constructed polygon containing the points.
         */
        static Polygon2<T> createPolygon( const Set<Vector2<T>> &points );

        /**
         * @brief Create a polygon from an array of points.
         *
         * Points are copied into the new polygon preserving their order in the array.
         *
         * @param points Input array of points.
         * @return Constructed polygon containing the points.
         */
        static Polygon2<T> createPolygon( const Array<Vector2<T>> &points );

        /**
         * @brief Order a set of 2D points around a center by angular order.
         *
         * Points are ordered by angle around `centerPoint`, starting from angle zero
         * and proceeding by increasing angle. The `pointOrdering` parameter is
         * provided for future extension but is currently unused (see implementation).
         *
         * @param polygonPoints Input set of points.
         * @param centerPoint Center point used to compute angles.
         * @param pointOrdering Optional ordering mode (reserved).
         * @return Set of ordered points.
         */
        static Set<Vector2<T>> orderPoints( const Set<Vector2<T>> &polygonPoints,
                                            const Vector2<T> &centerPoint, u8 pointOrdering = 0 );

        /**
         * @brief Order an array of 2D points around a center by angular order.
         *
         * @param polygonPoints Input array of points.
         * @param centerPoint Center point used to compute angles.
         * @param pointOrdering Optional ordering mode (reserved).
         * @return Array of ordered points.
         */
        static Array<Vector2<T>> orderPoints( const Array<Vector2<T>> &polygonPoints,
                                              const Vector2<T> &centerPoint, u8 pointOrdering = 0 );

        /**
         * @brief Project polygon vertices onto an axis and compute min/max projection interval.
         *
         * Useful for separating axis tests (SAT).
         *
         * @param polygon Polygon to project.
         * @param axis Axis vector to project onto (does not need to be normalized).
         * @param min Output minimum projection value.
         * @param max Output maximum projection value.
         */
        static void getPolygonIntervals( const Polygon2<real_Num> &polygon,
                                         const Vector2<real_Num> &axis, f32 &min, f32 &max );

        /**
         * @brief Project 3D polygon vertices onto an axis and compute min/max projection interval.
         *
         * @param polygon Polygon to project.
         * @param axis Axis vector to project onto (does not need to be normalized).
         * @param min Output minimum projection value.
         * @param max Output maximum projection value.
         */
        static void getPolygonIntervals( const Polygon3<real_Num> &polygon,
                                         const Vector3<real_Num> &axis, f32 &min, f32 &max );

        /**
         * @brief Compute distance between two 1D intervals.
         *
         * If intervals overlap, the returned value will be <= 0. If they are
         * separated, a positive distance is returned (distance from A to B).
         *
         * @param minA Minimum of interval A.
         * @param maxA Maximum of interval A.
         * @param minB Minimum of interval B.
         * @param maxB Maximum of interval B.
         * @return Interval separation (<= 0 means overlap).
         */
        static f32 intervalDistance( f32 minA, f32 maxA, f32 minB, f32 maxB );

        /**
         * @brief Safely divide two values, returning the numerator if the denominator is near zero.
         * @param value Numerator.
         * @param divisor Denominator.
         * @return Quotient if divisor is not near zero; otherwise returns value.
         */
        static T safeDivide( T value, T divisor );
    };

    using MathUtilF = MathUtil<f32>;
    using MathUtilD = MathUtil<f64>;

}  // namespace workphone

#endif  // MathUtil_h__
