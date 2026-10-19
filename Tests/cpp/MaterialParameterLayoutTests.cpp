#include <Workphone/Graphics/MaterialShader.hpp>
#include <Workphone/State/States/MaterialPassStateData.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
#include <thread>
#include <atomic>

using namespace workphone;
using namespace workphone::render;

namespace
{
    void require( bool result, const char *message )
    { if( !result ) throw std::runtime_error( message ); }

    // Frozen storage shape before this optimization. sizeof compares the same
    // base/pointer/configuration layout, without constructing a legacy state.
    struct LegacyPassStateStorage : StateData
    {
        ColourF tintColour = ColourF::White;
        ColourF specularColour = ColourF::White;
        ColourF ambientColour = ColourF::White;
        ColourF diffuseColour = ColourF::White;
        ColourF emissiveColour = ColourF::Black;

        FixedArray<SmartPtr<render::ITexture>, (u32)PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES>
            textures = {};
        FixedArray<bool, (u32)PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES> textureDirty = {};

        f32 alphaClip = 0.5f;
        f32 opacity = 1.0f;
        f32 refractionAmount = 0.0f;
        f32 refractionIor = 1.45f;

        f32 metalness = 0.0f;
        f32 roughness = 0.5f;
        f32 specular = 0.05f;
        f32 normalStrength = 1.0f;
        f32 aoStrength = 1.0f;
        f32 heightScale = 0.02f;
        f32 parallaxSteps = 16.0f;
        f32 emissionIntensity = 1.0f;
        f32 clearCoat = 0.0f;
        f32 clearCoatRoughness = 0.1f;
        f32 anisotropy = 0.0f;

        f32 uvTilingX = 1.0f;
        f32 uvTilingY = 1.0f;
        f32 uvOffsetX = 0.0f;
        f32 uvOffsetY = 0.0f;
        f32 uvRotation = 0.0f;
        f32 uvTriplanarScale = 1.0f;
        f32 uvAniso = 1.0f;

        f32 detailTilingX = 1.0f;
        f32 detailTilingY = 1.0f;
        f32 detailOffsetX = 0.0f;
        f32 detailOffsetY = 0.0f;
        f32 detailRotation = 0.0f;
        f32 detailStrength = 0.0f;
        f32 detailNormalStrength = 1.0f;

        f32 maxTextureSize = 2048.0f;
        f32 renderQueue = 2000.0f;
        f32 sortPriority = 0.0f;
        f32 stencilRef = 0.0f;
        f32 stencilReadMask = 255.0f;
        f32 stencilWriteMask = 255.0f;
        f32 layerMaskStrength = 1.0f;
        f32 previewExposure = 1.0f;
        f32 previewRotation = 0.0f;

        u32 preset = 0u;
        u32 renderMode = 0u;
        u32 workflow = 0u;
        u32 blendMode = 0; /**< Blending mode (implementation-specific). */
        u32 cullMode = 2;  /**< Face culling mode (none=1, clockwise=2, anticlockwise=3). */
        u32 uvSet = 0u;
        u32 uvProjection = 0u;
        u32 uvWrapU = 0u;
        u32 uvWrapV = 0u;
        u32 uvFilter = 1u;
        u32 detailBlendMode = 0u;
        u32 metallicSource = 0u;
        u32 roughnessSource = 0u;
        u32 aoSource = 0u;
        u32 opacitySource = 0u;
        u32 heightSource = 0u;
        u32 srcBlend = 0u;
        u32 dstBlend = 0u;
        u32 depthTest = 2u;
        u32 layerBlend = 0u;
        u32 materialVariant = 0u;
        u32 technique = 0u;
        u32 pcCompression = 0u;
        u32 macCompression = 0u;
        u32 iosCompression = 0u;
        u32 androidCompression = 0u;
        u32 previewShape = 0u;
        u32 previewEnvironment = 0u;

        u32 flags = 0;

        MaterialType materialType = MaterialType::Standard;

        hash_type renderTechnique = 0; /**< Render technique hash. */

        FixedString<WP_MAX_CLASSNAME> vertexShaderName;
        FixedString<WP_MAX_CLASSNAME> fragmentShaderName;
        FixedString<WP_MAX_CLASSNAME> geometryShaderName;
        FixedString<WP_MAX_PATH> shaderPath;

        FixedString<WP_MAX_PATH> keywords;
    };

