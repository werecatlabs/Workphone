#ifndef WP_ByteSwap_h__
#define WP_ByteSwap_h__

#include <Workphone/WorkphoneTypes.hpp>

namespace workphone
{
    /**
     * @class Byteswap
     * @brief Utility class for byte order (endianness) conversion of various data types.
     *
     * Provides static methods to swap the byte order of 16-bit, 32-bit, and floating-point values.
     * Useful for converting between little-endian and big-endian representations.
     */
    class WPCore_API Byteswap
    {
    public:
        /**
         * @brief Swaps the byte order of a 16-bit unsigned integer.
         * @param num The 16-bit unsigned integer to swap.
         * @return The value with its byte order reversed.
         */
        static u16 byteswap( u16 num );

        /**
         * @brief Swaps the byte order of a 16-bit signed integer.
         * @param num The 16-bit signed integer to swap.
         * @return The value with its byte order reversed.
         */
        static s16 byteswap( s16 num );

        /**
         * @brief Swaps the byte order of a 32-bit unsigned integer.
         * @param num The 32-bit unsigned integer to swap.
         * @return The value with its byte order reversed.
         */
        static u32 byteswap( u32 num );

        /**
         * @brief Swaps the byte order of a 32-bit signed integer.
         * @param num The 32-bit signed integer to swap.
         * @return The value with its byte order reversed.
         */
        static s32 byteswap( s32 num );

        /**
         * @brief Swaps the byte order of a 32-bit floating point value.
         * @param num The 32-bit float to swap.
         * @return The value with its byte order reversed.
         */
        static f32 byteswap( f32 num );

        /**
         * @brief No-op overload for 8-bit unsigned integers.
         *
         * Swapping bytes for 8-bit values is unnecessary, so this function simply returns the input
         * value unchanged.
         * @param num The 8-bit unsigned integer.
         * @return The input value, unchanged.
         */
        static u8 byteswap( u8 num );

        /**
         * @brief No-op overload for 8-bit character values.
         *
         * Swapping bytes for 8-bit character values is unnecessary, so this function simply returns the
         * input value unchanged.
         * @param num The 8-bit character value.
         * @return The input value, unchanged.
         */
        static c8 byteswap( c8 num );
    };
}  // namespace workphone

#endif  // ByteSwap_h__
