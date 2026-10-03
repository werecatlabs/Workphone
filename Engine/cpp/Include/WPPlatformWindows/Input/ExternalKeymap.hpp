#ifndef ExternalKeymap_h__
#define ExternalKeymap_h__

#include <Workphone/Interface/Input/IKeymap.hpp>

/**
 * @file ExternalKeymap.hpp
 * @brief Keymap adapter for translating between engine and external keycodes.
 */

namespace workphone
{
    /**
     * @brief Adapter that maps between engine-internal keycodes and external
     *        (platform or OS) keycode values.
     *
     * ExternalKeymap provides virtual methods that can be overridden by
     * platform-specific implementations to translate keycodes received from
     * an external source (windowing system, OS) into the engine's internal
     * representation and vice-versa.
     */
    class WPCore_API ExternalKeymap : public IKeymap
    {
    public:
        /**
         * @brief Construct a new ExternalKeymap object.
         *
         * Performs any necessary initialization for the keymap. Subclasses
         * may extend or replace behaviour.
         */
        ExternalKeymap();

        /**
         * @brief Virtual destructor to allow proper cleanup in derived types.
         */
        ~ExternalKeymap() override;

        /**
         * @brief Translate an external/platform keycode to an internal keycode.
         *
         * Implementations should return the corresponding engine keycode for
         * the provided external keycode. If no mapping exists, a sensible
         * fallback (for example a negative value) should be returned.
         *
         * @param keycode External/platform keycode to translate.
         * @return s32 Engine-internal keycode.
         */
        virtual s32 getKeycode( s32 keycode ) const;

        /**
         * @brief Translate an internal engine keycode to an external keycode.
         *
         * Returns the platform/external equivalent of the given internal
         * keycode when a mapping is available.
         *
         * @param keycode Engine-internal keycode to translate.
         * @return s32 External/platform keycode.
         */
        virtual s32 getExternalKeycode( s32 keycode ) const;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // ExternalKeymap_h__
