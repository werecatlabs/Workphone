#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/MaterialProgramBinding.hpp>
#include <cstring>
#include <cstdint>
#include <limits>

namespace workphone::render
{
    namespace
    {
        using Type = MaterialShaderParameterType;

        bool isResource( Type type )
        {
            return type == Type::Texture || type == Type::Buffer || type == Type::Sampler;
        }

        bool validProgramName( const String &name )
        {
            return !name.empty() && name.size() < WP_SHADER_MAX_NAME &&
                std::strlen( name.c_str() ) == name.size();
        }

        wp_uniform_type uniformType( Type type )
        {
            switch( type )
            {
            case Type::Scalar: return WORKPHONE_UNIFORM_TYPE_FLOAT;
            case Type::Colour:
            case Type::Vector4: return WORKPHONE_UNIFORM_TYPE_VEC4;
            case Type::Vector2: return WORKPHONE_UNIFORM_TYPE_VEC2;
            case Type::Matrix: return WORKPHONE_UNIFORM_TYPE_MAT4;
            case Type::Int: return WORKPHONE_UNIFORM_TYPE_INT;
            case Type::UInt: return WORKPHONE_UNIFORM_TYPE_UINT;
            case Type::Bool: return WORKPHONE_UNIFORM_TYPE_BOOL;
            case Type::IntVector2: return WORKPHONE_UNIFORM_TYPE_IVEC2;
            case Type::IntVector4: return WORKPHONE_UNIFORM_TYPE_IVEC4;
            default: return static_cast<wp_uniform_type>( -1 );
            }
        }

        wp_shader_resource_kind resourceKind( Type type )
        {
            switch( type )
            {
            case Type::Texture: return WORKPHONE_SHADER_RESOURCE_TEXTURE;
            case Type::Buffer: return WORKPHONE_SHADER_RESOURCE_BUFFER;
            case Type::Sampler: return WORKPHONE_SHADER_RESOURCE_SAMPLER;
            default: return WORKPHONE_SHADER_RESOURCE_NONE;
            }
        }

        // Pointer arithmetic and comparisons across unrelated allocations use
        // integer ranges; reject address overflow before any output mutation.
        bool validRange( const void *pointer, size_t size )
        {
            return size == 0 || ( pointer && reinterpret_cast<uintptr_t>( pointer ) <=
                std::numeric_limits<uintptr_t>::max() - size );
        }

        bool overlaps( const void *a, size_t aSize, const void *b, size_t bSize )
        {
            if( !aSize || !bSize ) return false;
            const auto x = reinterpret_cast<uintptr_t>( a );
            const auto y = reinterpret_cast<uintptr_t>( b );
            return x < y + bSize && y < x + aSize;
        }
    }

