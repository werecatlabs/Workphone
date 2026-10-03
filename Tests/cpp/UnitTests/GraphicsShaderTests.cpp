#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Graphics/IShader.hpp>
#include <Workphone/Interface/Graphics/IComputeShader.hpp>
#include <Workphone/Graphics/Shader.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

namespace
{
    // Convenience alias so the tests read close to the IShader contract.
    using IShader = workphone::render::IShader;
    using Shader = workphone::render::Shader;

    // Locate a macro define by name in the list returned by getDefines().
    const IShader::Define *findDefine( const Array<IShader::Define> &defines, const String &name )
    {
        for( const auto &define : defines )
        {
            if( define.name == name )
                return &define;
        }
        return nullptr;
    }

    // True when any diagnostic carries exactly the supplied message text.
    bool hasDiagnosticMessage( const Array<IShader::Diagnostic> &diagnostics, const String &message )
    {
        for( const auto &diagnostic : diagnostics )
        {
            if( diagnostic.message == message )
                return true;
        }
        return false;
    }

    // True when any diagnostic has the supplied severity.
    bool hasDiagnosticSeverity( const Array<IShader::Diagnostic> &diagnostics,
                                IShader::DiagnosticSeverity severity )
    {
        for( const auto &diagnostic : diagnostics )
        {
            if( diagnostic.severity == severity )
                return true;
        }
        return false;
    }

    // Configures a shader with a known language and non-empty source so that
    // compile() can succeed.  Returns the shader so calls can chain.
    SmartPtr<Shader> makeCompilableShader()
    {
        auto shader = make_ptr<Shader>();
        shader->setLanguage( IShader::Language::GLSL );
        shader->setSource( "#version 450\nvoid main() {}\n" );
        return shader;
    }
}  // namespace

BOOST_AUTO_TEST_SUITE( GraphicsShaderTests )

// =============================================================================
// Construction and default state
// =============================================================================

BOOST_AUTO_TEST_CASE( shader_default_state )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    BOOST_CHECK( shader->getStage() == IShader::Stage::Vertex );
    BOOST_CHECK( shader->getLanguage() == IShader::Language::Unknown );
    BOOST_CHECK( shader->getSource().empty() );
    BOOST_CHECK( shader->getEntryPoint() == "main" );
    BOOST_CHECK( shader->getProfile().empty() );
    BOOST_CHECK( shader->getDefines().empty() );
    BOOST_CHECK( shader->getDiagnostics().empty() );
    BOOST_CHECK( shader->getStatus() == IShader::Status::Empty );
    BOOST_CHECK( !shader->isCompiled() );

    const auto reflection = shader->getReflection();
    BOOST_CHECK( reflection.resources.empty() );
    BOOST_CHECK( reflection.inputs.empty() );
    BOOST_CHECK( reflection.outputs.empty() );
    BOOST_CHECK( reflection.pushConstantSize == 0u );

    BOOST_CHECK( shader->getBinary().empty() );

    void *nativeHandle = reinterpret_cast<void *>( 0x1 );
    shader->getNativeHandle( &nativeHandle );
    BOOST_CHECK( nativeHandle == nullptr );
}

BOOST_AUTO_TEST_CASE( shader_make_ptr_constructs_valid_object )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );
    BOOST_CHECK( shader->getStatus() == IShader::Status::Empty );
    BOOST_CHECK( !shader->isCompiled() );
}

// =============================================================================
// Type info and inheritance
// =============================================================================

BOOST_AUTO_TEST_CASE( shader_type_info_is_non_zero_and_consistent )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    const auto info1 = shader->getTypeInfo();
    const auto info2 = shader->getTypeInfo();
    BOOST_CHECK( info1 != 0 );
    BOOST_CHECK( info1 == info2 );
}

BOOST_AUTO_TEST_CASE( shader_inheritance_relationships )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    BOOST_CHECK( shader->isDerived<IShader>() );
    BOOST_CHECK( shader->isDerived<ISharedObject>() );
    BOOST_CHECK( shader->isDerived<IObject>() );
    BOOST_CHECK( !shader->isDerived<render::IComputeShader>() );
}

