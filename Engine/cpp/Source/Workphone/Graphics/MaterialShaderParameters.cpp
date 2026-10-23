#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/MaterialShaderParameters.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <limits>

namespace workphone::render
{
    const MaterialResourceHandle InvalidMaterialResourceHandle = ~u64( 0 );
    u32 GetMaterialShaderParameterStride( MaterialShaderParameterType type )
    {
        switch( type )
        {
        case MaterialShaderParameterType::Scalar:
        case MaterialShaderParameterType::Int:
        case MaterialShaderParameterType::UInt:
        case MaterialShaderParameterType::Bool: return 4;
        case MaterialShaderParameterType::Colour: return sizeof( ColourF );
        case MaterialShaderParameterType::Vector2: return sizeof( Vector2<f32> );
        case MaterialShaderParameterType::IntVector2: return sizeof( Vector2<s32> );
        case MaterialShaderParameterType::Vector4: return sizeof( Vector4<f32> );
        case MaterialShaderParameterType::IntVector4: return sizeof( Vector4<s32> );
        case MaterialShaderParameterType::Matrix: return sizeof( Matrix4<f32> );
        case MaterialShaderParameterType::Texture:
        case MaterialShaderParameterType::Buffer:
        case MaterialShaderParameterType::Sampler: return sizeof( MaterialResourceHandle );
        default: return 0;
        }
    }

    u64 GetMaterialShaderParameterId( const String &name )
    {
        u64 id = 14695981039346656037ull;
        for( auto c : name ) { id ^= static_cast<unsigned char>( c ); id *= 1099511628211ull; }
        return id;
    }

    MaterialShaderParameterLayout::Ptr MaterialShaderParameterLayout::Create(
        const Array<MaterialShaderParameterInfo> &info )
    {
        static std::atomic<u64> nextId{ 1 };
        u64 bufferBytes = info.capacity() * sizeof(MaterialShaderParameterInfo) + info.size() * sizeof(Lookup);
        u64 valueBytes = 0;
        for( const auto &p : info )
        {
            const auto stride = GetMaterialShaderParameterStride(p.m_type);
            if( p.m_name.empty() || !stride ) return {};
            valueBytes = ((valueBytes + 3ull) & ~3ull) + stride;
            if( valueBytes > std::numeric_limits<u32>::max() ) return {};
            // Includes inline names conservatively; a copied name never needs
            // more heap capacity than the source string's retained capacity.
            bufferBytes += p.m_name.capacity() + 1;
        }
        auto budget = reserveSourceBudgetBytes( static_cast<size_t>(bufferBytes + valueBytes) );
        auto layout = std::allocate_shared<MaterialShaderParameterLayout>( SourceBudgetAllocator<MaterialShaderParameterLayout>{} );
        layout->m_storageBudget = std::move(budget);
        layout->m_lookup.reserve(info.size());
        layout->m_info = info;
        u64 offset = 0;
        for( u32 i = 0; i < layout->m_info.size(); ++i )
        {
            auto &p = layout->m_info[i];
            p.m_strideInBytes = GetMaterialShaderParameterStride( p.m_type );
            if( p.m_name.empty() || !p.m_strideInBytes ) return {};
            offset = ( offset + 3ull ) & ~3ull;
            if( offset + p.m_strideInBytes > std::numeric_limits<u32>::max() ) return {};
            p.m_offsetInBytes = static_cast<u32>( offset );
            offset += p.m_strideInBytes;
            layout->m_lookup.push_back( { GetMaterialShaderParameterId( p.m_name ), i } );
        }
        std::sort( layout->m_lookup.begin(), layout->m_lookup.end(),
                   []( const Lookup &a, const Lookup &b ) { return a.id < b.id; } );
        for( size_t i = 1; i < layout->m_lookup.size(); ++i )
            if( layout->m_lookup[i - 1].id == layout->m_lookup[i].id ) return {};
        auto identity = nextId.load( std::memory_order_relaxed );
        do
        {
            if( identity == std::numeric_limits<u64>::max() ) return {};
        } while( !nextId.compare_exchange_weak( identity, identity + 1, std::memory_order_relaxed ) );
        layout->m_id = identity;
        layout->m_defaults.assign( static_cast<size_t>( offset ), 0 );
        for( const auto &p : layout->m_info )
            if( p.m_type == MaterialShaderParameterType::Texture ||
                p.m_type == MaterialShaderParameterType::Buffer ||
                p.m_type == MaterialShaderParameterType::Sampler )
                std::memcpy( layout->m_defaults.data() + p.m_offsetInBytes,
                             &InvalidMaterialResourceHandle, sizeof( MaterialResourceHandle ) );
        const char *names[] = { "Albedo", "Metalness", "Roughness", "Emissive", "Opacity", "AlphaClip", "Color" };
        for( u32 i = 0; i < static_cast<u32>( MaterialBuiltinParameter::Count ); ++i )
            layout->m_builtin[i] = layout->FindParameter( names[i] );
        return layout;
    }

