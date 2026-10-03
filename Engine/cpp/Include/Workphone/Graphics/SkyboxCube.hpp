#ifndef SkyboxCube_h__
#define SkyboxCube_h__

#include <Workphone/Interface/Graphics/ISkyboxCube.hpp>
#include <Workphone/Graphics/Sky.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Skybox implementation using a cube representation.
         *
         * Derives from the templated `Sky` base class specialized with
         * `ISkyboxCube`. This class provides skybox-specific handling for
         * state messages and state change notifications that renderers can
         * use to update GPU-side resources.
         */
        class WPCore_API SkyboxCube : public Sky<ISkyboxCube>
        {
        public:
            /// Construct a new SkyboxCube instance.
            SkyboxCube();

            /// Virtual destructor.
            ~SkyboxCube() override;

            /**
             * @brief Handle a state message targeted at this skybox.
             *
             * Implementations should inspect the incoming message and perform
             * any necessary updates to the object's state or resources. The
             * function returns true when the message was handled.
             *
             * @param message State message to handle.
             * @return true if handled, false otherwise.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            /**
             * @brief Notification that the object's state has changed.
             *
             * Called after the engine applies changes to the state's data so
             * the skybox can react (for example, recreate GPU resources or
             * update renderer-specific objects).
             *
             * @param state The updated state object for this skybox.
             * @return true if the change was handled, false otherwise.
             */
            bool handleStateChanged( SmartPtr<IState> &state );

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // SkyboxCube_h__