// =============================================================================
// Stage
// =============================================================================

BOOST_AUTO_TEST_CASE( shader_set_stage_round_trips_all_valid_stages )
{
    const IShader::Stage stages[] = { IShader::Stage::Vertex,
                                      IShader::Stage::Fragment,
                                      IShader::Stage::Geometry,
                                      IShader::Stage::TessellationControl,
                                      IShader::Stage::TessellationEvaluation,
                                      IShader::Stage::Compute,
                                      IShader::Stage::RayGeneration,
                                      IShader::Stage::AnyHit,
                                      IShader::Stage::ClosestHit,
                                      IShader::Stage::Miss,
                                      IShader::Stage::Callable,
                                      IShader::Stage::Intersection };

    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    for( const auto stage : stages )
    {
        shader->setStage( stage );
        BOOST_CHECK( shader->getStage() == stage );
        BOOST_CHECK( shader->getDiagnostics().empty() );
    }
}

BOOST_AUTO_TEST_CASE( shader_set_stage_rejects_invalid_value )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    shader->setStage( IShader::Stage::Fragment );
    BOOST_CHECK( shader->getStage() == IShader::Stage::Fragment );

    const auto invalidStage = static_cast<IShader::Stage>( 0xFF );
    shader->setStage( invalidStage );

    // The invalid value must not replace the previously accepted stage.
    BOOST_CHECK( shader->getStage() == IShader::Stage::Fragment );

    const auto diagnostics = shader->getDiagnostics();
    BOOST_CHECK( diagnostics.size() == 1u );
    BOOST_CHECK( hasDiagnosticSeverity( diagnostics, IShader::DiagnosticSeverity::Error ) );
    BOOST_CHECK( hasDiagnosticMessage( diagnostics, "Invalid shader stage." ) );
}

BOOST_AUTO_TEST_CASE( shader_set_stage_after_compile_reverts_status )
{
    auto shader = makeCompilableShader();
    BOOST_REQUIRE( shader );
    BOOST_REQUIRE( shader->compile() );
    BOOST_CHECK( shader->isCompiled() );

    shader->setStage( IShader::Stage::Vertex );
    BOOST_CHECK( shader->getStatus() == IShader::Status::SourceReady );
    BOOST_CHECK( !shader->isCompiled() );
}

// =============================================================================
// Language
// =============================================================================

BOOST_AUTO_TEST_CASE( shader_set_language_round_trips )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );
    BOOST_CHECK( shader->getLanguage() == IShader::Language::Unknown );

    const IShader::Language languages[] = { IShader::Language::GLSL,  IShader::Language::HLSL,
                                            IShader::Language::Metal, IShader::Language::SPIRV,
                                            IShader::Language::DXIL,  IShader::Language::DXBC,
                                            IShader::Language::WGSL };

    for( const auto language : languages )
    {
        shader->setLanguage( language );
        BOOST_CHECK( shader->getLanguage() == language );
    }
}

BOOST_AUTO_TEST_CASE( shader_set_language_after_compile_reverts_status )
{
    auto shader = makeCompilableShader();
    BOOST_REQUIRE( shader );
    BOOST_REQUIRE( shader->compile() );
    BOOST_CHECK( shader->isCompiled() );

    shader->setLanguage( IShader::Language::HLSL );
    BOOST_CHECK( shader->getStatus() == IShader::Status::SourceReady );
    BOOST_CHECK( !shader->isCompiled() );
    BOOST_CHECK( shader->getLanguage() == IShader::Language::HLSL );
}

// =============================================================================
// Source
// =============================================================================

BOOST_AUTO_TEST_CASE( shader_set_source_updates_status )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );
    BOOST_CHECK( shader->getStatus() == IShader::Status::Empty );

    shader->setSource( "#version 450\nvoid main() {}\n" );
    BOOST_CHECK( shader->getSource() == "#version 450\nvoid main() {}\n" );
    BOOST_CHECK( shader->getStatus() == IShader::Status::SourceReady );

    shader->setSource( "" );
    BOOST_CHECK( shader->getSource().empty() );
    BOOST_CHECK( shader->getStatus() == IShader::Status::Empty );
}

