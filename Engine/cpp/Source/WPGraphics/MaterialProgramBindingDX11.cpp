#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/MaterialProgramBinding.hpp>

#if defined( WP_PLATFORM_WIN32 ) && WP_BUILD_RENDERER_DX11
#    include <d3d11.h>
#    include <d3d11shader.h>
#    include <d3dcompiler.h>
#    include <cstring>
#    include <memory>
#endif

namespace workphone::render
{
#if defined( WP_PLATFORM_WIN32 ) && WP_BUILD_RENDERER_DX11
    namespace
    {
        struct ReleaseReflection
        {
            void operator()( ID3D11ShaderReflection *reflection ) const
            {
                if( reflection )
                {
                    reflection->Release();
                }
            }
        };

        bool matchesStage( UINT version, wp_shader_type stage )
        {
            switch( stage )
            {
            case WORKPHONE_SHADER_TYPE_VERTEX:
                return D3D11_SHVER_GET_TYPE( version ) == D3D11_SHVER_VERTEX_SHADER;
            case WORKPHONE_SHADER_TYPE_PIXEL:
                return D3D11_SHVER_GET_TYPE( version ) == D3D11_SHVER_PIXEL_SHADER;
            case WORKPHONE_SHADER_TYPE_GEOMETRY:
                return D3D11_SHVER_GET_TYPE( version ) == D3D11_SHVER_GEOMETRY_SHADER;
            case WORKPHONE_SHADER_TYPE_HULL:
                return D3D11_SHVER_GET_TYPE( version ) == D3D11_SHVER_HULL_SHADER;
            case WORKPHONE_SHADER_TYPE_DOMAIN:
                return D3D11_SHVER_GET_TYPE( version ) == D3D11_SHVER_DOMAIN_SHADER;
            case WORKPHONE_SHADER_TYPE_COMPUTE:
                return D3D11_SHVER_GET_TYPE( version ) == D3D11_SHVER_COMPUTE_SHADER;
            default:
                return false;
            }
        }

        bool validName( const char *name )
        {
            return name && name[0] && std::strlen( name ) < WP_SHADER_MAX_NAME;
        }

        bool containsName( const Array<String> &names, const char *name )
        {
            for( const auto &selected : names )
            {
                if( selected == name )
                {
                    return true;
                }
            }
            return false;
        }

        bool validSelection( ID3D11ShaderReflection &reflection, UINT boundResources,
                             const Array<String> &names, bool constantBuffers )
        {
            const size_t capacity = constantBuffers ? D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT
                                                    : WP_SHADER_MAX_PARAMS;
            if( names.size() > capacity )
            {
                return false;
            }
            for( size_t i = 0; i < names.size(); ++i )
            {
                const auto &name = names[i];
                if( !validName( name.c_str() ) || std::strlen( name.c_str() ) != name.size() )
                {
                    return false;
                }
                for( size_t j = 0; j < i; ++j )
                {
                    if( names[j] == name )
                    {
                        return false;
                    }
                }
                UINT matches = 0;
                for( UINT j = 0; j < boundResources; ++j )
                {
                    D3D11_SHADER_INPUT_BIND_DESC input{};
                    if( FAILED( reflection.GetResourceBindingDesc( j, &input ) ) || !input.Name ||
                        !input.Name[0] )
                    {
                        return false;
                    }
                    if( name == input.Name )
                    {
                        if( ( input.Type == D3D_SIT_CBUFFER ) != constantBuffers )
                        {
                            return false;
                        }
                        ++matches;
                    }
                }
                // An optimized-away or misspelled request must not silently
                // become an empty map. Duplicate reflected names are ambiguous.
                if( matches != 1 )
                {
                    return false;
                }
            }
            return true;
        }

        bool validBindingRange( const D3D11_SHADER_INPUT_BIND_DESC &input )
        {
            UINT limit = 0;
            switch( input.Type )
            {
            case D3D_SIT_CBUFFER:
                limit = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
                break;
            case D3D_SIT_SAMPLER:
                limit = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
                break;
            case D3D_SIT_TEXTURE:
            case D3D_SIT_TBUFFER:
            case D3D_SIT_STRUCTURED:
            case D3D_SIT_BYTEADDRESS:
                limit = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
                break;
            case D3D_SIT_UAV_RWTYPED:
            case D3D_SIT_UAV_RWSTRUCTURED:
            case D3D_SIT_UAV_RWBYTEADDRESS:
            case D3D_SIT_UAV_APPEND_STRUCTURED:
            case D3D_SIT_UAV_CONSUME_STRUCTURED:
            case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                limit = 64;  // DX11.1 maximum, independently owned UAVs may be unselected.
                break;
            default:
                return false;
            }
            return input.BindCount && input.BindPoint < limit &&
                   input.BindCount <= limit - input.BindPoint;
        }