    void contracts()
    {
        Array<MaterialShaderParameterInfo> info = {
            { MaterialShaderParameterType::Scalar, "Roughness" },
            { MaterialShaderParameterType::Int, "Count" },
            { MaterialShaderParameterType::UInt, "Mask" },
            { MaterialShaderParameterType::Bool, "Enabled" },
            { MaterialShaderParameterType::Colour, "Albedo" },
            { MaterialShaderParameterType::Vector4, "Direction" },
            { MaterialShaderParameterType::Texture, "Map" },
            { MaterialShaderParameterType::Buffer, "Data" },
            { MaterialShaderParameterType::Sampler, "Filter" },
            { MaterialShaderParameterType::Matrix, "Transform" } };
        auto layout = MaterialShaderParameterLayout::Create( info );
        require( bool( layout ), "valid layout" );
        MaterialShaderParametersInstance a, b;
        a.Reset( layout ); b.Reset( layout );
        require( &a.GetParameterInfo() == &b.GetParameterInfo(), "shared metadata" );
        require( !a.ValidateResourceHandles(), "resources default to invalid" );
        const auto roughness = a.Resolve<f32>( "Roughness" );
        require( roughness.IsValid() && !a.Resolve<s32>( "Roughness" ).IsValid(), "typed resolution" );
        require( !a.FindParameter( "roughness" ).IsValid(), "case sensitive names" );
        require( a.Set( roughness, 0.35f ), "typed scalar write" );
        auto revision = a.GetRevision();
        require( a.Set( roughness, 0.35f ) && a.GetRevision() == revision, "no-op preserves revision" );
        f32 value = 0;
        require( a.Get( roughness, value ) && value == 0.35f, "typed read" );
        require( b.Set( roughness, 0.7f ) && a.GetScalar( roughness.handle ) == 0.35f,
                 "same-layout handles work across independent instances" );
        auto copy = a;
        require( copy.GetLayout() == a.GetLayout() && copy.Set( roughness, 0.6f ) &&
                 a.GetScalar( roughness.handle ) == 0.35f, "clone values independent" );
        const auto before = a.GetParameterMemory();
        require( !a.SetScalar( roughness.handle, s32( 42 ) ), "same-size wrong scalar type rejected" );
        require( !a.SetVector4( a.FindParameter( "Albedo" ), Vector4F{} ), "colour/vector distinction" );
        auto forged = roughness.handle;
        forged.m_offsetInBytes = std::numeric_limits<u32>::max();
        require( !a.SetScalar( forged, 2.0f ), "overflow offset rejected" );
        forged = roughness.handle; forged.m_strideInBytes = 8;
        require( !a.SetScalar( forged, 2.0f ), "forged stride rejected" );
        require( std::equal( before.begin(), before.end(), a.GetParameterMemory().begin() ),
                 "failed writes preserve every byte" );
        require( a.GetRevision() == revision, "failed writes preserve revision" );
        MaterialShaderParametersInstance other;
        other.Reset( info );
        require( !other.SetScalar( roughness.handle, 1.0f ), "unrelated layout rejected" );
        a.Reset( info );
        require( !a.Set( roughness, 1.0f ), "stale handle after reset rejected" );
        a.Clear();
        require( !a.IsValid() && !a.Set( roughness, 1.0f ), "cleared handle rejected" );
        value = 19.0f;
        require( !a.Get( roughness, value ) && value == 19.0f, "failed typed read leaves output" );
        require( a.GetResource( roughness.handle ) == InvalidMaterialResourceHandle,
                 "failed resource read keeps sentinel" );
        a.Reset( layout );
        require( a.SetScalar( a.FindParameter( "Count" ), s32( -7 ) ) &&
                 a.GetInt( a.FindParameter( "Count" ) ) == -7, "signed integer" );
        require( a.SetScalar( a.FindParameter( "Mask" ), u32( 0xf0000000u ) ) &&
                 a.GetUInt( a.FindParameter( "Mask" ) ) == 0xf0000000u, "unsigned integer" );
        require( a.SetBool( a.FindParameter( "Enabled" ), true ) &&
                 a.GetBool( a.FindParameter( "Enabled" ) ), "bool uses explicit packed type" );
        const auto enabled = a.Resolve<bool>( "Enabled" );
        bool enabledValue = false;
        require( enabled.IsValid() && a.Set( enabled, true ) && a.Get( enabled, enabledValue ) &&
                 enabledValue, "typed bool handle uses four-byte packed storage" );
        auto matrix = Matrix4F::identity();
        matrix.ptr()[3] = 7.0f;
        require( a.SetMatrix( a.FindParameter( "Transform" ), matrix ) &&
                 a.GetMatrix( a.FindParameter( "Transform" ) ).ptr()[3] == 7.0f,
                 "CPU matrix bytes do not require typed alignment" );
        require( a.SetTexture( a.FindParameter( "Map" ), 101 ) &&
                 a.SetBuffer( a.FindParameter( "Data" ), 102 ) &&
                 a.SetSampler( a.FindParameter( "Filter" ), 103 ) &&
                 a.ValidateResourceHandles(), "resource kinds" );
        require( !a.SetBuffer( a.FindParameter( "Map" ), 1 ), "resource type mismatch" );
        require( a.GetResource( a.FindParameter( "Map" ) ) == 101, "resource value retained" );
        a.InitializeDefaultResourceHandles();
        require( !a.ValidateResourceHandles(), "explicit resource reset" );
        const auto lookups = a.GetNameLookupCount();
        for( unsigned i = 0; i < 1000; ++i )
            require( a.SetScalar( a.GetBuiltin( MaterialBuiltinParameter::Roughness ), float( i ) ),
                     "cached builtin write" );
        require( a.GetNameLookupCount() == lookups, "no builtin name lookup" );
        auto duplicate = info; duplicate.push_back( info[0] );
        require( !MaterialShaderParameterLayout::Create( duplicate ), "duplicate names rejected" );
        duplicate = info; duplicate[0].m_type = static_cast<MaterialShaderParameterType>( 999 );
        require( !MaterialShaderParameterLayout::Create( duplicate ), "invalid type rejected" );
        duplicate = info; duplicate[0].m_name.clear();
        require( !MaterialShaderParameterLayout::Create( duplicate ), "empty name rejected" );
        require( GetMaterialShaderParameterId( "hello" ) == 0xa430d84680aabd0bull, "stable ID encoding" );

        auto &registry = MaterialShaderRegistry::instance();
        MaterialShader shader( "LayoutContract" );
        shader.addParameter( MaterialShaderParameterType::Scalar, "Roughness" );
        registry.registerShader( shader );
        registry.find( "LayoutContract" )->buildInstance( a );
        registry.find( "LayoutContract" )->buildInstance( b );
        require( a.GetLayout() == b.GetLayout(), "registered shader shares layout" );
        const auto retained = a.Resolve<f32>( "Roughness" );
        for( unsigned i = 0; i < 128; ++i )
        {
            MaterialShader extra( String( "Extra" ) + std::to_string( i ).c_str() );
            extra.addParameter( MaterialShaderParameterType::Scalar, "Value" );
            registry.registerShader( extra );
        }
        MaterialShader replacement( "LayoutContract" );
        replacement.addParameter( MaterialShaderParameterType::Colour, "Roughness" );
        registry.registerShader( replacement );
        registry.find( "LayoutContract" )->buildInstance( b );
        require( a.Set( retained, 0.8f ) && !b.Set( retained, 0.8f ),
                 "registry growth/replacement preserves retained versions and rejects stale binding" );
    }