    MaterialShaderParameterHandle MaterialShaderParameterLayout::GetParameter( u32 index ) const
    {
        if( index >= m_info.size() ) return {};
        const auto &p = m_info[index];
        return { p.m_strideInBytes, p.m_offsetInBytes, m_id, index };
    }
    MaterialShaderParameterHandle MaterialShaderParameterLayout::FindParameter( const String &name ) const
    {
        const auto id = GetMaterialShaderParameterId( name );
        auto it = std::lower_bound( m_lookup.begin(), m_lookup.end(), id,
                                  []( const Lookup &a, u64 b ) { return a.id < b; } );
        if( it == m_lookup.end() || it->id != id || m_info[it->index].m_name != name ) return {};
        return GetParameter( it->index );
    }
    MaterialShaderParameterHandle MaterialShaderParameterLayout::GetBuiltin( MaterialBuiltinParameter semantic ) const
    {
        const auto i = static_cast<u32>( semantic );
        return i < static_cast<u32>( MaterialBuiltinParameter::Count ) ? m_builtin[i] : MaterialShaderParameterHandle{};
    }
    size_t MaterialShaderParameterLayout::GetStorageBytes() const
    {
        size_t bytes = sizeof( *this ) + m_info.capacity() * sizeof( MaterialShaderParameterInfo ) +
                       m_lookup.capacity() * sizeof( Lookup ) + m_defaults.capacity();
        for( const auto &p : m_info )
        {
#if WP_USE_WP_STRING && WP_USE_STANDARD_STRING_ALLOCATOR
            // The string object above already includes its inline characters.
            if( p.m_name.using_inline_storage() ) continue;
#endif
            bytes += p.m_name.capacity() + 1;
        }
        return bytes;
    }

    MaterialShaderParametersInstance::MaterialShaderParametersInstance(
        const MaterialShaderParametersInstance &other ) :
        m_layout( other.m_layout ), m_parametersMemory( other.m_parametersMemory ),
        m_revision( other.m_revision ), m_nameLookups( other.m_nameLookups ) {}

    MaterialShaderParametersInstance &MaterialShaderParametersInstance::operator=(
        const MaterialShaderParametersInstance &other )
    {
        if( this == &other ) return *this;
        if( m_layout == other.m_layout && GetParameterMemory() == other.GetParameterMemory() ) return *this;
        auto handoff = m_owner && !m_owner->m_synchronizingShaderParameters ?
            m_owner->prepareShaderSurfaceHandoff( other.m_layout ) : nullptr;
        auto bytes = other.m_parametersMemory;
        m_layout = other.m_layout;
        m_parametersMemory = std::move( bytes );
        ++m_revision;
        if( m_owner ) m_owner->commitShaderSurfaceHandoff( std::move( handoff ) );
        NotifyOwner( 0xffu );
        return *this;
    }

