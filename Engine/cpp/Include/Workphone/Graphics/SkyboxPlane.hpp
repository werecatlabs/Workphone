#ifndef SkyboxPlane_h__
#define SkyboxPlane_h__

#include <Workphone/Interface/Graphics/ISkyboxPlane.hpp>
#include <Workphone/Graphics/Sky.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Skybox implementation using a planar representation.
         *
         * This class specializes the templated Sky base for a plane-based
         * skybox implementation (commonly used for skydomes, parallax
         * backgrounds, or layered 2D scenes). It provides plane-specific
         * handling for state messages and resource updates required by
         * renderer implementations.
         *
         * Key features for production use across different game genres:
         * - Multi-layer texture support for parallax scrolling backgrounds
         * - Configurable plane orientation and positioning
         * - Distance-based fading and LOD control
         * - Support for both static and animated backgrounds
         * - Compatible with 2D platformers, RPGs, visual novels, and 3D games
         *
         * Usage examples:
         * - 2D Platformer: Multiple scrolling layers at different distances
         * - RPG: Static background art with optional day/night cycles
         * - Visual Novel: Character backgrounds with fade transitions
         * - 3D Game: Distant horizon planes or skydome rendering
         */
        class WPCore_API SkyboxPlane : public Sky<ISkyboxPlane>
        {
        public:
            /// Construct a new SkyboxPlane instance.
            SkyboxPlane();

            /// Virtual destructor.
            ~SkyboxPlane() override;

            /**
             * @brief Handles a state message.
             *
             * Processes incoming state messages specific to planar skybox operations.
             * Can be extended to support custom messages for layer control, animation,
             * or transition effects.
             *
             * @param message The state message to handle.
             * @return True if the message was handled, false otherwise.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            /**
             * @brief Handles a state change.
             *
             * Called when the skybox state changes. This method applies the new
             * state to the renderer through the scene manager, supporting both
             * material-based and texture-based rendering modes.
             *
             * @param state The new state.
             * @return True if the state change was handled, false otherwise.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // SkyboxPlane_h__