BOOST_AUTO_TEST_CASE( shader_set_source_after_compile_reverts_status )
{
    auto shader = makeCompilableShader();
    BOOST_REQUIRE( shader );
    BOOST_REQUIRE( shader->compile() );
    BOOST_CHECK( shader->isCompiled() );

    shader->setSource( "#version 450\nvoid main() { ++i; }\n" );
    BOOST_CHECK( shader->getStatus() == IShader::Status::SourceReady );
    BOOST_CHECK( !shader->isCompiled() );
}

// =============================================================================
// Entry point
// =============================================================================

BOOST_AUTO_TEST_CASE( shader_entry_point_default_and_set )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );
    BOOST_CHECK( shader->getEntryPoint() == "main" );

    shader->setEntryPoint( "vs_main" );
    BOOST_CHECK( shader->getEntryPoint() == "vs_main" );
}

BOOST_AUTO_TEST_CASE( shader_set_entry_point_after_compile_reverts_status )
{
    auto shader = makeCompilableShader();
    BOOST_REQUIRE( shader );
    BOOST_REQUIRE( shader->compile() );
    BOOST_CHECK( shader->isCompiled() );

    shader->setEntryPoint( "vs_main" );
    BOOST_CHECK( shader->getStatus() == IShader::Status::SourceReady );
    BOOST_CHECK( !shader->isCompiled() );
}

// =============================================================================
// Profile
// =============================================================================

BOOST_AUTO_TEST_CASE( shader_profile_default_and_set )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );
    BOOST_CHECK( shader->getProfile().empty() );

    shader->setProfile( "450" );
    BOOST_CHECK( shader->getProfile() == "450" );
}

BOOST_AUTO_TEST_CASE( shader_set_profile_after_compile_reverts_status )
{
    auto shader = makeCompilableShader();
    BOOST_REQUIRE( shader );
    BOOST_REQUIRE( shader->compile() );
    BOOST_CHECK( shader->isCompiled() );

    shader->setProfile( "vs_6_6" );
    BOOST_CHECK( shader->getStatus() == IShader::Status::SourceReady );
    BOOST_CHECK( !shader->isCompiled() );
}

// =============================================================================
// Defines
// =============================================================================

BOOST_AUTO_TEST_CASE( shader_set_defines_replaces_list )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    const Array<IShader::Define> defines = { IShader::Define{ "A", "1" }, IShader::Define{ "B", "2" } };
    shader->setDefines( defines );

    const auto result = shader->getDefines();
    BOOST_CHECK( result.size() == 2u );
    BOOST_REQUIRE( findDefine( result, "A" ) );
    BOOST_CHECK( findDefine( result, "A" )->value == "1" );
    BOOST_REQUIRE( findDefine( result, "B" ) );
    BOOST_CHECK( findDefine( result, "B" )->value == "2" );
}

BOOST_AUTO_TEST_CASE( shader_set_defines_after_compile_reverts_status )
{
    auto shader = makeCompilableShader();
    BOOST_REQUIRE( shader );
    BOOST_REQUIRE( shader->compile() );
    BOOST_CHECK( shader->isCompiled() );

    shader->setDefines( { IShader::Define{ "A", "1" } } );
    BOOST_CHECK( shader->getStatus() == IShader::Status::SourceReady );
    BOOST_CHECK( !shader->isCompiled() );
}

BOOST_AUTO_TEST_CASE( shader_set_define_adds_and_updates )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    shader->setDefine( "A", "1" );
    shader->setDefine( "B" );       // default empty value
    shader->setDefine( "A", "2" );  // update existing

    const auto defines = shader->getDefines();
    BOOST_CHECK( defines.size() == 2u );

    const auto *a = findDefine( defines, "A" );
    BOOST_REQUIRE( a );
    BOOST_CHECK( a->value == "2" );

    const auto *b = findDefine( defines, "B" );
    BOOST_REQUIRE( b );
    BOOST_CHECK( b->value.empty() );
}