    MaterialShaderParametersInstance::MaterialShaderParametersInstance(
        MaterialShaderParametersInstance &&other ) noexcept
    {
        try
        {
            auto handoff = other.m_owner && !other.m_owner->m_synchronizingShaderParameters ?
                other.m_owner->prepareShaderSurfaceHandoff( MaterialShaderParameterLayout::Ptr{} ) : nullptr;
            m_layout = std::move( other.m_layout );
            m_parametersMemory = std::move( other.m_parametersMemory );
            m_revision = other.m_revision;
            m_nameLookups = other.m_nameLookups;
            if( other.m_owner ) other.m_owner->commitShaderSurfaceHandoff( std::move( handoff ) );
            ++other.m_revision;
            other.NotifyOwner( 0xffu );
        }
        catch( ... )
        {
            // Staging is the only allocating step. On failure the physical
            // source retains its values/observer; this new instance stays empty.
        }
    }

    MaterialShaderParametersInstance &MaterialShaderParametersInstance::operator=(
        MaterialShaderParametersInstance &&other ) noexcept
    {
        if( this == &other ) return *this;
        const bool changed = m_layout != other.m_layout || GetParameterMemory() != other.GetParameterMemory();
        try
        {
            auto destinationHandoff = changed && m_owner && !m_owner->m_synchronizingShaderParameters ?
                m_owner->prepareShaderSurfaceHandoff( other.m_layout ) : nullptr;
            auto sourceHandoff = other.m_owner && !other.m_owner->m_synchronizingShaderParameters ?
                other.m_owner->prepareShaderSurfaceHandoff( MaterialShaderParameterLayout::Ptr{} ) : nullptr;
            // Stage both physical owners before changing either instance.
            m_layout = std::move( other.m_layout );
            m_parametersMemory = std::move( other.m_parametersMemory );
            if( m_owner ) m_owner->commitShaderSurfaceHandoff( std::move( destinationHandoff ) );
            if( other.m_owner ) other.m_owner->commitShaderSurfaceHandoff( std::move( sourceHandoff ) );
            if( changed ) { ++m_revision; NotifyOwner( 0xffu ); }
            ++other.m_revision;
            other.NotifyOwner( 0xffu );
        }
        catch( ... )
        {
            // A failed fallback allocation leaves both instances and owners
            // unchanged while preserving the existing noexcept move contract.
        }
        return *this;
    }

    void MaterialShaderParametersInstance::NotifyOwner( u8 mask ) noexcept
    {
        if( !m_owner || !mask || m_owner->m_synchronizingShaderParameters ) return;
        if( mask == 0xffu )
        {
            // Replacement supersedes edits queued for the old schema. Reapply
            // compatible declared semantics, leaving missing/wrong-type fallback
            // and independent pass overrides in their existing owners.
            m_owner->m_pendingShaderSemantics = 0;
            mask = static_cast<u8>( ( 1u << static_cast<u32>( MaterialBuiltinParameter::Count ) ) - 1u );
        }
        m_owner->shaderParametersChanged( mask );
    }

    void MaterialShaderParametersInstance::BeginEdit() noexcept
    {
        if( m_owner ) ++m_owner->m_shaderParameterEditDepth;
    }

    void MaterialShaderParametersInstance::EndEdit() noexcept
    {
        if( m_owner && --m_owner->m_shaderParameterEditDepth == 0 )
            m_owner->shaderParametersChanged( 0 );
    }

