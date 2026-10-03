#ifndef __Render_IMaterial_h__
#define __Render_IMaterial_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector4.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render
    {

        enum MaterialPassFlag : u32
        {
            transparentFlag = 1u << 0u,
            cutoutFlag = 1u << 1u,
            emissionEnabledFlag = 1u << 2u,
            refractionEnabledFlag = 1u << 3u,
            doubleSidedFlag = 1u << 4u,
            receiveShadowsFlag = 1u << 5u,
            castShadowsFlag = 1u << 6u,
            generateMipmapsFlag = 1u << 7u,
            srgbFlag = 1u << 8u,
            normalMapFlag = 1u << 9u,
            textureStreamingFlag = 1u << 10u,
            depthWriteFlag = 1u << 11u,
            gpuInstancingFlag = 1u << 12u,
            srpBatcherFlag = 1u << 13u,
            receiveDecalsFlag = 1u << 14u,
            showUvCheckerFlag = 1u << 15u,
            showWireframeFlag = 1u << 16u,
            showTangentsFlag = 1u << 17u,
            showMipLevelsFlag = 1u << 18u,
        };

        /**
         * @brief Interface for a material.
         * This class is an interface for a material, which can be used to render different types of
         * objects such as terrain, skyboxes, and UI elements. This interface extends the IResource
         * interface.
         */
        class WPCore_API IMaterial : public IResource
        {
        public:
            /**
             * @brief Hash value for set texture.
             */
            static const hash_type SET_TEXTURE_HASH;

            /**
             * @brief Hash value for float fragment.
             */
            static const hash_type FRAGMENT_FLOAT_HASH;

            /**
             * @brief Hash value for Vector2f fragment.
             */
            static const hash_type FRAGMENT_VECTOR2F_HASH;

            /**
             * @brief Hash value for Vector3f fragment.
             */
            static const hash_type FRAGMENT_VECTOR3F_HASH;

            /**
             * @brief Hash value for Vector4f fragment.
             */
            static const hash_type FRAGMENT_VECTOR4F_HASH;

            /**
             * @brief Hash value for colour fragment.
             */
            static const hash_type FRAGMENT_COLOUR_HASH;

            /**
             * @brief Hash value for lighting enabled.
             */
            static const hash_type LIGHTING_ENABLED_HASH;

            /**< Whether the pass is transparent (affects blending/sorting). */
            static const u32 transparentFlag;

            /**< Whether alpha cutout/alpha test is enabled. */
            static const u32 cutoutFlag;

            /** @brief Whether lighting is enabled. */
            static const u32 lightingEnabledFlag;

            /** @brief Whether depth writing is enabled. */
            static const u32 depthWriteEnabledFlag;

            /** @brief Whether depth checking is enabled. */
            static const u32 depthCheckEnabledFlag;

            /** @brief Whether emission is enabled. */
            static const u32 enableEmissionFlag;

            /** @brief Whether refraction is enabled. */
            static const u32 enableRefractionFlag;

            /**
             * @brief Material type string.
             */
            static const String materialTypeStr;

            IMaterial();

            IMaterial( u32 poolTypeId ) : IResource( poolTypeId )
            {
            }

            /**
             * @brief Virtual destructor for IMaterial.
             * This virtual destructor is responsible for deallocating memory for derived classes.
             */
            ~IMaterial() override;

            /**
             * @brief Gets the root node of the material.
             * This method retrieves the root node of the material, which can be used to access the
             * material tree structure.
             * @return Smart pointer to the root node.
             */
            virtual SmartPtr<IMaterialNode> getRoot() const = 0;

            /**
             * @brief Sets the root node of the material.
             * This method sets the root node of the material to the given value.
             * @param root The new root node.
             */
            virtual void setRoot( SmartPtr<IMaterialNode> root ) = 0;

            /**
             * @brief Adds a material technique.
             * This method creates and adds a new material technique to the material. A material
             * technique is responsible for setting up and executing a specific rendering technique for
             * the material.
             * @return Smart pointer to the newly created material technique.
             */
            virtual SmartPtr<IMaterialTechnique> createTechnique() = 0;

            /**
             * @brief Removes a material technique.
             * This method removes the given material technique from the material.
             * @param technique The material technique to remove.
             */
            virtual void removeTechnique( SmartPtr<IMaterialTechnique> technique ) = 0;

            /**
             * @brief Removes all material techniques.
             * This method removes all material techniques from the material.
             */
            virtual void removeAllTechniques() = 0;

            /**
             * @brief Gets a material technique by index.
             * @param index The index of the material technique to get.
             * @return Smart pointer to the material technique at the given index.
             */
            virtual SmartPtr<IMaterialTechnique> getTechnique( u32 index ) const = 0;

            /**
             * @brief Gets the material techniques.
             * This method retrieves an array of all material techniques associated with the material.
             * @return An array of smart pointers to IMaterialTechnique objects.
             */
            virtual Array<SmartPtr<IMaterialTechnique>> getTechniques() const = 0;

            /**
             * @brief Sets the material techniques.
             * This method sets the material techniques to the given array of smart pointers to
             * IMaterialTechnique objects.
             * @param techniques An array of smart pointers to IMaterialTechnique objects.
             */
            virtual void setTechniques( const Array<SmartPtr<IMaterialTechnique>> &techniques ) = 0;

            /**
             * @brief Gets the material technique associated with the given scheme.
             *
             * This method retrieves the material technique associated with the given scheme. A scheme is
             * an identifier that can be used to specify a particular rendering technique for the
             * material.
             *
             * @param scheme The scheme to look up.
             * @return Smart pointer to the material technique associated with the scheme, or a null
             * pointer if no such technique exists.
             */
            virtual SmartPtr<IMaterialTechnique> getTechniqueByScheme( hash32 scheme ) const = 0;

            /**
             * @brief Gets the number of material techniques.
             * This method retrieves the number of material techniques associated with the material.
             * @return The number of material techniques.
             */
            virtual u32 getNumTechniques() const = 0;

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

            /** Gets an array of textures associated with the material.
             * @return An array of smart pointers to the textures associated with the material.
             */
            virtual Array<SmartPtr<ITexture>> getTextures() const = 0;

            /** Sets an array of textures associated with the material.
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
             * @brief Get the cube texture currently assigned to this material.
             * @return Smart pointer to the cube texture or null if none.
             */
            virtual SmartPtr<ITexture> getCubeTexture() const = 0;

            /**
             * @brief Assign a cube texture to this material.
             * @param cubeTexture Smart pointer to a cube texture resource.
             */
            virtual void setCubeTexture( SmartPtr<ITexture> cubeTexture ) = 0;

            /** Sets if lighting is enabled.
             * @param enabled True if lighting is enabled, false otherwise.
             * @param passIdx The index of the pass to set lighting for.
             */
            virtual void setLightingEnabled( bool enabled, s32 passIdx = -1 ) = 0;

            /** Returns whether or not dynamic lighting is enabled.
             * @param passIdx The index of the pass to check lighting for.
             * @return True if lighting is enabled, false otherwise.
             */
            virtual bool getLightingEnabled( s32 passIdx = -1 ) const = 0;

            /**
             * @brief Sets a cubic texture for the material.
             * This method sets a cubic texture for the specified layer index to the given array of
             * textures. Cubic textures are textures that represent the six faces of a cube, such as for
             * a skybox.
             *
             * @param textures An array of smart pointers to the textures to set.
             * @param layerIdx The index of the layer to set the cubic texture for.
             */
            virtual void setCubicTexture( const Array<SmartPtr<ITexture>> &textures,
                                          u32 layerIdx = 0 ) = 0;

            /**
             * @brief Sets a cubic texture for the material.
             *
             * This method sets a cubic texture for the specified layer index to the file with the given
             * name. Cubic textures are textures that represent the six faces of a cube, such as for a
             * skybox.
             *
             * @param fileName The name of the file containing the texture to set.
             * @param uvw Whether to apply the UVW mapping mode to the texture.
             * @param layerIdx The index of the layer to set the cubic texture for.
             */
            virtual void setCubicTexture( const String &fileName, bool uvw, u32 layerIdx = 0 ) = 0;

            /**
             * @brief Sets the scale of a material texture.
             *
             * This method sets the scale of the texture associated with the given indices to the given
             * value.
             *
             * @param scale The scale to set for the texture.
             * @param textureIndex The index of the texture to set the scale for.
             * @param passIndex The index of the pass to set the scale for.
             * @param techniqueIndex The index of the technique to set the scale for.
             */
            virtual void setScale( const Vector3<real_Num> &scale, u32 textureIndex = 0,
                                   u32 passIndex = 0, u32 techniqueIndex = 0 ) = 0;

            /** Gets the metalness value of the material.
             * @return The metalness value of the material.
             */
            virtual f32 getMetalness() const = 0;

            /** Sets the metalness value of the material.
             * @param metalness The metalness value to set.
             */
            virtual void setMetalness( f32 metalness ) = 0;

            /** Gets the roughness value of the material.
             * @return The roughness value of the material.
             */
            virtual f32 getRoughness() const = 0;

            /** Sets the roughness value of the material.
             * @param roughness The roughness value to set.
             */
            virtual void setRoughness( f32 roughness ) = 0;

            /** Gets the diffuse colour of the material.
             * @return The diffuse colour of the material.
             */
            virtual ColourF getDiffuse() const = 0;

            /* Sets the diffuse colour of the material.
             * @param diffuse The diffuse colour to set.
             */
            virtual void setDiffuse( const ColourF &diffuse ) = 0;

            /** Gets the specular colour of the material.
             * @return The specular colour of the material.
             */
            virtual ColourF getSpecular() const = 0;

            /** Sets the specular colour of the material.
             * @param specular The specular colour to set.
             */
            virtual void setSpecular( const ColourF &specular ) = 0;

            /** Gets the emissive colour of the material.
             * @return The emissive colour of the material.
             */
            virtual ColourF getEmissive() const = 0;

            /** Sets the emissive colour of the material.
             * @param emissive The emissive colour to set.
             */
            virtual void setEmissive( const ColourF &emissive ) = 0;

            /**
             * @brief Gets the material type.s
             * This method retrieves the type of the material.
             * @return The type of the material.
             */
            virtual MaterialType getMaterialType() const = 0;

            /**
             * @brief Sets the material type.
             * This method sets the type of the material to the given value.
             * @param materialType The new type of the material.
             */
            virtual void setMaterialType( MaterialType materialType ) = 0;

            /** Gets whether the material is transparent.
             * @return True if the material is transparent, false otherwise.
             */
            virtual bool isTransparent() const = 0;

            /** Sets whether the material is transparent.
             * @param transparent True if the material is transparent, false otherwise.
             */
            virtual void setTransparent( bool transparent ) = 0;

            /** Gets if the material is cutout.
             * @return True if the material is cutout, false otherwise.
             */
            virtual bool isCutout() const = 0;

            /** Sets if the material is cutout.
             * @param cutout True if the material is cutout, false otherwise.
             */
            virtual void setCutout( bool cutout ) = 0;

            /** Gets if the material is cutout.
             * @return True if the material is cutout, false otherwise.
             */
            virtual bool isEmissionEnabled() const = 0;

            /** Sets if the material is cutout.
             * @param cutout True if the material is cutout, false otherwise.
             */
            virtual void setEmissionEnabled( bool enabled ) = 0;

            /**
             * @brief Query whether refraction is enabled for this pass.
             * @return True if refraction is enabled, false otherwise.
             */
            virtual bool isRefractionEnabled() const = 0;

            /**
             * @brief Enable or disable refraction for this pass.
             * @param enabled True to enable refraction, false to disable.
             */
            virtual void setRefractionEnabled( bool enabled ) = 0;

            /**
             * @brief Store an editor-only floating point property on the material.
             */
            virtual void setEditorFloat( const String &name, f32 value ) = 0;

            /**
             * @brief Read an editor-only floating point property from the material.
             */
            virtual f32 getEditorFloat( const String &name, f32 defaultValue ) const = 0;

            /**
             * @brief Store an editor-only unsigned integer property on the material.
             */
            virtual void setEditorUInt( const String &name, u32 value ) = 0;

            /**
             * @brief Read an editor-only unsigned integer property from the material.
             */
            virtual u32 getEditorUInt( const String &name, u32 defaultValue ) const = 0;

            /**
             * @brief Store an editor-only boolean property on the material.
             */
            virtual void setEditorBool( const String &name, bool value ) = 0;

            /**
             * @brief Read an editor-only boolean property from the material.
             */
            virtual bool getEditorBool( const String &name, bool defaultValue ) const = 0;

            /**
             * @brief Store an editor-only string property on the material.
             */
            virtual void setEditorString( const String &name, const String &value ) = 0;

            /**
             * @brief Read an editor-only string property from the material.
             */
            virtual String getEditorString( const String &name, const String &defaultValue ) const = 0;

            /**
             * @brief Set the high-level render mode used by the material editor.
             */
            virtual void setRenderMode( u32 mode ) = 0;

            /**
             * @brief Get the high-level render mode used by the material editor.
             */
            virtual u32 getRenderMode() const = 0;

            /**
             * @brief Set the material's PBR workflow mode.
             */
            virtual void setWorkflow( u32 workflow ) = 0;

            /**
             * @brief Get the material's PBR workflow mode.
             */
            virtual u32 getWorkflow() const = 0;

            /**
             * @brief Set a scalar specular amount by converting it to a greyscale specular colour.
             */
            virtual void setSpecularAmount( f32 value ) = 0;

            /**
             * @brief Get the scalar specular amount from the current specular colour.
             */
            virtual f32 getSpecularAmount() const = 0;

            /**
             * @brief Set the strength applied to the main normal map.
             */
            virtual void setNormalStrength( f32 value ) = 0;

            /**
             * @brief Get the strength applied to the main normal map.
             */
            virtual f32 getNormalStrength() const = 0;

            /**
             * @brief Set the strength applied to detail normal maps.
             */
            virtual void setDetailNormalStrength( f32 value ) = 0;

            /**
             * @brief Get the strength applied to detail normal maps.
             */
            virtual f32 getDetailNormalStrength() const = 0;

            /**
             * @brief Set the stored alpha clip threshold.
             */
            virtual void setAlphaClip( f32 value ) = 0;

            /**
             * @brief Get the stored alpha clip threshold.
             */
            virtual f32 getAlphaClip() const = 0;

            /**
             * @brief Set the stored opacity and update the diffuse alpha channel.
             */
            virtual void setOpacity( f32 value ) = 0;

            /**
             * @brief Get the stored opacity.
             */
            virtual f32 getOpacity() const = 0;

            /**
             * @brief Set pass scene blending.
             */
            virtual void setBlendMode( u32 blendMode ) = 0;

            /**
             * @brief Get pass scene blending.
             */
            virtual u32 getBlendMode() const = 0;

            /**
             * @brief Set whether depth writes are enabled.
             */
            virtual void setDepthWrite( bool enabled ) = 0;

            /**
             * @brief Query whether depth writes are enabled.
             */
            virtual bool getDepthWrite() const = 0;

            /**
             * @brief Set the depth test mode stored by the material editor.
             */
            virtual void setDepthTest( u32 mode ) = 0;

            /**
             * @brief Get the depth test mode stored by the material editor.
             */
            virtual u32 getDepthTest() const = 0;

            /**
             * @brief Set face culling mode on all passes.
             */
            virtual void setCullMode( u32 mode ) = 0;

            /**
             * @brief Get face culling mode.
             */
            virtual u32 getCullMode() const = 0;

            /**
             * @brief Set the primary texture UV tiling.
             */
            virtual void setUVTiling( const Vector2F &tiling ) = 0;

            /**
             * @brief Get the primary texture UV tiling.
             */
            virtual Vector2F getUVTiling() const = 0;

            /**
             * @brief Set the primary texture UV offset.
             */
            virtual void setUVOffset( const Vector2F &offset ) = 0;

            /**
             * @brief Get the primary texture UV offset.
             */
            virtual Vector2F getUVOffset() const = 0;

            /**
             * @brief Set the primary texture UV rotation in degrees.
             */
            virtual void setUVRotation( f32 rotation ) = 0;

            /**
             * @brief Get the primary texture UV rotation in degrees.
             */
            virtual f32 getUVRotation() const = 0;

            /** Set the texture-coordinate projection mode used by the material. */
            virtual void setUVProjection( u32 projection ) = 0;

            /** Get the texture-coordinate projection mode used by the material. */
            virtual u32 getUVProjection() const = 0;

            /** Select the mesh UV set used when mesh UV projection is active. */
            virtual void setUVSet( u32 uvSet ) = 0;

            /** Get the selected mesh UV set. */
            virtual u32 getUVSet() const = 0;

            /** Set the coordinate scale used by generated projection modes. */
            virtual void setTriplanarScale( f32 scale ) = 0;

            /** Get the coordinate scale used by generated projection modes. */
            virtual f32 getTriplanarScale() const = 0;

            /**
             * @brief Get the cubic textures.
             * @return A vector of cubic textures.
             */
            virtual Array<SmartPtr<ITexture>> getCubicTextures() const = 0;

            /**
             * @brief Set the cubic textures.
             * @param textures A vector of cubic textures.
             */
            virtual void setCubicTextures( const Array<SmartPtr<ITexture>> &cubicTextures ) = 0;

            /**
             * Makes the material dirty.
             * This method marks the material as dirty, which means that it needs to be updated.
             */
            virtual void makeDirty() = 0;

            /**
             * @brief Handles a state message.
             * @param message The state message to handle.
             * @return True if the message was handled, false otherwise.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override = 0;

            /**
             * @brief Handles a state change.
             * @param state The state that has changed.
             * @return True if the state change was handled, false otherwise.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // IMaterial_h__
