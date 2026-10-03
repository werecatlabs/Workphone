#ifndef workphone_util_h__
#define workphone_util_h__

#include "workphone_prerequisites.h"

/**
 * @file workphone_util.h
 * @brief General utility functions for memory management, string operations, and hashing.
 */

/**
 * @brief Allocates a block of memory of the specified size.
 * @param size The number of bytes to allocate.
 * @return Pointer to the allocated memory, or NULL if allocation fails.
 */
WORKPHONE_API void *wp_mem_alloc( wp_u32 size );

/**
 * @brief Frees a block of memory previously allocated by wp_mem_alloc or wp_calloc.
 * @param p Pointer to the memory block to free.
 */
WORKPHONE_API void wp_mem_free( void *p );

/**
 * @brief Copies a specified number of bytes from the source to the destination.
 * @param dest Pointer to the destination array.
 * @param src Pointer to the source of data to be copied.
 * @param n Number of bytes to copy.
 * @return Pointer to the destination array.
 */
WORKPHONE_API void *wp_memcpy( void *dest, const void *src, wp_u32 n );

/**
 * @brief Allocates memory for an array of elements and initializes all bits to zero.
 * @param count Number of elements to allocate.
 * @param size Size of each element in bytes.
 * @return Pointer to the allocated memory, or NULL if allocation fails.
 */
WORKPHONE_API void *wp_calloc( wp_u32 count, wp_u32 size );

/**
 * @brief Computes the length of a string, with a maximum limit.
 * @param s The string to measure.
 * @param maxlen The maximum number of characters to check.
 * @return The length of the string, or maxlen if the string is longer.
 */
WORKPHONE_API wp_u32 wp_strlen_s( const wp_c8 *s, wp_u32 maxlen );

/**
 * @brief Computes the length of a string, checking at most maxlen characters.
 * @param s The string to measure.
 * @param maxlen The maximum number of characters to check.
 * @return The length of the string, or maxlen if no null-terminator was found.
 */
WORKPHONE_API wp_u32 wp_strnlen( const wp_c8 *s, wp_u32 maxlen );

/**
 * @brief Creates a duplicate of a null-terminated string.
 * @param s The string to duplicate.
 * @return Pointer to the newly allocated duplicate string, or NULL if allocation fails.
 */
WORKPHONE_API wp_c8 *wp_strdup( const wp_c8 *s );

/**
 * @brief Creates a duplicate of a string with a maximum length.
 * @param s The string to duplicate.
 * @param n The maximum number of characters to duplicate.
 * @return Pointer to the newly allocated duplicate string, or NULL if allocation fails.
 */
WORKPHONE_API wp_c8 *wp_strndup( const wp_c8 *s, wp_u32 n );

/**
 * @brief Checks if the given text matches a regular expression at the current position.
 * @param regexp The regular expression pattern.
 * @param text The text to check.
 * @return Non-zero if there is a match, 0 otherwise.
 */
WORKPHONE_API wp_s32 wp_str_match_here( const wp_c8 *regexp, const wp_c8 *text );

/**
 * @brief Checks if the given text matches a regular expression using wildcards/stars.
 * @param c Control flag for matching behavior.
 * @param regexp The regular expression pattern.
 * @param text The text to check.
 * @return Non-zero if there is a match, 0 otherwise.
 */
WORKPHONE_API wp_s32 wp_str_match_star( wp_s32 c, const wp_c8 *regexp, const wp_c8 *text );

/**
 * @brief Converts a string to a double-precision floating-point number.
 * @param str The string to convert.
 * @param endptr Pointer to a pointer that will be updated to the first character not converted.
 * @return The converted double-precision value.
 */
WORKPHONE_API wp_f64 wp_strtod( const wp_c8 *str, wp_c8 **endptr );

/**
 * @brief Computes a MurmurHash value for the given key.
 * @param key Pointer to the data to hash.
 * @param len Length of the data to hash.
 * @param seed Seed value for the hashing algorithm.
 * @return The calculated hash value.
 */
WORKPHONE_API wp_hash wp_murmur_hash( const void *key, wp_s32 len, wp_hash seed );

#endif  // workphone_util_h__