BOOST_AUTO_TEST_CASE( shader_set_define_ignores_empty_name )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    shader->setDefine( "", "1" );
    BOOST_CHECK( shader->getDefines().empty() );
}

BOOST_AUTO_TEST_CASE( shader_set_define_after_compile_reverts_status )
{
    auto shader = makeCompilableShader();
    BOOST_REQUIRE( shader );
    BOOST_REQUIRE( shader->compile() );
    BOOST_CHECK( shader->isCompiled() );

    shader->setDefine( "A", "1" );
    BOOST_CHECK( shader->getStatus() == IShader::Status::SourceReady );
    BOOST_CHECK( !shader->isCompiled() );
}

BOOST_AUTO_TEST_CASE( shader_remove_define_removes_only_named_entry )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    shader->setDefine( "A", "1" );
    shader->setDefine( "B", "2" );

    shader->removeDefine( "A" );
    const auto defines = shader->getDefines();
    BOOST_CHECK( defines.size() == 1u );
    BOOST_CHECK( !findDefine( defines, "A" ) );
    BOOST_REQUIRE( findDefine( defines, "B" ) );

    // Removing a missing name is a no-op.
    shader->removeDefine( "missing" );
    BOOST_CHECK( shader->getDefines().size() == 1u );
}

BOOST_AUTO_TEST_CASE( shader_remove_define_after_compile_reverts_status )
{
    auto shader = makeCompilableShader();
    BOOST_REQUIRE( shader );
    shader->setDefine( "A", "1" );
    BOOST_REQUIRE( shader->compile() );
    BOOST_CHECK( shader->isCompiled() );

    shader->removeDefine( "A" );
    BOOST_CHECK( shader->getStatus() == IShader::Status::SourceReady );
    BOOST_CHECK( !shader->isCompiled() );
}

BOOST_AUTO_TEST_CASE( shader_clear_defines_empties_list )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    shader->setDefine( "A", "1" );
    shader->setDefine( "B", "2" );
    BOOST_CHECK( shader->getDefines().size() == 2u );

    shader->clearDefines();
    BOOST_CHECK( shader->getDefines().empty() );
}

BOOST_AUTO_TEST_CASE( shader_clear_defines_after_compile_reverts_status )
{
    auto shader = makeCompilableShader();
    BOOST_REQUIRE( shader );
    shader->setDefine( "A", "1" );
    BOOST_REQUIRE( shader->compile() );
    BOOST_CHECK( shader->isCompiled() );

    shader->clearDefines();
    BOOST_CHECK( shader->getStatus() == IShader::Status::SourceReady );
    BOOST_CHECK( !shader->isCompiled() );
}

// =============================================================================
// Compile
// =============================================================================

BOOST_AUTO_TEST_CASE( shader_compile_succeeds_with_source )
{
    auto shader = makeCompilableShader();
    BOOST_REQUIRE( shader );

    BOOST_CHECK( shader->compile() );
    BOOST_CHECK( shader->getStatus() == IShader::Status::Compiled );
    BOOST_CHECK( shader->isCompiled() );
    BOOST_CHECK( shader->getDiagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( shader_compile_succeeds_with_binary_only )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    const Array<u8> binary = { 0x01, 0x02, 0x03, 0x04 };
    BOOST_REQUIRE( shader->setBinary( binary, IShader::Language::SPIRV ) );
    BOOST_CHECK( shader->isCompiled() );

    // Recompiling a binary-only shader must still succeed: the contract is that
    // the last successfully compiled binary remains usable.
    BOOST_CHECK( shader->compile() );
    BOOST_CHECK( shader->getStatus() == IShader::Status::Compiled );
    BOOST_CHECK( shader->isCompiled() );
}

BOOST_AUTO_TEST_CASE( shader_compile_fails_from_empty_default_state )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    BOOST_CHECK( !shader->compile() );
    BOOST_CHECK( shader->getStatus() == IShader::Status::Failed );
    BOOST_CHECK( !shader->isCompiled() );

    const auto diagnostics = shader->getDiagnostics();
    BOOST_CHECK( diagnostics.size() == 2u );
    BOOST_CHECK( hasDiagnosticMessage( diagnostics, "A source or binary language must be specified." ) );
    BOOST_CHECK( hasDiagnosticMessage( diagnostics, "Shader has neither source nor binary data." ) );
    BOOST_CHECK( hasDiagnosticSeverity( diagnostics, IShader::DiagnosticSeverity::Error ) );
}