    MaterialProgramBindingMap::Ptr MaterialProgramBindingMap::Create(
        MaterialShaderParameterLayout::Ptr layout, wp_shader_program *program,
        u64 deviceGeneration, wp_shader_type stage,
        const Array<MaterialProgramBuffer> &buffers, const Array<MaterialProgramField> &fields,
        const Array<MaterialProgramResource> &resources, String &error )
    {
        error.clear();
        auto fail = [&]( const String &reason ) -> Ptr { error = reason; return {}; };
        if( !layout || !program || !deviceGeneration || stage < WORKPHONE_SHADER_TYPE_VERTEX ||
            stage > WORKPHONE_SHADER_TYPE_COMPUTE )
            return fail( "Invalid material layout, program, device generation or shader stage" );
        if( buffers.size() > 14 || fields.size() > WP_SHADER_MAX_PARAMS ||
            resources.size() > WP_SHADER_MAX_PARAMS )
            return fail( "Material binding map exceeds backend parameter capacity" );

        auto map = std::allocate_shared<MaterialProgramBindingMap>(
            SourceBudgetAllocator<MaterialProgramBindingMap>{} );
        map->m_layout = std::move( layout );
        map->m_deviceGeneration = deviceGeneration;
        map->m_stage = stage;
        map->m_buffers.reserve( buffers.size() );
        map->m_copies.reserve( fields.size() );
        map->m_resources.reserve( resources.size() );
        for( auto buffer : buffers )
        {
            if( buffer.slot >= 14 || !buffer.size || buffer.size > 65536 || buffer.size % 16 )
                return fail( "Invalid constant buffer size or register" );
            for( const auto &existing : map->m_buffers )
                if( existing.slot == buffer.slot ) return fail( "Duplicate constant buffer register" );
            buffer.packedOffset = map->m_packedSize;
            // Bounded above by fourteen 64KiB buffers.
            map->m_packedSize += buffer.size;
            map->m_buffers.push_back( buffer );
        }
        const auto &info = map->m_layout->GetParameterInfo();
        for( const auto &field : fields )
        {
            if( !validProgramName( field.name ) || field.bufferIndex >= map->m_buffers.size() || isResource( field.type ) ||
                !GetMaterialShaderParameterStride( field.type ) )
                return fail( "Invalid constant field: " + field.name );
            const auto &buffer = map->m_buffers[field.bufferIndex];
            const auto size = GetMaterialShaderParameterStride( field.type );
            if( field.size != size || field.offset > buffer.size || size > buffer.size - field.offset ||
                field.offset % 4 || ( field.columnMajor && field.type != Type::Matrix ) ||
                ( field.type == Type::Matrix ? field.offset % 16 != 0 : field.offset % 16 + size > 16 ) )
                return fail( "Invalid reflected constant range or shape: " + field.name );
            const auto source = map->m_layout->FindParameter( field.name );
            if( !source.IsValid() ) return fail( "Missing material constant: " + field.name );
            const auto sourceType = info[source.m_parameterIndex].m_type;
            if( sourceType != field.type && !( sourceType == Type::Colour && field.type == Type::Vector4 ) )
                return fail( "Material constant type differs from reflection: " + field.name );
            const auto handle = wp_shader_program_find_param( program,
                reinterpret_cast<const wp_c8 *>( field.name.c_str() ) );
            const auto *definition = wp_shader_program_get_param_def( program,
                static_cast<wp_s32>( handle.parameter_index ) );
            if( !wp_shader_program_validate_param_handle( program, handle ) || !definition ||
                definition->type != uniformType( field.type ) || definition->array_size != 1 ||
                definition->resource_kind != WORKPHONE_SHADER_RESOURCE_NONE )
                return fail( "Program constant definition differs from reflection: " + field.name );
            const u32 destination = buffer.packedOffset + field.offset;
            for( const auto &copy : map->m_copies )
                if( destination < copy.destination + copy.size && copy.destination < destination + size )
                    return fail( "Overlapping reflected constants: " + field.name );
            map->m_copies.push_back( { source.m_offsetInBytes, destination, size, field.columnMajor } );
        }
        for( const auto &resource : resources )
        {
            if( !validProgramName( resource.name ) || !isResource( resource.type ) ||
                resource.slot >= ( resource.type == Type::Sampler ? 16u : 128u ) )
                return fail( "Invalid resource kind or register: " + resource.name );
            const auto source = map->m_layout->FindParameter( resource.name );
            if( !source.IsValid() || info[source.m_parameterIndex].m_type != resource.type )
                return fail( "Missing material resource or mismatched kind: " + resource.name );
            const auto handle = wp_shader_program_find_param( program,
                reinterpret_cast<const wp_c8 *>( resource.name.c_str() ) );
            const auto *definition = wp_shader_program_get_param_def( program,
                static_cast<wp_s32>( handle.parameter_index ) );
            if( !wp_shader_program_validate_param_handle( program, handle ) || !definition ||
                definition->array_size != 1 || definition->resource_kind != resourceKind( resource.type ) )
                return fail( "Program resource definition differs from reflection: " + resource.name );
            for( const auto &existing : map->m_resources )
                if( existing.slot == resource.slot &&
                    ( existing.type == Type::Sampler ) == ( resource.type == Type::Sampler ) )
                    return fail( "Overlapping resource registers: " + resource.name );
            map->m_resources.push_back( { source.m_offsetInBytes, resource.slot, resource.type } );
        }
        // Retain only after candidate validation; neither shared program values
        // nor any previous map is modified while building a replacement.
        wp_shader_program_retain( program );
        map->m_program = program;
        map->m_definitionRevision = wp_shader_program_get_layout_revision( program );
        map->m_pipelineRevision = wp_shader_program_get_pipeline_revision( program );
        map->m_linkedGeneration = wp_shader_program_get_linked_generation( program );
        return map;
    }

