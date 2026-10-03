#ifndef IPhysicsDebug_h__
#define IPhysicsDebug_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Core/ColourF.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for simple physics debug drawing.
         *
         * Implementations of this interface provide lightweight, transient debug
         * drawing primitives (for example lines) used to visualise physics state
         * such as collision shapes, raycasts, contact points and forces.
         *
         * Implementations are expected to be inexpensive and typically render
         * debug geometry for a single frame (or until the debug renderer is cleared).
         * This interface inherits from ISharedObject and lifetime is managed via
         * the project's shared object semantics.
         */
        class WPCore_API IPhysicsDebug : public ISharedObject
        {
        public:
            /** @brief Virtual destructor. */
            ~IPhysicsDebug() override;

            /**
             * @brief Draw a line between two points in world space.
             *
             * Draws a single line from @p start to @p end using the supplied @p color.
             * This method is intended for transient debug visualization (one-frame).
             *
             * Implementations should not store pointers or references to the provided
             * Vector3 or ColourF objects; the values should be copied if needed.
             *
             * @note Thread-safety: callers may invoke this from the physics update
             *       thread, but concrete implementations may not be thread-safe.
             *       If necessary, forward the call to the render thread or use a
             *       thread-safe queue inside the implementation.
             *
             * @param[in] start   The start position of the line in world-space units.
             * @param[in] end     The end position of the line in world-space units.
             * @param[in] color   The colour of the line. Colour component values are
             *                    expected to be in the [0,1] range for RGBA channels.
             */
            virtual void drawLine( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                                   const ColourF &color ) = 0;

            /** Macro used for runtime class registration. */
            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif  // IPhysicsDebug_h__