BOOST_AUTO_TEST_CASE( shader_compile_fails_without_source_or_binary )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );
    shader->setLanguage( IShader::Language::GLSL );

    BOOST_CHECK( !shader->compile() );
    BOOST_CHECK( shader->getStatus() == IShader::Status::Failed );

    const auto diagnostics = shader->getDiagnostics();
    BOOST_CHECK( diagnostics.size() == 1u );
    BOOST_CHECK( hasDiagnosticMessage( diagnostics, "Shader has neither source nor binary data." ) );
}

BOOST_AUTO_TEST_CASE( shader_compile_fails_with_unknown_language )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );
    shader->setSource( "#version 450\nvoid main() {}\n" );

    BOOST_CHECK( !shader->compile() );
    BOOST_CHECK( shader->getStatus() == IShader::Status::Failed );

    const auto diagnostics = shader->getDiagnostics();
    BOOST_CHECK( diagnostics.size() == 1u );
    BOOST_CHECK( hasDiagnosticMessage( diagnostics, "A source or binary language must be specified." ) );
}

BOOST_AUTO_TEST_CASE( shader_compile_fails_with_empty_entry_point )
{
    auto shader = makeCompilableShader();
    BOOST_REQUIRE( shader );
    shader->setEntryPoint( "" );

    BOOST_CHECK( !shader->compile() );
    BOOST_CHECK( shader->getStatus() == IShader::Status::Failed );

    const auto diagnostics = shader->getDiagnostics();
    BOOST_CHECK( diagnostics.size() == 1u );
    BOOST_CHECK( hasDiagnosticMessage( diagnostics, "Shader entry point cannot be empty." ) );
}

BOOST_AUTO_TEST_CASE( shader_compile_clears_previous_diagnostics_on_success )
{
    auto shader = makeCompilableShader();
    BOOST_REQUIRE( shader );

    // Produce a diagnostic via an invalid stage assignment.
    shader->setStage( static_cast<IShader::Stage>( 0xFF ) );
    BOOST_CHECK( !shader->getDiagnostics().empty() );

    // A successful compile must clear any previously accumulated diagnostics.
    BOOST_REQUIRE( shader->compile() );
    BOOST_CHECK( shader->getDiagnostics().empty() );
    BOOST_CHECK( shader->isCompiled() );
}

BOOST_AUTO_TEST_CASE( shader_compile_sets_compiling_status_transitively )
{
    auto shader = makeCompilableShader();
    BOOST_REQUIRE( shader );

    // Before compile the shader is in SourceReady (source is set).
    BOOST_CHECK( shader->getStatus() == IShader::Status::SourceReady );

    BOOST_REQUIRE( shader->compile() );
    BOOST_CHECK( shader->getStatus() == IShader::Status::Compiled );
}

// =============================================================================
// Reload
// =============================================================================

BOOST_AUTO_TEST_CASE( shader_reload_recompiles_current_configuration )
{
    auto shader = makeCompilableShader();
    BOOST_REQUIRE( shader );

    BOOST_REQUIRE( shader->reload() );
    BOOST_CHECK( shader->isCompiled() );

    // Breaking the configuration then reloading must fail.
    shader->setEntryPoint( "" );
    BOOST_CHECK( !shader->reload() );
    BOOST_CHECK( shader->getStatus() == IShader::Status::Failed );
    BOOST_CHECK( !shader->isCompiled() );
}

// =============================================================================
// Invalidate
// =============================================================================

