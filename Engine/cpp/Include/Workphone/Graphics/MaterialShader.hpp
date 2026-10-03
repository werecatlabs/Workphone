#ifndef __WP_MaterialShader_h__
#define __WP_MaterialShader_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector4.hpp>
#include <Workphone/Graphics/MaterialShaderParameters.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @file MaterialShader.hpp
         * @brief Data-driven material shader descriptor and registry.
         *
         * @details Step 3 of the Esoterica material integration. This is the C++ analogue of
         *          Esoterica's MaterialShader concept (Engine/Render/Shaders/EngineShader.h),
         *          adapted to lioncat's OgreNext-backed renderer: it deliberately carries no
         *          RHI. A MaterialShader is a *descriptor* that publishes:
         *            - a stable shaderID (what the editor picks and what Material::setShaderID
         *              refers to),
         *            - a set of flags (alpha test / alpha blend / two sided),
         *            - a list of MaterialShaderParameterInfo describing every parameter slot.
         *
         *          The actual GPU program binding is owned by the renderer backend (OgreNext);
         *          this layer only describes the *data contract* so the editor can introspect
         *          parameters and generate UI, and so a Material can own a
         *          MaterialShaderParametersInstance built from a MaterialShader's parameter list.
         *
         *          PBR is one registered MaterialShader ("DefaultPBR"); Unlit / ColorOnly are
         *          examples of non-PBR workflows. The forward-shading pass can therefore stop
         *          assuming PBR and instead read the material's bound shader + parameter buffer.
         *
         * @see MaterialShaderParameters
         * @see Material
         * @since Data-Driven Materials Integration Step 3
         */

        //---------------------------------------------------------------------------------------
        // Shader flags (ported from Esoterica MaterialShaderFlags)
        //---------------------------------------------------------------------------------------

        /**
         * @enum MaterialShaderFlag
         * @brief Per-shader rendering flags, exposed to the editor as toggles.
         */
        enum class MaterialShaderFlag : u32
        {
            AlphaTest = 1u << 0u,   ///< Alpha-cutout / alpha-test pass.
            AlphaBlend = 1u << 1u,  ///< Alpha-blended (transparent) pass.
            TwoSided = 1u << 2u,    ///< Disable back-face culling.
        };

        //---------------------------------------------------------------------------------------
        // MaterialShader descriptor
        //---------------------------------------------------------------------------------------

        /**
         * @class MaterialShader
         * @brief Descriptor for a data-driven material shader: name + flags + parameter list.
         *
         * @details This is a value type; copyable and stored by value in the registry. A
         *          material shader author builds one of these, registers it, and the editor /
         *          renderer discover it by shaderID. Use buildInstance() to construct a
         *          correctly-sized MaterialShaderParametersInstance for a material bound to
         *          this shader.
         */
        class WPCore_API MaterialShader
        {
        public:
            /** @brief Default constructor; an invalid/empty descriptor until m_shaderID is set. */
            MaterialShader() = default;

            /** @brief Construct a descriptor with an id and flags; parameter info is added via
             * addParameter(). */
            MaterialShader( const String &shaderID, u32 flags = 0 ) :
                m_shaderID( shaderID ),
                m_flags( flags )
            {
            }

            /** @brief Append a typed, named parameter slot to this shader's published list. */
            MaterialShader &addParameter( MaterialShaderParameterType type, const String &name )
            {
                m_parameterInfo.emplace_back( type, name );
                return *this;
            }

            /** @brief True once a shaderID has been assigned. */
            bool isValid() const
            {
                return !m_shaderID.empty();
            }

            /** @brief The stable identifier materials and the editor refer to. */
            const String &getShaderID() const
            {
                return m_shaderID;
            }

            /** @brief Bitmask of MaterialShaderFlag values. */
            u32 getFlags() const
            {
                return m_flags;
            }

            /** @brief Whether a given flag bit is set. */
            bool hasFlag( MaterialShaderFlag flag ) const
            {
                return ( m_flags & static_cast<u32>( flag ) ) != 0;
            }

            /** @brief The shader's published parameter list (read-only). */
            const Array<MaterialShaderParameterInfo> &getParameterInfo() const
            {
                return m_parameterInfo;
            }

            /**
             * @brief Build a parameter instance sized for this shader and pre-fill resource
             *        slots with the invalid-handle sentinel.
             */
            void buildInstance( MaterialShaderParametersInstance &outInstance ) const;

        private:
            String m_shaderID;
            u32 m_flags = 0;
            Array<MaterialShaderParameterInfo> m_parameterInfo;
        };

        //---------------------------------------------------------------------------------------
        // Renderer-facing surface contract (ported concept from Esoterica MaterialShaderPBR.esh)
        //
        // This documents the standard surface output a PBR-style material shader fills for the
        // forward-shading pass. Decoupling the renderer from "PBR" means the pass reads one of
        // these structs instead of hard-coded metalness/roughness fields; non-PBR shaders can
        // supply a simpler output. The OgreNext pass does not consume this yet (step 3b); it is
        // captured here so the data-driven path has a stable target to migrate onto.
        //---------------------------------------------------------------------------------------

        /**
         * @struct MaterialShaderSurfaceOutput
         * @brief The standard surface quantities a material shader produces for lighting.
         */
        struct MaterialShaderSurfaceOutput
        {
            ColourF m_albedo = ColourF( 0.5f, 0.5f, 0.5f, 1.0f );    ///< Base colour.
            f32 m_metalness = 0.0f;                                  ///< [0,1].
            f32 m_roughness = 1.0f;                                  ///< [0,1].
            f32 m_occlusion = 1.0f;                                  ///< [0,1].
            f32 m_opacity = 1.0f;                                    ///< [0,1].
            Vector3F m_normal = Vector3F( 0.0f, 0.0f, 1.0f );        ///< World-space normal.
            ColourF m_emissive = ColourF( 0.0f, 0.0f, 0.0f, 1.0f );  ///< Emissive colour.
        };

        //---------------------------------------------------------------------------------------
        // Registry
        //---------------------------------------------------------------------------------------

        /**
         * @class MaterialShaderRegistry
         * @brief Process-wide registry of available data-driven material shaders.
         *
         * @details The editor queries this to populate the shader picker (step 4); the renderer
         *          and Material look up a descriptor by shaderID. Built-in shaders (DefaultPBR,
         *          Unlit, ColorOnly) are registered lazily and idempotently via
         *          registerBuiltinShaders().
         */
        class WPCore_API MaterialShaderRegistry
        {
        public:
            /** @brief Access the process-wide registry instance. */
            static MaterialShaderRegistry &instance();

            /** @brief Register a material shader descriptor. Re-registering an existing shaderID
             *         replaces the previous entry. */
            void registerShader( const MaterialShader &shader );

            /** @brief Look up a registered shader by id. Returns nullptr if not found. */
            const MaterialShader *find( const String &shaderID ) const;

            /** @brief All registered shaders (for editor shader-picker population). */
            const Array<MaterialShader> &getShaders() const
            {
                return m_shaders;
            }

            /** @brief Lazily register the engine's built-in shaders. Idempotent. */
            void registerBuiltinShaders();

        private:
            MaterialShaderRegistry() = default;

            Array<MaterialShader> m_shaders;
            bool m_builtinsRegistered = false;
        };

    }  // namespace render
}  // namespace workphone

#endif  // __WP_MaterialShader_h__