    void snapshotContracts()
    {
        TypeManager types;
        types.load(); TypeManager::setInstance( &types );
        {
            MaterialPassStateData state;
            const auto original = state.getRenderState();
            require( original && original->roughness == 0.5f, "initial immutable defaults" );
            state.lock();
            state.setFloatSetting( MaterialFloatSetting::roughness, 0.2f );
            state.lock();
            state.uvTilingX = 4.0f;
            state.unlock();
            require( state.getRenderState() == original, "nested edits remain unpublished" );
            state.unlock();
            const auto edited = state.getRenderState();
            require( edited != original && edited->roughness == 0.2f && edited->uvTilingX == 4.0f &&
                     edited->revision == original->revision + 1, "coherent coalesced publication" );
            require( original->roughness == 0.5f && original->uvTilingX == 1.0f,
                     "retained old snapshot stays immutable" );
            state.lock(); state.roughness = 0.2f; state.unlock();
            require( state.getRenderState() == edited, "no-op edit reuses record" );
            state.setStringSetting( MaterialStringSetting::shaderPath, "Shaders/EditorOnly" );
            require( state.getRenderState() == edited, "cold edit reuses runtime record" );
            f32 value = 0;
            require( state.getFloatSetting( MaterialFloatSetting::roughness, value ) && value == 0.2f,
                     "typed setting read" );
            require( !state.setFloatSetting( MaterialFloatSetting::Count, 10.0f ) &&
                     state.getRenderState() == edited, "unknown setting preserves runtime record" );
            require( !state.setUIntSetting( MaterialUIntSetting::materialType, 999 ),
                     "typed material type validates enum" );
            state.lock(); state.roughness = 0; state.uvOffsetX = 0; state.unlock();
            std::atomic<bool> finished{ false }, coherent{ true };
            std::thread writer( [&] {
                for( unsigned i = 1; i <= 1000; ++i )
                {
                    state.lock(); state.roughness = float( i ); state.uvOffsetX = float( i ); state.unlock();
                }
                finished.store( true );
            } );
            do
            {
                const auto snapshot = state.getRenderState();
                if( snapshot->roughness != snapshot->uvOffsetX ) coherent.store( false );
            } while( !finished.load() );
            writer.join();
            require( coherent.load(), "concurrent readers see complete immutable versions" );
        }
        TypeManager::setInstance( nullptr );
    }

