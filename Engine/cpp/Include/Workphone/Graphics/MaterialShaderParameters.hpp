#ifndef __WP_MaterialShaderParameters_h__
#define __WP_MaterialShaderParameters_h__
#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector4.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <memory>

namespace workphone::render
{
    using MaterialResourceHandle = u64;
    WPCore_API extern const MaterialResourceHandle InvalidMaterialResourceHandle;
    // Existing values retained. Scalar/Vector2/Vector4 explicitly describe floats.
    enum class MaterialShaderParameterType
    {
        Scalar, Colour, Vector2, Vector4, Matrix, Texture, Buffer, Sampler,
        Int, UInt, Bool, IntVector2, IntVector4
    };
    WPCore_API u32 GetMaterialShaderParameterStride( MaterialShaderParameterType type );
    // FNV-1a 64 over exact name bytes; IDs are scoped to a shader schema.
    WPCore_API u64 GetMaterialShaderParameterId( const String &name );

    struct MaterialShaderParameterInfo
    {
        MaterialShaderParameterType m_type = MaterialShaderParameterType::Scalar;
        String m_name;
        u32 m_strideInBytes = 0;
        u32 m_offsetInBytes = 0;
        MaterialShaderParameterInfo( MaterialShaderParameterType type, String name ) :
            m_type( type ), m_name( std::move( name ) ),
            m_strideInBytes( GetMaterialShaderParameterStride( type ) ) {}
        MaterialShaderParameterInfo() = default;
    };

    struct MaterialShaderParameterHandle
    {
        // Offset/stride retained for source compatibility, verified against the layout.
        u32 m_strideInBytes = 0;
        u32 m_offsetInBytes = 0;
        u64 m_layoutId = 0;
        u32 m_parameterIndex = 0;
        bool IsValid() const { return m_layoutId != 0 && m_strideInBytes != 0; }
    };
    enum class MaterialBuiltinParameter : u32
    { Albedo, Metalness, Roughness, Emissive, Opacity, AlphaClip, Color, Count };

    /** Shared immutable CPU schema. GPU packing is a separate backend contract. */
    class WPCore_API MaterialShaderParameterLayout
    {
    public:
        using Ptr = std::shared_ptr<const MaterialShaderParameterLayout>;
        static Ptr Create( const Array<MaterialShaderParameterInfo> &info );
        const Array<MaterialShaderParameterInfo> &GetParameterInfo() const { return m_info; }
        const Array<u8> &GetDefaults() const { return m_defaults; }
        u64 GetId() const { return m_id; }
        MaterialShaderParameterHandle FindParameter( const String &name ) const;
        MaterialShaderParameterHandle GetParameter( u32 index ) const;
        MaterialShaderParameterHandle GetBuiltin( MaterialBuiltinParameter semantic ) const;
        size_t GetStorageBytes() const;
    private:
        struct Lookup { u64 id; u32 index; };
        Array<MaterialShaderParameterInfo> m_info;
        Array<Lookup> m_lookup;
        Array<u8> m_defaults;
        MaterialShaderParameterHandle m_builtin[static_cast<u32>( MaterialBuiltinParameter::Count )]{};
        u64 m_id = 0;
    };

    template <class T> struct MaterialParameterType;
#define WP_MATERIAL_TYPE(T, Kind) template <> struct MaterialParameterType<T> \
    { static constexpr auto value = MaterialShaderParameterType::Kind; };
    WP_MATERIAL_TYPE( f32, Scalar )
    WP_MATERIAL_TYPE( s32, Int )
    WP_MATERIAL_TYPE( u32, UInt )
    WP_MATERIAL_TYPE( bool, Bool )
    WP_MATERIAL_TYPE( ColourF, Colour )
    WP_MATERIAL_TYPE( Vector2<f32>, Vector2 )
    WP_MATERIAL_TYPE( Vector2<s32>, IntVector2 )
    WP_MATERIAL_TYPE( Vector4<f32>, Vector4 )
    WP_MATERIAL_TYPE( Vector4<s32>, IntVector4 )
    WP_MATERIAL_TYPE( Matrix4<f32>, Matrix )
#undef WP_MATERIAL_TYPE
    template <class T> struct MaterialParameterHandle
    {
        MaterialShaderParameterHandle handle;
        bool IsValid() const { return handle.IsValid(); }
    };

    /** Owning-thread use or external synchronization required. Copies share schemas,
     * but own independent bytes. Effective writes (including through retained mutable
     * references) advance the revision. Raw mutable bytes are never exposed.
     */
    class WPCore_API MaterialShaderParametersInstance
    {
    public:
        MaterialShaderParametersInstance() = default;
        ~MaterialShaderParametersInstance() = default;
        MaterialShaderParametersInstance( const MaterialShaderParametersInstance & ) = default;
        MaterialShaderParametersInstance &operator=( const MaterialShaderParametersInstance & ) = default;
        MaterialShaderParametersInstance( MaterialShaderParametersInstance && ) noexcept = default;
        MaterialShaderParametersInstance &operator=( MaterialShaderParametersInstance && ) noexcept = default;