    void MaterialShaderParametersInstance::Reset( const Array<MaterialShaderParameterInfo> &info )
    {
        auto layout = MaterialShaderParameterLayout::Create( info );
        if( layout ) Reset( std::move( layout ) );
    }
    void MaterialShaderParametersInstance::Reset( MaterialShaderParameterLayout::Ptr layout )
    {
        auto handoff = m_owner && !m_owner->m_synchronizingShaderParameters ?
            m_owner->prepareShaderSurfaceHandoff( layout ) : nullptr;
        m_parametersMemory.reset();
        m_layout = std::move( layout );
        ++m_revision;
        if( m_owner ) m_owner->commitShaderSurfaceHandoff( std::move( handoff ) );
        NotifyOwner( 0xffu );
    }
    void MaterialShaderParametersInstance::Clear() { Reset( MaterialShaderParameterLayout::Ptr{} ); }
    const Array<MaterialShaderParameterInfo> &MaterialShaderParametersInstance::GetParameterInfo() const
    {
        static const Array<MaterialShaderParameterInfo> empty;
        return m_layout ? m_layout->GetParameterInfo() : empty;
    }
    size_t MaterialShaderParametersInstance::GetInstanceStorageBytes() const
    { return sizeof( *this ) + GetValueStorageBytes(); }
    size_t MaterialShaderParametersInstance::GetValueStorageBytes() const
    { return m_parametersMemory ? sizeof(Array<u8>) + m_parametersMemory->capacity() : 0; }
    const Array<u8> &MaterialShaderParametersInstance::GetParameterMemory() const
    {
        static const Array<u8> empty;
        return m_parametersMemory ? *m_parametersMemory : m_layout ? m_layout->GetDefaults() : empty;
    }
    MaterialShaderParameterHandle MaterialShaderParametersInstance::FindParameter( const String &name ) const
    {
        ++m_nameLookups;
        return m_layout ? m_layout->FindParameter( name ) : MaterialShaderParameterHandle{};
    }
    MaterialShaderParameterHandle MaterialShaderParametersInstance::GetParameter( u32 index ) const
    { return m_layout ? m_layout->GetParameter( index ) : MaterialShaderParameterHandle{}; }
    MaterialShaderParameterHandle MaterialShaderParametersInstance::GetBuiltin( MaterialBuiltinParameter semantic ) const
    { return m_layout ? m_layout->GetBuiltin( semantic ) : MaterialShaderParameterHandle{}; }
    bool MaterialShaderParametersInstance::IsHandleValid( MaterialShaderParameterHandle h,
        MaterialShaderParameterType type, u32 size ) const
    {
        if( !m_layout || !h.IsValid() || h.m_layoutId != m_layout->GetId() ||
            h.m_parameterIndex >= GetParameterInfo().size() ) return false;
        const auto &p = GetParameterInfo()[h.m_parameterIndex];
        return p.m_type == type && p.m_strideInBytes == size && h.m_strideInBytes == size &&
               h.m_offsetInBytes == p.m_offsetInBytes && h.m_offsetInBytes <= GetParameterBufferSize() &&
               size <= GetParameterBufferSize() - h.m_offsetInBytes;
    }
    bool MaterialShaderParametersInstance::WriteBytes( MaterialShaderParameterHandle h,
        const void *value, u32 size, MaterialShaderParameterType type )
    {
        if( !value || !IsHandleValid( h, type, size ) ) return false;
        const auto &memory = GetParameterMemory();
        if( std::memcmp( memory.data() + h.m_offsetInBytes, value, size ) != 0 )
        {
            // Allocate before mutation/notification: failure leaves values and
            // revision intact. No-op and rejected writes never detach.
            if(!m_parametersMemory || !m_parametersMemory.unique())
                m_parametersMemory = copySourceBudgetArray(memory);
            std::memcpy( m_parametersMemory->data() + h.m_offsetInBytes, value, size );
            ++m_revision;
            if( m_owner )
            {
                static_assert( static_cast<u32>( MaterialBuiltinParameter::Count ) <= 8,
                               "extend the publication mask when adding material semantics" );
                u8 mask = 0;
                for( u32 i = 0; i < static_cast<u32>( MaterialBuiltinParameter::Count ); ++i )
                {
                    const auto builtin = GetBuiltin( static_cast<MaterialBuiltinParameter>( i ) );
                    if( builtin.IsValid() && builtin.m_parameterIndex == h.m_parameterIndex )
                        mask |= static_cast<u8>( 1u << i );
                }
                NotifyOwner( mask );
            }
        }
        return true;
    }
    bool MaterialShaderParametersInstance::ReadBytes( MaterialShaderParameterHandle h,
        void *value, u32 size, MaterialShaderParameterType type ) const
    {
        if( !value || !IsHandleValid( h, type, size ) ) return false;
        std::memcpy( value, GetParameterMemory().data() + h.m_offsetInBytes, size );
        return true;
    }

#define WP_MATERIAL_SET(Name, ValueType, Kind) \
    bool MaterialShaderParametersInstance::Name( MaterialShaderParameterHandle h, ValueType value ) \
    { return WriteBytes( h, &value, sizeof( value ), MaterialShaderParameterType::Kind ); }
    WP_MATERIAL_SET( SetScalar, f32, Scalar )
    WP_MATERIAL_SET( SetScalar, s32, Int )
    WP_MATERIAL_SET( SetScalar, u32, UInt )
    WP_MATERIAL_SET( SetColour, const ColourF &, Colour )
    WP_MATERIAL_SET( SetVector2, const Vector2<f32> &, Vector2 )
    WP_MATERIAL_SET( SetVector2, const Vector2<s32> &, IntVector2 )
    WP_MATERIAL_SET( SetVector4, const Vector4<f32> &, Vector4 )
    WP_MATERIAL_SET( SetVector4, const Vector4<s32> &, IntVector4 )
    WP_MATERIAL_SET( SetMatrix, const Matrix4<f32> &, Matrix )
    WP_MATERIAL_SET( SetTexture, MaterialResourceHandle, Texture )
    WP_MATERIAL_SET( SetBuffer, MaterialResourceHandle, Buffer )
    WP_MATERIAL_SET( SetSampler, MaterialResourceHandle, Sampler )
#undef WP_MATERIAL_SET
#define WP_MATERIAL_GET(Name, ValueType, Kind) \
    ValueType MaterialShaderParametersInstance::Name( MaterialShaderParameterHandle h ) const \
    { ValueType value{}; ReadBytes( h, &value, sizeof( value ), MaterialShaderParameterType::Kind ); return value; }
    WP_MATERIAL_GET( GetScalar, f32, Scalar )
    WP_MATERIAL_GET( GetInt, s32, Int )
    WP_MATERIAL_GET( GetUInt, u32, UInt )
    WP_MATERIAL_GET( GetColour, ColourF, Colour )
    WP_MATERIAL_GET( GetVector2f, Vector2<f32>, Vector2 )
    WP_MATERIAL_GET( GetVector4f, Vector4<f32>, Vector4 )
    WP_MATERIAL_GET( GetMatrix, Matrix4<f32>, Matrix )
#undef WP_MATERIAL_GET
    bool MaterialShaderParametersInstance::SetBool( MaterialShaderParameterHandle h, bool value )
    { const u32 packed = value ? 1u : 0u; return WriteBytes( h, &packed, sizeof( packed ), MaterialShaderParameterType::Bool ); }
    bool MaterialShaderParametersInstance::GetBool( MaterialShaderParameterHandle h ) const
    { u32 value = 0; ReadBytes( h, &value, sizeof( value ), MaterialShaderParameterType::Bool ); return value != 0; }
    MaterialResourceHandle MaterialShaderParametersInstance::GetResource( MaterialShaderParameterHandle h ) const
    {
        auto value = InvalidMaterialResourceHandle;
        if( m_layout && h.m_parameterIndex < GetParameterInfo().size() )
        {
            const auto type = GetParameterInfo()[h.m_parameterIndex].m_type;
            if( type == MaterialShaderParameterType::Texture || type == MaterialShaderParameterType::Buffer ||
                type == MaterialShaderParameterType::Sampler ) ReadBytes( h, &value, sizeof( value ), type );
        }
        return value;
    }
    void MaterialShaderParametersInstance::InitializeDefaultResourceHandles()
    {
        auto edit = Edit();
        for( u32 i = 0; i < GetParameterInfo().size(); ++i )
        {
            const auto type = GetParameterInfo()[i].m_type;
            if( type == MaterialShaderParameterType::Texture || type == MaterialShaderParameterType::Buffer ||
                type == MaterialShaderParameterType::Sampler )
                WriteBytes( GetParameter( i ), &InvalidMaterialResourceHandle, sizeof( MaterialResourceHandle ), type );
        }
    }
    bool MaterialShaderParametersInstance::ValidateResourceHandles() const
    {
        for( u32 i = 0; i < GetParameterInfo().size(); ++i )
        {
            const auto type = GetParameterInfo()[i].m_type;
            if( ( type == MaterialShaderParameterType::Texture || type == MaterialShaderParameterType::Buffer ||
                  type == MaterialShaderParameterType::Sampler ) &&
                GetResource( GetParameter( i ) ) == InvalidMaterialResourceHandle ) return false;
        }
        return true;
    }
}