    // Reference reproduces the old copied-metadata/linear-name-scan representation.
    struct LegacyInstance
    {
        Array<MaterialShaderParameterInfo> info;
        Array<u8> bytes;
        void set( const String &name, float value )
        {
            for( const auto &p : info ) if( p.m_name == name )
            { std::memcpy( bytes.data() + p.m_offsetInBytes, &value, sizeof( value ) ); return; }
        }
    };
    void benchmark()
    {
        std::cout << "legacy_pass_state_bytes=" << sizeof( LegacyPassStateStorage )
                  << " pass_state_bytes=" << sizeof( MaterialPassStateData )
                  << " render_snapshot_bytes=" << sizeof( MaterialRenderState ) << '\n';
        constexpr size_t count = 10000;
        for( unsigned parameterCount : { 8u, 32u, 128u } )
        {
            Array<MaterialShaderParameterInfo> info;
            for( unsigned i = 0; i < parameterCount; ++i )
                info.push_back( { MaterialShaderParameterType::Scalar,
                                 String( "MaterialParameterWithLongName" ) + std::to_string( i ).c_str() } );
            const auto layout = MaterialShaderParameterLayout::Create( info );
            std::vector<MaterialShaderParametersInstance> compact( count );
            std::vector<LegacyInstance> legacy( count );
            size_t compactBytes = layout->GetStorageBytes(), legacyBytes = 0;
            for( size_t i = 0; i < count; ++i )
            {
                compact[i].Reset( layout );
                compactBytes += compact[i].GetInstanceStorageBytes();
                legacy[i].info = layout->GetParameterInfo();
                legacy[i].bytes = layout->GetDefaults();
                legacyBytes += sizeof( LegacyInstance ) + legacy[i].bytes.capacity() +
                    legacy[i].info.capacity() * sizeof( MaterialShaderParameterInfo );
                for( const auto &p : legacy[i].info ) legacyBytes += p.m_name.capacity() + 1;
            }
            const auto name = info.back().m_name;
            const auto handle = compact.front().Resolve<f32>( name );
            auto measure = [&]( bool old )
            {
                std::vector<double> samples;
                for( unsigned sample = 0; sample < 7; ++sample )
                {
                    auto start = std::chrono::steady_clock::now();
                    for( unsigned frame = 0; frame < 20; ++frame )
                        for( size_t i = 0; i < count; ++i )
                            if( old ) legacy[i].set( name, float( frame + i ) );
                            else compact[i].Set( handle, float( frame + i ) );
                    samples.push_back( std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - start ).count() );
                }
                std::sort( samples.begin(), samples.end() ); return samples[3];
            };
            const auto oldMs = measure( true ), newMs = measure( false );
            double checksum = 0;
            for( size_t i = 0; i < count; ++i )
            {
                float oldValue = 0;
                std::memcpy( &oldValue, legacy[i].bytes.data() + handle.handle.m_offsetInBytes, 4 );
                require( compact[i].GetScalar( handle.handle ) == oldValue, "benchmark parity" );
                checksum += oldValue;
            }
            require( compactBytes * 2 < legacyBytes, "shared-layout storage target" );
            std::cout << "parameters=" << parameterCount << " instances=" << count
                      << " legacy_accounted_bytes=" << legacyBytes << " shared_accounted_bytes=" << compactBytes
                      << " legacy_median_ms=" << oldMs << " handles_median_ms=" << newMs
                      << " checksum=" << checksum << '\n';
        }
    }
}

int main( int argc, char **argv )
{
    try
    {
        contracts();
        snapshotContracts();
        if( argc > 1 && std::strcmp( argv[1], "--benchmark" ) == 0 ) benchmark();
        std::cout << "Material parameter layout contracts passed\n";
        return 0;
    }
    catch( const std::exception &e ) { std::cerr << e.what() << '\n'; return 1; }
}
