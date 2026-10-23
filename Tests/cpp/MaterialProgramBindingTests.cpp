#include <WPGraphics/MaterialProgramBinding.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "DX11TestEvidence.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

using namespace workphone;
using namespace workphone::render;
using Microsoft::WRL::ComPtr;

namespace
{
    using Type = MaterialShaderParameterType;
    using Program = std::unique_ptr<wp_shader_program, decltype( &wp_shader_program_destroy )>;
    void require( bool condition, const char *message )
    { if( !condition ) throw std::runtime_error( message ); }
    void succeeded( HRESULT result, const char *message )
    { require( SUCCEEDED( result ), message ); }
    const wp_c8 *nativeText( const char *text )
    { return reinterpret_cast<const wp_c8 *>( text ); }

    wp_uniform_type uniformType( Type type )
    {
        switch( type )
        {
        case Type::Scalar: return WORKPHONE_UNIFORM_TYPE_FLOAT;
        case Type::Int: return WORKPHONE_UNIFORM_TYPE_INT;
        case Type::UInt: return WORKPHONE_UNIFORM_TYPE_UINT;
        case Type::Bool: return WORKPHONE_UNIFORM_TYPE_BOOL;
        case Type::Vector2: return WORKPHONE_UNIFORM_TYPE_VEC2;
        case Type::Vector4: case Type::Colour: return WORKPHONE_UNIFORM_TYPE_VEC4;
        case Type::IntVector2: return WORKPHONE_UNIFORM_TYPE_IVEC2;
        case Type::IntVector4: return WORKPHONE_UNIFORM_TYPE_IVEC4;
        case Type::Matrix: return WORKPHONE_UNIFORM_TYPE_MAT4;
        default: return WORKPHONE_UNIFORM_TYPE_INT;
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
    Program programFor( const MaterialShaderParameterLayout::Ptr &layout )
    {
        Program program( wp_shader_program_create(), &wp_shader_program_destroy );
        require( bool( program ), "native program allocates" );
        for( const auto &info : layout->GetParameterInfo() )
            require( wp_shader_program_register_param( program.get(), nativeText( info.m_name.c_str() ),
                uniformType( info.m_type ), 1, WORKPHONE_SHADER_PARAM_VARIABILITY_PER_OBJECT,
                WORKPHONE_SHADER_AUTO_PARAM_NONE, resourceKind( info.m_type ) ) >= 0,
                "every canonical kind has an exact native parameter definition" );
        return program;
    }
    Array<MaterialShaderParameterInfo> mixedInfo( bool resources = true )
    {
        Array<MaterialShaderParameterInfo> result = {
            {Type::Scalar, "Factor"}, {Type::UInt, "Mask"}, {Type::Bool, "Enabled"},
            {Type::Int, "Count"}, {Type::Vector2, "Uv"}, {Type::Vector4, "Tint"},
            {Type::Colour, "Color"}, {Type::IntVector2, "Pair"}, {Type::IntVector4, "Flags"},
            {Type::Matrix, "Row"}, {Type::Matrix, "Column"} };
        if( resources )
        {
            result.emplace_back( Type::Texture, "Albedo" );
            result.emplace_back( Type::Buffer, "Data" );
            result.emplace_back( Type::Sampler, "Filter" );
        }
        return result;
    }
    Matrix4F asymmetricMatrix()
    {
        auto matrix = Matrix4F::identity();
        for( u32 index = 0; index < 16; ++index ) matrix.ptr()[index] = float( index + 1 );
        return matrix;
    }
    MaterialShaderParametersInstance valuesFor( MaterialShaderParameterLayout::Ptr layout )
    {
        MaterialShaderParametersInstance values;
        values.Reset( std::move( layout ) );
        require( values.SetScalar( values.FindParameter( "Factor" ), 0.5f ) &&
            values.SetScalar( values.FindParameter( "Mask" ), u32( 0xf0000000u ) ) &&
            values.SetBool( values.FindParameter( "Enabled" ), true ) &&
            values.SetScalar( values.FindParameter( "Count" ), s32( -7 ) ) &&
            values.SetVector2( values.FindParameter( "Uv" ), Vector2F( .25f, .75f ) ) &&
            values.SetVector4( values.FindParameter( "Tint" ), Vector4F( .4f, .8f, 1.2f, 2.f ) ) &&
            values.SetColour( values.FindParameter( "Color" ), ColourF( .1f, .2f, .3f, .4f ) ) &&
            values.SetVector2( values.FindParameter( "Pair" ), Vector2<s32>( -3, 9 ) ) &&
            values.SetVector4( values.FindParameter( "Flags" ), Vector4<s32>( -1, 2, -3, 4 ) ) &&
            values.SetMatrix( values.FindParameter( "Row" ), asymmetricMatrix() ) &&
            values.SetMatrix( values.FindParameter( "Column" ), asymmetricMatrix() ),
            "test values exercise exact numeric types and asymmetric matrices" );
        if( values.FindParameter( "Albedo" ).IsValid() )
            require( values.SetTexture( values.FindParameter( "Albedo" ), 0x100000001ull ) &&
                values.SetBuffer( values.FindParameter( "Data" ), 0x200000002ull ) &&
                values.SetSampler( values.FindParameter( "Filter" ), 0x300000003ull ),
                "resources preserve all 64 handle bits" );
        return values;
    }
    template <class T> void store( std::vector<u8> &bytes, size_t offset, const T &value )
    {
        require( offset <= bytes.size() && sizeof( value ) <= bytes.size() - offset,
                 "reference packing remains bounded" );
        std::memcpy( bytes.data() + offset, &value, sizeof( value ) );
    }
    void storeMatrix( std::vector<u8> &bytes, size_t offset, bool transpose )
    {
        const auto matrix = asymmetricMatrix();
        for( size_t row = 0; row < 4; ++row ) for( size_t column = 0; column < 4; ++column )
            store( bytes, offset + 4 * ( row * 4 + column ),
                   matrix.ptr()[transpose ? column * 4 + row : row * 4 + column] );
    }
    struct Recipe
    {
        Array<MaterialProgramBuffer> buffers = {{5, 128, 999}, {2, 160, 999}};
        Array<MaterialProgramField> fields = {
            {"Enabled", Type::Bool, 0, 0, 4}, {"Count", Type::Int, 0, 4, 4},
            {"Factor", Type::Scalar, 0, 8, 4}, {"Mask", Type::UInt, 0, 12, 4},
            {"Pair", Type::IntVector2, 0, 16, 8}, {"Tint", Type::Vector4, 0, 32, 16},
            {"Row", Type::Matrix, 0, 64, 64}, {"Uv", Type::Vector2, 1, 0, 8},
            {"Color", Type::Vector4, 1, 16, 16}, {"Flags", Type::IntVector4, 1, 32, 16},
            {"Column", Type::Matrix, 1, 80, 64, true}};
        Array<MaterialProgramResource> resources = {
            {"Albedo", Type::Texture, 3}, {"Data", Type::Buffer, 6}, {"Filter", Type::Sampler, 1}};
        MaterialProgramBindingMap::Ptr create( MaterialShaderParameterLayout::Ptr layout,
            wp_shader_program *program, String &error, u64 device = 7 ) const
        {
            return MaterialProgramBindingMap::Create( std::move( layout ), program, device,
                WORKPHONE_SHADER_TYPE_PIXEL, buffers, fields, resources, error );
        }
    };
    std::vector<u8> genericExpected()
    {
        std::vector<u8> expected( 288, 0 );
        store( expected, 0, u32( 1 ) ); store( expected, 4, s32( -7 ) );
        store( expected, 8, .5f ); store( expected, 12, u32( 0xf0000000u ) );
        store( expected, 16, s32( -3 ) ); store( expected, 20, s32( 9 ) );
        const std::array<float, 4> tint = {.4f, .8f, 1.2f, 2.f}; store( expected, 32, tint );
        storeMatrix( expected, 64, false );
        store( expected, 128, .25f ); store( expected, 132, .75f );
        const std::array<float, 4> color = {.1f, .2f, .3f, .4f}; store( expected, 144, color );
        const std::array<s32, 4> flags = {-1, 2, -3, 4}; store( expected, 160, flags );
        storeMatrix( expected, 208, true );
        return expected;
    }
    void genericContracts()
    {
        const auto layout = MaterialShaderParameterLayout::Create( mixedInfo() );
        require( bool( layout ), "mixed schema creates" );
        auto program = programFor( layout );
        Recipe recipe; String error;
        auto map = recipe.create( layout, program.get(), error );
        require( map && error.empty() && map->IsCurrent( layout, program.get(), 7 ), "valid map creates" );
        require( map->GetPackedSize() == 288 && map->GetBufferCount() == 2 &&
            map->GetResourceCount() == 3 && map->GetStage() == WORKPHONE_SHADER_TYPE_PIXEL &&
            map->GetStorageBytes() >= sizeof( MaterialProgramBindingMap ), "map reports owned packing storage" );
        require( map->GetBuffer( 0 )->slot == 5 && map->GetBuffer( 0 )->packedOffset == 0 &&
            map->GetBuffer( 1 )->slot == 2 && map->GetBuffer( 1 )->packedOffset == 128 &&
            !map->GetBuffer( 2 ), "buffer ordinal and GPU slot remain distinct" );
        auto a = valuesFor( layout ), b = a;
        require( b.SetScalar( b.FindParameter( "Factor" ), .75f ) &&
            b.SetTexture( b.FindParameter( "Albedo" ), 0xf000000001ull ), "second material edits independently" );
        const auto nativeSize = wp_shader_program_get_param_buffer_size( program.get() );
        const auto nativeData = static_cast<const u8 *>( wp_shader_program_get_param_buffer( program.get() ) );
        const std::vector<u8> nativeBefore( nativeData, nativeData + nativeSize );
        const auto nativeRevision = wp_shader_program_get_parameter_revision( program.get() );
        const auto aLookups = a.GetNameLookupCount(), bLookups = b.GetNameLookupCount();
        std::vector<u8> bytes( map->GetPackedSize() + 16, 0x7b );
        std::array<MaterialProgramResourceValue, 4> resources{};
        resources.back().handle = 0xfeedbeefull;
        const auto expected = genericExpected();
        for( unsigned repeat = 0; repeat < 64; ++repeat )
        {
            for( const auto *values : {&a, &b, &a} )
            {
                require( map->Pack( *values, 7, bytes.data(), bytes.size(), resources.data(), resources.size() ),
                    "A/B/A packing succeeds without shared program writes" );
                auto reference = expected;
                if( values == &b ) store( reference, 8, .75f );
                require( std::equal( reference.begin(), reference.end(), bytes.begin() ),
                    "every packed byte matches independent reference including zero padding" );
                require( std::all_of( bytes.begin() + map->GetPackedSize(), bytes.end(),
                    []( u8 byte ) { return byte == 0x7b; } ) && resources.back().handle == 0xfeedbeefull,
                    "packing preserves caller capacity beyond used output regions" );
                require( resources[0].type == Type::Texture && resources[0].slot == 3 &&
                    resources[0].handle == (values == &b ? 0xf000000001ull : 0x100000001ull) &&
                    resources[1].type == Type::Buffer && resources[1].slot == 6 && resources[1].handle == 0x200000002ull &&
                    resources[2].type == Type::Sampler && resources[2].slot == 1 && resources[2].handle == 0x300000003ull,
                    "resource output contains numeric handles and independent target namespaces" );
            }
        }
        require( a.GetNameLookupCount() == aLookups && b.GetNameLookupCount() == bLookups,
            "warm packing performs zero instance name lookups" );
        require( wp_shader_program_get_parameter_revision( program.get() ) == nativeRevision &&
            std::memcmp( nativeBefore.data(), wp_shader_program_get_param_buffer( program.get() ), nativeSize ) == 0,
            "packing never mutates the native program shared parameter storage" );

        const auto preservedBytes = bytes;
        std::array<u8, sizeof( resources )> preservedResources{};
        std::memcpy( preservedResources.data(), resources.data(), sizeof( resources ) );
        auto preserved = [&] {
            require( bytes == preservedBytes && std::memcmp( resources.data(), preservedResources.data(), sizeof( resources ) ) == 0,
                "failed packing preserves both output regions byte for byte" );
        };
        require( !map->Pack( a, 8, bytes.data(), bytes.size(), resources.data(), resources.size() ), "foreign device rejects" ); preserved();
        require( !map->Pack( a, 7, bytes.data(), map->GetPackedSize() - 1, resources.data(), resources.size() ), "short constants reject" ); preserved();
        require( !map->Pack( a, 7, bytes.data(), bytes.size(), resources.data(), 2 ), "short resources reject" ); preserved();
        require( !map->Pack( a, 7, nullptr, bytes.size(), resources.data(), resources.size() ), "null constants reject" ); preserved();
        require( !map->Pack( a, 7, bytes.data(), bytes.size(), nullptr, resources.size() ), "null resources reject" ); preserved();
        MaterialShaderParametersInstance replacement; replacement.Reset( mixedInfo() );
        require( !map->Pack( replacement, 7, bytes.data(), bytes.size(), resources.data(), resources.size() ) &&
            !map->IsCurrent( replacement.GetLayout(), program.get(), 7 ), "identical fields with another schema identity reject" ); preserved();
        auto other = programFor( layout );
        require( !map->IsCurrent( layout, other.get(), 7 ) && !map->IsCurrent( layout, program.get(), 0 ),
            "foreign program and zero device generation reject" );
        alignas( MaterialProgramResourceValue ) std::array<u8, 512> overlapping;
        overlapping.fill( 0xa5 ); const auto overlapBefore = overlapping;
        require( !map->Pack( a, 7, overlapping.data(), map->GetPackedSize(),
            reinterpret_cast<MaterialProgramResourceValue *>( overlapping.data() + 16 ), 3 ) && overlapping == overlapBefore,
            "overlapping output regions reject before any write" );
        auto invalidResources = a;
        invalidResources.InitializeDefaultResourceHandles();
        require( map->Pack( invalidResources, 7, bytes.data(), bytes.size(), resources.data(), resources.size() ) &&
            resources[0].handle == InvalidMaterialResourceHandle && resources[1].handle == InvalidMaterialResourceHandle &&
            resources[2].handle == InvalidMaterialResourceHandle, "invalid resource sentinels survive for backend readiness checks" );

        auto rejected = [&]( const Recipe &candidate, const char *message ) {
            error.clear(); require( !candidate.create( layout, program.get(), error ) && !error.empty(), message );
            require( map->IsCurrent( layout, program.get(), 7 ), "rejected candidate preserves the usable map" );
        };
        auto bad = recipe; bad.buffers[0].size = 127; rejected( bad, "nonaligned buffer size rejects" );
        bad = recipe; bad.buffers[0].size = 0; rejected( bad, "zero buffer size rejects" );
        bad = recipe; bad.buffers[0].size = 65552; rejected( bad, "oversized DX11 constant buffer rejects" );
        bad = recipe; bad.buffers[0].slot = 14; rejected( bad, "constant buffer slot overflow rejects" );
        bad = recipe; bad.buffers[1].slot = bad.buffers[0].slot; rejected( bad, "duplicate constant buffer slot rejects" );
        bad = recipe; bad.fields[0].bufferIndex = 2; rejected( bad, "foreign buffer ordinal rejects" );
        bad = recipe; bad.fields[0].offset = std::numeric_limits<u32>::max(); rejected( bad, "field offset overflow rejects" );
        bad = recipe; bad.fields[0].offset = 127; rejected( bad, "field out of bounds rejects" );
        bad = recipe; bad.fields[1].offset = 0; rejected( bad, "overlapping destination fields reject" );
        bad = recipe; bad.fields[4].offset = 12; rejected( bad, "vector crossing a reflected register rejects" );
        bad = recipe; bad.fields[0].size = 8; rejected( bad, "reflected scalar width mismatch rejects" );
        bad = recipe; bad.fields[0].type = Type::Int; rejected( bad, "same-sized bool/int confusion rejects" );
        bad = recipe; bad.fields[3].type = Type::Int; rejected( bad, "same-sized unsigned/signed confusion rejects" );
        bad = recipe; bad.fields[0].columnMajor = true; rejected( bad, "transpose flag on scalar rejects" );
        bad = recipe; bad.fields[0].name = "Missing"; rejected( bad, "unresolved reflected name rejects" );
        bad = recipe; bad.fields.push_back( bad.fields.front() ); rejected( bad, "duplicate reflected parameter rejects" );
        bad = recipe; bad.resources[0].type = Type::Buffer; rejected( bad, "resource kind mismatch rejects" );
        bad = recipe; bad.resources[0].slot = 128; rejected( bad, "SRV slot overflow rejects" );
        bad = recipe; bad.resources[2].slot = 16; rejected( bad, "sampler slot overflow rejects" );
        bad = recipe; bad.resources[1].slot = bad.resources[0].slot; rejected( bad, "texture and buffer share one SRV namespace" );
        bad = recipe; bad.resources.push_back( bad.resources.front() ); rejected( bad, "duplicate resource rejects" );
        error.clear();
        require( !MaterialProgramBindingMap::Create( layout, program.get(), 7, static_cast<wp_shader_type>( 99 ),
            recipe.buffers, recipe.fields, recipe.resources, error ) && !error.empty(), "unknown shader stage rejects" );

        // A native definition of equal byte width must still retain signedness/kind.
        for( const auto type : {WORKPHONE_UNIFORM_TYPE_INT, WORKPHONE_UNIFORM_TYPE_FLOAT, WORKPHONE_UNIFORM_TYPE_BOOL} )
        {
            auto one = MaterialShaderParameterLayout::Create( {{Type::UInt, "Mask"}} );
            Program wrong( wp_shader_program_create(), &wp_shader_program_destroy );
            require( wp_shader_program_register_uniform( wrong.get(), nativeText("Mask"), type ) == 0, "wrong-kind fixture registers" );
            error.clear();
            require( !MaterialProgramBindingMap::Create( one, wrong.get(), 7, WORKPHONE_SHADER_TYPE_PIXEL,
                {{0,16}}, {{"Mask",Type::UInt,0,0,4}}, {}, error ) && !error.empty(), "native wrong kind rejects equal size" );
        }
        for( const auto entry : {std::pair<Type, wp_uniform_type>{Type::IntVector2, WORKPHONE_UNIFORM_TYPE_VEC2},
                                std::pair<Type, wp_uniform_type>{Type::IntVector4, WORKPHONE_UNIFORM_TYPE_VEC4}} )
        {
            auto one = MaterialShaderParameterLayout::Create( {{entry.first, "Value"}} );
            Program wrong( wp_shader_program_create(), &wp_shader_program_destroy );
            require( wp_shader_program_register_uniform( wrong.get(), nativeText("Value"), entry.second ) == 0, "wrong-vector fixture registers" );
            error.clear();
            require( !MaterialProgramBindingMap::Create( one, wrong.get(), 7, WORKPHONE_SHADER_TYPE_PIXEL,
                {{0,16}}, {{"Value",entry.first,0,0,GetMaterialShaderParameterStride(entry.first)}}, {}, error ) && !error.empty(),
                "integer vectors cannot use equal-sized float program definitions" );
        }
        {
            const auto one = MaterialShaderParameterLayout::Create( {{Type::Scalar,"Value"}} );
            Program arrayProgram( wp_shader_program_create(), &wp_shader_program_destroy );
            require( wp_shader_program_register_param( arrayProgram.get(), nativeText("Value"), WORKPHONE_UNIFORM_TYPE_FLOAT, 2,
                WORKPHONE_SHADER_PARAM_VARIABILITY_PER_OBJECT, WORKPHONE_SHADER_AUTO_PARAM_NONE, WORKPHONE_SHADER_RESOURCE_NONE ) == 0,
                "native array fixture registers" );
            error.clear();
            require( !MaterialProgramBindingMap::Create( one, arrayProgram.get(), 7, WORKPHONE_SHADER_TYPE_PIXEL,
                {{0,16}}, {{"Value",Type::Scalar,0,0,4}}, {}, error ) && !error.empty(), "array native definition cannot silently truncate" );
        }
        {
            const auto one = MaterialShaderParameterLayout::Create( {{Type::Texture,"Value"}} );
            Program wrong( wp_shader_program_create(), &wp_shader_program_destroy );
            require( wp_shader_program_register_param( wrong.get(), nativeText("Value"), WORKPHONE_UNIFORM_TYPE_INT, 1,
                WORKPHONE_SHADER_PARAM_VARIABILITY_PER_OBJECT, WORKPHONE_SHADER_AUTO_PARAM_NONE, WORKPHONE_SHADER_RESOURCE_BUFFER ) == 0,
                "wrong native resource fixture registers" );
            error.clear();
            require( !MaterialProgramBindingMap::Create( one, wrong.get(), 7, WORKPHONE_SHADER_TYPE_PIXEL,
                {}, {}, {{"Value",Type::Texture,0}}, error ) && !error.empty(), "native resource kind must match canonical kind" );
        }
        // CPU strings can contain NULs; C registry names cannot. Never bind a
        // longer CPU identity to its truncated native prefix.
        for( const auto type : {Type::Scalar, Type::Texture} )
        {
            const String embedded( "x\0suffix", 8 );
            const auto one = MaterialShaderParameterLayout::Create( {{type,embedded}} );
            Program prefix( wp_shader_program_create(), &wp_shader_program_destroy );
            require( wp_shader_program_register_param( prefix.get(), nativeText("x"), uniformType(type), 1,
                WORKPHONE_SHADER_PARAM_VARIABILITY_PER_OBJECT, WORKPHONE_SHADER_AUTO_PARAM_NONE, resourceKind(type) ) == 0,
                "embedded-NUL prefix fixture registers" );
            error.clear();
            const auto candidate = type == Type::Scalar ? MaterialProgramBindingMap::Create( one, prefix.get(), 7,
                WORKPHONE_SHADER_TYPE_PIXEL, {{0,16}}, {{embedded,type,0,0,4}}, {}, error ) :
                MaterialProgramBindingMap::Create( one, prefix.get(), 7, WORKPHONE_SHADER_TYPE_PIXEL,
                    {}, {}, {{embedded,type,0}}, error );
            require( one && !candidate && !error.empty(), "embedded-NUL constant/resource identities reject rather than alias a prefix" );
        }
        // An alias into a large canonical owner is sized sufficiently but still forbidden.
        auto aliasLayout = MaterialShaderParameterLayout::Create( {{Type::Scalar,"Factor"}, {Type::Matrix,"Unused"}} );
        auto aliasProgram = programFor( aliasLayout );
        auto aliasMap = MaterialProgramBindingMap::Create( aliasLayout, aliasProgram.get(), 7, WORKPHONE_SHADER_TYPE_PIXEL,
            {{0,16}}, {{"Factor",Type::Scalar,0,0,4}}, {}, error );
        MaterialShaderParametersInstance aliasValues; aliasValues.Reset( aliasLayout );
        require( aliasValues.SetScalar( aliasValues.FindParameter( "Factor" ), .5f ) && aliasMap,
            "source-alias fixture creates" );
        const auto sourceBefore = aliasValues.GetParameterMemory();
        require( !aliasMap->Pack( aliasValues, 7, const_cast<u8 *>( aliasValues.GetParameterMemory().data() ), 16, nullptr, 0 ) &&
            aliasValues.GetParameterMemory() == sourceBefore, "output cannot overwrite borrowed canonical bytes" );
        {
            auto owner = programFor( aliasLayout );
            const auto lifetimeMap = MaterialProgramBindingMap::Create( aliasLayout, owner.get(), 7, WORKPHONE_SHADER_TYPE_PIXEL,
                {{0,16}}, {{"Factor",Type::Scalar,0,0,4}}, {}, error );
            require( bool(lifetimeMap), "program-retention fixture creates" );
            owner.reset();
            std::vector<u8> packed(16,0xa3), reference(16,0); store(reference,0,.5f);
            require( lifetimeMap->Pack( aliasValues, 7, packed.data(), packed.size(), nullptr, 0 ) && packed == reference,
                "map retains the native program after its original owner is destroyed" );
        }

        wp_shader_program_set_permutation_mask( program.get(), WP_SHADER_PERMUTATION_ALPHA_TEST );
        require( !map->IsCurrent( layout, program.get(), 7 ) &&
            !map->Pack( a, 7, bytes.data(), bytes.size(), resources.data(), resources.size() ), "pending pipeline edits invalidate map" );
        map = recipe.create( layout, program.get(), error );
        require( map && wp_shader_program_register_uniform( program.get(), nativeText("NewField"), WORKPHONE_UNIFORM_TYPE_FLOAT ) >= 0 &&
            !map->IsCurrent( layout, program.get(), 7 ), "definition revision invalidates map" );
        map = recipe.create( layout, program.get(), error );
        require( bool( map ), "replacement map captures new definition revision" );
        int nativeToken = 0;
        wp_shader_program_set_native( program.get(), &nativeToken );
        require( !map->IsCurrent( layout, program.get(), 7 ), "successful native generation invalidates old map" );
        auto retained = recipe.create( layout, program.get(), error );
        require( retained && retained->IsCurrent( layout, program.get(), 7 ), "new native generation gets a new valid map" );
        wp_shader_program_set_native( program.get(), nullptr );
        require( !retained->IsCurrent( layout, program.get(), 7 ), "cleared native generation invalidates map" );
        std::cout << "Generic binding maps: exact types, reordered offsets, zero padding, A/B/A isolation, resources and invalidation passed\n";
    }

    ComPtr<ID3DBlob> compile( const char *source, const char *profile )
    {
        ComPtr<ID3DBlob> bytecode, diagnostics;
        const auto result = D3DCompile( source, std::strlen( source ), "MaterialProgramBindingTests", nullptr, nullptr,
            "main", profile, D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
            bytecode.GetAddressOf(), diagnostics.GetAddressOf() );
        if( FAILED( result ) && diagnostics ) std::cerr << static_cast<const char *>( diagnostics->GetBufferPointer() );
        succeeded( result, "real HLSL compiles" );
        return bytecode;
    }
    const char *pixelSource =
        "cbuffer Values : register(b3) {\n"
        " uint Mask : packoffset(c0.x); float Factor : packoffset(c0.y);\n"
        " int Count : packoffset(c0.z); bool Enabled : packoffset(c0.w);\n"
        " int2 Pair : packoffset(c1.x); float2 Uv : packoffset(c1.z);\n"
        " int4 Flags : packoffset(c2); float4 Tint : packoffset(c3);\n"
        " row_major float4x4 Row : packoffset(c4);\n"
        " column_major float4x4 Column : packoffset(c8); float4 Color : packoffset(c12); };\n"
        "float4 main() : SV_Target {\n"
        " bool valid = Mask == 0xf0000000u && Count == -7 && Enabled &&\n"
        " all(Pair == int2(-3,9)) && all(Flags == int4(-1,2,-3,4)) && all(Uv == float2(.25,.75)) &&\n"
        " all(Color == float4(.1,.2,.3,.4));\n"
        " [unroll] for(uint r=0;r<4;++r) [unroll] for(uint c=0;c<4;++c)\n"
        " valid = valid && Row[r][c] == float(r*4+c+1) && Column[r][c] == float(r*4+c+1);\n"
        " return valid ? Tint * Factor : float4(1,0,1,1); }";
    std::vector<u8> reflectedExpected( bool second )
    {
        std::vector<u8> result( 208, 0 );
        store( result, 0, u32( 0xf0000000u ) ); store( result, 4, second ? 1.f : .5f );
        store( result, 8, s32( -7 ) ); store( result, 12, u32( 1 ) );
        store( result, 16, s32( -3 ) ); store( result, 20, s32( 9 ) );
        store( result, 24, .25f ); store( result, 28, .75f );
        const std::array<s32,4> flags = {-1,2,-3,4}; store( result, 32, flags );
        const std::array<float,4> tint = second ? std::array<float,4>{.1f,.2f,.3f,1.f} :
            std::array<float,4>{.4f,.8f,1.2f,2.f}; store( result, 48, tint );
        storeMatrix( result, 64, false ); storeMatrix( result, 128, true );
        const std::array<float,4> color = {.1f,.2f,.3f,.4f}; store( result, 192, color );
        return result;
    }
    void reflectionContracts()
    {
        const auto layout = MaterialShaderParameterLayout::Create( mixedInfo( false ) );
        auto program = programFor( layout ); String error;
        const auto bytecode = compile( pixelSource, "ps_5_0" );
        const auto map = CreateDX11MaterialProgramBindingMap( layout, program.get(), 11,
            WORKPHONE_SHADER_TYPE_PIXEL, bytecode->GetBufferPointer(), bytecode->GetBufferSize(), error );
        require( map && error.empty() && map->GetPackedSize() == 208 && map->GetBufferCount() == 1 &&
            map->GetBuffer( 0 )->slot == 3 && !map->GetResourceCount(), "actual DXBC reflection builds the expected map" );
        auto a = valuesFor( layout ), b = a;
        require( b.SetScalar( b.FindParameter( "Factor" ), 1.f ) &&
            b.SetVector4( b.FindParameter( "Tint" ), Vector4F( .1f,.2f,.3f,1.f ) ), "GPU second instance edits" );
        std::vector<u8> bytes( map->GetPackedSize(), 0x6a );
        require( map->Pack( a, 11, bytes.data(), bytes.size(), nullptr, 0 ) && bytes == reflectedExpected( false ),
            "real reflection offsets and both matrix storage orders match independent reference" );

        ComPtr<ID3D11Device> device; ComPtr<ID3D11DeviceContext> context;
        D3D_FEATURE_LEVEL level{};
        succeeded( D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
            D3D11_SDK_VERSION, device.GetAddressOf(), &level, context.GetAddressOf() ), "WARP test device creates" );
        require( recordDX11TestDevice( device.Get() ), "WARP adapter provenance records" );
        const auto vertexBytecode = compile(
            "float4 main(uint i:SV_VertexID):SV_Position { float2 p=float2((i<<1)&2,i&2);"
            "return float4(p*float2(2,-2)+float2(-1,1),.5,1); }", "vs_5_0" );
        ComPtr<ID3D11VertexShader> vertex; ComPtr<ID3D11PixelShader> pixel;
        succeeded( device->CreateVertexShader( vertexBytecode->GetBufferPointer(), vertexBytecode->GetBufferSize(),
            nullptr, vertex.GetAddressOf() ), "real vertex shader creates" );
        succeeded( device->CreatePixelShader( bytecode->GetBufferPointer(), bytecode->GetBufferSize(),
            nullptr, pixel.GetAddressOf() ), "real reflected pixel shader creates" );
        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = map->GetPackedSize(); bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        ComPtr<ID3D11Buffer> constants, readback;
        succeeded( device->CreateBuffer( &bufferDesc, nullptr, constants.GetAddressOf() ), "GPU constants allocate" );
        bufferDesc.Usage = D3D11_USAGE_STAGING; bufferDesc.BindFlags = 0; bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        succeeded( device->CreateBuffer( &bufferDesc, nullptr, readback.GetAddressOf() ), "GPU constant readback allocates" );
        D3D11_TEXTURE2D_DESC targetDesc{};
        targetDesc.Width = targetDesc.Height = 64; targetDesc.MipLevels = targetDesc.ArraySize = 1;
        targetDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; targetDesc.SampleDesc.Count = 1;
        targetDesc.Usage = D3D11_USAGE_DEFAULT; targetDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
        ComPtr<ID3D11Texture2D> target, staging; ComPtr<ID3D11RenderTargetView> targetView;
        succeeded( device->CreateTexture2D( &targetDesc, nullptr, target.GetAddressOf() ), "64x64 render target allocates" );
        succeeded( device->CreateRenderTargetView( target.Get(), nullptr, targetView.GetAddressOf() ), "render target view creates" );
        targetDesc.Usage = D3D11_USAGE_STAGING; targetDesc.BindFlags = 0; targetDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        succeeded( device->CreateTexture2D( &targetDesc, nullptr, staging.GetAddressOf() ), "pixel readback allocates" );
        D3D11_RASTERIZER_DESC rasterDesc{}; rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = D3D11_CULL_NONE; rasterDesc.DepthClipEnable = TRUE;
        ComPtr<ID3D11RasterizerState> raster;
        succeeded( device->CreateRasterizerState( &rasterDesc, raster.GetAddressOf() ), "raster state creates" );
        context->RSSetState( raster.Get() );
        const D3D11_VIEWPORT viewport = {0,0,64,64,0,1}; context->RSSetViewports( 1, &viewport );
        context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
        context->VSSetShader( vertex.Get(), nullptr, 0 ); context->PSSetShader( pixel.Get(), nullptr, 0 );
        auto *rtv = targetView.Get(); context->OMSetRenderTargets( 1, &rtv, nullptr );
        auto *constantBuffer = constants.Get(); context->PSSetConstantBuffers( 3, 1, &constantBuffer );
        const auto lookups = a.GetNameLookupCount();
        for( const auto *instance : {&a, &b, &a} )
        {
            require( map->Pack( *instance, 11, bytes.data(), bytes.size(), nullptr, 0 ), "GPU instance packs" );
            const auto expected = reflectedExpected( instance == &b );
            require( bytes == expected, "GPU instance packed bytes match independent reference" );
            context->UpdateSubresource( constants.Get(), 0, nullptr, bytes.data(), 0, 0 );
            const float clear[4] = {0,0,0,0}; context->ClearRenderTargetView( targetView.Get(), clear );
            context->Draw( 3, 0 );
            ComPtr<ID3D11Buffer> bound; context->PSGetConstantBuffers( 3, 1, bound.GetAddressOf() );
            require( bound.Get() == constants.Get(), "reflected b3 slot is actually bound" );
            context->CopyResource( readback.Get(), bound.Get() );
            D3D11_MAPPED_SUBRESOURCE mapped{};
            succeeded( context->Map( readback.Get(), 0, D3D11_MAP_READ, 0, &mapped ), "GPU constant bytes map" );
            const bool exact = std::memcmp( mapped.pData, expected.data(), expected.size() ) == 0;
            context->Unmap( readback.Get(), 0 ); require( exact, "GPU buffer retains every reflected packed byte" );
            context->CopyResource( staging.Get(), target.Get() );
            succeeded( context->Map( staging.Get(), 0, D3D11_MAP_READ, 0, &mapped ), "GPU pixels map" );
            const auto *center = static_cast<const u8 *>( mapped.pData ) + 32 * mapped.RowPitch + 32 * 4;
            const std::array<int,4> expectedPixel = instance == &b ? std::array<int,4>{26,51,77,255} :
                std::array<int,4>{51,102,153,255};
            bool equal = true;
            for( size_t channel = 0; channel < 4; ++channel ) equal &= std::abs( int( center[channel] ) - expectedPixel[channel] ) <= 1;
            context->Unmap( staging.Get(), 0 );
            require( equal, "actual shader pixels validate signedness, bool, vectors, row/column matrices and A/B/A isolation" );
        }
        require( a.GetNameLookupCount() == lookups, "real shader packing performs zero instance name lookups" );
        context->ClearState(); context->Flush();
        error.clear();
        require( !CreateDX11MaterialProgramBindingMap( layout, program.get(), 11, WORKPHONE_SHADER_TYPE_VERTEX,
            bytecode->GetBufferPointer(), bytecode->GetBufferSize(), error ) && !error.empty(), "DXBC stage mismatch rejects" );
        error.clear();
        const std::array<u8,8> invalidBytecode = {0,1,2,3,4,5,6,7};
        require( !CreateDX11MaterialProgramBindingMap( layout, program.get(), 11, WORKPHONE_SHADER_TYPE_PIXEL,
            invalidBytecode.data(), invalidBytecode.size(), error ) && !error.empty(), "invalid bytecode diagnoses reflection failure" );
        std::cout << "DX11 binding maps: real HLSL reflection, exact GPU constant bytes and WARP A/B/A pixels passed\n";
    }

    void unsupportedReflectionContracts()
    {
        struct Case { const char *source; Array<MaterialShaderParameterInfo> info; const char *message; };
        const Case cases[] = {
            {"cbuffer C{float Value[2];}; float4 main():SV_Target{return Value[0]+Value[1];}",
                {{Type::Scalar,"Value"}}, "scalar arrays reject rather than copy only their first element"},
            {"cbuffer C{float3 Value;}; float4 main():SV_Target{return float4(Value,1);}",
                {{Type::Vector4,"Value"}}, "unsupported float3 shape rejects"},
            {"cbuffer C{row_major float3x3 Value;}; float4 main():SV_Target{return float4(Value[0],Value[1][1]);}",
                {{Type::Matrix,"Value"}}, "unsupported matrix shape rejects"},
            {"struct S{float4 a;}; cbuffer C{S Value;}; float4 main():SV_Target{return Value.a;}",
                {{Type::Vector4,"Value"}}, "nested struct reflection rejects"},
            {"Texture2D Value[2]:register(t0); SamplerState Filter; float4 main():SV_Target{"
                "return Value[0].SampleLevel(Filter,float2(.5,.5),0)+Value[1].SampleLevel(Filter,float2(.5,.5),0);}",
                {{Type::Texture,"Value"},{Type::Sampler,"Filter"}}, "resource arrays reject"},
            {"cbuffer C{uint Value;}; float4 main():SV_Target{return Value;}",
                {{Type::Int,"Value"}}, "unsigned HLSL cannot bind signed canonical values"},
            {"cbuffer C{bool Value;}; float4 main():SV_Target{return Value?1:0;}",
                {{Type::UInt,"Value"}}, "HLSL bool cannot bind an equal-sized integer"}
        };
        for( const auto &test : cases )
        {
            const auto layout = MaterialShaderParameterLayout::Create( test.info );
            auto program = programFor( layout ); const auto bytecode = compile( test.source, "ps_5_0" );
            String error;
            require( !CreateDX11MaterialProgramBindingMap( layout, program.get(), 11, WORKPHONE_SHADER_TYPE_PIXEL,
                bytecode->GetBufferPointer(), bytecode->GetBufferSize(), error ) && !error.empty(), test.message );
        }
        const auto layout = MaterialShaderParameterLayout::Create(
            {{Type::Texture,"Albedo"},{Type::Buffer,"Data"},{Type::Sampler,"Filter"}} );
        auto program = programFor( layout );
        const auto bytecode = compile( "Texture2D Albedo:register(t9); Buffer<float4> Data:register(t10);"
            "SamplerState Filter:register(s5); float4 main():SV_Target{"
            "return Albedo.SampleLevel(Filter,float2(.5,.5),0)+Data[0];}", "ps_5_0" );
        String error;
        auto map = CreateDX11MaterialProgramBindingMap( layout, program.get(), 11, WORKPHONE_SHADER_TYPE_PIXEL,
            bytecode->GetBufferPointer(), bytecode->GetBufferSize(), error );
        require( map && !map->GetPackedSize() && map->GetResourceCount() == 3,
            "real resource-only reflection needs no dummy constant buffer" );
        MaterialShaderParametersInstance values; values.Reset( layout );
        require( values.SetTexture( values.FindParameter("Albedo"), 101 ) && values.SetBuffer( values.FindParameter("Data"), 202 ) &&
            values.SetSampler( values.FindParameter("Filter"), 303 ), "reflected resource values set" );
        std::array<MaterialProgramResourceValue,3> resources{};
        require( map->Pack( values, 11, nullptr, 0, resources.data(), resources.size() ), "resource-only map packs" );
        bool texture = false, buffer = false, sampler = false;
        for( const auto &resource : resources )
        {
            texture |= resource.type == Type::Texture && resource.slot == 9 && resource.handle == 101;
            buffer |= resource.type == Type::Buffer && resource.slot == 10 && resource.handle == 202;
            sampler |= resource.type == Type::Sampler && resource.slot == 5 && resource.handle == 303;
        }
        require( texture && buffer && sampler, "actual reflection retains distinct SRV/sampler slots and handle kinds" );
        std::cout << "Unsupported DXBC shapes diagnose failure; resource-only reflection preserves typed slots\n";
    }

    void selectedReflectionContracts()
    {
        const auto layout = MaterialShaderParameterLayout::Create( {{Type::Scalar,"Factor"}} );
        auto program = programFor( layout );
        const auto bytecode = compile(
            "cbuffer Frame:register(b0){float3 Camera;float FrameValues[2];};"
            "cbuffer Material:register(b2){float Factor;};"
            "Texture2D External:register(t4);SamplerState ExternalFilter:register(s1);"
            "float4 main():SV_Target{return Factor+Camera.x+FrameValues[0]+FrameValues[1]+"
            "External.SampleLevel(ExternalFilter,float2(.5,.5),0);}", "ps_5_0" );
        String error;
        auto create = [&]( const MaterialProgramReflectionSelection *selection ) {
            error.clear();
            return CreateDX11MaterialProgramBindingMap( layout, program.get(), 17, WORKPHONE_SHADER_TYPE_PIXEL,
                bytecode->GetBufferPointer(), bytecode->GetBufferSize(), error, selection );
        };
        require( !create( nullptr ) && !error.empty(), "mapping all bindings rejects unsupported external Frame shapes" );
        MaterialProgramReflectionSelection selected;
        selected.constantBuffers = {"Material"};
        const auto map = create( &selected );
        require( map && error.empty() && map->GetBufferCount() == 1 && map->GetBuffer(0)->slot == 2 &&
            map->GetPackedSize() == 16 && !map->GetResourceCount(),
            "explicit material ownership excludes external frame buffers and resources" );
        MaterialShaderParametersInstance values; values.Reset( layout );
        require( values.SetScalar( values.FindParameter("Factor"), .75f ), "selected material value sets" );
        std::vector<u8> bytes( 16, 0xa7 ), expected( 16, 0 ); store( expected, 0, .75f );
        require( map->Pack( values, 17, bytes.data(), bytes.size(), nullptr, 0 ) && bytes == expected,
            "selected Material cbuffer packs only its owned values with zeroed padding" );
        const MaterialProgramReflectionSelection empty;
        const auto emptyMap = create( &empty );
        require( emptyMap && !emptyMap->GetPackedSize() && !emptyMap->GetResourceCount() &&
            emptyMap->Pack( values, 17, nullptr, 0, nullptr, 0 ), "empty explicit selection owns no shader bindings" );
        for( const auto &names : {Array<String>{"Missing"}, Array<String>{"Material","Material"}, Array<String>{"External"},
                                 Array<String>{""}, Array<String>{String("Material\0suffix", 15)}} )
        {
            selected.constantBuffers = names;
            require( !create( &selected ) && !error.empty(), "missing, duplicate or resource-as-cbuffer selection rejects" );
        }
        selected.constantBuffers = {"Material"};
        for( const auto &names : {Array<String>{"Missing"}, Array<String>{"External","External"}, Array<String>{"Material"}} )
        {
            selected.resources = names;
            require( !create( &selected ) && !error.empty(), "missing, duplicate or cbuffer-as-resource selection rejects" );
        }
        selected.resources.clear();
        require( map->IsCurrent( layout, program.get(), 17 ), "rejected selection candidates leave retained map current" );
        const String longExternalName( 300, 'E' );
        const String longSource = "cbuffer " + longExternalName +
            ":register(b0){float3 Camera;};cbuffer Material:register(b2){float Factor;};"
            "float4 main():SV_Target{return Factor+Camera.x;}";
        const auto longBytecode = compile( longSource.c_str(), "ps_5_0" );
        error.clear();
        require( bool( CreateDX11MaterialProgramBindingMap( layout, program.get(), 17, WORKPHONE_SHADER_TYPE_PIXEL,
            longBytecode->GetBufferPointer(), longBytecode->GetBufferSize(), error, &selected ) ),
            "unowned external names need not fit the native material parameter name capacity" );
        std::cout << "DXBC ownership selection: material/view separation and strict selector validation passed\n";
    }
}

int main()
{
    struct Shutdown { ~Shutdown() { WorldStreamingScheduler::shutdownShared(); } } shutdown;
    try
    {
        genericContracts(); reflectionContracts(); unsupportedReflectionContracts(); selectedReflectionContracts();
        std::cout << "Material program binding and packing contracts passed\n";
        return 0;
    }
    catch( const std::exception &error ) { std::cerr << "Material program binding failure: " << error.what() << '\n'; return 1; }
}
