// Data-driven material shader parameter layer.
//
// Foundation step 1 of the Esoterica material integration. This is the
// self-contained (option B) port of Esoterica's MaterialShaderParametersInstance
// (Engine/Render/Shaders/EngineShader.h). The instance owns its own byte
// buffer and a copy of the parameter info so it compiles / serializes without
// any RHI dependency; the GPU upload path will read this memory in step 3.

#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Graphics/MaterialShaderParameters.hpp"

#include <cstring>

namespace workphone
{
    namespace render
    {

        //---------------------------------------------------------------------------------------
        // Opaque resource handle sentinel
        //---------------------------------------------------------------------------------------

        const MaterialResourceHandle InvalidMaterialResourceHandle =
            static_cast<MaterialResourceHandle>( ~0ULL );

        //---------------------------------------------------------------------------------------
        // Parameter type -> byte stride
        //---------------------------------------------------------------------------------------

        u32 GetMaterialShaderParameterStride( MaterialShaderParameterType type )
        {
            switch( type )
            {
            case MaterialShaderParameterType::Scalar:
                return static_cast<u32>( sizeof( u32 ) );
            case MaterialShaderParameterType::Colour:
                return static_cast<u32>( sizeof( ColourF ) );
            case MaterialShaderParameterType::Vector2:
                return static_cast<u32>( sizeof( Vector2<f32> ) );
            case MaterialShaderParameterType::Vector4:
                return static_cast<u32>( sizeof( Vector4<f32> ) );
            case MaterialShaderParameterType::Matrix:
                return static_cast<u32>( sizeof( Matrix4<f32> ) );
            case MaterialShaderParameterType::Texture:
                return static_cast<u32>( sizeof( MaterialResourceHandle ) );
            case MaterialShaderParameterType::Buffer:
                return static_cast<u32>( sizeof( MaterialResourceHandle ) );
            case MaterialShaderParameterType::Sampler:
                return static_cast<u32>( sizeof( MaterialResourceHandle ) );
            default:
                WP_ASSERT( false );
                return 0;
            }
        }

        //---------------------------------------------------------------------------------------
        // Lifetime
        //---------------------------------------------------------------------------------------

        MaterialShaderParametersInstance::MaterialShaderParametersInstance() = default;
        MaterialShaderParametersInstance::~MaterialShaderParametersInstance() = default;

        void MaterialShaderParametersInstance::Reset(
            const Array<MaterialShaderParameterInfo> &parameterInfo )
        {
            m_parameterInfo = parameterInfo;

            u32 offset = 0;
            for( auto &info : m_parameterInfo )
            {
                // 4-byte align every slot; resource handles are 8 bytes and naturally satisfy this,
                // scalars/vectors/colours pack to 4-byte boundaries.
                const u32 alignMask = 3u;
                offset = ( offset + alignMask ) & ~alignMask;

                info.m_strideInBytes = GetMaterialShaderParameterStride( info.m_type );
                info.m_offsetInBytes = offset;
                offset += info.m_strideInBytes;
            }

            m_parametersMemory.assign( offset, 0 );
        }

        void MaterialShaderParametersInstance::Clear()
        {
            m_parameterInfo.clear();
            m_parametersMemory.clear();
        }

        //---------------------------------------------------------------------------------------
        // Lookup
        //---------------------------------------------------------------------------------------

        MaterialShaderParameterHandle MaterialShaderParametersInstance::FindParameter(
            const String &parameterName ) const
        {
            for( const auto &info : m_parameterInfo )
            {
                if( info.m_name == parameterName )
                {
                    return { info.m_strideInBytes, info.m_offsetInBytes };
                }
            }
            return {};
        }

        //---------------------------------------------------------------------------------------
        // Bounds-checked raw access
        //---------------------------------------------------------------------------------------

        void MaterialShaderParametersInstance::WriteBytes( MaterialShaderParameterHandle parameter,
                                                           const void *value, u32 valueSize )
        {
            WP_ASSERT( parameter.IsValid() );
            WP_ASSERT( parameter.m_strideInBytes == valueSize );
            WP_ASSERT( parameter.m_offsetInBytes + valueSize <= m_parametersMemory.size() );
            if( !parameter.IsValid() ||
                parameter.m_offsetInBytes + valueSize > m_parametersMemory.size() )
            {
                return;
            }
            std::memcpy( m_parametersMemory.data() + parameter.m_offsetInBytes, value, valueSize );
        }

        void MaterialShaderParametersInstance::ReadBytes( MaterialShaderParameterHandle parameter,
                                                          void *outValue, u32 valueSize ) const
        {
            WP_ASSERT( parameter.IsValid() );
            WP_ASSERT( parameter.m_strideInBytes == valueSize );
            WP_ASSERT( parameter.m_offsetInBytes + valueSize <= m_parametersMemory.size() );
            if( !parameter.IsValid() ||
                parameter.m_offsetInBytes + valueSize > m_parametersMemory.size() )
            {
                std::memset( outValue, 0, valueSize );
                return;
            }
            std::memcpy( outValue, m_parametersMemory.data() + parameter.m_offsetInBytes, valueSize );
        }

        //---------------------------------------------------------------------------------------
        // Typed setters
        //---------------------------------------------------------------------------------------

        void MaterialShaderParametersInstance::SetScalar( MaterialShaderParameterHandle parameter,
                                                          f32 value )
        {
            WriteBytes( parameter, &value, static_cast<u32>( sizeof( value ) ) );
        }

        void MaterialShaderParametersInstance::SetScalar( MaterialShaderParameterHandle parameter,
                                                          s32 value )
        {
            WriteBytes( parameter, &value, static_cast<u32>( sizeof( value ) ) );
        }

