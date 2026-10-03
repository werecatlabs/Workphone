#ifndef Skybox_h__
#define Skybox_h__

#include <Workphone/Interface/Graphics/ISkybox.hpp>
#include <Workphone/Graphics/Sky.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief A renderable skybox object.
         *
         * The Skybox class implements the ISkybox interface and represents a
         * cube/spherical sky background that is rendered at a configurable
         * distance from the camera. It holds a material used for rendering and
         * a reference to the scene manager that owns or updates it.
         *
         * Inherits from SharedGraphicsObject to provide shared ownership semantics
         * required by the engine's graphics object lifecycle.
         */
        class WPCore_API Skybox : public Sky<ISkybox>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes a new Skybox instance with default state. Concrete
             * initialization of rendering resources is performed by the
             * platform-specific implementation (if any) when the object is used.
             */
            Skybox();

            /**
             * @brief Virtual destructor.
             *
             * Releases any resources owned by the Skybox. Override ensures proper
             * cleanup in derived classes and when handled via base pointers.
             */
            ~Skybox() override;

            /**
             * @brief Sets a texture for the material.
             * This method sets the texture for the specified layer index to the given texture.
             * @param texture Smart pointer to the texture to set.
             * @param layerIdx The index of the layer to set the texture for.
             */
            void setTexture( SmartPtr<ITexture> texture, u32 layerIdx = 0 );

            /**
             * @brief Sets a texture for the material.
             * This method sets the texture for the specified layer index to the file with the given
             * name.
             * @param fileName The name of the file containing the texture to set.
             * @param layerIdx The index of the layer to set the texture for.
             */
            void setTexture( const String &fileName, u32 layerIdx = 0 );

            /** Gets an array of textures associated with the material.
             * @return An array of smart pointers to the textures associated with the material.
             */
            Array<SmartPtr<ITexture>> getTextures() const;

            /** Sets an array of textures associated with the material.
             * @param textures An array of smart pointers to the textures to set.
             */
            void setTextures( const Array<SmartPtr<ITexture>> &textures );

            /**
             * @brief Gets the name of the texture associated with the given layer index.
             * This method retrieves the name of the texture associated with the given layer index.
             * @param layerIdx The index of the layer to retrieve the texture name for.
             * @return The name of the texture associated with the layer index.
             */
            String getTextureName( u32 layerIdx = 0 ) const;

            /**
             * @brief Gets the texture associated with the given layer index.
             * This method retrieves the texture associated with the given layer index.
             * @param layerIdx The index of the layer to retrieve the texture for.
             * @return Smart pointer to the texture associated with the layer index, or a null pointer if
             * no such texture exists.
             */
            SmartPtr<ITexture> getTexture( u32 layerIdx = 0 ) const;

            /**
             * @brief Get the scene manager associated with this skybox.
             * @return SmartPtr<IGraphicsScene> Scene manager that this skybox is attached to.
             *
             * The scene manager may be used to obtain scene-wide state required
             * for updating or rendering the skybox.
             */
            SmartPtr<IGraphicsScene> getScene() const override;

            /**
             * @brief Set the scene manager for this skybox.
             * @param sceneManager SmartPtr to the IGraphicsScene that will manage this skybox.
             *
             * The scene manager is not owned by the skybox; a SmartPtr is used to
             * maintain the reference semantics consistent with the engine.
             */
            void setScene( SmartPtr<IGraphicsScene> scene ) override;

            /**
             * @brief Query whether the skybox is visible.
             * @return true if the skybox is currently flagged visible; false otherwise.
             */
            bool isVisible() const override;

            /**
             * @brief Set the visibility flag for the skybox.
             * @param visible If true the skybox will be rendered; if false it will be skipped.
             */
            void setVisible( bool visible ) override;

            /**
             * @brief Get the rendering distance for the skybox.
             * @return f32 Distance (in world units) at which the skybox should be rendered.
             *
             * The distance controls how the skybox is positioned relative to the camera.
             * Specific behavior depends on the renderer (e.g., fixed distance or scaled).
             */
            f32 getDistance() const override;

            /**
             * @brief Set the rendering distance for the skybox.
             * @param distance Distance (in world units) to use when rendering the skybox.
             *
             * A reasonable default is typically provided by the renderer. Values should
             * be positive; negative values may be ignored by implementations.
             */
            void setDistance( f32 distance ) override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // Skybox_h__
