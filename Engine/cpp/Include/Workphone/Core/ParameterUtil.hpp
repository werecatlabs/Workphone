#ifndef WPParamUtil_h__
#define WPParamUtil_h__

#include <Workphone/Core/Parameter.hpp>
#include <Workphone/Math/AABB2.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{

    /**
     * @class ParameterUtil
     * @brief A utility class that provides helper functions for working with Parameters and common data
     * types.
     *
     * This class contains static methods to convert between Parameters and various data types
     * such as vectors and AABBs. It also provides utilities for parameter type conversion.
     */
    class WPCore_API ParameterUtil
    {
    public:
        /**
         * @brief Creates a parameter list from a 2D vector.
         * @param vec The input 2D vector to convert.
         * @return Parameters containing the vector's x and y components.
         */
        static Parameters setVector2( const Vector2F &vec );

        /**
         * @brief Creates a 2D vector from a parameter list.
         * @param params The input parameters containing vector components.
         * @return Vector2F constructed from the parameter values.
         */
        static Vector2F getVector2( const Parameters &params );

        /**
         * @brief Creates a 3D vector from a parameter list.
         * @param params The input parameters containing vector components.
         * @return Vector3<real_Num> constructed from the parameter values.
         */
        static Vector3<real_Num> getVector3( const Parameters &params );

        /**
         * @brief Creates a 2D axis-aligned bounding box from a parameter list.
         * @param params The input parameters containing AABB components.
         * @return AABB2F constructed from the parameter values.
         */
        static AABB2F getAABB2( const Parameters &params );

        /**
         * @brief Creates a 3D axis-aligned bounding box from a parameter list.
         * @param params The input parameters containing AABB components.
         * @return AABB3F constructed from the parameter values.
         */
        static AABB3F getAABB3( const Parameters &params );

        /**
         * @brief Converts a parameter type enum value to its string representation.
         * @param paramType The parameter type enum value to convert.
         * @return String representation of the parameter type.
         */
        static String getParamTypeAsString( ParameterType paramType );

        /// Overload accepting a numeric parameter type value (for callers that pass u32)
        static String getParamTypeAsString( u32 paramType );

        /**
         * @brief Converts a string representation of a parameter type to its enum value.
         * @param paramTypeStr The string representation of the parameter type.
         * @return The corresponding parameter type enum value.
         */
        static ParameterType getParamTypeFromString( const String &paramTypeStr );
    };
}  // namespace workphone

#endif  // WPParamUtil_h__
