#ifndef CMaterialPass_h__
#define CMaterialPass_h__

#include <Workphone/Interface/Graphics/IMaterialPass.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>
#include <Workphone/Graphics/MaterialNode.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @brief Concrete implementation of an IMaterialPass.
         *
         * A material pass represents a single rendering pass within a material.
         * It owns and manages texture units, lighting and shading parameters,
         * blending and depth state, and other per-pass properties.
         *
         * This class derives from MaterialNode<IMaterialPass> which provides
         * a shared object lifecycle and node behaviour used by the material
         * system.
         */
        class WPCore_API MaterialPass : public MaterialNode<IMaterialPass>
        {
        public:
            /**
             * @brief Set the state context for this pass.
             *
             * MaterialNode<IMaterialPass>::setStateContext is a no-op so a
             * standalone MaterialPass (with no parent Material) cannot store
             * its state context. Override the chain to actually persist the
             * state context locally so the property accessors
             * (isTransparent, isCutout, isEmissionEnabled, etc.) work.
             */
            void setStateContext( SmartPtr<IStateContext> stateContext );

            SmartPtr<IStateContext> getStateContext() const;

            IStateContext *getStateContextPtr() const;
            static const String nameStr;
            static const String materialNameStr;
            static const String mainTextureStr;
            static const String vertexShaderStr;
            static const String fragmentShaderStr;
            static const String geometryShaderStr;

            /**
             * @brief State listener specific to MaterialPass.
             *
             * Observes node/state messages and applies pass-specific changes
             * (for example responding to a state restore or parameter update).
             */
            class WPCore_API MaterialPassStateListener : public MaterialNodeStateListener
            {
            public:
                MaterialPassStateListener();
                ~MaterialPassStateListener() override;

                /**
                 * @brief Handle an incoming state message.
                 *
                 * @param message The state message to handle.
                 * @return true if the message was handled and should not propagate.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handle a full state change.
                 *
                 * @param state The new state object.
                 * @return true if the change was handled.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;
            };

            /**
             * @brief Construct a new MaterialPass.
             *
             * Initializes default material colours, metalness/roughness and
             * prepares internal containers. Heavy GPU/resource setup is
             * performed lazily by load/reload/setupMaterial.
             */
            MaterialPass();

            /**
             * @brief Virtual destructor.
             *
             * Ensures derived cleanup is called and owned resources are released.
             */
            ~MaterialPass() override;

            /**
             * @brief Load pass data from a serializable shared object.
             *
             * Implementations should restore textures, parameters and internal
             * state from @p data.
             *
             * @param data Serialized pass data (may be null).
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Reload pass data from a serializable shared object.
             *
             * Called when underlying resources need to be re-created (for example
             * after device/context loss). Implementations should re-apply textures,
             * shader parameters and other GPU state.
             *
             * @param data Serialized pass data (may be null).
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload any GPU or runtime resources associated with the pass.
             *
             * Should release references to textures/shaders to free GPU memory.
             *
             * @param data Optional data used during unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IMaterialPass::setSceneBlending
             *
             * @param blendType Scene blending mode identifier (implementation-specific).
             */
            void setSceneBlending( u32 blendType ) override;

            /**
             * @brief Query whether depth testing is enabled for this pass.
             *
             * @return true if depth testing is enabled, false otherwise.
             */
            bool isDepthCheckEnabled() const override;

            /**
             * @brief Enable or disable depth testing for this pass.
             *
             * @param enabled true to enable depth testing, false to disable.
             */
            void setDepthCheckEnabled( bool enabled ) override;

            /**
             * @brief Query whether depth writes are enabled for this pass.
             *
             * @return true if depth writes are enabled, false otherwise.
             */
            bool isDepthWriteEnabled() const override;

            /**
             * @brief Enable or disable depth writes for this pass.
             *
             * @param enabled true to enable depth writes, false to disable.
             */
            void setDepthWriteEnabled( bool enabled ) override;

            /**
             * @brief Get the face culling mode used by this pass.
             *
             * @return culling mode (implementation-specific enumeration value).
             */
            u32 getCullingMode() const override;

            /**
             * @brief Set the face culling mode for this pass.
             *
             * @param mode Culling mode identifier (implementation-specific).
             */
            void setCullingMode( u32 mode ) override;

            /**
             * @brief Enable or disable lighting for this pass.
             *
             * @param enabled true to enable dynamic lighting, false to disable.
             * @param passIdx Optional index to target a specific sub-pass (-1 for default).
             */
            void setLightingEnabled( bool enabled ) override;

            /**
             * @brief Query whether dynamic lighting is enabled.
             *
             * @param passIdx Optional index to query a specific sub-pass (-1 for default).
             * @return true if lighting is enabled, false otherwise.
             */
            bool getLightingEnabled() const override;

            /**
             * @copydoc IMaterialPass::createTextureUnit
             *
             * @return A new texture unit owned by this pass.
             */
            SmartPtr<IMaterialTexture> createTextureUnit() override;

            /**
             * @copydoc IMaterialPass::addTextureUnit
             *
             * Adds an externally created texture unit to this pass. The pass
             * takes ownership via smart pointer semantics.
             *
             * @param textureUnit Texture unit to add.
             */
            void addTextureUnit( SmartPtr<IMaterialTexture> textureUnit ) override;

            /**
             * @copydoc IMaterialPass::removeTextureUnit
             *
             * Removes the provided texture unit from the pass.
             *
             * @param textureUnit Texture unit to remove.
             */
            void removeTextureUnit( SmartPtr<IMaterialTexture> textureUnit ) override;

            /**
             * @brief Retrieve the array of texture units used by this pass.
             *
             * @return Array of smart pointers to IMaterialTexture.
             */
            Array<SmartPtr<IMaterialTexture>> getTextureUnits() const override;

            /**
             * @copydoc IMaterialPass::getNumTexturesNodes
             *
             * @return Number of texture units/nodes attached to this pass.
             */
            size_t getNumTexturesNodes() const override;

            /**
             * @brief Set a texture resource into a specified texture layer.
             *
             * @param texture Texture to assign.
             * @param layerIdx Layer index (default 0).
             */
            void setTexture( SmartPtr<ITexture> texture, u32 layerIdx = 0 ) override;

            /**
             * @copydoc IMaterialPass::setTexture
             *
             * @param fileName Path to texture file to load and assign.
             * @param layerIdx Layer index (default 0).
             */
            void setTexture( const String &fileName, u32 layerIdx = 0 ) override;

            /**
             * @copydoc IMaterialPass::setCubicTexture
             *
             * @param fileName Path to cubemap file (or cube description accepted by the system).
             * @param uvw If true, treat the texture coordinate set differently
             * (implementation-specific).
             * @param layerIdx Layer index (default 0).
             */
            void setCubicTexture( const String &fileName, bool uvw, u32 layerIdx = 0 ) override;

            /**
             * @copydoc IMaterialPass::setFragmentParam
             *
             * Set a single float uniform/parameter for the fragment shader of this pass.
             *
             * @param name Parameter name.
             * @param value Float value.
             */
            void setFragmentParam( const String &name, f32 value ) override;

            /**
             * @copydoc IMaterialPass::setFragmentParam
             *
             * Set a vec2 fragment uniform/parameter.
             *
             * @param name Parameter name.
             * @param value Two-component value.
             */
            void setFragmentParam( const String &name, const Vector2<real_Num> &value ) override;

            /**
             * @copydoc IMaterialPass::setFragmentParam
             *
             * Set a vec3 fragment uniform/parameter.
             *
             * @param name Parameter name.
             * @param value Three-component value.
             */
            void setFragmentParam( const String &name, const Vector3<real_Num> &value ) override;

            /**
             * @copydoc IMaterialPass::setFragmentParam
             *
             * Set a vec4 fragment uniform/parameter.
             *
             * @param name Parameter name.
             * @param value Four-component value.
             */
            void setFragmentParam( const String &name, const Vector4F &value ) override;

            /**
             * @copydoc IMaterialPass::setFragmentParam
             *
             * Set a colour fragment uniform/parameter.
             *
             * @param name Parameter name.
             * @param value Colour value.
             */
            void setFragmentParam( const String &name, const ColourF &value ) override;

            /** @copydoc IMaterialPass::getVertexShaderName */
            String getVertexShaderName() const override;

            /** @copydoc IMaterialPass::setVertexShaderName */
            void setVertexShaderName( const String &name ) override;

            /** @copydoc IMaterialPass::getFragmentShaderName */
            String getFragmentShaderName() const override;

            /** @copydoc IMaterialPass::setFragmentShaderName */
            void setFragmentShaderName( const String &name ) override;

            /** @copydoc IMaterialPass::getGeometryShaderName */
            String getGeometryShaderName() const override;

            /** @copydoc IMaterialPass::setGeometryShaderName */
            void setGeometryShaderName( const String &name ) override;

            /**
             * @copydoc IMaterialPass::getRenderTechnique
             *
             * @return Hash identifier of the render technique used by this pass.
             */
            hash_type getRenderTechnique() const override;

            /**
             * @copydoc IMaterialPass::setRenderTechnique
             *
             * @param renderTechnique Hash identifier of the technique to use.
             */
            void setRenderTechnique( hash_type renderTechnique ) override;

            /**
             * @copydoc IMaterialPass::getAmbient
             *
             * @return Ambient colour for this pass.
             */
            ColourF getAmbient() const override;

            /**
             * @copydoc IMaterialPass::setAmbient
             *
             * @param ambient New ambient colour.
             */
            void setAmbient( const ColourF &ambient ) override;

            /**
             * @copydoc IMaterialPass::getDiffuse
             *
             * @return Diffuse colour for this pass.
             */
            ColourF getDiffuse() const override;

            /**
             * @copydoc IMaterialPass::setDiffuse
             *
             * @param diffuse New diffuse colour.
             */
            void setDiffuse( const ColourF &diffuse ) override;

            /**
             * @copydoc IMaterialPass::getSpecular
             *
             * @return Specular colour for this pass.
             */
            ColourF getSpecular() const override;

            /**
             * @copydoc IMaterialPass::setSpecular
             *
             * @param specular New specular colour.
             */
            void setSpecular( const ColourF &specular ) override;

            /**
             * @copydoc IMaterialPass::getEmissive
             *
             * @return Emissive colour for this pass.
             */
            ColourF getEmissive() const override;

            /**
             * @copydoc IMaterialPass::setEmissive
             *
             * @param emissive New emissive colour.
             */
            void setEmissive( const ColourF &emissive ) override;

            /**
             * @brief Get the tint colour applied to this pass.
             *
             * Tint is typically used as a simple multiply colour applied on top
             * of base colours and textures.
             *
             * @return Current tint colour.
             */
            ColourF getTint() const;

            /**
             * @brief Set the tint colour for this pass.
             *
             * @param tint New tint colour.
             */
            void setTint( const ColourF &tint );

            /**
             * @copydoc IMaterialPass::getMetalness
             *
             * @return Metalness factor in range [0,1].
             */
            f32 getMetalness() const override;

            /**
             * @copydoc IMaterialPass::setMetalness
             *
             * @param metalness Metalness factor in range [0,1].
             */
            void setMetalness( f32 metalness ) override;

            /**
             * @copydoc IMaterialPass::getRoughness
             *
             * @return Roughness factor in range [0,1].
             */
            f32 getRoughness() const override;

            /**
             * @copydoc IMaterialPass::setRoughness
             *
             * @param roughness Roughness factor in range [0,1].
             */
            void setRoughness( f32 roughness ) override;

            /**
             * @copydoc IObject::toData
             *
             * Serialize this pass to a shared object (for persistence or editor).
             *
             * @return Serialized representation.
             */
            SmartPtr<ISharedObject> toData() const override;

            /**
             * @copydoc IObject::fromData
             *
             * Deserialize and restore the pass's state from @p data.
             *
             * @param data Serialized representation to restore from.
             */
            void fromData( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IResource::getProperties
             *
             * @return Properties object describing this pass for editor/inspector.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc IResource::setProperties
             *
             * Replace this pass's properties. Implementations should apply
             * any relevant values (colours, textures, flags) after setting.
             *
             * @param properties New properties to apply.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @copydoc IResource::getChildObjects
             *
             * @return Child objects (texture units, etc.) exposed by this pass.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Check whether this pass is flagged as transparent.
             *
             * @return true if rendered as transparent.
             */
            bool isTransparent() const override;

            /**
             * @brief Mark this pass as transparent or opaque.
             *
             * When transparent, blending and sorting behaviours are expected
             * to be adjusted by the renderer.
             *
             * @param transparent true to mark as transparent.
             */
            void setTransparent( bool transparent ) override;

            /**
             * @brief Check whether this pass uses alpha cutout (alpha testing).
             *
             * @return true if cutout is enabled.
             */
            bool isCutout() const override;

            /**
             * @brief Enable or disable alpha cutout behaviour for this pass.
             *
             * @param cutout true to enable cutout (alpha test) behaviour.
             */
            void setCutout( bool cutout ) override;

            /** Gets if the material is cutout.
             * @return True if the material is cutout, false otherwise.
             */
            virtual bool isEmissionEnabled() const;

            /** Sets if the material is cutout.
             * @param cutout True if the material is cutout, false otherwise.
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

            u32 getFlags() const;
            void setFlags( u32 flags );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Create texture slots for this pass.
             *
             * Called during initialization to ensure the pass has the expected
             * number of texture units/slots. Can be overridden by derived classes.
             */
            virtual void createTextureSlots();

            /**
             * @brief Apply material setup to the renderer.
             *
             * Called after parameters or resources change to (re)configure GPU state,
             * bind textures and update shader constants.
             */
            virtual void setupMaterial();

            /**
             * @brief Replace the current texture units array.
             *
             * @param textures New texture unit array to set.
             */
            void setTextureUnits( const Array<SmartPtr<IMaterialTexture>> &textures );

            /** @name Material colour and PBR parameters
             *  These members store per-pass material properties used by the shader.
             *  They are kept protected to allow derived classes to access/modify them.
             */
            //@{
            ColourF m_tint = ColourF::White;     /**< Multiply tint applied to the pass. */
            ColourF m_ambient = ColourF::White;  /**< Ambient light colour contribution. */
            ColourF m_diffuse = ColourF::White;  /**< Diffuse (base) colour. */
            ColourF m_specular = ColourF::White; /**< Specular highlight colour. */
            ColourF m_emissive = ColourF::Black; /**< Emissive (self-lit) colour. */

            f32 m_metalness = 0.5f; /**< PBR metalness factor [0,1]. */
            f32 m_roughness = 0.5f; /**< PBR roughness factor [0,1]. */

            /**< Thread-safe container of textures. */
            ConcurrentArray<SmartPtr<IMaterialTexture>> m_textures;

            /** Local state context storage.
             *  MaterialNode<T>::setStateContext is a no-op so we keep our own
             *  AtomicSmartPtr<IStateContext> for the lifetime of the pass.
             */
            AtomicSmartPtr<IStateContext> m_localStateContext;
            //@}
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CPass_h__