        bool fieldType( const D3D11_SHADER_TYPE_DESC &type, MaterialProgramField &field )
        {
            // Array[1] still has Elements == 1. It cannot be represented by the
            // current scalar/vector/matrix CPU schema, just like larger arrays.
            if( type.Elements || type.Members )
            {
                return false;
            }
            if( type.Class == D3D_SVC_SCALAR && type.Rows == 1 && type.Columns == 1 )
            {
                switch( type.Type )
                {
                case D3D_SVT_FLOAT:
                    field.type = MaterialShaderParameterType::Scalar;
                    break;
                case D3D_SVT_INT:
                    field.type = MaterialShaderParameterType::Int;
                    break;
                case D3D_SVT_UINT:
                    field.type = MaterialShaderParameterType::UInt;
                    break;
                case D3D_SVT_BOOL:
                    field.type = MaterialShaderParameterType::Bool;
                    break;
                default:
                    return false;
                }
                field.size = 4;
                return true;
            }
            if( type.Class == D3D_SVC_VECTOR && type.Rows == 1 &&
                ( type.Columns == 2 || type.Columns == 4 ) )
            {
                if( type.Type == D3D_SVT_FLOAT )
                {
                    field.type = type.Columns == 2 ? MaterialShaderParameterType::Vector2
                                                   : MaterialShaderParameterType::Vector4;
                }
                else if( type.Type == D3D_SVT_INT )
                {
                    field.type = type.Columns == 2 ? MaterialShaderParameterType::IntVector2
                                                   : MaterialShaderParameterType::IntVector4;
                }
                else
                {
                    return false;
                }
                field.size = type.Columns * 4;
                return true;
            }
            if( ( type.Class == D3D_SVC_MATRIX_ROWS || type.Class == D3D_SVC_MATRIX_COLUMNS ) &&
                type.Type == D3D_SVT_FLOAT && type.Rows == 4 && type.Columns == 4 )
            {
                field.type = MaterialShaderParameterType::Matrix;
                field.columnMajor = type.Class == D3D_SVC_MATRIX_COLUMNS;
                field.size = 64;
                return true;
            }
            return false;
        }

        bool textureDimension( D3D_SRV_DIMENSION dimension )
        {
            switch( dimension )
            {
            case D3D_SRV_DIMENSION_TEXTURE1D:
            case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
            case D3D_SRV_DIMENSION_TEXTURE2D:
            case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
            case D3D_SRV_DIMENSION_TEXTURE2DMS:
            case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
            case D3D_SRV_DIMENSION_TEXTURE3D:
            case D3D_SRV_DIMENSION_TEXTURECUBE:
            case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
                return true;
            default:
                return false;
            }
        }

        bool bufferDimension( D3D_SRV_DIMENSION dimension )
        {
            return dimension == D3D_SRV_DIMENSION_BUFFER || dimension == D3D_SRV_DIMENSION_BUFFEREX;
        }

        MaterialProgramBindingMap::Ptr rejectReflection( String &error, const char *message )
        {
            error = message;
            return {};
        }
    }  // namespace
#endif

