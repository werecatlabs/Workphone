#ifndef __WP_MaterialShaderParameters_h__
#define __WP_MaterialShaderParameters_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector4.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <Workphone/Core/ColourF.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @file MaterialShaderParameters.hpp
         * @brief Data-driven material shader parameter layer.
         *
         * @details This is the foundation of the data-driven material system ported from the
         *          Esoterica engine (Engine/Render/Shaders/EngineShader.h). Instead of the
         *          fixed PBR property model (diffuse / metalness / roughness) hard-coded into
         *          the legacy Material, a material shader publishes a list of typed
         *          parameters (MaterialShaderParameterInfo) and each material instance owns a
         *          flat byte buffer (MaterialShaderParametersInstance) that the editor and
         *          renderer read/write through stable, named handles.
         *
         *          PBR becomes one MaterialShader implementation among many; new shading
         *          workflows (unlit, foliage, hair, custom) only need to register their
         *          parameter list, not change the renderer or the editor.
         *
         *          This first port is intentionally self-contained: the instance owns its own
         *          parameter memory and a copy of the parameter info, so it compiles and
         *          serializes independently of the GPU upload path. The renderer-decoupling
         *          step will later read this owned memory when uploading constant buffers.
         *
         * @see Material
         * @since Data-Driven Materials Integration Step 1
         * @author Graphics Team
         */

        //---------------------------------------------------------------------------------------
        // Opaque resource handle
        //---------------------------------------------------------------------------------------

        /**
         * @brief Opaque, renderer-agnostic handle to a GPU resource (texture / buffer / sampler).
         *
         * @details Kept as a plain 64-bit value so the parameter layer has no dependency on any
         *          specific RHI. Concrete renderers cast this to their native handle type when
         *          binding. The value #InvalidMaterialResourceHandle marks an unbound slot.
         *
         *          This deliberately mirrors Esoterica's RHI::GenericResourceHandle usage but
         *          without pulling the RHI header into the material interface, which keeps the
         *          editor (and any tooling) free of graphics-backend coupling.
         */
        using MaterialResourceHandle = u64;

        /**
         * @brief Sentinel value for an unbound / invalid resource slot.
         *
         * @see MaterialResourceHandle
         */
        WPCore_API extern const MaterialResourceHandle InvalidMaterialResourceHandle;

        //---------------------------------------------------------------------------------------
        // Parameter type
        //---------------------------------------------------------------------------------------

        /**
         * @enum MaterialShaderParameterType
         * @brief The runtime type of a single material shader parameter.
         *
         * @details The type drives both the byte stride of the parameter and how the editor
         *          presents it (slider, colour swatch, texture slot, vector editor). It is the
         *          RHI-decoupled analogue of Esoterica's StringID parameter-type convention,
         *          but as a closed enum it is cheaper to switch on and trivial to reflect.
         */
        enum class MaterialShaderParameterType
        {
            Scalar,   ///< 4-byte scalar (float / int32 / uint32 packed as one u32).
            Colour,   ///< 16-byte RGBA colour (stored as a linear ColourF).
            Vector2,  ///< 8-byte 2-component vector (Vector2<f32> or Vector2<s32>).
            Vector4,  ///< 16-byte 4-component vector (Vector4<f32> or Vector4<s32>).
            Matrix,   ///< 64-byte 4x4 matrix (Matrix4<f32>).
            Texture,  ///< Opaque texture resource handle (MaterialResourceHandle).
            Buffer,   ///< Opaque buffer resource handle  (MaterialResourceHandle).
            Sampler   ///< Opaque sampler state handle    (MaterialResourceHandle).
        };

        /**
         * @brief The byte stride of a parameter of the given type.
         * @param type The parameter type.
         * @return The size in bytes occupied by one value of @p type in the parameter buffer.
         */
        WPCore_API u32 GetMaterialShaderParameterStride( MaterialShaderParameterType type );

        //---------------------------------------------------------------------------------------
        // Parameter info / handle
        //---------------------------------------------------------------------------------------

        /**
         * @struct MaterialShaderParameterInfo
         * @brief Description of a single parameter as published by a material shader.
         *
         * @details A material shader owns a list of these. Each entry gives the parameter a
         *          stable name (used by materials and the editor to look it up), a type, and the
         *          byte offset/stride within the parameter buffer. Offsets are assigned once
         *          when the parameter list is finalized (see
         *          MaterialShaderParametersInstance::Reset) and never change for the lifetime
         *          of the shader, so handles stay valid.
         */
        struct MaterialShaderParameterInfo
        {
            MaterialShaderParameterType m_type = MaterialShaderParameterType::Scalar;
            String m_name;
            u32 m_strideInBytes = 0;
            u32 m_offsetInBytes = 0;

            /**
             * @brief Construct a parameter info entry with a name and type; stride is derived
             *        from the type, offset is assigned later by the owning instance.
             */
            MaterialShaderParameterInfo( MaterialShaderParameterType type, String name ) :
                m_type( type ),
                m_name( std::move( name ) ),
                m_strideInBytes( GetMaterialShaderParameterStride( type ) ),
                m_offsetInBytes( 0 )
            {
            }

            MaterialShaderParameterInfo() = default;
        };

        /**
         * @struct MaterialShaderParameterHandle
         * @brief A cheap, stable reference to a parameter within a MaterialShaderParametersInstance.
         *
         * @details Resolved once from a name via MaterialShaderParametersInstance::FindParameter
         *          and then reused for every subsequent get/set. A default-constructed handle is
         *          invalid; IsValid() returns false when the parameter was not found.
         */
        struct MaterialShaderParameterHandle
        {
            u32 m_strideInBytes = 0;
            u32 m_offsetInBytes = 0;

            /** @brief True if this handle refers to a real parameter slot. */
            inline bool IsValid() const
            {
                return m_strideInBytes != 0;
            }
        };

        //---------------------------------------------------------------------------------------
        // Parameter instance
        //---------------------------------------------------------------------------------------

        /**
         * @class MaterialShaderParametersInstance
         * @brief Self-owned, data-driven container for a single material's shader parameters.
         *
         * @details This is the per-material analogue of Esoterica's
         *          MaterialShaderParametersInstance. Unlike the Esoterica version, which is a
         *          view into a render-system page-allocated buffer, this implementation owns
         *          its own byte buffer and a copy of the parameter info. That makes it:
         *            - trivially serializable (just save the info + the bytes),
         *            - independent of the renderer / RHI (no graphics header needed to use it),
         *            - safe to construct on the editor / tooling side.
         *
         *          The GPU upload path (step 3 of the integration) will later read
         *          GetParameterMemory() and copy it into the renderer's constant buffer.
         *
         *          Typical usage:
         *          @code
         *          Array<MaterialShaderParameterInfo> info;
         *          info.push_back( { MaterialShaderParameterType::Colour,  "Albedo" } );
         *          info.push_back( { MaterialShaderParameterType::Scalar,   "Roughness" } );
         *          info.push_back( { MaterialShaderParameterType::Texture, "AlbedoMap" } );
         *
         *          MaterialShaderParametersInstance params;
         *          params.Reset( info );
         *
         *          auto albedo = params.FindParameter( "Albedo" );
         *          params.SetColour( albedo, ColourF( 0.7f, 0.3f, 0.1f ) );
         *          params.SetScalar( params.FindParameter( "Roughness" ), 0.8f );
         *          params.InitializeDefaultResourceHandles();
         *          @endcode
         *
         * @par Thread Safety:
         *          Not thread-safe. Access must be confined to the owning thread (editor thread
         *          or render task) unless externally synchronized.
         */
        class WPCore_API MaterialShaderParametersInstance
        {
        public:
            /** @brief Default constructor; an empty, invalid instance until Reset() is called. */
            MaterialShaderParametersInstance();

            /** @brief Destructor. */
            ~MaterialShaderParametersInstance();

            /** @brief Copy / move (owning containers are copied / moved with the instance). */
            MaterialShaderParametersInstance( const MaterialShaderParametersInstance & ) = default;
            MaterialShaderParametersInstance &operator=( const MaterialShaderParametersInstance & ) =
                default;
            MaterialShaderParametersInstance( MaterialShaderParametersInstance && ) noexcept = default;
            MaterialShaderParametersInstance &operator=( MaterialShaderParametersInstance && ) noexcept =
                default;

            //-----------------------------------------------------------------------------------
            // Lifetime
            //-----------------------------------------------------------------------------------

            /**
             * @brief (Re)build the parameter buffer from a shader's parameter description.
             *
             * @details Assigns each parameter a byte offset (parameters are laid out in the
             *          order they appear in @p parameterInfo, 4-byte aligned) and allocates a
             *          flat byte buffer large enough to hold them all. Existing values are
             *          discarded and the new buffer is zeroed. After this call IsValid() is
             *          true (provided @p parameterInfo is non-empty).
             *
             * @param parameterInfo The shader's published parameter list.
             */
            void Reset( const Array<MaterialShaderParameterInfo> &parameterInfo );

            /** @brief Clear all storage and return to the invalid (empty) state. */
            void Clear();

            /** @brief True once Reset() has been called with a non-empty parameter list. */
            inline bool IsValid() const
            {
                return !m_parameterInfo.empty();
            }

            //-----------------------------------------------------------------------------------
            // Introspection (used by the editor to auto-generate the property grid)
            //-----------------------------------------------------------------------------------

            /**
             * @brief The parameter description this instance was built from.
             * @return A read-only view of the parameter info list.
             */
            inline const Array<MaterialShaderParameterInfo> &GetParameterInfo() const
            {
                return m_parameterInfo;
            }

            /**
             * @brief The raw, contiguous parameter byte buffer.
             *
             * @details Used by the renderer upload path (step 3) and by serialization. Layout
             *          matches GetParameterInfo() offsets exactly.
             */
            inline const Array<u8> &GetParameterMemory() const
            {
                return m_parametersMemory;
            }

            /** @brief Total size in bytes of the parameter buffer. */
            inline u32 GetParameterBufferSize() const
            {
                return static_cast<u32>( m_parametersMemory.size() );
            }

            /**
             * @brief Look up a parameter by name.
             * @param parameterName The parameter name (must match the published info).
             * @return A handle for direct get/set, or an invalid handle if not found.
             */
            MaterialShaderParameterHandle FindParameter( const String &parameterName ) const;

            //-----------------------------------------------------------------------------------
            // Setters (type-aware; assert the handle stride matches the value)
            //-----------------------------------------------------------------------------------

            void SetScalar( MaterialShaderParameterHandle parameter, f32 value );
            void SetScalar( MaterialShaderParameterHandle parameter, s32 value );
            void SetScalar( MaterialShaderParameterHandle parameter, u32 value );
            void SetColour( MaterialShaderParameterHandle parameter, const ColourF &value );
            void SetVector2( MaterialShaderParameterHandle parameter, const Vector2<f32> &value );
            void SetVector2( MaterialShaderParameterHandle parameter, const Vector2<s32> &value );
            void SetVector4( MaterialShaderParameterHandle parameter, const Vector4<f32> &value );
            void SetVector4( MaterialShaderParameterHandle parameter, const Vector4<s32> &value );
            void SetMatrix( MaterialShaderParameterHandle parameter, const Matrix4<f32> &value );
            void SetTexture( MaterialShaderParameterHandle parameter, MaterialResourceHandle value );
            void SetBuffer( MaterialShaderParameterHandle parameter, MaterialResourceHandle value );
            void SetSampler( MaterialShaderParameterHandle parameter, MaterialResourceHandle value );

            //-----------------------------------------------------------------------------------
            // Getters
            //-----------------------------------------------------------------------------------

            f32 GetScalar( MaterialShaderParameterHandle parameter ) const;
            ColourF GetColour( MaterialShaderParameterHandle parameter ) const;
            Vector2<f32> GetVector2f( MaterialShaderParameterHandle parameter ) const;
            Vector4<f32> GetVector4f( MaterialShaderParameterHandle parameter ) const;
            Matrix4<f32> GetMatrix( MaterialShaderParameterHandle parameter ) const;
            MaterialResourceHandle GetResource( MaterialShaderParameterHandle parameter ) const;

            //-----------------------------------------------------------------------------------
            // Resource handle helpers (mirror Esoterica's InitializeDefaultResourceHandles /
            // ValidateResourceHandles, but driven by the MaterialShaderParameterType enum)
            //-----------------------------------------------------------------------------------

            /**
             * @brief Reset every Texture / Buffer / Sampler slot to the invalid handle.
             *
             * @details Call after Reset() (or before binding) so unassigned resource slots have
             *          a well-defined sentinel rather than garbage.
             */
            void InitializeDefaultResourceHandles();

            /**
             * @brief Verify that every Texture / Buffer / Sampler slot has a bound resource.
             * @return False if any resource-typed parameter still holds the invalid handle.
             */
            bool ValidateResourceHandles() const;

        private:
            /**
             * @brief Bounds-checked write of @p value bytes at the parameter's offset.
             * @details Asserts the handle is valid and the write stays in buffer. All typed
             *          setters funnel through here.
             */
            void WriteBytes( MaterialShaderParameterHandle parameter, const void *value, u32 valueSize );

            /**
             * @brief Bounds-checked read of @p valueSize bytes from the parameter's offset.
             * @details Asserts the handle is valid and the read stays in buffer.
             */
            void ReadBytes( MaterialShaderParameterHandle parameter, void *outValue,
                            u32 valueSize ) const;

        private:
            Array<MaterialShaderParameterInfo>
                m_parameterInfo;           ///< The shader's parameter description (copied).
            Array<u8> m_parametersMemory;  ///< Flat, contiguous byte buffer of all parameter values.
        };

    }  // namespace render
}  // namespace workphone

#endif  // __WP_MaterialShaderParameters_h__
