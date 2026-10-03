#ifndef _SoundListener3_H
#define _SoundListener3_H

#include <WPAudio/WPAudioPrerequisites.hpp>
#include <Workphone/Interface/Sound/ISoundListener3.hpp>

#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
#        include <x3daudio.h>
#    endif
#endif

namespace workphone
{
    /**
     * @class WPAudioSoundListener
     * @brief 3D audio listener implementation for the built-in Workphone audio backend.
     *
     * WPAudioSoundListener represents a listener in 3D audio space. It encapsulates
     * the listener's position, orientation, and velocity, which are used to calculate
     * spatialized audio effects such as distance attenuation, panning, and Doppler shift.
     *
     * For XAudio2 on Windows, this class maintains a native X3DAUDIO_LISTENER structure
     * that is automatically synchronized with the listener's state, improving performance
     * by avoiding repeated conversions during audio processing.
     *
     * @note The listener uses a right-handed coordinate system by default, but automatically
     *       converts to the platform's native system (e.g., left-handed for XAudio2).
     */
    class WPAudioSoundListener : public ISoundListener3
    {
    public:
        /**
         * @brief Construct a new WPAudioSoundListener.
         *
         * Initializes the listener at the origin with default orientation (facing +Z, up +Y).
         */
        WPAudioSoundListener();

        /**
         * @brief Destroy the WPAudioSoundListener.
         */
        ~WPAudioSoundListener() override;

        /**
         * @brief Set the 3D position of the listener.
         *
         * @param position The new position in world space.
         */
        void setPosition( const Vector3F &position ) override;

        /**
         * @brief Get the current 3D position of the listener.
         *
         * @return The listener's position in world space.
         */
        Vector3F getPosition() const override;

        /**
         * @brief Set the forward direction vector of the listener.
         *
         * This vector determines which direction the listener is facing.
         * It is automatically normalized.
         *
         * @param vForwardVector The forward direction vector.
         */
        void setForwardVector( const Vector3F &vForwardVector ) override;

        /**
         * @brief Get the forward direction vector of the listener.
         *
         * @return The normalized forward direction vector.
         */
        Vector3F getForwardVector() const override;

        /**
         * @brief Set the velocity of the listener.
         *
         * Used for Doppler effect calculations in 3D audio.
         *
         * @param velocity The velocity vector in units per second.
         */
        void setVelocity( const Vector3F &velocity ) override;

        /**
         * @brief Get the current velocity of the listener.
         *
         * @return The velocity vector in units per second.
         */
        Vector3F getVelocity() const override;

        /**
         * @brief Set the up direction vector of the listener.
         *
         * This vector, along with the forward vector, defines the listener's orientation.
         * It is automatically normalized and should be perpendicular to the forward vector
         * for proper orientation.
         *
         * @param vUpVector The up direction vector.
         */
        void setUpVector( const Vector3F &vUpVector );

        /**
         * @brief Get the up direction vector of the listener.
         *
         * @return The normalized up direction vector.
         */
        Vector3F getUpVector() const;

        /**
         * @brief Set both forward and up vectors simultaneously.
         *
         * This is more efficient than calling setForwardVector() and setUpVector()
         * separately, as it updates the native listener structure only once.
         *
         * @param forward The forward direction vector.
         * @param up The up direction vector.
         */
        void setOrientation( const Vector3F &forward, const Vector3F &up );

        /**
         * @brief Get both forward and up vectors simultaneously.
         *
         * @param forward Output parameter for the forward vector.
         * @param up Output parameter for the up vector.
         */
        void getOrientation( Vector3F &forward, Vector3F &up ) const;

#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
        /**
         * @brief Get a const reference to the native X3DAUDIO_LISTENER structure.
         *
         * This provides direct access to the platform-specific listener data for
         * use in X3DAudio calculations, avoiding the need for conversions.
         *
         * @return Const reference to the X3DAUDIO_LISTENER structure.
         */
        const X3DAUDIO_LISTENER &getNativeListener() const;

        /**
         * @brief Get a non-const reference to the native X3DAUDIO_LISTENER structure.
         *
         * @return Non-const reference to the X3DAUDIO_LISTENER structure.
         */
        X3DAUDIO_LISTENER &getNativeListener();
#    endif
#endif

        WP_CLASS_REGISTER_DECL;

    private:
#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
        /**
         * @brief Update the native X3DAUDIO_LISTENER structure.
         *
         * Called internally whenever listener properties change to keep the
         * native structure synchronized.
         */
        void updateNativeListener();

        /// Native X3DAudio listener structure for efficient 3D audio processing
        X3DAUDIO_LISTENER m_nativeListener = {};

        /// Listener cone for directional hearing (typically omnidirectional)
        X3DAUDIO_CONE m_listenerCone = {};
#    endif
#endif

        /// Current position of the listener in world space
        Vector3F m_vCurPosition;

        /// Current velocity of the listener (for Doppler effect)
        Vector3F m_vVelocity;

        /// Forward direction vector (normalized)
        Vector3F m_vForwardVector;

        /// Up direction vector (normalized)
        Vector3F m_vUpVector;
    };

}  // namespace workphone

#endif
