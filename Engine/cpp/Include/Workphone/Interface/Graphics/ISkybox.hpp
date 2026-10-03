#ifndef ISkybox_h__
#define ISkybox_h__

#include <Workphone/Interface/Graphics/ISky.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * Represents a skybox.
         */
        class WPCore_API ISkybox : public ISky
        {
        public:
            /** Virtual destructor. */
            ~ISkybox() override;

            /**
             * Gets the scene manager to use.
             * @return The scene manager.
             */
            SmartPtr<IGraphicsScene> getScene() const override = 0;

            /**
             * Sets the scene manager to use.
             * @param scene The scene manager to use.
             */
            void setScene( SmartPtr<IGraphicsScene> scene ) override = 0;

            /**
             * Gets the visibility of the skybox.
             * @return True if the skybox is visible, false otherwise.
             */
            bool isVisible() const override = 0;

            /**
             * Sets the visibility of the skybox.
             * @param visible Whether or not the skybox should be visible.
             */
            void setVisible( bool visible ) override = 0;

            /**
             * Gets the distance of the skybox.
             * @return The distance of the skybox.
             */
            f32 getDistance() const override = 0;

            /**
             * Sets the distance of the skybox.
             * @param distance The distance of the skybox.
             */
            void setDistance( f32 distance ) override = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // ISkybox_h__