BOOST_AUTO_TEST_CASE( shader_invalidate_clears_binary_and_reflection_after_source_compile )
{
    auto shader = makeCompilableShader();
    BOOST_REQUIRE( shader );
    BOOST_REQUIRE( shader->compile() );
    BOOST_CHECK( shader->isCompiled() );

    shader->invalidate();
    BOOST_CHECK( !shader->isCompiled() );
    BOOST_CHECK( shader->getStatus() == IShader::Status::SourceReady );
    BOOST_CHECK( shader->getBinary().empty() );

    const auto reflection = shader->getReflection();
    BOOST_CHECK( reflection.resources.empty() );
    BOOST_CHECK( reflection.pushConstantSize == 0u );
}

BOOST_AUTO_TEST_CASE( shader_invalidate_resets_binary_only_shader_to_empty )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    const Array<u8> binary = { 0x10, 0x20 };
    BOOST_REQUIRE( shader->setBinary( binary, IShader::Language::SPIRV ) );
    BOOST_CHECK( shader->isCompiled() );

    shader->invalidate();
    BOOST_CHECK( !shader->isCompiled() );
    BOOST_CHECK( shader->getStatus() == IShader::Status::Empty );
    BOOST_CHECK( shader->getBinary().empty() );
    BOOST_CHECK( shader->getSource().empty() );
}

// =============================================================================
// Diagnostics
// =============================================================================

BOOST_AUTO_TEST_CASE( shader_diagnostics_get_and_clear )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    shader->setStage( static_cast<IShader::Stage>( 0xFF ) );
    BOOST_CHECK( shader->getDiagnostics().size() == 1u );

    shader->clearDiagnostics();
    BOOST_CHECK( shader->getDiagnostics().empty() );
}

// =============================================================================
// Reflection and resource lookup
// =============================================================================

BOOST_AUTO_TEST_CASE( shader_reflection_default_is_empty )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    const auto reflection = shader->getReflection();
    BOOST_CHECK( reflection.resources.empty() );
    BOOST_CHECK( reflection.inputs.empty() );
    BOOST_CHECK( reflection.outputs.empty() );
    BOOST_CHECK( reflection.pushConstantSize == 0u );
}

BOOST_AUTO_TEST_CASE( shader_has_resource_returns_false_for_empty_reflection )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    // The portable base class never populates reflection, so any lookup fails.
    BOOST_CHECK( !shader->hasResource( "uColor" ) );
    BOOST_CHECK( !shader->hasResource( "" ) );
}

// =============================================================================
// Binary
// =============================================================================

BOOST_AUTO_TEST_CASE( shader_get_binary_default_is_empty )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );
    BOOST_CHECK( shader->getBinary().empty() );
}

BOOST_AUTO_TEST_CASE( shader_set_binary_rejects_empty_payload )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );
    shader->setLanguage( IShader::Language::GLSL );

    const Array<u8> emptyBinary;
    BOOST_CHECK( !shader->setBinary( emptyBinary, IShader::Language::GLSL ) );

    // State must be untouched by the rejected call.
    BOOST_CHECK( shader->getBinary().empty() );
    BOOST_CHECK( shader->getStatus() == IShader::Status::Empty );
    BOOST_CHECK( shader->getLanguage() == IShader::Language::GLSL );
}

BOOST_AUTO_TEST_CASE( shader_set_binary_rejects_unknown_language )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    const Array<u8> binary = { 0x01, 0x02 };
    BOOST_CHECK( !shader->setBinary( binary, IShader::Language::Unknown ) );

    BOOST_CHECK( shader->getBinary().empty() );
    BOOST_CHECK( shader->getLanguage() == IShader::Language::Unknown );
    BOOST_CHECK( shader->getStatus() == IShader::Status::Empty );
}

BOOST_AUTO_TEST_CASE( shader_set_binary_accepts_valid_payload )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    const Array<u8> binary = { 0x01, 0x02, 0x03, 0x04 };
    BOOST_CHECK( shader->setBinary( binary, IShader::Language::SPIRV ) );

    BOOST_CHECK( shader->isCompiled() );
    BOOST_CHECK( shader->getStatus() == IShader::Status::Compiled );
    BOOST_CHECK( shader->getLanguage() == IShader::Language::SPIRV );
    BOOST_CHECK( shader->getSource().empty() );
    BOOST_CHECK( shader->getDiagnostics().empty() );

    const auto stored = shader->getBinary();
    BOOST_CHECK( stored.size() == binary.size() );
    for( size_t i = 0; i < binary.size(); ++i )
        BOOST_CHECK( stored[i] == binary[i] );
}

