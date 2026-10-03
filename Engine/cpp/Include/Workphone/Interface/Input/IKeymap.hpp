#ifndef IKeymap_h__
#define IKeymap_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    /**
     * @brief Interface for key mapping between external/native key codes and the engine's internal key
     * codes.
     *
     * Implementations of this interface provide a mapping layer that translates platform- or
     * library-specific key codes (for example from OIS, SDL, Win32, etc.) into the engine's canonical
     * key codes and vice versa.
     *
     * The exact numeric ranges and semantics of the key codes are implementation-defined. Callers should
     * consult a concrete implementation for details on unmapped values (for example whether an unmapped
     * key returns a sentinel value or the original code).
     *
     * @ingroup Input
     */
    class WPCore_API IKeymap : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Allows derived implementations to clean up resources correctly through a base pointer.
         */
        ~IKeymap() override;

        /**
         * @brief Map an external/native key code to the engine's internal key code.
         *
         * Implementations should translate a platform-specific or external input key code into the
         * internal key code used throughout the engine. This is used when receiving raw key events from
         * the OS or an input library and converting them to the engine's canonical representation.
         *
         * @param keycode External/native key code to map.
         * @return The mapped internal key code. Behavior for unmapped inputs is implementation-defined.
         */
        virtual s32 getKeycode( s32 keycode ) const = 0;

        /**
         * @brief Map an internal key code to the external/native key code.
         *
         * This is the inverse mapping of `getKeycode`. It is useful when the engine needs to
         * synthesize or forward events to an external system that expects platform-specific key codes.
         *
         * @param keycode Internal key code to map to an external/native representation.
         * @return The mapped external/native key code. Behavior for unmapped inputs is
         * implementation-defined.
         */
        virtual s32 getExternalKeycode( s32 keycode ) const = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IKeymap_h__
