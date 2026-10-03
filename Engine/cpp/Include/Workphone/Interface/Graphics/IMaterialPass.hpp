#ifndef IMaterialPass_h__
#define IMaterialPass_h__

#include <Workphone/Interface/Graphics/IMaterialNode.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector4.hpp>
#include <Workphone/Core/ColourF.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Interface representing a single material pass.
         *
         * A material pass encapsulates a single rendering step of a material
         * (blending, depth state, textures, shader parameters and basic PBR/lighting
         * properties). Concrete implementations are responsible for applying these
         * settings to the underlying renderer.
         *
         * @note This interface inherits from `IMaterialNode` and is intended to be
         * extended by renderer-specific pass implementations.
         */
        class WPCore_API IMaterialPass : public IMaterialNode
        {
        public:
            /** Hash key used to identify the diffuse fragment parameter. */
            static const hash_type DIFFUSE_HASH;

            /** Hash key used to identify the emission fragment parameter. */
            static const hash_type EMISSION_HASH;

            static const String ambientStr;
            static const String diffuseStr;
            static const String specularStr;
            static const String emissiveStr;
            static const String tintStr;
            static const String metalnessStr;
            static const String roughnessStr;
            static const String lightingEnabledStr;
            static const String transparentStr;
            static const String cutoutStr;
            static const String enableEmissionStr;
            static const String enableRefractionStr;

            /**
             * @brief Virtual destructor.
             *
             * Ensures derived classes are properly destructed through this interface.
             */
            ~IMaterialPass() override;

            /**
             * @brief Set the scene blending mode for this pass.
             *
             * The meaning of `blendType` is implementation-specific (typically an
             * enum value describing source/destination blend factors or preset modes).
             *
             * @param blendType Blend mode identifier.
             */
            virtual void setSceneBlending( u32 blendType ) = 0;

            /**
             * @brief Query whether depth testing is enabled for this pass.
             *
             * When true, fragments are tested against the depth buffer and may be
             * discarded if occluded.
             *
             * @return True if depth test is enabled, false otherwise.
             */
            virtual bool isDepthCheckEnabled() const = 0;

            /**
             * @brief Enable or disable depth testing for this pass.
             *
             * @param enabled True to enable depth testing, false to disable.
             */
            virtual void setDepthCheckEnabled( bool enabled ) = 0;

            /**
             * @brief Query whether depth writes are enabled for this pass.
             *
             * When true, successful fragments will update the depth buffer.
             *
             * @return True if depth writes are enabled, false otherwise.
             */
            virtual bool isDepthWriteEnabled() const = 0;

            /**
             * @brief Enable or disable writing to the depth buffer.
             *
             * @param enabled True to enable depth writes, false to disable.
             */
            virtual void setDepthWriteEnabled( bool enabled ) = 0;

            /**
             * @brief Get the current face culling mode.
             *
             * Value is implementation-specific (commonly an enum for none/front/back).
             *
             * @return The current culling mode identifier.
             */
            virtual u32 getCullingMode() const = 0;

            /**
             * @brief Set the face culling mode for this pass.
             *
             * @param mode Culling mode identifier (implementation-specific).
             */
            virtual void setCullingMode( u32 mode ) = 0;

            /**
             * @brief Enable or disable lighting for this pass.
             *
             * When enabled, lighting calculations (depending on shader/renderer)
             * should affect the final colour produced by this pass.
             *
             * @param enabled True to enable lighting, false to disable.
             */
            virtual void setLightingEnabled( bool enabled ) = 0;

            /**
             * @brief Query whether dynamic lighting is enabled for this pass.
             *
             * @return True if lighting is enabled, false otherwise.
             */
            virtual bool getLightingEnabled() const = 0;

            /**
             * @brief Create and return a new texture unit attached to this pass.
             *
             * The returned `SmartPtr<IMaterialTexture>` is owned/shared according to
             * the project's smart pointer semantics.
             *
             * @return A smart pointer to the newly created texture unit.
             */
            virtual SmartPtr<IMaterialTexture> createTextureUnit() = 0;

            /**
             * @brief Add an existing texture unit to this pass.
             *
             * @param textureUnit Smart pointer to the texture unit to add.
             */
            virtual void addTextureUnit( SmartPtr<IMaterialTexture> textureUnit ) = 0;

            /**
             * @brief Remove a texture unit from this pass.
             *
             * If the texture unit is not associated with this pass no action is taken.
             *
             * @param textureUnit Smart pointer to the texture unit to remove.
             */
            virtual void removeTextureUnit( SmartPtr<IMaterialTexture> textureUnit ) = 0;

            /**
             * @brief Retrieve all texture units attached to this pass.
             *
             * @return An array containing smart pointers to the texture units.
             */
            virtual Array<SmartPtr<IMaterialTexture>> getTextureUnits() const = 0;

            /**
             * @brief Get the number of texture nodes/units attached to this pass.
             *
             * @return The number of texture nodes.
             */
            virtual size_t getNumTexturesNodes() const = 0;

            /**
             * @brief Set a texture object for the specified texture layer.
             *
             * @param texture Smart pointer to the texture object.
             * @param layerIdx Index of the texture layer (default = 0).
             */
            virtual void setTexture( SmartPtr<ITexture> texture, u32 layerIdx = 0 ) = 0;

            /**
             * @brief Load and set a texture from file for the specified layer.
             *
             * @param fileName Path to the texture file.
             * @param layerIdx Index of the texture layer (default = 0).
             */
            virtual void setTexture( const String &fileName, u32 layerIdx = 0 ) = 0;

            /**
             * @brief Load and set a cubic (cube) texture for the specified layer.
             *
             * @param fileName Path to the cube texture file or base name depending on implementation.
             * @param uvw If true, use 3D texture coordinates (u,v,w) for sampling; otherwise use 2D
             * fallback.
             * @param layerIdx Index of the texture layer (default = 0).
             */
            virtual void setCubicTexture( const String &fileName, bool uvw, u32 layerIdx = 0 ) = 0;

            /**
             * @brief Set a scalar fragment (shader) parameter.
             *
             * Typically maps to a uniform/parameter in the fragment shader.
             *
             * @param name Name of the parameter.
             * @param value Scalar value to assign.
             */
            virtual void setFragmentParam( const String &name, f32 value ) = 0;

            /**
             * @brief Set a two-component vector fragment parameter.
             *
             * @param name Name of the parameter.
             * @param value Vector2 value to assign.
             */
            virtual void setFragmentParam( const String &name, const Vector2<real_Num> &value ) = 0;

            /**
             * @brief Set a three-component vector fragment parameter.
             *
             * @param name Name of the parameter.
             * @param value Vector3 value to assign.
             */
            virtual void setFragmentParam( const String &name, const Vector3<real_Num> &value ) = 0;

            /**
             * @brief Set a four-component vector fragment parameter.
             *
             * @param name Name of the parameter.
             * @param value Vector4F value to assign.
             */
            virtual void setFragmentParam( const String &name, const Vector4F &value ) = 0;

            /**
             * @brief Set a colour fragment parameter.
             *
             * @param name Name of the parameter.
             * @param value ColourF value to assign.
             */
            virtual void setFragmentParam( const String &name, const ColourF &value ) = 0;

            /**
             * @brief Get the custom vertex shader program name for this pass.
             *
             * An empty string means the renderer should use its default shader path.
             *
             * @return Vertex shader program resource name.
             */
            virtual String getVertexShaderName() const = 0;

            /**
             * @brief Set the custom vertex shader program name for this pass.
             *
             * @param name Vertex shader program resource name, or empty to clear it.
             */
            virtual void setVertexShaderName( const String &name ) = 0;

            /**
             * @brief Get the custom fragment shader program name for this pass.
             *
             * @return Fragment shader program resource name.
             */
            virtual String getFragmentShaderName() const = 0;

            /**
             * @brief Set the custom fragment shader program name for this pass.
             *
             * @param name Fragment shader program resource name, or empty to clear it.
             */
            virtual void setFragmentShaderName( const String &name ) = 0;

            /**
             * @brief Get the custom geometry shader program name for this pass.
             *
             * @return Geometry shader program resource name.
             */
            virtual String getGeometryShaderName() const = 0;

            /**
             * @brief Set the custom geometry shader program name for this pass.
             *
             * @param name Geometry shader program resource name, or empty to clear it.
             */
            virtual void setGeometryShaderName( const String &name ) = 0;

            /**
             * @brief Get the render technique identifier used by this pass.
             *
             * The technique is represented as a hashed identifier (`hash_type`) and
             * typically corresponds to a named shader/material technique.
             *
             * @return Hash of the render technique.
             */
            virtual hash_type getRenderTechnique() const = 0;

            /**
             * @brief Set the render technique used by this pass.
             *
             * @param renderTechnique Hash identifier of the technique to use.
             */
            virtual void setRenderTechnique( hash_type renderTechnique ) = 0;

            /**
             * @brief Get the ambient colour component for this pass.
             *
             * @return Ambient colour as `ColourF`.
             */
            virtual ColourF getAmbient() const = 0;

            /**
             * @brief Set the ambient colour component for this pass.
             *
             * @param ambient Ambient colour.
             */
            virtual void setAmbient( const ColourF &ambient ) = 0;

            /**
             * @brief Get the diffuse colour component for this pass.
             *
             * @return Diffuse colour as `ColourF`.
             */
            virtual ColourF getDiffuse() const = 0;

            /**
             * @brief Set the diffuse colour component for this pass.
             *
             * @param diffuse Diffuse colour.
             */
            virtual void setDiffuse( const ColourF &diffuse ) = 0;

            /**
             * @brief Get the specular colour component for this pass.
             *
             * @return Specular colour as `ColourF`.
             */
            virtual ColourF getSpecular() const = 0;

            /**
             * @brief Set the specular colour component for this pass.
             *
             * @param specular Specular colour.
             */
            virtual void setSpecular( const ColourF &specular ) = 0;

            /**
             * @brief Get the emissive (self-illumination) colour of this pass.
             *
             * @return Emissive colour as `ColourF`.
             */
            virtual ColourF getEmissive() const = 0;

            /**
             * @brief Set the emissive (self-illumination) colour of this pass.
             *
             * @param emissive Emissive colour.
             */
            virtual void setEmissive( const ColourF &emissive ) = 0;

            /**
             * @brief Get the metalness factor for this pass.
             *
             * Typical PBR material parameter in range [0, 1].
             *
             * @return Metalness value.
             */
            virtual f32 getMetalness() const = 0;

            /**
             * @brief Set the metalness factor for this pass.
             *
             * @param metalness Metalness value, typically within [0, 1].
             */
            virtual void setMetalness( f32 metalness ) = 0;

            /**
             * @brief Get the roughness factor for this pass.
             *
             * Typical PBR material parameter in range [0, 1], where 0 is smooth and 1 is rough.
             *
             * @return Roughness value.
             */
            virtual f32 getRoughness() const = 0;

            /**
             * @brief Set the roughness factor for this pass.
             *
             * @param roughness Roughness value, typically within [0, 1].
             */
            virtual void setRoughness( f32 roughness ) = 0;

            /**
             * @brief Query whether this material pass is considered transparent.
             *
             * @return True if the pass is transparent and requires blending, false otherwise.
             */
            virtual bool isTransparent() const = 0;

            /**
             * @brief Mark this material pass as transparent or opaque.
             *
             * @param transparent True to mark pass as transparent (enable blending), false otherwise.
             */
            virtual void setTransparent( bool transparent ) = 0;

            /**
             * @brief Query whether this pass uses alpha-cutout (alpha-tested) rendering.
             *
             * @return True if cutout (alpha test) is enabled, false otherwise.
             */
            virtual bool isCutout() const = 0;

            /**
             * @brief Enable or disable alpha-cutout (alpha-tested) behaviour.
             *
             * @param cutout True to enable cutout, false to disable.
             */
            virtual void setCutout( bool cutout ) = 0;

            /**
             * @brief Query whether emissive effects are enabled for this pass.
             *
             * @return True if emission is enabled, false otherwise.
             */
            virtual bool isEmissionEnabled() const = 0;

            /**
             * @brief Enable or disable emissive rendering for this pass.
             *
             * @param enabled True to enable emission, false to disable.
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

            virtual u32 getFlags() const = 0;
            virtual void setFlags( u32 flags ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // IMaterialPass_h__