    MaterialProgramBindingMap::Ptr CreateDX11MaterialProgramBindingMap(
        MaterialShaderParameterLayout::Ptr layout, wp_shader_program *program, u64 deviceGeneration,
        wp_shader_type stage, const void *bytecode, size_t bytecodeSize, String &error,
        const MaterialProgramReflectionSelection *selection )
    {
#if defined( WP_PLATFORM_WIN32 ) && WP_BUILD_RENDERER_DX11
        error.clear();
        if( !bytecode || bytecodeSize < 4 || std::memcmp( bytecode, "DXBC", 4 ) != 0 )
        {
            return rejectReflection( error, "A compiled DXBC shader container is required." );
        }

        ID3D11ShaderReflection *nativeReflection = nullptr;
        const auto reflected = D3DReflect( bytecode, bytecodeSize, IID_ID3D11ShaderReflection,
                                           reinterpret_cast<void **>( &nativeReflection ) );
        std::unique_ptr<ID3D11ShaderReflection, ReleaseReflection> reflection( nativeReflection );
        if( FAILED( reflected ) || !reflection )
        {
            return rejectReflection( error, "DXBC shader reflection failed." );
        }

        D3D11_SHADER_DESC shader{};
        if( FAILED( reflection->GetDesc( &shader ) ) || !matchesStage( shader.Version, stage ) ||
            D3D11_SHVER_GET_MAJOR( shader.Version ) < 4 || D3D11_SHVER_GET_MAJOR( shader.Version ) > 5 ||
            ( D3D11_SHVER_GET_MAJOR( shader.Version ) == 5 &&
              D3D11_SHVER_GET_MINOR( shader.Version ) != 0 ) )
        {
            return rejectReflection( error, "DXBC shader stage or shader model does not match DX11." );
        }
        if( reflection->GetNumInterfaceSlots() )
        {
            return rejectReflection( error, "Dynamically linked shader interfaces are not supported." );
        }
        constexpr UINT maxResources = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT +
                                      D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT +
                                      D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
        // Unselected texture buffers and UAVs remain owned by other adapters.
        const UINT maxBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT +
                                ( selection ? D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT : 0 );
        if( shader.ConstantBuffers > maxBuffers ||
            shader.BoundResources > maxResources + ( selection ? 64u : 0u ) )
        {
            return rejectReflection( error, "DXBC resource counts exceed DX11 binding limits." );
        }
        if( selection &&
            ( !validSelection( *reflection, shader.BoundResources, selection->constantBuffers, true ) ||
              !validSelection( *reflection, shader.BoundResources, selection->resources, false ) ) )
        {
            return rejectReflection(
                error,
                "Reflection selection has invalid, duplicate, missing or wrongly classified names." );
        }

        Array<MaterialProgramBuffer> buffers;
        Array<MaterialProgramField> fields;
        Array<MaterialProgramResource> resources;
        buffers.reserve( selection ? selection->constantBuffers.size() : shader.ConstantBuffers );
        UINT boundBuffers = 0;
        for( UINT i = 0; i < shader.BoundResources; ++i )
        {
            D3D11_SHADER_INPUT_BIND_DESC input{};
            if( FAILED( reflection->GetResourceBindingDesc( i, &input ) ) || !input.Name ||
                !input.Name[0] || !validBindingRange( input ) )
            {
                return rejectReflection( error,
                                         "Invalid reflected resource binding or register range." );
            }
            const bool constantBuffer = input.Type == D3D_SIT_CBUFFER;
            if( selection &&
                !containsName( constantBuffer ? selection->constantBuffers : selection->resources,
                               input.Name ) )
            {
                continue;
            }
            if( !validName( input.Name ) || input.BindCount != 1 )
            {
                return rejectReflection( error, "Selected resource arrays are not supported." );
            }
            if( constantBuffer )
            {
                if( input.BindPoint >= D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT )
                {
                    return rejectReflection( error,
                                             "Reflected constant-buffer register is out of range." );
                }
                ++boundBuffers;
                continue;
            }

            MaterialProgramResource resource;
            resource.name = input.Name;
            resource.slot = input.BindPoint;
            if( input.Type == D3D_SIT_SAMPLER )
            {
                resource.type = MaterialShaderParameterType::Sampler;
                if( resource.slot >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT )
                {
                    return rejectReflection( error, "Reflected sampler register is out of range." );
                }
            }
            else if( input.Type == D3D_SIT_TEXTURE )
            {
                if( textureDimension( input.Dimension ) )
                {
                    resource.type = MaterialShaderParameterType::Texture;
                }
                else if( bufferDimension( input.Dimension ) )
                {
                    resource.type = MaterialShaderParameterType::Buffer;
                }
                else
                {
                    return rejectReflection( error, "Unsupported reflected texture dimension." );
                }
            }
            else if( ( input.Type == D3D_SIT_STRUCTURED || input.Type == D3D_SIT_BYTEADDRESS ) &&
                     bufferDimension( input.Dimension ) )
            {
                resource.type = MaterialShaderParameterType::Buffer;
            }
            else
            {
                return rejectReflection(
                    error, "UAVs, texture buffers and unsupported resources are not supported." );
            }
            if( resource.type != MaterialShaderParameterType::Sampler &&
                resource.slot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT )
            {
                return rejectReflection( error, "Reflected shader-resource register is out of range." );
            }

            // Most DXBC resources only expose BindCount. When variable metadata
            // survives compilation, also reject a one-element resource array.
            auto *variable = reflection->GetVariableByName( input.Name );
            D3D11_SHADER_VARIABLE_DESC variableInfo{};
            if( variable && SUCCEEDED( variable->GetDesc( &variableInfo ) ) )
            {
                auto *variableType = variable->GetType();
                D3D11_SHADER_TYPE_DESC type{};
                if( !variableType || FAILED( variableType->GetDesc( &type ) ) || type.Elements ||
                    type.Members )
                {
                    return rejectReflection(
                        error, "Reflected resource arrays or structures are not supported." );
                }
            }
            resources.push_back( std::move( resource ) );
        }
        UINT reflectedBuffers = 0;
        for( UINT i = 0; i < shader.ConstantBuffers; ++i )
        {
            auto *buffer = reflection->GetConstantBufferByIndex( i );
            D3D11_SHADER_BUFFER_DESC bufferInfo{};
            if( !buffer || FAILED( buffer->GetDesc( &bufferInfo ) ) || !bufferInfo.Name ||
                !bufferInfo.Name[0] )
            {
                return rejectReflection( error, "Invalid reflected constant-buffer descriptor." );
            }
            if( selection && !containsName( selection->constantBuffers, bufferInfo.Name ) )
            {
                continue;
            }
            ++reflectedBuffers;
            if( !validName( bufferInfo.Name ) || bufferInfo.Type != D3D_CT_CBUFFER || !bufferInfo.Size ||
                bufferInfo.Size % 16 || bufferInfo.Size > D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16 ||
                bufferInfo.Variables > WP_SHADER_MAX_PARAMS ||
                fields.size() + bufferInfo.Variables + resources.size() > WP_SHADER_MAX_PARAMS )
            {
                return rejectReflection( error,
                                         "Invalid or unsupported reflected constant-buffer layout." );
            }
            D3D11_SHADER_INPUT_BIND_DESC input{};
            if( FAILED( reflection->GetResourceBindingDescByName( bufferInfo.Name, &input ) ) ||
                input.Type != D3D_SIT_CBUFFER || input.BindCount != 1 ||
                input.BindPoint >= D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT )
            {
                return rejectReflection(
                    error, "Reflected constant-buffer binding does not match its layout." );
            }
            const auto bufferIndex = static_cast<u32>( buffers.size() );
            buffers.push_back( { input.BindPoint, bufferInfo.Size, 0 } );
            for( UINT j = 0; j < bufferInfo.Variables; ++j )
            {
                auto *variable = buffer->GetVariableByIndex( j );
                D3D11_SHADER_VARIABLE_DESC variableInfo{};
                if( !variable || FAILED( variable->GetDesc( &variableInfo ) ) ||
                    !validName( variableInfo.Name ) || variableInfo.StartOffset % 4 ||
                    variableInfo.StartOffset > bufferInfo.Size ||
                    variableInfo.Size > bufferInfo.Size - variableInfo.StartOffset )
                {
                    return rejectReflection( error, "Invalid reflected constant-buffer variable." );
                }
                auto *variableType = variable->GetType();
                D3D11_SHADER_TYPE_DESC type{};
                MaterialProgramField field;
                field.name = variableInfo.Name;
                field.bufferIndex = bufferIndex;
                field.offset = variableInfo.StartOffset;
                if( !variableType || FAILED( variableType->GetDesc( &type ) ) ||
                    !fieldType( type, field ) || field.size != variableInfo.Size )
                {
                    return rejectReflection( error,
                                             "Unsupported reflected field type, shape or array." );
                }
                fields.push_back( std::move( field ) );
            }
        }
        if( reflectedBuffers != boundBuffers )
        {
            return rejectReflection(
                error, "Every selected constant buffer must have one reflected layout and binding." );
        }
        return MaterialProgramBindingMap::Create( std::move( layout ), program, deviceGeneration, stage,
                                                  buffers, fields, resources, error );
#else
        (void)layout;
        (void)program;
        (void)deviceGeneration;
        (void)stage;
        (void)bytecode;
        (void)bytecodeSize;
        (void)selection;
        error = "DXBC reflection requires the Windows DX11 renderer.";
        return {};
#endif
    }
}  // namespace workphone::render
