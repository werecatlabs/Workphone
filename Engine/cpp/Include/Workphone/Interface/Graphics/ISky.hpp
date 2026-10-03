#ifndef __ISky_h__
#define __ISky_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * Represents a sky.
         */
        class WPCore_API ISky : public ISharedObject
        {
        public:
            ISky();

            /** Virtual destructor. */
            ~ISky() override;

            /**
             * @brief Sets a texture for the material.
             * This method sets the texture for the specified layer index to the given texture.
             * @param texture Smart pointer to the texture to set.
             * @param layerIdx The index of the layer to set the texture for.
             */
            virtual void setTexture( SmartPtr<ITexture> texture, u32 layerIdx = 0 ) = 0;

            /**
             * @brief Sets a texture for the material.
             * This method sets the texture for the specified layer index to the file with the given
             * name.
             * @param fileName The name of the file containing the texture to set.
             * @param layerIdx The index of the layer to set the texture for.
             */
            virtual void setTexture( const String &fileName, u32 layerIdx = 0 ) = 0;

            /**
             * @brief Gets an array of textures associated with the material.
             * @return An array of smart pointers to the textures associated with the material.
             */
            virtual Array<SmartPtr<ITexture>> getTextures() const = 0;

            /**
             * @brief Sets an array of textures associated with the material.
             * @param textures An array of smart pointers to the textures to set.
             */
            virtual void setTextures( const Array<SmartPtr<ITexture>> &textures ) = 0;

            /**
             * @brief Gets the name of the texture associated with the given layer index.
             * This method retrieves the name of the texture associated with the given layer index.
             * @param layerIdx The index of the layer to retrieve the texture name for.
             * @return The name of the texture associated with the layer index.
             */
            virtual String getTextureName( u32 layerIdx = 0 ) const = 0;

            /**
             * @brief Gets the texture associated with the given layer index.
             * This method retrieves the texture associated with the given layer index.
             * @param layerIdx The index of the layer to retrieve the texture for.
             * @return Smart pointer to the texture associated with the layer index, or a null pointer if
             * no such texture exists.
             */
            virtual SmartPtr<ITexture> getTexture( u32 layerIdx = 0 ) const = 0;

            /**
             * Sets the material to use for the skybox.
             * @param material The material to use.
             */
            virtual void setMaterial( SmartPtr<IMaterial> material ) = 0;

            /**
             * Gets the material used by the skybox.
             * @return The material used.
             */
            virtual SmartPtr<IMaterial> getMaterial() const = 0;

            /**
             * @brief Gets the scene manager to use.
             * @return The scene manager.
             */
            virtual SmartPtr<IGraphicsScene> getScene() const = 0;

            /**
             * @brief Sets the scene manager to use.
             * @param scene The scene manager to use.
             */
            virtual void setScene( SmartPtr<IGraphicsScene> scene ) = 0;

            /**
             * @brief Gets the visibility of the skybox.
             * @return True if the skybox is visible, false otherwise.
             */
            virtual bool isVisible() const = 0;

            /**
             * @brief Sets the visibility of the skybox.
             * @param visible Whether or not the skybox should be visible.
             */
            virtual void setVisible( bool visible ) = 0;

            /**
             * @brief Gets the distance of the skybox.
             * @return The distance of the skybox.
             */
            virtual f32 getDistance() const = 0;

            /**
             * @brief Sets the distance of the skybox.
             * @param distance The distance of the skybox.
             */
            virtual void setDistance( f32 distance ) = 0;

            /**
             * @brief Handles a state message.
             * @param message The state message to handle.
             * @return True if the message was handled, false otherwise.
             */
            virtual bool handleStateMessage( const SmartPtr<IStateMessage> &message ) = 0;

            /**
             * @brief Handles a state change.
             * @param state The new state.
             * @return True if the state change was handled, false otherwise.
             */
            virtual bool handleStateChanged( SmartPtr<IState> &state ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // __ISky_h__
