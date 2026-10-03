#ifndef _WP_PropertiesBinarySerializer_H
#define _WP_PropertiesBinarySerializer_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    /**
     * Serializes a Properties hierarchy to the engine's portable binary scene representation.
     *
     * The serializer deliberately knows nothing about actors or component classes. Property names,
     * values, type names, read-only state, attributes, and child groups are stored verbatim so new
     * component types can use the format without changing this code.
     */
    class WPCore_API PropertiesBinarySerializer
    {
    public:
        static constexpr u16 formatVersion = 1;

        /** Returns a complete binary document, including its magic and versioned header. */
        static Array<u8> serialize( const Properties &properties );

        /**
         * Parses a complete binary document.
         *
         * The destination is only changed after the entire document has passed validation.
         */
        static bool deserialize( const Array<u8> &bytes, Properties &properties,
                                 String *error = nullptr );

        static bool deserialize( const u8 *bytes, size_Num size, Properties &properties,
                                 String *error = nullptr );

        /** Cheap format probe used before attempting a full parse. */
        static bool hasBinaryHeader( const u8 *bytes, size_Num size );
    };
}  // namespace workphone

#endif  // _WP_PropertiesBinarySerializer_H
