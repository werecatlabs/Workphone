#include <Workphone/Graphics/MaterialShader.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Graphics/MaterialPublicationScope.hpp>
#include <Workphone/State/States/MaterialPassStateData.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/FixedString.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
#include <thread>
#include <atomic>
#include <unordered_set>
#include "MaterialPassStorageReference.hpp"

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
        require(!a.GetValueStorageIdentity() && !b.GetValueStorageIdentity() &&
                &a.GetParameterMemory() == &layout->GetDefaults() &&
                &a.GetParameterMemory() == &b.GetParameterMemory(), "default values have one layout owner");
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
        require(copy.GetValueStorageIdentity() == a.GetValueStorageIdentity(), "clone initially shares value payload");
        require(copy.Set(roughness, 0.35f) && copy.GetValueStorageIdentity() == a.GetValueStorageIdentity(),
                "no-op writes do not detach clone values");
        require(!copy.SetScalar(roughness.handle, s32(2)) &&
                copy.GetValueStorageIdentity() == a.GetValueStorageIdentity(), "invalid writes do not detach clones");
        require( copy.GetLayout() == a.GetLayout() && copy.Set( roughness, 0.6f ) &&
                 a.GetScalar( roughness.handle ) == 0.35f, "clone values independent" );
        require(copy.GetValueStorageIdentity() != a.GetValueStorageIdentity(), "effective clone edit detaches its values");
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

    void schemaMigrationContracts()
    {
        using Type = MaterialShaderParameterType;
        const auto before = MaterialShaderParameterLayout::Create({{Type::Scalar,"Number"},{Type::Colour,"Tint"},
            {Type::Texture,"Texture"},{Type::IntVector4,"Indices"},{Type::Matrix,"Transform"},{Type::Scalar,"Preserve"}});
        const auto after = MaterialShaderParameterLayout::Create({{Type::Matrix,"Transform"},{Type::Scalar,"Added"},
            {Type::UInt,"Number"},{Type::Texture,"Texture"},{Type::IntVector4,"Indices"},{Type::Colour,"Renamed"},
            {Type::Scalar,"Preserve"},{Type::Sampler,"NewSampler"}});
        require(before && after,"schema migration layouts finalize");
        MaterialShaderParametersInstance first, second;
        first.Reset(before); second.Reset(before);
        const auto oldHandle = first.FindParameter("Preserve");
        auto matrix = Matrix4F::identity(); matrix[0][1]=.125f; matrix[3][2]=9.5f;
        require(first.SetScalar(first.FindParameter("Number"),2.75f) &&
            first.SetColour(first.FindParameter("Tint"),ColourF(.1f,.2f,.3f,.4f)) &&
            first.SetTexture(first.FindParameter("Texture"),0) &&
            first.SetVector4(first.FindParameter("Indices"),Vector4<s32>(1,-2,3,-4)) &&
            first.SetMatrix(first.FindParameter("Transform"),matrix) && first.SetScalar(oldHandle,.625f),
            "migration fixture sets exact typed values, including numeric resource zero");
        const auto oldRevision = first.GetRevision();
        const auto oldBytes = first.GetParameterMemory();
        const auto oldIdentity = first.GetValueStorageIdentity();
        auto replacement = first.PrepareMigration(after);
        const auto &values = replacement->Values();
        Vector4<s32> indices;
        require(replacement->IsCurrent() && first.GetLayout() == before && first.GetRevision() == oldRevision &&
            first.GetParameterMemory() == oldBytes && first.GetValueStorageIdentity() == oldIdentity,
            "preparation leaves live layout, values, identity and revision unchanged");
        require(values.GetLayout() == after && values.GetRevision() == oldRevision+1 &&
            values.GetScalar(values.FindParameter("Preserve")) == .625f &&
            values.GetUInt(values.FindParameter("Number")) == 0 && values.GetScalar(values.FindParameter("Added")) == 0 &&
            values.GetColour(values.FindParameter("Renamed")) == ColourF(0,0,0,0) &&
            values.GetResource(values.FindParameter("Texture")) == 0 &&
            values.GetResource(values.FindParameter("NewSampler")) == InvalidMaterialResourceHandle &&
            values.Get(MaterialParameterHandle<Vector4<s32>>{values.FindParameter("Indices")},indices) &&
            indices == Vector4<s32>(1,-2,3,-4) && values.GetMatrix(values.FindParameter("Transform")) == matrix,
            "migration preserves exact name/type values through reorder and defaults renamed/new/type-changed fields");
        auto scheduler = WorldStreamingScheduler::shared();
        const auto source = size_t(StreamMemoryClass::SourceCpu);
        struct Restore
        {
            std::shared_ptr<WorldStreamingScheduler> scheduler;
            u64 limit;
            ~Restore() { String ignored; scheduler->setBudget(StreamMemoryClass::SourceCpu,limit,ignored); }
        } restore{scheduler,scheduler->stats().limits[source]};
        const auto prepared = scheduler->stats(); String error;
        require(scheduler->setBudget(StreamMemoryClass::SourceCpu,prepared.used[source],error),"migration commit budget applies");
        require(replacement->IsCurrent() && replacement->Commit() && !replacement->Commit(),
            "a current replacement commits once, without allocating, at zero headroom");
        const auto committed = scheduler->stats();
        require(committed.used[source] == prepared.used[source] &&
            committed.rejectedReservations[source] == prepared.rejectedReservations[source] &&
            first.GetLayout() == after && first.GetRevision() == oldRevision+1 &&
            !first.SetScalar(oldHandle,.8f) && !replacement->IsCurrent(),
            "commit invalidates old handles once and performs no SourceCpu allocation/rejection");
        scheduler->setBudget(StreamMemoryClass::SourceCpu,restore.limit,error);
        auto stale = second.PrepareMigration(after);
        second.SetScalar(second.FindParameter("Number"),4.0f);
        const auto modified = second.GetParameterMemory();
        require(!stale->IsCurrent() && !stale->Commit() && second.GetLayout() == before && second.GetParameterMemory() == modified,
            "a concurrent owner-thread value edit invalidates staging and stale commit preserves newer values");
        stale.reset(); replacement.reset();
        const auto baseline = scheduler->stats();
        require(scheduler->setBudget(StreamMemoryClass::SourceCpu,baseline.used[source],error),"migration denial budget applies");
        bool denied = false;
        try { second.PrepareMigration(after); } catch(const std::bad_alloc &) { denied = true; }
        const auto failed = scheduler->stats();
        scheduler->setBudget(StreamMemoryClass::SourceCpu,restore.limit,error);
        require(denied && failed.used[source] == baseline.used[source] &&
            failed.rejectedReservations[source] > baseline.rejectedReservations[source] &&
            second.GetLayout() == before && second.GetParameterMemory() == modified,
            "cold migration admission denial preserves all live state and rolls back exact lease usage");
        auto retry = second.PrepareMigration(after);
        require(retry->Commit() && second.GetLayout() == after,"restored admission permits migration retry");
    }

    void layoutStorageAccountingContracts()
    {
#if WP_USE_WP_STRING && WP_USE_STANDARD_STRING_ALLOCATOR
        Array<MaterialShaderParameterInfo> shortInfo = {{MaterialShaderParameterType::Scalar, "Short"}};
        auto shortLayout = MaterialShaderParameterLayout::Create( shortInfo );
        shortInfo[0].m_name = String( String::inline_capacity(), 's' );
        auto fullInlineLayout = MaterialShaderParameterLayout::Create( shortInfo );
        require( shortLayout->GetStorageBytes() == fullInlineLayout->GetStorageBytes(),
                 "layout accounting includes inline characters only once in the parameter object" );
        shortInfo[0].m_name = String( String::inline_capacity() + 1, 'l' );
        auto longLayout = MaterialShaderParameterLayout::Create( shortInfo );
        const auto &storedName = longLayout->GetParameterInfo()[0].m_name;
        require( !storedName.using_inline_storage() && longLayout->GetStorageBytes() ==
                     shortLayout->GetStorageBytes() + storedName.capacity() + 1,
                 "layout accounting charges actual external string capacity in addition to the object" );
#endif
    }

    void surfaceStorageContracts()
    {
        MaterialPassStateData state;
        const auto original = state.getRenderState();
        require( original && &state.getSurfaceState() == original.get() &&
                     state.getPendingSurfaceStorageBytes() == 0 && state.getSurfaceAuthoringStorageBytes() == 0,
                 "resting pass reads its published surface without an independently writable copy" );
        require( sizeof( MaterialPassStateData ) < sizeof( InlinePassStateStorageBeforeSurfaceMigration ),
                 "pass object retires its inline rendered-value and resource-owner copy" );
        state.setFloatSetting( MaterialFloatSetting::roughness, original->roughness );
        state.setSurfaceValue( &MaterialRenderState::roughness, original->roughness );
        state.setSurfaceState( *original );
        require( state.getPendingSurfaceStorageBytes() == 0 && state.getRenderState() == original,
                 "typed and bulk no-op setters allocate no pending surface" );
        state.lock();
        state.setSurfaceValue( &MaterialRenderState::roughness, 0.2f );
        require( state.getPendingSurfaceStorageBytes() == sizeof( MaterialRenderState ) &&
                     &state.getSurfaceState() != original.get() && state.getSurfaceState().roughness == 0.2f &&
                     state.getRenderState() == original,
                 "effective writes stage one surface while immutable readers retain the original" );
        state.lock();
        state.setSurfaceValue( &MaterialRenderState::uvTilingX, 4.0f );
        state.unlock();
        require( state.getRenderState() == original && state.getPendingSurfaceStorageBytes() == sizeof( MaterialRenderState ),
                 "nested writes share the existing pending allocation" );
        state.unlock();
        const auto edited = state.getRenderState();
        require( edited != original && edited->roughness == 0.2f && edited->uvTilingX == 4.0f &&
                     edited->revision == original->revision + 1 && &state.getSurfaceState() == edited.get() &&
                     state.getPendingSurfaceStorageBytes() == 0 && original->roughness == 0.5f,
                 "outer commit transfers complete values into the sole resting owner and retires staging" );
        state.lock();
        state.setSurfaceValue( &MaterialRenderState::roughness, 0.7f );
        state.setSurfaceValue( &MaterialRenderState::roughness, 0.2f );
        state.unlock();
        require( state.getRenderState() == edited && state.getPendingSurfaceStorageBytes() == 0 &&
                     &state.getSurfaceState() == edited.get(),
                 "net-zero edits discard staging and retain the published identity" );
        auto replacement = state.getSurfaceState();
        replacement.roughness = 0.6f;
        replacement.uvOffsetX = 0.25f;
        replacement.revision = ~u64( 0 ); // Callers cannot forge the publication sequence.
        state.lock(); state.setSurfaceState( replacement ); state.unlock();
        const auto bulk = state.getRenderState();
        require( bulk->roughness == 0.6f && bulk->uvOffsetX == 0.25f &&
                     bulk->revision == edited->revision + 1 && edited->roughness == 0.2f &&
                     state.getPendingSurfaceStorageBytes() == 0,
                 "bulk edits copy values through the tracked boundary and own revision sequencing" );
        const ColourF tint( 0.2f, 0.3f, 0.4f, 0.5f );
        state.setTintColour( ColourF::White ); state.setRenderTechnique( 0 );
        require( state.getSurfaceAuthoringStorageBytes() == 0,
                 "default tint and technique metadata allocate nothing" );
        state.setTintColour( tint );
        const auto metadataBytes = state.getSurfaceAuthoringStorageBytes();
        state.setRenderTechnique( hash_type( 71 ) );
        require( metadataBytes > 0 && state.getSurfaceAuthoringStorageBytes() == metadataBytes &&
                     state.getTintColour() == tint && state.getRenderTechnique() == hash_type( 71 ) &&
                     state.getRenderState() == bulk && state.getPendingSurfaceStorageBytes() == 0,
                 "tint and technique share cold storage without duplicating or publishing a render surface" );
        state.setTintColour( ColourF::White );
        require( state.getSurfaceAuthoringStorageBytes() == metadataBytes,
                 "clearing one cold field preserves the other field" );
        state.setRenderTechnique( 0 );
        require( state.getSurfaceAuthoringStorageBytes() == 0 && state.getRenderState() == bulk,
                 "clearing the final cold surface field retires its allocation" );
        std::cout << "surface_storage before_inline_pass_bytes=" << sizeof( InlinePassStateStorageBeforeSurfaceMigration )
                  << " resting_pass_bytes=" << sizeof( MaterialPassStateData )
                  << " published_surface_bytes=" << sizeof( MaterialRenderState )
                  << " pending_surface_bytes=" << state.getPendingSurfaceStorageBytes()
                  << " cold_surface_metadata_bytes=" << metadataBytes
                  << " allocator_and_control_blocks_excluded=1\n";
    }

    void numericAuthoringContracts()
    {
        MaterialPassStateData state;
        const auto snapshot = state.getRenderState();
        require(state.getNumericAuthoringStorageBytes() == 0, "default pass has no numeric authoring allocation");
        const char *floatNames[] = {"maxTextureSize","renderQueue","sortPriority","stencilRef",
            "stencilReadMask","stencilWriteMask","layerMaskStrength","previewExposure","previewRotation",
            "specular","heightScale","parallaxSteps","anisotropy","detailTilingX","detailTilingY",
            "detailOffsetX","detailOffsetY","detailRotation","detailStrength","detailNormalStrength"};
        const f32 defaults[] = {2048,2000,0,0,255,255,1,1,0, .05f,.02f,16,0,1,1,0,0,0,0,1};
        const char *uintNames[] = {"materialVariant","technique","pcCompression","macCompression",
            "iosCompression","androidCompression","previewShape","previewEnvironment",
            "preset","renderMode","workflow","uvSet","detailBlendMode","heightSource","srcBlend","dstBlend","layerBlend"};
        for(u32 i = 0; i < sizeof(floatNames) / sizeof(floatNames[0]); ++i)
        {
            f32 value = -1;
            require(state.getEditorFloat(floatNames[i], value) && value == defaults[i] &&
                state.setFloatSetting(MaterialPassStateData::resolveFloatSetting(floatNames[i]), defaults[i]),
                "cold float defaults and numeric writes use the shared schema");
        }
        for(const auto *name : uintNames)
        {
            u32 value = ~0u;
            require(state.getEditorUInt(name, value) && value == 0 && state.setEditorUInt(name, 0),
                "cold uint default reads/writes remain compatible");
        }
        Properties floats, uints, bools, strings;
        state.writeEditorSettings(floats, uints, bools, strings);
        require(state.getNumericAuthoringStorageBytes() == 0 && state.getRenderState() == snapshot,
            "default operations and property export allocate no numeric authoring storage or runtime version");
        // Insert out of order, including mask bits above 31, then erase at both ends.
        state.setEditorUInt("layerBlend", 0xdeadbeefu);
        require(state.getNumericAuthoringStorageBytes() == 12, "one override stores only mask and one word");
        state.setEditorFloat("maxTextureSize", 1024.0f);
        state.setEditorUInt("renderMode", 3u);
        u32 selector = 0;
        f32 textureLimit = 0;
        require(state.getNumericAuthoringStorageBytes() == 20 &&
            state.getEditorUInt("layerBlend", selector) && selector == 0xdeadbeefu &&
            state.getEditorFloat("maxTextureSize", textureLimit) && textureLimit == 1024.0f,
            "out-of-order packed insertion preserves high-mask values and earlier words");
        state.setEditorUInt("renderMode", 0u);
        require(state.getNumericAuthoringStorageBytes() == 16 &&
            state.getEditorFloat("maxTextureSize", textureLimit) && textureLimit == 1024.0f &&
            state.getEditorUInt("layerBlend", selector) && selector == 0xdeadbeefu,
            "erasing the middle word preserves both prefix and suffix");
        state.setEditorUInt("renderMode", 3u);
        state.setEditorFloat("maxTextureSize", 2048.0f);
        require(state.getNumericAuthoringStorageBytes() == 16 &&
            state.getEditorUInt("renderMode", selector) && selector == 3u &&
            state.getEditorUInt("layerBlend", selector) && selector == 0xdeadbeefu,
            "erasing the first word compacts retained selectors");
        state.setEditorUInt("layerBlend", 0u);
        require(state.getNumericAuthoringStorageBytes() == 12 &&
            state.getEditorUInt("renderMode", selector) && selector == 3u,
            "erasing the last word preserves the remaining override");
        state.setEditorUInt("renderMode", 0u);
        require(state.getNumericAuthoringStorageBytes() == 0, "last packed override reset releases allocation");
        state.setEditorFloat("previewExposure", 2.0f);
        require(state.getNumericAuthoringStorageBytes() == 12, "one float also uses one packed word");
        for(u32 i = 0; i < sizeof(floatNames) / sizeof(floatNames[0]); ++i)
        {
            f32 value = 0;
            require(state.getEditorFloat(floatNames[i], value) && value == (i == 7 ? 2.0f : defaults[i]),
                "first allocation preserves every untouched nonzero schema default");
        }
        state.setEditorFloat("previewExposure", 1.0f);
        require(state.getNumericAuthoringStorageBytes() == 0, "single setting reset releases the authoring block");
        for(u32 i = 0; i < sizeof(floatNames) / sizeof(floatNames[0]); ++i) state.setEditorFloat(floatNames[i], defaults[i] + f32(i + 2));
        for(u32 i = 0; i < sizeof(uintNames) / sizeof(uintNames[0]); ++i) state.setEditorUInt(uintNames[i], i + 1);
        const auto authoredBytes = state.getNumericAuthoringStorageBytes();
        require(authoredBytes == 156 && state.getRenderState() == snapshot,
            "cold writes allocate authoring storage without replacing the runtime record");
        state.writeEditorSettings(floats, uints, bools, strings);
        MaterialPassStateData restored;
        for(u32 i = 0; i < sizeof(floatNames) / sizeof(floatNames[0]); ++i)
        {
            f32 value = 0;
            require(floats.getPropertyValue(floatNames[i], value) && value == defaults[i] + f32(i + 2) &&
                restored.setEditorFloat(floatNames[i], value), "cold float properties round trip");
            state.setEditorFloat(floatNames[i], defaults[i]);
        }
        require(state.getNumericAuthoringStorageBytes() == 76,
            "resetting floats compacts the allocation to the remaining uint words");
        for(u32 i = 0; i < sizeof(uintNames) / sizeof(uintNames[0]); ++i)
        {
            u32 retained = 0;
            require(state.getEditorUInt(uintNames[i], retained) && retained == i + 1,
                "packed erase preserves every remaining uint value");
            u32 value = 0;
            require(uints.getPropertyValue(uintNames[i], value) && value == i + 1 &&
                restored.setEditorUInt(uintNames[i], value), "cold uint properties round trip");
            state.setEditorUInt(uintNames[i], 0);
        }
        require(state.getNumericAuthoringStorageBytes() == 0, "restoring every default releases numeric authoring storage");
        state.setEditorFloat("previewRotation", -0.0f);
        f32 signedZero = 1;
        state.getEditorFloat("previewRotation", signedZero);
        require(std::signbit(signedZero) && state.getNumericAuthoringStorageBytes() == 12,
            "cold floats preserve signed zero instead of silently replacing authored bits");
        const u32 nanBits = 0x7fc12345u;
        f32 authoredNan = 0;
        std::memcpy(&authoredNan, &nanBits, sizeof(authoredNan));
        state.setEditorFloat("previewRotation", authoredNan);
        state.getEditorFloat("previewRotation", authoredNan);
        u32 restoredNanBits = 0;
        std::memcpy(&restoredNanBits, &authoredNan, sizeof(restoredNanBits));
        require(restoredNanBits == nanBits && state.getNumericAuthoringStorageBytes() == 12,
            "packed float storage preserves authored NaN payload bits");
        state.setEditorFloat("previewRotation", 0.0f);
        require(state.getNumericAuthoringStorageBytes() == 0 &&
            !state.setEditorFloat("unknown", 2) && !state.setEditorUInt("unknown", 2),
            "default reset and rejected settings leave no numeric authoring allocation");
        std::cout << "pass_state_bytes=" << sizeof(state) << " numeric_authoring_bytes=" << authoredBytes
            << " sparse_single_override_bytes=12 moved_numeric_value_bytes=148" << '\n';
    }

    void stringAuthoringContracts()
    {
        MaterialPassStateData state;
        const auto original = state.getRenderState();
        require(state.getStringAuthoringStorageBytes() == 0 && state.getVertexShaderName().empty() &&
            state.getFragmentShaderName().empty() && state.getGeometryShaderName().empty(),
            "default pass has no string authoring allocation");
        state.setVertexShaderName(""); state.setEditorString("keywords", "");
        require(state.getStringAuthoringStorageBytes() == 0, "empty string writes allocate nothing");
        state.setVertexShaderName("VS");
        const auto singleBytes = state.getStringAuthoringStorageBytes();
        require(singleBytes == 23, "single shader name stores five offsets and its terminated bytes");
        state.setEditorString("keywords", "FOG");
        state.setGeometryShaderName("GS");
        state.setFragmentShaderName("PS");
        state.setStringSetting(MaterialStringSetting::shaderPath, "Shaders/Test");
        require(state.getVertexShaderName() == "VS" && state.getFragmentShaderName() == "PS" &&
            state.getGeometryShaderName() == "GS", "out-of-order packing preserves stage names");
        const auto allBytes = state.getStringAuthoringStorageBytes();
        require(allBytes == 46, "populated string storage accounts only for offsets and actual text");
        state.setVertexShaderName("VS");
        require(state.getStringAuthoringStorageBytes() == allBytes && state.getRenderState() == original,
            "same string writes preserve storage accounting and runtime snapshot");
        state.setFragmentShaderName("");
        state.setVertexShaderName("VertexLonger");
        String text;
        require(state.getGeometryShaderName() == "GS" && state.getEditorString("keywords", text) && text == "FOG" &&
            state.getEditorString("shaderPath", text) && text == "Shaders/Test",
            "string removal and growth preserve other packed values");
        Properties floats, uints, bools, strings;
        state.writeEditorSettings(floats, uints, bools, strings);
        MaterialPassStateData restored;
        for(const auto *name : {"shaderPath", "keywords"})
        {
            require(strings.getPropertyValue(name, text) && restored.setEditorString(name, text),
                "packed editor strings retain property keys and round trip");
        }
        require(restored.getEditorString("keywords", text) && text == "FOG", "restored keyword value");
        const String longStage(WP_MAX_CLASSNAME - 1, 's'), longPath(WP_MAX_PATH - 1, 'p');
        state.setGeometryShaderName(longStage);
        state.setEditorString("shaderPath", longPath);
        require(state.getGeometryShaderName() == longStage && state.getEditorString("shaderPath", text) && text == longPath,
            "maximum supported stage and path lengths retain their full contents");
#if !WP_ENABLE_ASSERTS
        const auto validBytes = state.getStringAuthoringStorageBytes();
        bool rejected = false;
        try { state.setGeometryShaderName(String(WP_MAX_CLASSNAME + 1, 'x')); }
        catch(const std::length_error &) { rejected = true; }
        require(rejected && state.getGeometryShaderName() == longStage &&
            state.getStringAuthoringStorageBytes() == validBytes, "oversized text preserves the prior authoring allocation");
#endif
        const String embedded("A\0ignored", 9);
        state.setVertexShaderName(embedded);
        require(state.getVertexShaderName() == "A", "legacy C-string embedded-null behavior is preserved");
        state.setVertexShaderName(""); state.setGeometryShaderName("");
        state.setEditorString("shaderPath", ""); state.setEditorString("keywords", "");
        require(state.getStringAuthoringStorageBytes() == 0 && state.getRenderState() == original &&
            !state.setStringSetting(MaterialStringSetting::Count, "Invalid"),
            "final clear retires string storage without publishing a runtime version");
        struct LegacyStrings
        {
            FixedString<WP_MAX_CLASSNAME> vertex, fragment, geometry;
            FixedString<WP_MAX_PATH> path, keywords;
        };
        std::cout << "string_authoring_legacy_bytes=" << sizeof(LegacyStrings)
                  << " single_name_bytes=" << singleBytes << " five_strings_bytes=" << allBytes << '\n';
    }

    void snapshotContracts()
    {
        TypeManager types;
        types.load(); TypeManager::setInstance( &types );
        numericAuthoringContracts();
        stringAuthoringContracts();
        surfaceStorageContracts();
        {
            Material material;
            auto &values = material.getShaderParameters();
            const auto layout = MaterialShaderParameterLayout::Create({{MaterialShaderParameterType::Scalar,"Number"}});
            values.Reset(layout);
            require(values.IsMigrationReady(),"unloaded material owner is ready for schema staging");
            {
                auto edit = values.Edit();
                require(!values.IsMigrationReady(),"parameter edit scope defers schema migration");
                bool busy = false;
                try { values.PrepareMigration(layout); } catch(const std::runtime_error &) { busy = true; }
                require(busy && values.GetLayout() == layout,"busy edit migration preserves the live owner");
            }
            {
                MaterialPublicationScope publication;
                require(!values.IsMigrationReady(),"material publication scope defers schema migration");
                publication.commit();
                require(values.IsMigrationReady(),"publication commit restores migration readiness");
            }
        }
        {
            MaterialPassStateData state;
            const auto original = state.getRenderState();
            require( original && original->roughness == 0.5f, "initial immutable defaults" );
            state.lock();
            state.setFloatSetting( MaterialFloatSetting::roughness, 0.2f );
            state.lock();
            state.setSurfaceValue( &MaterialRenderState::uvTilingX, 4.0f );
            state.unlock();
            require( state.getRenderState() == original, "nested edits remain unpublished" );
            state.unlock();
            const auto edited = state.getRenderState();
            require( edited != original && edited->roughness == 0.2f && edited->uvTilingX == 4.0f &&
                     edited->revision == original->revision + 1, "coherent coalesced publication" );
            require( original->roughness == 0.5f && original->uvTilingX == 1.0f,
                     "retained old snapshot stays immutable" );
            state.lock(); state.setSurfaceValue( &MaterialRenderState::roughness, 0.2f ); state.unlock();
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
            state.lock(); state.setSurfaceValue( &MaterialRenderState::roughness, 0.0f );
            state.setSurfaceValue( &MaterialRenderState::uvOffsetX, 0.0f ); state.unlock();
            std::atomic<bool> finished{ false }, coherent{ true };
            std::thread writer( [&] {
                for( unsigned i = 1; i <= 1000; ++i )
                {
                    state.lock(); state.setSurfaceValue( &MaterialRenderState::roughness, float( i ) );
                    state.setSurfaceValue( &MaterialRenderState::uvOffsetX, float( i ) ); state.unlock();
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
    size_t populationBytes(const std::vector<MaterialShaderParametersInstance> &instances,
                           const MaterialShaderParameterLayout::Ptr &layout)
    {
        size_t bytes = instances.capacity() * sizeof(MaterialShaderParametersInstance) + layout->GetStorageBytes();
        std::unordered_set<const void *> values;
        for(const auto &instance : instances)
            if(auto identity = instance.GetValueStorageIdentity(); identity && values.insert(identity).second)
                bytes += instance.GetValueStorageBytes();
        return bytes;
    }
    void sharingContracts()
    {
        Array<MaterialShaderParameterInfo> info;
        for(u32 i = 0; i < 128; ++i) info.push_back({MaterialShaderParameterType::Scalar, "Value" + String(std::to_string(i).c_str())});
        auto layout = MaterialShaderParameterLayout::Create(info);
        const auto value = layout->FindParameter("Value0");
        MaterialShaderParametersInstance prototype; prototype.Reset(layout);
        std::vector<MaterialShaderParametersInstance> instances(10000, prototype);
        const auto defaults = populationBytes(instances, layout);
        for(auto &instance : instances)
        {
            const auto revision = instance.GetRevision();
            require(instance.SetScalar(value, 0.0f) && instance.GetRevision() == revision &&
                    !instance.GetValueStorageIdentity(), "default no-op population stays allocation-free");
        }
        prototype.SetScalar(value, 0.75f);
        instances.assign(10000, prototype);
        require(instances.front().GetValueStorageIdentity() == instances.back().GetValueStorageIdentity(),
                "authored clones share one immutable payload");
        const auto clones = populationBytes(instances, layout);
        for(size_t i = 0; i < instances.size(); i += 100) instances[i].SetScalar(value, 0.5f);
        const auto sparse = populationBytes(instances, layout);
        require(prototype.GetScalar(value) == 0.75f && instances[1].GetScalar(value) == 0.75f &&
                instances[0].GetScalar(value) == 0.5f, "sparse edits preserve all other clones");
        const auto identity = instances[0].GetValueStorageIdentity();
        instances[0].SetScalar(value, 0.25f);
        require(identity == instances[0].GetValueStorageIdentity(), "unique authored values edit in place");
        for(auto &instance : instances) instance.SetScalar(value, 0.125f);
        const auto edited = populationBytes(instances, layout);
        struct EagerInstanceReference
        {
            MaterialShaderParameterLayout::Ptr layout;
            Array<u8> values;
            u64 revision, lookups;
            Material *owner;
        };
        const auto eager = layout->GetStorageBytes() + instances.capacity() *
            (sizeof(EagerInstanceReference) + layout->GetDefaults().size());
        require(defaults < eager && clones < eager && sparse < eager,
                "shared/default/sparse populations reduce accounted value storage");
        for(const auto &instance : instances) require(instance.GetScalar(value) == 0.125f, "all edited values remain independent");
        MaterialShaderParametersInstance moved(std::move(instances[0]));
        require(!instances[0].IsValid() && instances[0].GetParameterMemory().empty() && moved.GetScalar(value) == 0.125f,
                "move leaves an empty source and retains authored values");
        moved.Reset(layout);
        require(!moved.GetValueStorageIdentity() && moved.GetScalar(value) == 0.0f, "reset releases authored payload and borrows defaults");
        std::cout << "value_sharing instances=10000 parameters=128 eager_reference_bytes=" << eager
                  << " defaults_bytes=" << defaults
                  << " clone_bytes=" << clones << " one_percent_edited_bytes=" << sparse
                  << " all_edited_bytes=" << edited << " payload_control_blocks_excluded=1\n";
    }
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
            size_t legacyBytes = 0;
            for( size_t i = 0; i < count; ++i )
            {
                compact[i].Reset( layout );
                legacy[i].info = layout->GetParameterInfo();
                legacy[i].bytes = layout->GetDefaults();
                legacyBytes += sizeof( LegacyInstance ) + legacy[i].bytes.capacity() +
                    legacy[i].info.capacity() * sizeof( MaterialShaderParameterInfo );
                for( const auto &p : legacy[i].info )
                {
#if WP_USE_WP_STRING && WP_USE_STANDARD_STRING_ALLOCATOR
                    if( p.m_name.using_inline_storage() ) continue;
#endif
                    legacyBytes += p.m_name.capacity() + 1;
                }
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
            const auto defaultBytes = populationBytes(compact, layout);
            const auto oldMs = measure( true ), newMs = measure( false );
            const auto compactBytes = populationBytes(compact, layout);
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
                      << " shared_default_bytes=" << defaultBytes
                      << " legacy_median_ms=" << oldMs << " handles_median_ms=" << newMs
                      << " checksum=" << checksum << '\n';
        }
    }
}

int main( int argc, char **argv )
{
    struct SchedulerShutdown
    {
        ~SchedulerShutdown() { workphone::WorldStreamingScheduler::shutdownShared(); }
    } schedulerShutdown;
    try
    {
        contracts();
        layoutStorageAccountingContracts();
        schemaMigrationContracts();
        snapshotContracts();
        sharingContracts();
        if( argc > 1 && std::strcmp( argv[1], "--benchmark" ) == 0 ) benchmark();
        std::cout << "Material parameter layout contracts passed\n";
        return 0;
    }
    catch( const std::exception &e ) { std::cerr << e.what() << '\n'; return 1; }
}
