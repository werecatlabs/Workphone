#ifndef _VectorUtil_H
#define _VectorUtil_H

#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{

    /**
     * @class VectorUtil
     * @brief Utility class providing functions for vector operations and conversions
     *
     * This class provides a collection of static utility functions for working with vectors,
     * including compression/decompression of 2D vectors and type conversion between different
     * vector types (2D and 3D).
     */
    class WPCore_API VectorUtil
    {
    public:
        /**
         * @brief Union for storing compressed 2D vector data
         *
         * This union allows accessing the compressed vector data either as a single 32-bit value
         * or as an array of two 16-bit values.
         */
        union CompressedVector2
        {
            u32 packed;   ///< The compressed vector as a single 32-bit value
            u16 vals[2];  ///< The compressed vector as two 16-bit values
        };

        /**
         * @brief Compresses a 2D vector into a 32-bit unsigned integer
         * @tparam T The type of the vector components
         * @param vector The input vector to compress
         * @return A 32-bit unsigned integer containing the compressed vector data
         *
         * The compression is done by converting each float component to a half-precision float
         * using BitUtil::floatToHalf.
         */
        template <class T>
        static u32 compressVector2( const Vector2<T> &vector );

        /**
         * @brief Decompresses a 32-bit unsigned integer back into a 2D vector
         * @tparam T The type of the vector components
         * @param value The compressed vector data
         * @return The decompressed Vector2
         */
        template <class T>
        static Vector2<T> decompressVector2( u32 value );

        /**
         * @brief Converts a Vector2D to a Vector2 of specified type
         * @tparam T The target type for the vector components
         * @param vec The input Vector2D
         * @return A Vector2 with components of type T
         */
        template <class T>
        static Vector2<T> toVector2( const Vector2D &vec );

        /**
         * @brief Converts a Vector2F to a Vector2 of specified type
         * @tparam T The target type for the vector components
         * @param vec The input Vector2F
         * @return A Vector2 with components of type T
         */
        template <class T>
        static Vector2<T> toVector2( const Vector2F &vec );

        /**
         * @brief Converts a Vector3D to a Vector3 of specified type
         * @tparam T The target type for the vector components
         * @param vec The input Vector3D
         * @return A Vector3 with components of type T
         */
        template <class T>
        static Vector3<T> toVector3( const Vector3D &vec );

        /**
         * @brief Converts a Vector3F to a Vector3 of specified type
         * @tparam T The target type for the vector components
         * @param vec The input Vector3F
         * @return A Vector3 with components of type T
         */
        template <class T>
        static Vector3<T> toVector3( const Vector3F &vec );
    };

    template <class T>
    u32 VectorUtil::compressVector2( const Vector2<T> &vector )
    {
        CompressedVector2 packed;
        packed.vals[0] = BitUtil::floatToHalf( vector.X() );
        packed.vals[1] = BitUtil::floatToHalf( vector.Y() );
        return packed.packed;
    }

    template <class T>
    Vector2<T> VectorUtil::decompressVector2( u32 value )
    {
        CompressedVector2 packed;
        packed.packed = value;
        T x = static_cast<T>( BitUtil::halfToFloat( packed.vals[0] ) );
        T y = static_cast<T>( BitUtil::halfToFloat( packed.vals[1] ) );
        return Vector2<T>( x, y );
    }

    template <class T>
    Vector2<T> VectorUtil::toVector2( const Vector2D &vec )
    {
        return Vector2<T>( vec.X(), vec.Y() );
    }

    template <class T>
    Vector2<T> VectorUtil::toVector2( const Vector2F &vec )
    {
        return Vector2<T>( vec.X(), vec.Y() );
    }

    template <class T>
    Vector3<T> VectorUtil::toVector3( const Vector3D &vec )
    {
        return Vector3<T>( vec.X(), vec.Y(), vec.Z() );
    }

    template <class T>
    Vector3<T> VectorUtil::toVector3( const Vector3F &vec )
    {
        return Vector3<T>( vec.X(), vec.Y(), vec.Z() );
    }

}  // namespace workphone

#endif