    MaterialProgramBindingMap::~MaterialProgramBindingMap()
    {
        wp_shader_program_destroy( m_program );
    }

    bool MaterialProgramBindingMap::IsCurrent( const MaterialShaderParameterLayout::Ptr &layout,
        const wp_shader_program *program, u64 deviceGeneration ) const
    {
        return m_program && program == m_program && layout == m_layout &&
            deviceGeneration == m_deviceGeneration &&
            wp_shader_program_get_layout_revision( program ) == m_definitionRevision &&
            wp_shader_program_get_pipeline_revision( program ) == m_pipelineRevision &&
            wp_shader_program_get_linked_generation( program ) == m_linkedGeneration;
    }

    bool MaterialProgramBindingMap::Pack( const MaterialShaderParametersInstance &instance,
        u64 deviceGeneration, void *bytes, size_t byteCapacity,
        MaterialProgramResourceValue *resources, size_t resourceCapacity ) const
    {
        if( !IsCurrent( instance.GetLayout(), m_program, deviceGeneration ) ||
            byteCapacity < m_packedSize || resourceCapacity < m_resources.size() ) return false;
        const auto &source = instance.GetParameterMemory();
        const size_t resourceBytes = m_resources.size() * sizeof( MaterialProgramResourceValue );
        if( source.size() != m_layout->GetDefaults().size() ||
            !validRange( bytes, m_packedSize ) || !validRange( resources, resourceBytes ) ||
            !validRange( source.data(), source.size() ) ||
            overlaps( bytes, m_packedSize, resources, resourceBytes ) ||
            overlaps( bytes, m_packedSize, source.data(), source.size() ) ||
            overlaps( resources, resourceBytes, source.data(), source.size() ) ) return false;
        if( m_packedSize ) std::memset( bytes, 0, m_packedSize );
        auto *destination = static_cast<u8 *>( bytes );
        for( const auto &copy : m_copies )
        {
            if( copy.transpose )
            {
                for( u32 row = 0; row < 4; ++row )
                    for( u32 column = 0; column < 4; ++column )
                        std::memcpy( destination + copy.destination + ( row * 4 + column ) * 4,
                            source.data() + copy.source + ( column * 4 + row ) * 4, 4 );
            }
            else std::memcpy( destination + copy.destination, source.data() + copy.source, copy.size );
        }
        for( size_t i = 0; i < m_resources.size(); ++i )
        {
            const auto &binding = m_resources[i];
            MaterialResourceHandle handle;
            std::memcpy( &handle, source.data() + binding.source, sizeof( handle ) );
            resources[i] = { binding.type, binding.slot, handle };
        }
        return true;
    }

    const MaterialProgramBuffer *MaterialProgramBindingMap::GetBuffer( size_t index ) const
    {
        return index < m_buffers.size() ? &m_buffers[index] : nullptr;
    }

    size_t MaterialProgramBindingMap::GetStorageBytes() const
    {
        return sizeof( *this ) + m_buffers.capacity() * sizeof( MaterialProgramBuffer ) +
            m_copies.capacity() * sizeof( Copy ) + m_resources.capacity() * sizeof( Resource );
    }
}
