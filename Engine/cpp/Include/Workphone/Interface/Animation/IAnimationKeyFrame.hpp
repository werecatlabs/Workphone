#ifndef IAnimationKeyFrame_h__
#define IAnimationKeyFrame_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @file IAnimationKeyFrame.hpp
     * @brief Interface for animation key frames.
     *
     * This file declares the interface for a single animation key frame.
     * Implementations store the key frame time and the associated value (value
     * storage and type are provided by concrete subclasses).
     */

    /**
     * @brief Interface representing a single animation key frame.
     *
     * Implementations of this interface represent an individual key frame in
     * an animation sequence. This interface exposes only the time position of
     * the key frame; concrete classes are responsible for storing and applying
     * the actual animated value (for example, transform, scalar, or vector).
     *
     * Note: The interpretation of the time units is determined by the animation
     * system (commonly seconds).
     */
    class WPCore_API IAnimationKeyFrame : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of derived classes through this interface.
         */
        ~IAnimationKeyFrame() override;

        /**
         * @brief Get the time position of this key frame.
         *
         * @return The time position of the key frame. Units and origin (e.g.
         *         zero-based timeline start) depend on the animation system;
         *         typically measured in seconds.
         */
        virtual f32 getTime() const = 0;

        /**
         * @brief Set the time position of this key frame.
         *
         * @param time The new time position for the key frame. The caller is
         *             responsible for ensuring the value is valid within the
         *             animation timeline (e.g. non-negative if required).
         */
        virtual void setTime( f32 time ) = 0;

        /** Macro used for runtime class registration / reflection. */
        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IAnimationKeyFrame_h__
