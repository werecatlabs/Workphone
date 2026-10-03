#ifndef CMaterial_h__
#define CMaterial_h__

#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Graphics/ResourceGraphics.hpp>
#include <Workphone/Graphics/MaterialShaderParameters.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class Material
         * @brief Concrete implementation of IMaterial providing comprehensive material support for the
         * graphics engine.
         *
         * The Material class serves as the primary implementation of the IMaterial interface, providing
         * complete material functionality for rendering objects in the graphics engine. It supports
         * multiple rendering techniques, texture management, physically-based rendering (PBR)
         * properties, and various material types including standard, terrain, skybox, and UI materials.
         *
         * Materials in this engine support:
         * - Multiple rendering techniques with automatic scheme-based selection
         * - Multi-layer texture support with configurable scaling and UV coordinates
         * - Physically-based rendering properties (metalness, roughness, diffuse, specular, emissive)
         * - Cubic texture mapping for reflections and skyboxes
         * - Fragment shader parameter customization
         * - Transparency and cutout rendering modes
         * - Node-based material graph systems
         * - State-based loading and resource management
         *
         * The class inherits from ResourceGraphics<IMaterial> which provides resource management,
         * serialization, and state handling capabilities. It uses a state listener pattern for
         * handling loading state changes and resource updates.
         *
         * @see IMaterial
         * @see ResourceGraphics
         * @see IMaterialTechnique
         * @see ITexture
         *
         * @par Thread Safety:
         * This class is designed to be thread-safe when used within the graphics system's
         * render task context. State changes should be performed on the render thread.
         *
         * @par Example Usage:
         * @code
         * // Create a new material
         * auto material = fb::make_ptr<Material>();
         * material->setMaterialType(MaterialType::Standard);
         *
         * // Set PBR properties
         * material->setDiffuse(ColourF(0.7f, 0.3f, 0.1f, 1.0f));
         * material->setMetalness(0.2f);
         * material->setRoughness(0.8f);
         *
         * // Add a diffuse texture
         * material->setTexture("textures/wood_diffuse.jpg", 0);
         *
         * // Create and configure a technique
         * auto technique = material->createTechnique();
         *
         * // Apply to a renderable object
         * renderableObject->setMaterial(material);
         * @endcode
         *
         * @since Engine Version 1.0
         * @author Graphics Team
         */
        class WPCore_API Material : public ResourceGraphics<IMaterial>
        {
        public:
            static const String materialTypeStr;
            static const String materialTypeDataStr;
            /** Property name used to persist the bound data-driven shader id. */
            static const String shaderIDStr;
            /** Property-name prefix for persisted data-driven shader parameters. */
            static const String shaderParamPrefixStr;
            /** Property-name prefix for persisted data-driven texture slot paths. */
            static const String shaderTexturePrefixStr;

            /**
             * @class MaterialStateListener
             * @brief Internal state listener for handling material state changes and messages.
             *
             * This nested class provides state management functionality for the Material class.
             * It handles loading state changes, state messages, and ensures proper resource
             * cleanup during material lifecycle operations.
             *
             * The state listener pattern allows the material to respond to external state
             * changes such as resource loading/unloading, graphics context changes, and
             * system-level events that may affect material resources.
             *
             * @see IStateListener
             * @see Material
             */
            class WPCore_API MaterialStateListener : public IStateListener
            {
            public:
                /**
                 * @brief Default constructor.
                 *
                 * Creates a state listener without an associated material owner.
                 * The owner must be set separately using setOwner().
                 */
                MaterialStateListener();

                /**
                 * @brief Constructor with material owner.
                 *
                 * Creates a state listener associated with the specified material.
                 *
                 * @param material Pointer to the material that owns this listener
                 */
                MaterialStateListener( Material *material );

                /**
                 * @brief Virtual destructor.
                 *
                 * Properly cleans up the state listener and removes any
                 * remaining state context associations.
                 */
                ~MaterialStateListener() override;

                /**
                 * @brief Handles the unload state change.
                 *
                 * Called when the material or its resources need to be unloaded.
                 * This ensures proper cleanup of graphics resources.
                 *
                 * @param data Optional context data for the unload operation
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handles incoming state messages.
                 *
                 * Processes state messages that may affect the material,
                 * such as resource loading notifications or graphics context changes.
                 *
                 * @param message The state message to handle
                 * @return True if the message was handled, false otherwise
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handles state transitions.
                 *
                 * Called when the material's state changes, allowing the listener
                 * to respond to loading state transitions and resource changes.
                 *
                 * @param state Reference to the new state
                 * @return True if the state change was handled successfully
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Gets the material that owns this listener.
                 *
                 * Returns a smart pointer to the material associated with this listener.
                 *
                 * @return Smart pointer to the owner material
                 */
                SmartPtr<Material> getOwner() const;

                /**
                 * @brief Sets the material owner for this listener.
                 *
                 * Associates this listener with the specified material.
                 *
                 * @param owner Smart pointer to the material to associate with
                 */
                void setOwner( SmartPtr<Material> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                /** Weak pointer to the owning material to avoid circular references */
                AtomicWeakPtr<Material> m_owner;
            };

            /**
             * @brief Default constructor.
             *
             * Initializes a new Material instance with default values. The material is created
             * in an unloaded state and requires proper initialization before use.
             *
             * @post Material is created with default properties and no textures assigned
             * @post Material type is set to MaterialType::Standard
             * @post Loading state is set to LoadingState::Unloaded
             */
            Material();

            /**
             * @brief Virtual destructor.
             *
             * Properly cleans up the material resources, including techniques, textures,
             * and state listeners. Ensures all graphics resources are released.
             *
             * @note The destructor will automatically unload the material if it's still loaded
             */
            ~Material() override;

            /**
             * @brief Saves the material to a file.
             *
             * Serializes the complete material definition including techniques, textures,
             * properties, and parameters to the specified file path. The file format
             * is typically JSON or binary depending on the file extension.
             *
             * @param filePath The file path where the material should be saved
             *
             * @throws FileSystemException if the file cannot be written
             * @throws SerializationException if the material data cannot be serialized
             *
             * @par Supported Formats:
             * - .material (JSON format)
             * - .mat (Binary format)
             *
             * @see loadFromFile()
             */
            void saveToFile( const String &filePath ) override;

            /**
             * @brief Loads the material from a file.
             *
             * Deserializes the material definition from the specified file, including
             * all techniques, textures, properties, and shader parameters.
             *
             * @param filePath The file path from which to load the material
             *
             * @throws FileSystemException if the file cannot be read
             * @throws SerializationException if the material data cannot be deserialized
             * @throws ResourceNotFoundException if referenced textures cannot be found
             *
             * @post Material is loaded with all properties from the file
             * @post Loading state is set to LoadingState::Loaded
             *
             * @see saveToFile()
             */
            void loadFromFile( const String &filePath ) override;

            /**
             * @brief Saves the material using its current resource path.
             *
             * Performs a save operation using the material's internally stored file path.
             * This is typically used for auto-save functionality or when updating
             * an existing material resource.
             *
             * @throws FileSystemException if the file cannot be written
             * @throws ResourceException if no file path is set
             *
             * @see saveToFile()
             */
            void save() override;

            /**
             * @brief Loads the material from shared object data.
             *
             * Initializes the material from a shared object containing serialized
             * material data. This is typically used during resource loading from
             * asset databases or network sources.
             *
             * @param data Shared object containing the material data
             *
             * @pre data must contain valid material serialization data
             * @post Material is initialized with properties from the data object
             *
             * @see reload(), unload()
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Reloads the material from shared object data.
             *
             * Updates the material properties by reloading from the provided data.
             * This preserves the current loading state while updating material properties.
             *
             * @param data Shared object containing the updated material data
             *
             * @note This method is typically called when material assets are modified
             * @see load(), unload()
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the material and releases its resources.
             *
             * Releases all textures, techniques, and graphics resources associated
             * with this material. The material enters an unloaded state.
             *
             * @param data Optional data object for unload context
             *
             * @post All graphics resources are released
             * @post Loading state is set to LoadingState::Unloaded
             * @post Techniques and textures are cleared
             *
             * @see load(), reload()
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Sets a texture for the specified layer.
             *
             * Assigns a texture object to the given texture layer index. The texture
             * will be used for rendering operations on this material.
             *
             * @param texture The texture object to assign
             * @param layerIdx The texture layer index (default: 0)
             *
             * @pre texture must be a valid, loaded texture object
             * @pre layerIdx must be within the valid range for the material type
             *
             * @post The texture is assigned to the specified layer
             * @post Material is marked as dirty for re-compilation
             *
             * @see getTexture(), setTexture(const String&, u32)
             */
            void setTexture( SmartPtr<ITexture> texture, u32 layerIdx = 0 ) override;

            /**
             * @brief Sets a texture from a file name for the specified layer.
             *
             * Loads and assigns a texture from the given file path to the specified
             * texture layer. The texture is loaded through the texture manager.
             *
             * @param fileName The file path of the texture to load
             * @param layerIdx The texture layer index (default: 0)
             *
             * @throws ResourceNotFoundException if the texture file cannot be found
             * @throws TextureLoadException if the texture cannot be loaded
             *
             * @post A texture is loaded from the file and assigned to the layer
             * @post Material is marked as dirty for re-compilation
             *
             * @see setTexture(SmartPtr<ITexture>, u32), getTextureName()
             */
            void setTexture( const String &fileName, u32 layerIdx = 0 ) override;

            /**
             * @brief Retrieves all textures assigned to this material.
             *
             * Returns an array containing all texture objects currently assigned
             * to this material across all texture layers.
             *
             * @return Array of texture smart pointers
             *
             * @note The returned array may contain null pointers for unassigned layers
             * @see setTextures(), getTexture()
             */
            Array<SmartPtr<ITexture>> getTextures() const override;

            /**
             * @brief Sets multiple textures for this material.
             *
             * Assigns an array of textures to the material's texture layers.
             * The array index corresponds to the texture layer index.
             *
             * @param textures Array of texture objects to assign
             *
             * @pre Each texture in the array should be valid and loaded
             * @post All textures are assigned to their respective layers
             * @post Material is marked as dirty for re-compilation
             *
             * @see getTextures(), setTexture()
             */
            void setTextures( const Array<SmartPtr<ITexture>> &textures ) override;

            /**
             * @brief Gets the file name of the texture at the specified layer.
             *
             * Returns the original file path of the texture assigned to the given layer.
             * This is useful for serialization and debugging purposes.
             *
             * @param layerIdx The texture layer index (default: 0)
             * @return The file name/path of the texture, or empty string if none assigned
             *
             * @see setTexture(const String&, u32), getTexture()
             */
            String getTextureName( u32 layerIdx = 0 ) const override;

            /**
             * @brief Gets the texture object at the specified layer.
             *
             * Retrieves the texture object assigned to the given texture layer.
             *
             * @param layerIdx The texture layer index (default: 0)
             * @return Smart pointer to the texture object, or nullptr if none assigned
             *
             * @see setTexture(), getTextureName()
             */
            SmartPtr<ITexture> getTexture( u32 layerIdx = 0 ) const override;

            /**
             * @brief Get the cube texture currently assigned to this material.
             * @return Smart pointer to the cube texture or null if none.
             */
            SmartPtr<ITexture> getCubeTexture() const;

            /**
             * @brief Assign a cube texture to this material.
             * @param cubeTexture Smart pointer to a cube texture resource.
             */
            void setCubeTexture( SmartPtr<ITexture> cubeTexture );

            /**
             * @brief Enables or disables lighting for this material.
             *
             * Controls whether lighting calculations are performed when rendering
             * with this material. This affects the specified rendering pass.
             *
             * @param enabled True to enable lighting, false to disable
             * @param passIdx The rendering pass index (-1 for all passes)
             *
             * @post Lighting state is updated for the specified pass(es)
             * @post Material is marked as dirty for re-compilation
             *
             * @see getLightingEnabled()
             */
            void setLightingEnabled( bool enabled, s32 passIdx = -1 ) override;

            /**
             * @brief Checks if lighting is enabled for this material.
             *
             * Returns the lighting state for the specified rendering pass.
             *
             * @param passIdx The rendering pass index (-1 for first pass)
             * @return True if lighting is enabled, false otherwise
             *
             * @see setLightingEnabled()
             */
            bool getLightingEnabled( s32 passIdx = -1 ) const override;

            /**
             * @brief Sets a cubic texture from texture objects.
             *
             * Assigns an array of six textures to form a cubic texture (skybox/cubemap)
             * for the specified layer. The textures should represent the six faces
             * of a cube in the standard order.
             *
             * @param textures Array of 6 texture objects for cube faces
             * @param layerIdx The texture layer index (default: 0)
             *
             * @pre textures array must contain exactly 6 valid texture objects
             * @pre All textures should have the same dimensions and format
             * @post A cubic texture is created and assigned to the layer
             *
             * @see setCubicTexture(const String&, bool, u32)
             */
            void setCubicTexture( const Array<SmartPtr<ITexture>> &textures, u32 layerIdx = 0 ) override;

            /**
             * @brief Sets a cubic texture from a file.
             *
             * Loads a cubic texture from a single file (cubemap format) or
             * generates one from a single texture using the specified UV mapping.
             *
             * @param fileName The file path of the cubic texture
             * @param uvw True to use UVW coordinates, false for UV only
             * @param layerIdx The texture layer index (default: 0)
             *
             * @throws ResourceNotFoundException if the texture file cannot be found
             * @throws TextureLoadException if the texture cannot be loaded as a cubemap
             *
             * @post A cubic texture is loaded and assigned to the layer
             *
             * @see setCubicTexture(Array<SmartPtr<ITexture>>, u32)
             */
            void setCubicTexture( const String &fileName, bool uvw, u32 layerIdx = 0 ) override;

            /**
             * @brief Sets a floating-point parameter for fragment shaders.
             *
             * Assigns a single floating-point value to the named fragment shader parameter.
             * This allows runtime customization of shader behavior.
             *
             * @param name The parameter name as defined in the shader
             * @param value The floating-point value to assign
             *
             * @post The parameter is set for all techniques using fragment shaders
             * @post Material is marked as dirty for parameter updates
             *
             * @see setFragmentParam() overloads for other data types
             */
            virtual void setFragmentParam( const String &name, f32 value );

            /**
             * @brief Sets a 2D vector parameter for fragment shaders.
             *
             * Assigns a 2D vector value to the named fragment shader parameter.
             *
             * @param name The parameter name as defined in the shader
             * @param value The 2D vector value to assign
             *
             * @see setFragmentParam(const String&, f32)
             */
            virtual void setFragmentParam( const String &name, const Vector2<real_Num> &value );

            /**
             * @brief Sets a 3D vector parameter for fragment shaders.
             *
             * Assigns a 3D vector value to the named fragment shader parameter.
             *
             * @param name The parameter name as defined in the shader
             * @param value The 3D vector value to assign
             *
             * @see setFragmentParam(const String&, f32)
             */
            virtual void setFragmentParam( const String &name, const Vector3<real_Num> &value );

            /**
             * @brief Sets a 4D vector parameter for fragment shaders.
             *
             * Assigns a 4D vector value to the named fragment shader parameter.
             *
             * @param name The parameter name as defined in the shader
             * @param value The 4D vector value to assign
             *
             * @see setFragmentParam(const String&, f32)
             */
            virtual void setFragmentParam( const String &name, const Vector4F &value );

            /**
             * @brief Sets a color parameter for fragment shaders.
             *
             * Assigns a color value to the named fragment shader parameter.
             * The color is typically passed as a 4-component vector (RGBA).
             *
             * @param name The parameter name as defined in the shader
             * @param value The color value to assign
             *
             * @see setFragmentParam(const String&, f32)
             */
            virtual void setFragmentParam( const String &name, const ColourF &value );

            /**
             * @brief Creates a new rendering technique for this material.
             *
             * Instantiates a new material technique that can be configured with
             * specific rendering passes, shaders, and render states.
             *
             * @return Smart pointer to the newly created technique
             *
             * @post A new technique is created and added to the material
             * @post The technique is available for scheme-based selection
             *
             * @see removeTechnique(), getTechniques()
             */
            SmartPtr<IMaterialTechnique> createTechnique() override;
            /**
             * @brief Adds an existing technique to the material's technique list.
             *
             * Public wrapper that simply appends the technique to the internal list.
             * For new techniques prefer createTechnique().
             *
             * @param technique The technique to add
             */
            void addTechnique( SmartPtr<IMaterialTechnique> technique );

            /**
             * @brief Removes a specific technique from this material.
             *
             * Removes the specified technique from the material's technique list.
             * The technique resources are properly cleaned up.
             *
             * @param technique The technique to remove
             *
             * @pre technique must be a valid technique belonging to this material
             * @post The technique is removed from the material
             * @post Technique resources are cleaned up
             *
             * @see createTechnique(), removeAllTechniques()
             */
            void removeTechnique( SmartPtr<IMaterialTechnique> technique ) override;

            /**
             * @brief Removes all techniques from this material.
             *
             * Clears all rendering techniques from the material and releases
             * their resources. The material will need new techniques to be renderable.
             *
             * @post All techniques are removed and their resources released
             * @post Material has no rendering techniques
             *
             * @see removeTechnique(), createTechnique()
             */
            void removeAllTechniques() override;

            /**
             * @brief Gets a technique by its index.
             *
             * Retrieves the technique at the specified index from the material's
             * list of techniques.
             *
             * @param index The index of the technique to retrieve
             * @return Smart pointer to the technique at the given index
             *
             * @throws OutOfBoundsException if the index is invalid
             *
             * @see createTechnique(), getTechniques()
             */
            SmartPtr<IMaterialTechnique> getTechnique( u32 index ) const override;

            /**
             * @brief Gets all techniques belonging to this material.
             *
             * Returns an array containing all rendering techniques currently
             * associated with this material.
             *
             * @return Array of technique smart pointers
             *
             * @see setTechniques(), createTechnique()
             */
            Array<SmartPtr<IMaterialTechnique>> getTechniques() const override;

            /**
             * @brief Sets the techniques for this material.
             *
             * Replaces all current techniques with the provided array of techniques.
             * This is typically used during material loading or cloning operations.
             *
             * @param techniques Array of techniques to assign to this material
             *
             * @pre Each technique should be valid and properly configured
             * @post All current techniques are replaced with the new ones
             * @post Material can be rendered using the new techniques
             *
             * @see getTechniques(), addTechnique()
             */
            void setTechniques( const Array<SmartPtr<IMaterialTechnique>> &techniques ) override;

            /**
             * @brief Gets a technique by its scheme identifier.
             *
             * Retrieves the first technique that matches the specified scheme.
             * Schemes are used to select appropriate techniques based on rendering
             * context (e.g., "forward", "deferred", "shadow").
             *
             * @param scheme The scheme hash to search for
             * @return Smart pointer to the matching technique, or nullptr if not found
             *
             * @see createTechnique(), getTechniques()
             */
            SmartPtr<IMaterialTechnique> getTechniqueByScheme( hash32 scheme ) const override;

            /**
             * @brief Gets the number of techniques in this material.
             *
             * Returns the count of rendering techniques currently associated
             * with this material.
             *
             * @return The number of techniques
             *
             * @see getTechniques(), createTechnique()
             */
            u32 getNumTechniques() const override;

            /**
             * @brief Sets texture coordinate scaling for a specific texture.
             *
             * Configures the UV coordinate scaling for the specified texture,
             * pass, and technique combination. This affects how textures are
             * mapped onto rendered geometry.
             *
             * @param scale The 3D scaling vector (U, V, W components)
             * @param textureIndex The texture index within the pass (default: 0)
             * @param passIndex The pass index within the technique (default: 0)
             * @param techniqueIndex The technique index (default: 0)
             *
             * @pre The specified indices must be valid for this material
             * @post Texture scaling is updated for the specified texture unit
             *
             * @see getTexture(), setTexture()
             */
            void setScale( const Vector3<real_Num> &scale, u32 textureIndex = 0, u32 passIndex = 0,
                           u32 techniqueIndex = 0 ) override;

            /**
             * @brief Gets the metalness value for physically-based rendering.
             *
             * Returns the metalness property used in PBR calculations.
             * Metalness determines how much the material behaves like a metal
             * (1.0) versus a dielectric (0.0).
             *
             * @return The metalness value (typically 0.0 to 1.0)
             *
             * @see setMetalness(), getRoughness()
             */
            f32 getMetalness() const override;

            /**
             * @brief Sets the metalness value for physically-based rendering.
             *
             * Configures the metalness property for PBR materials.
             * This affects how light interacts with the material surface.
             *
             * @param metalness The metalness value (0.0 = dielectric, 1.0 = metal)
             *
             * @post Metalness property is updated
             * @post Material is marked as dirty for shader parameter updates
             *
             * @see getMetalness(), setRoughness()
             */
            void setMetalness( f32 metalness ) override;

            /**
             * @brief Gets the roughness value for physically-based rendering.
             *
             * Returns the surface roughness property used in PBR calculations.
             * Roughness determines how smooth (0.0) or rough (1.0) the surface appears.
             *
             * @return The roughness value (typically 0.0 to 1.0)
             *
             * @see setRoughness(), getMetalness()
             */
            f32 getRoughness() const override;

            /**
             * @brief Sets the roughness value for physically-based rendering.
             *
             * Configures the surface roughness property for PBR materials.
             * This affects the size and intensity of specular highlights.
             *
             * @param roughness The roughness value (0.0 = smooth, 1.0 = rough)
             *
             * @post Roughness property is updated
             * @post Material is marked as dirty for shader parameter updates
             *
             * @see getRoughness(), setMetalness()
             */
            void setRoughness( f32 roughness ) override;

            /**
             * @brief Gets the diffuse color of the material.
             *
             * Returns the base diffuse color used for lighting calculations.
             * This represents the material's inherent color under diffuse lighting.
             *
             * @return The diffuse color as a ColourF (RGBA float values)
             *
             * @see setDiffuse(), getSpecular()
             */
            ColourF getDiffuse() const override;

            /**
             * @brief Sets the diffuse color of the material.
             *
             * Configures the base diffuse color for lighting calculations.
             * This color is modulated with diffuse textures if present.
             *
             * @param diffuse The diffuse color to set (RGBA float values)
             *
             * @post Diffuse color is updated
             * @post Material is marked as dirty for shader parameter updates
             *
             * @see getDiffuse(), setSpecular()
             */
            void setDiffuse( const ColourF &diffuse ) override;

            /**
             * @brief Gets the specular color of the material.
             *
             * Returns the specular color used for specular highlight calculations.
             * This affects the color and intensity of reflective highlights.
             *
             * @return The specular color as a ColourF (RGBA float values)
             *
             * @see setSpecular(), getDiffuse()
             */
            ColourF getSpecular() const override;

            /**
             * @brief Sets the specular color of the material.
             *
             * Configures the specular color for highlight calculations.
             * This color affects the appearance of reflective surfaces.
             *
             * @param specular The specular color to set (RGBA float values)
             *
             * @post Specular color is updated
             * @post Material is marked as dirty for shader parameter updates
             *
             * @see getSpecular(), setDiffuse()
             */
            void setSpecular( const ColourF &specular ) override;

            /**
             * @brief Gets the emissive color of the material.
             *
             * Returns the emissive color that makes the material appear to glow.
             * Emissive materials contribute light to the scene regardless of lighting.
             *
             * @return The emissive color as a ColourF (RGBA float values)
             *
             * @see setEmissive(), getDiffuse()
             */
            ColourF getEmissive() const override;

            /**
             * @brief Sets the emissive color of the material.
             *
             * Configures the emissive color for self-illuminating effects.
             * This makes the material appear to emit light.
             *
             * @param emissive The emissive color to set (RGBA float values)
             *
             * @post Emissive color is updated
             * @post Material is marked as dirty for shader parameter updates
             *
             * @see getEmissive(), setDiffuse()
             */
            void setEmissive( const ColourF &emissive ) override;

            /**
             * @brief Gets the root node of the material graph.
             *
             * Returns the root node of the node-based material system.
             * This allows access to complex material graphs and procedural materials.
             *
             * @return Smart pointer to the root material node, or nullptr if not using node-based
             * materials
             *
             * @see setRoot()
             */
            SmartPtr<IMaterialNode> getRoot() const override;

            /**
             * @brief Sets the root node of the material graph.
             *
             * Configures the root node for node-based material systems.
             * This enables complex, procedural material definitions.
             *
             * @param root The root material node to set
             *
             * @post The material uses the specified node graph
             * @post Material is marked as dirty for recompilation
             *
             * @see getRoot()
             */
            void setRoot( SmartPtr<IMaterialNode> root ) override;

            /**
             * @brief Gets the renderer type hash for this material.
             *
             * Returns the hash identifier of the specific renderer type
             * this material is optimized for (e.g., Ogre, DirectX, OpenGL).
             *
             * @return The renderer type hash
             *
             * @see setRendererType()
             */
            hash_type getRendererType() const;

            /**
             * @brief Sets the renderer type hash for this material.
             *
             * Configures the material for a specific renderer type.
             * This affects how the material is compiled and optimized.
             *
             * @param rendererType The renderer type hash to set
             *
             * @post Material is configured for the specified renderer
             * @post Material may need recompilation for the new renderer
             *
             * @see getRendererType()
             */
            void setRendererType( hash_type rendererType );

            /**
             * @brief Gets the material type enumeration.
             *
             * Returns the type of this material (Standard, Terrain, Skybox, UI, etc.).
             * The material type determines which shaders and rendering paths are used.
             *
             * @return The MaterialType enumeration value
             *
             * @see setMaterialType(), MaterialType
             */
            MaterialType getMaterialType() const override;

            /**
             * @brief Sets the material type enumeration.
             *
             * Configures the type of this material, which determines the rendering
             * approach and available features. Changing the material type may
             * trigger recompilation and affect available properties.
             *
             * @param materialType The MaterialType to set
             *
             * @post Material type is updated
             * @post Material may be recompiled for the new type
             * @post Available features may change based on the new type
             *
             * @see getMaterialType(), MaterialType
             */
            void setMaterialType( MaterialType materialType ) override;

            /** @copydoc IMaterial::makeDirty */
            void makeDirty() override;

            /** @copydoc IObject::toData */
            SmartPtr<ISharedObject> toData() const override;

            /** @copydoc IObject::fromData */
            void fromData( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IResource::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IResource::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc IObject::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Checks if the material uses transparency.
             *
             * Returns whether this material requires transparency/alpha blending
             * during rendering. Transparent materials are typically rendered
             * in a separate pass after opaque objects.
             *
             * @return True if the material is transparent, false otherwise
             *
             * @see setTransparent(), isCutout()
             */
            bool isTransparent() const override;

            /**
             * @brief Sets the transparency state of the material.
             *
             * Configures whether this material should be rendered with
             * transparency/alpha blending. This affects the rendering order
             * and blending modes used.
             *
             * @param transparent True to enable transparency, false to disable
             *
             * @post Material transparency state is updated
             * @post Rendering order and blending modes may change
             *
             * @see isTransparent(), setCutout()
             */
            void setTransparent( bool transparent ) override;

            /**
             * @brief Checks if the material uses alpha cutout/testing.
             *
             * Returns whether this material uses alpha testing to discard
             * pixels below a certain alpha threshold. This is used for
             * materials like leaves, fences, or other masked objects.
             *
             * @return True if the material uses cutout, false otherwise
             *
             * @see setCutout(), isTransparent()
             */
            bool isCutout() const override;

            /**
             * @brief Sets the alpha cutout state of the material.
             *
             * Configures whether this material should use alpha testing
             * to discard pixels with alpha values below a threshold.
             *
             * @param cutout True to enable alpha cutout, false to disable
             *
             * @post Material cutout state is updated
             * @post Alpha testing settings may be modified
             *
             * @see isCutout(), setTransparent()
             */
            void setCutout( bool cutout ) override;

            /**
             * @brief Checks if emission is enabled for the material.
             * @return True if emission is enabled, false otherwise.
             */
            virtual bool isEmissionEnabled() const;

            /**
             * @brief Sets whether emission is enabled for the material.
             * @param enabled True to enable emission, false to disable.
             */
            virtual void setEmissionEnabled( bool enabled );

            /**
             * @brief Query whether refraction is enabled for this pass.
             * @return True if refraction is enabled, false otherwise.
             */
            virtual bool isRefractionEnabled() const;

            /**
             * @brief Enable or disable refraction for this pass.
             * @param enabled True to enable refraction, false to disable.
             */
            virtual void setRefractionEnabled( bool enabled );

            /** @brief Sets a float parameter for the editor. */
            void setEditorFloat( const String &name, f32 value ) override;

            /** @brief Gets a float parameter from the editor. */
            f32 getEditorFloat( const String &name, f32 defaultValue ) const override;

            /** @brief Sets an unsigned integer parameter for the editor. */
            void setEditorUInt( const String &name, u32 value ) override;

            /** @brief Gets an unsigned integer parameter from the editor. */
            u32 getEditorUInt( const String &name, u32 defaultValue ) const override;

            /** @brief Sets a boolean parameter for the editor. */
            void setEditorBool( const String &name, bool value ) override;

            /** @brief Gets a boolean parameter from the editor. */
            bool getEditorBool( const String &name, bool defaultValue ) const override;

            /** @brief Sets a string parameter for the editor. */
            void setEditorString( const String &name, const String &value ) override;

            /** @brief Gets a string parameter from the editor. */
            String getEditorString( const String &name, const String &defaultValue ) const override;

            /** @brief Sets the rendering mode for the material. */
            void setRenderMode( u32 mode ) override;

            /** @brief Gets the current rendering mode of the material. */
            u32 getRenderMode() const override;

            /** @brief Sets the PBR workflow (e.g., Metallic/Roughness or Specular/Glossiness). */
            void setWorkflow( u32 workflow ) override;

            /** @brief Gets the current PBR workflow. */
            u32 getWorkflow() const override;

            /** @brief Sets the specular amount for the material. */
            void setSpecularAmount( f32 value ) override;

            /** @brief Gets the specular amount of the material. */
            f32 getSpecularAmount() const override;

            /** @brief Sets the strength of the normal map. */
            void setNormalStrength( f32 value ) override;

            /** @brief Gets the strength of the normal map. */
            f32 getNormalStrength() const override;

            /** @brief Sets the strength of the detail normal map. */
            void setDetailNormalStrength( f32 value ) override;

            /** @brief Gets the strength of the detail normal map. */
            f32 getDetailNormalStrength() const override;

            /** @brief Sets the alpha clipping threshold. */
            void setAlphaClip( f32 value ) override;

            /** @brief Gets the alpha clipping threshold. */
            f32 getAlphaClip() const override;

            /** @brief Sets the global opacity of the material. */
            void setOpacity( f32 value ) override;

            /** @brief Gets the global opacity of the material. */
            f32 getOpacity() const override;

            /** @brief Sets the alpha blending mode. */
            void setBlendMode( u32 blendMode ) override;

            /** @brief Gets the current alpha blending mode. */
            u32 getBlendMode() const override;

            /** @brief Sets whether depth writing is enabled. */
            void setDepthWrite( bool enabled ) override;

            /** @brief Gets whether depth writing is enabled. */
            bool getDepthWrite() const override;

            /** @brief Sets the depth test comparison mode. */
            void setDepthTest( u32 mode ) override;

            /** @brief Gets the current depth test comparison mode. */
            u32 getDepthTest() const override;

            /** @brief Sets the face culling mode. */
            void setCullMode( u32 mode ) override;

            /** @brief Gets the current face culling mode. */
            u32 getCullMode() const override;

            /** @brief Sets the UV tiling for the material. */
            void setUVTiling( const Vector2F &tiling ) override;

            /** @brief Gets the current UV tiling. */
            Vector2F getUVTiling() const override;

            /** @brief Sets the UV offset for the material. */
            void setUVOffset( const Vector2F &offset ) override;

            /** @brief Gets the current UV offset. */
            Vector2F getUVOffset() const override;

            /** @brief Sets the UV rotation in radians. */
            void setUVRotation( f32 rotation ) override;

            /** @brief Gets the current UV rotation. */
            f32 getUVRotation() const override;

            /** @brief Sets the texture-coordinate projection mode. */
            void setUVProjection( u32 projection ) override;

            /** @brief Gets the texture-coordinate projection mode. */
            u32 getUVProjection() const override;

            /** @brief Sets the mesh UV channel used by the material. */
            void setUVSet( u32 uvSet ) override;

            /** @brief Gets the mesh UV channel used by the material. */
            u32 getUVSet() const override;

            /** @brief Sets the scale for generated projection coordinates. */
            void setTriplanarScale( f32 scale ) override;

            /** @brief Gets the scale for generated projection coordinates. */
            f32 getTriplanarScale() const override;

            /**
             * @brief Retrieves all cubic textures assigned to this material.
             * @return Array of smart pointers to cubic textures.
             */
            Array<SmartPtr<ITexture>> getCubicTextures() const;

            /**
             * @brief Assigns a set of cubic textures to this material.
             * @param cubicTextures Array of smart pointers to cubic textures.
             */
            void setCubicTextures( const Array<SmartPtr<ITexture>> &cubicTextures );

            /**
             * @brief Handles state messages for the material.
             * @param message The state message to process.
             * @return True if the message was handled, false otherwise.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message );
            /**
             * @brief Handles state transitions for the material.
             * @param state The new state to transition to.
             * @return True if the state change was handled, false otherwise.
             */
            bool handleStateChanged( SmartPtr<IState> &state );

            WP_CLASS_REGISTER_DECL;

            //---------------------------------------------------------------------------------------
            // Data-driven shader parameters (Esoterica integration, step 2)
            //
            // These methods are NOT part of IMaterial and do not change the existing PBR API.
            // A material may optionally reference a named "shader" (a data-driven material
            // shader that publishes a parameter list) and own a MaterialShaderParametersInstance.
            // Legacy PBR materials keep working unchanged; new / non-PBR materials set a
            // shaderID and a parameter list and never touch the PBR setters.
            // syncPBRToShaderParameters() bridges the two worlds by copying the legacy pass
            // values into the data-driven buffer for any named slots the shader declares
            // (e.g. "Albedo", "Metalness", "Roughness").
            //---------------------------------------------------------------------------------------

            /** @brief True when this material references a data-driven shader (non-PBR capable). */
            bool isDataDriven() const
            {
                return !m_shaderID.empty();
            }

            /** @brief The data-driven shader this material is bound to (empty for legacy PBR). */
            const String &getShaderID() const
            {
                return m_shaderID;
            }

            /** @brief Bind this material to a data-driven shader by name. Clears nothing;
             *
             * call setShaderParameters() to (re)build the parameter buffer. */
            void setShaderID( const String &shaderID )
            {
                m_shaderID = shaderID;
                m_shaderParametersDirty = true;
            }

            /** @brief The data-driven parameter values (read/write). Invalid until
             *
             * setShaderParameters() is called. */
            MaterialShaderParametersInstance &getShaderParameters()
            {
                return m_shaderParameters;
            }
            const MaterialShaderParametersInstance &getShaderParameters() const
            {
                return m_shaderParameters;
            }

            /** @brief (Re)build the parameter buffer from a shader's published parameter list
             * and
             * optionally bind the shader by name. */
            void setShaderParameters( const Array<MaterialShaderParameterInfo> &parameterInfo,
                                      const String &shaderID = String() )
            {
                if( !shaderID.empty() )
                {
                    m_shaderID = shaderID;
                }
                m_shaderParameters.Reset( parameterInfo );
                m_shaderParameters.InitializeDefaultResourceHandles();
                m_shaderParametersDirty = true;
            }

            /** @brief Copy legacy PBR pass values into the data-driven buffer for any matching
             * named
             * slots. Idempotent; no-op if not data-driven. */
            void syncPBRToShaderParameters();

            /** @brief Push the data-driven parameter buffer back into the legacy PBR setters the
             *
             * renderer already consumes (inverse of syncPBRToShaderParameters). Call on
             *
             * shader selection and on per-parameter edits so data-driven values render.
             *
             * Does not change the material type. */
            void applyShaderParametersToRenderer();

            /** @brief Route the material to the renderer workflow matching the bound shader id
             *
             * (DefaultPBR -\u003e Standard, Unlit/ColorOnly -\u003e UI). No-op if not data-driven. */
            void routeDataDrivenMaterialType();

            /** @brief Convenience: route the workflow then apply the parameter buffer to the
             *
             * renderer. Call once when the shader is bound. */
            void applyDataDrivenShaderToRenderer();

            /** @brief Set the texture resource path bound to a named data-driven texture slot.
             *  @details Stored separately from the GPU handle so it can be edited in the
             *           editor and persisted; the renderer backend binds it (step 3c). */
            void setShaderTexture( const String &paramName, const String &texturePath );

            /** @brief Get the texture resource path bound to a named data-driven texture slot. */
            String getShaderTexture( const String &paramName ) const;

            /** @brief Apply bound data-driven texture paths to the renderer (best-effort via the
             *         legacy setTexture(path, idx)). Override in a backend to bind to the correct
             *         Hlms Pbs/Unlit semantic slots (step 3c). */
            virtual void applyDataDrivenTextures();

        protected:
            /** @brief Initializes the internal state object for the material. */
            void createStateObject();

            /** Keeps pass flags and persisted editor aliases in sync. */
            void setSurfaceFlags( bool transparent, bool cutout );

            /**
             * @brief Creates and configures the material based on its type.
             *
             * This virtual method is called during material initialization to set up
             * type-specific properties, techniques, and default values based on the
             * material's type (Standard, Terrain, Skybox, etc.).
             *
             * @note This method is typically called internally during load operations
             */
            virtual void createMaterialByType();

            /** Root node of the material graph system */
            AtomicSmartPtr<IMaterialNode> m_root;

            /** Primary technique used for rendering (cached for performance) */
            AtomicSmartPtr<IMaterialTechnique> m_technique;

            /// Optional cube texture assigned to this material.
            AtomicSmartPtr<ITexture> m_cubeTexture;

            /** Hash identifier of the target renderer type */
            hash_type m_rendererType = 0;

            /** Collection of all techniques available for this material */
            ConcurrentArray<SmartPtr<IMaterialTechnique>> m_techniques;

            // Values assigned before the first technique must reach its passes.
            bool m_hasPendingOpacity = false;
            bool m_hasPendingRoughness = false;
            f32 m_pendingOpacity = 1.0f;
            f32 m_pendingRoughness = 0.0f;

            /** Cube texture used for environment mapping or reflections */
            ConcurrentArray<SmartPtr<ITexture>> m_cubicTextures;

            /** Static counter for generating unique material names */
            static u32 m_idExt;

            //--- Data-driven shader parameters (Esoterica integration, step 2) -----------------
            /** Data-driven shader this material is bound to (empty for legacy PBR materials). */
            String m_shaderID;
            /** Self-owned, data-driven parameter storage. See MaterialShaderParameters.hpp. */
            MaterialShaderParametersInstance m_shaderParameters;
            /** True when the data-driven buffer needs re-sync to the renderer. */
            bool m_shaderParametersDirty = false;

            //--- Data-driven texture slots (step 5c) -------------------------------------------
            /** @brief A named texture slot and the resource path bound to it. */
            struct ShaderTextureRef
            {
                String paramName;
                String texturePath;
            };
            /** Per-slot texture resource paths for the bound data-driven shader. */
            Array<ShaderTextureRef> m_shaderTextureRefs;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // IMaterial_h__