BOOST_AUTO_TEST_CASE( shader_set_binary_clears_existing_source )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );
    shader->setSource( "#version 450\nvoid main() {}\n" );
    BOOST_CHECK( !shader->getSource().empty() );

    const Array<u8> binary = { 0xAA, 0xBB };
    BOOST_REQUIRE( shader->setBinary( binary, IShader::Language::SPIRV ) );
    BOOST_CHECK( shader->getSource().empty() );
}

// =============================================================================
// Specialization constants
// =============================================================================

BOOST_AUTO_TEST_CASE( shader_specialization_constant_set_get_and_update )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    // An empty name is rejected and stores nothing.
    BOOST_CHECK( !shader->setSpecializationConstant( "", 1u ) );

    BOOST_CHECK( shader->setSpecializationConstant( "WORKGROUP_SIZE", 64u ) );
    u32 value = 0;
    BOOST_CHECK( shader->getSpecializationConstant( "WORKGROUP_SIZE", value ) );
    BOOST_CHECK( value == 64u );

    // Updating an existing constant overwrites the previous value.
    BOOST_CHECK( shader->setSpecializationConstant( "WORKGROUP_SIZE", 128u ) );
    BOOST_CHECK( shader->getSpecializationConstant( "WORKGROUP_SIZE", value ) );
    BOOST_CHECK( value == 128u );

    // A missing constant is not found.
    BOOST_CHECK( !shader->getSpecializationConstant( "MISSING", value ) );
}

BOOST_AUTO_TEST_CASE( shader_specialization_constant_after_compile_reverts_status )
{
    auto shader = makeCompilableShader();
    BOOST_REQUIRE( shader );
    BOOST_REQUIRE( shader->compile() );
    BOOST_CHECK( shader->isCompiled() );

    BOOST_CHECK( shader->setSpecializationConstant( "WORKGROUP_SIZE", 32u ) );
    BOOST_CHECK( shader->getStatus() == IShader::Status::SourceReady );
    BOOST_CHECK( !shader->isCompiled() );

    u32 value = 0;
    BOOST_CHECK( shader->getSpecializationConstant( "WORKGROUP_SIZE", value ) );
    BOOST_CHECK( value == 32u );
}

// =============================================================================
// Native handle
// =============================================================================

BOOST_AUTO_TEST_CASE( shader_native_handle_is_null_and_null_out_param_is_safe )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );

    void *handle = reinterpret_cast<void *>( 0x1 );
    shader->getNativeHandle( &handle );
    BOOST_CHECK( handle == nullptr );

    // A null out-pointer must not crash.
    shader->getNativeHandle( nullptr );
}

// =============================================================================
// Status transition summary
// =============================================================================

BOOST_AUTO_TEST_CASE( shader_status_transitions_follow_contract )
{
    auto shader = make_ptr<Shader>();
    BOOST_REQUIRE( shader );
    BOOST_CHECK( shader->getStatus() == IShader::Status::Empty );

    shader->setSource( "src" );
    BOOST_CHECK( shader->getStatus() == IShader::Status::SourceReady );

    shader->setLanguage( IShader::Language::GLSL );
    BOOST_REQUIRE( shader->compile() );
    BOOST_CHECK( shader->getStatus() == IShader::Status::Compiled );

    // Mutating any compiled configuration reverts to SourceReady while source
    // is present.
    shader->setProfile( "450" );
    BOOST_CHECK( shader->getStatus() == IShader::Status::SourceReady );

    BOOST_REQUIRE( shader->compile() );
    shader->setSource( "" );
    // No source and no binary leaves the shader Empty.
    BOOST_CHECK( shader->getStatus() == IShader::Status::Empty );
}

BOOST_AUTO_TEST_SUITE_END()