        void MaterialShaderParametersInstance::SetScalar( MaterialShaderParameterHandle parameter,
                                                          u32 value )
        {
            WriteBytes( parameter, &value, static_cast<u32>( sizeof( value ) ) );
        }

        void MaterialShaderParametersInstance::SetColour( MaterialShaderParameterHandle parameter,
                                                          const ColourF &value )
        {
            WriteBytes( parameter, &value, static_cast<u32>( sizeof( value ) ) );
        }

        void MaterialShaderParametersInstance::SetVector2( MaterialShaderParameterHandle parameter,
                                                           const Vector2<f32> &value )
        {
            WriteBytes( parameter, &value, static_cast<u32>( sizeof( value ) ) );
        }

        void MaterialShaderParametersInstance::SetVector2( MaterialShaderParameterHandle parameter,
                                                           const Vector2<s32> &value )
        {
            WriteBytes( parameter, &value, static_cast<u32>( sizeof( value ) ) );
        }

        void MaterialShaderParametersInstance::SetVector4( MaterialShaderParameterHandle parameter,
                                                           const Vector4<f32> &value )
        {
            WriteBytes( parameter, &value, static_cast<u32>( sizeof( value ) ) );
        }

        void MaterialShaderParametersInstance::SetVector4( MaterialShaderParameterHandle parameter,
                                                           const Vector4<s32> &value )
        {
            WriteBytes( parameter, &value, static_cast<u32>( sizeof( value ) ) );
        }

        void MaterialShaderParametersInstance::SetMatrix( MaterialShaderParameterHandle parameter,
                                                          const Matrix4<f32> &value )
        {
            WriteBytes( parameter, &value, static_cast<u32>( sizeof( value ) ) );
        }

        void MaterialShaderParametersInstance::SetTexture( MaterialShaderParameterHandle parameter,
                                                           MaterialResourceHandle value )
        {
            WriteBytes( parameter, &value, static_cast<u32>( sizeof( value ) ) );
        }

        void MaterialShaderParametersInstance::SetBuffer( MaterialShaderParameterHandle parameter,
                                                          MaterialResourceHandle value )
        {
            WriteBytes( parameter, &value, static_cast<u32>( sizeof( value ) ) );
        }

        void MaterialShaderParametersInstance::SetSampler( MaterialShaderParameterHandle parameter,
                                                           MaterialResourceHandle value )
        {
            WriteBytes( parameter, &value, static_cast<u32>( sizeof( value ) ) );
        }

        //---------------------------------------------------------------------------------------
        // Typed getters
        //---------------------------------------------------------------------------------------

        f32 MaterialShaderParametersInstance::GetScalar( MaterialShaderParameterHandle parameter ) const
        {
            f32 value = 0.0f;
            ReadBytes( parameter, &value, static_cast<u32>( sizeof( value ) ) );
            return value;
        }

        ColourF MaterialShaderParametersInstance::GetColour(
            MaterialShaderParameterHandle parameter ) const
        {
            ColourF value;
            ReadBytes( parameter, &value, static_cast<u32>( sizeof( value ) ) );
            return value;
        }

        Vector2<f32> MaterialShaderParametersInstance::GetVector2f(
            MaterialShaderParameterHandle parameter ) const
        {
            Vector2<f32> value;
            ReadBytes( parameter, &value, static_cast<u32>( sizeof( value ) ) );
            return value;
        }

        Vector4<f32> MaterialShaderParametersInstance::GetVector4f(
            MaterialShaderParameterHandle parameter ) const
        {
            Vector4<f32> value;
            ReadBytes( parameter, &value, static_cast<u32>( sizeof( value ) ) );
            return value;
        }

        Matrix4<f32> MaterialShaderParametersInstance::GetMatrix(
            MaterialShaderParameterHandle parameter ) const
        {
            Matrix4<f32> value;
            ReadBytes( parameter, &value, static_cast<u32>( sizeof( value ) ) );
            return value;
        }

        MaterialResourceHandle MaterialShaderParametersInstance::GetResource(
            MaterialShaderParameterHandle parameter ) const
        {
            MaterialResourceHandle value = InvalidMaterialResourceHandle;
            ReadBytes( parameter, &value, static_cast<u32>( sizeof( value ) ) );
            return value;
        }

        //---------------------------------------------------------------------------------------
        // Resource handle helpers
        //---------------------------------------------------------------------------------------

        void MaterialShaderParametersInstance::InitializeDefaultResourceHandles()
        {
            for( const auto &info : m_parameterInfo )
            {
                const bool isResource = info.m_type == MaterialShaderParameterType::Texture ||
                                        info.m_type == MaterialShaderParameterType::Buffer ||
                                        info.m_type == MaterialShaderParameterType::Sampler;
                if( !isResource )
                {
                    continue;
                }

                MaterialShaderParameterHandle handle{ info.m_strideInBytes, info.m_offsetInBytes };
                const MaterialResourceHandle sentinel = InvalidMaterialResourceHandle;
                WriteBytes( handle, &sentinel, static_cast<u32>( sizeof( sentinel ) ) );
            }
        }

        bool MaterialShaderParametersInstance::ValidateResourceHandles() const
        {
            for( const auto &info : m_parameterInfo )
            {
                const bool isResource = info.m_type == MaterialShaderParameterType::Texture ||
                                        info.m_type == MaterialShaderParameterType::Buffer ||
                                        info.m_type == MaterialShaderParameterType::Sampler;
                if( !isResource )
                {
                    continue;
                }

                MaterialShaderParameterHandle handle{ info.m_strideInBytes, info.m_offsetInBytes };
                if( GetResource( handle ) == InvalidMaterialResourceHandle )
                {
                    return false;
                }
            }
            return true;
        }

    }  // namespace render
}  // namespace workphone