        void Reset( const Array<MaterialShaderParameterInfo> &info );
        void Reset( MaterialShaderParameterLayout::Ptr layout );
        void Clear();
        bool IsValid() const { return m_layout && !GetParameterInfo().empty(); }
        const Array<MaterialShaderParameterInfo> &GetParameterInfo() const;
        const Array<u8> &GetParameterMemory() const { return m_parametersMemory; }
        u32 GetParameterBufferSize() const { return static_cast<u32>( m_parametersMemory.size() ); }
        const MaterialShaderParameterLayout::Ptr &GetLayout() const { return m_layout; }
        u64 GetRevision() const { return m_revision; }
        u64 GetNameLookupCount() const { return m_nameLookups; }
        size_t GetInstanceStorageBytes() const;
        MaterialShaderParameterHandle FindParameter( const String &name ) const;
        MaterialShaderParameterHandle GetParameter( u32 index ) const;
        MaterialShaderParameterHandle GetBuiltin( MaterialBuiltinParameter semantic ) const;
        bool IsHandleValid( MaterialShaderParameterHandle h, MaterialShaderParameterType type, u32 size ) const;
        template <class T> MaterialParameterHandle<T> Resolve( const String &name ) const
        {
            auto h = FindParameter( name );
            constexpr u32 size = std::is_same_v<T, bool> ? sizeof( u32 ) : sizeof( T );
            return { IsHandleValid( h, MaterialParameterType<T>::value, size ) ?
                     h : MaterialShaderParameterHandle{} };
        }
        template <class T> bool Set( MaterialParameterHandle<T> h, const T &value )
        {
            if constexpr( std::is_same_v<T, bool> ) return SetBool( h.handle, value );
            else return WriteBytes( h.handle, &value, sizeof( T ), MaterialParameterType<T>::value );
        }
        template <class T> bool Get( MaterialParameterHandle<T> h, T &value ) const
        {
            if constexpr( std::is_same_v<T, bool> )
            {
                if( !IsHandleValid( h.handle, MaterialShaderParameterType::Bool, sizeof( u32 ) ) ) return false;
                value = GetBool( h.handle ); return true;
            }
            else return ReadBytes( h.handle, &value, sizeof( T ), MaterialParameterType<T>::value );
        }

        // Invalid writes fail without mutation in Debug and Release.
        bool SetScalar( MaterialShaderParameterHandle h, f32 value );
        bool SetScalar( MaterialShaderParameterHandle h, s32 value );
        bool SetScalar( MaterialShaderParameterHandle h, u32 value );
        bool SetBool( MaterialShaderParameterHandle h, bool value );
        bool SetColour( MaterialShaderParameterHandle h, const ColourF &value );
        bool SetVector2( MaterialShaderParameterHandle h, const Vector2<f32> &value );
        bool SetVector2( MaterialShaderParameterHandle h, const Vector2<s32> &value );
        bool SetVector4( MaterialShaderParameterHandle h, const Vector4<f32> &value );
        bool SetVector4( MaterialShaderParameterHandle h, const Vector4<s32> &value );
        bool SetMatrix( MaterialShaderParameterHandle h, const Matrix4<f32> &value );
        bool SetTexture( MaterialShaderParameterHandle h, MaterialResourceHandle value );
        bool SetBuffer( MaterialShaderParameterHandle h, MaterialResourceHandle value );
        bool SetSampler( MaterialShaderParameterHandle h, MaterialResourceHandle value );
        f32 GetScalar( MaterialShaderParameterHandle h ) const;
        s32 GetInt( MaterialShaderParameterHandle h ) const;
        u32 GetUInt( MaterialShaderParameterHandle h ) const;
        bool GetBool( MaterialShaderParameterHandle h ) const;
        ColourF GetColour( MaterialShaderParameterHandle h ) const;
        Vector2<f32> GetVector2f( MaterialShaderParameterHandle h ) const;
        Vector4<f32> GetVector4f( MaterialShaderParameterHandle h ) const;
        Matrix4<f32> GetMatrix( MaterialShaderParameterHandle h ) const;
        MaterialResourceHandle GetResource( MaterialShaderParameterHandle h ) const;
        void InitializeDefaultResourceHandles();
        bool ValidateResourceHandles() const;
    private:
        bool WriteBytes( MaterialShaderParameterHandle h, const void *value, u32 size, MaterialShaderParameterType type );
        bool ReadBytes( MaterialShaderParameterHandle h, void *value, u32 size, MaterialShaderParameterType type ) const;
        MaterialShaderParameterLayout::Ptr m_layout;
        Array<u8> m_parametersMemory;
        u64 m_revision = 0;
        mutable u64 m_nameLookups = 0;
    };
}
#endif
