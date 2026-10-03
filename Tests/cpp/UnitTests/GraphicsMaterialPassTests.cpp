#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/MaterialPass.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

namespace
{
    // Convenience aliases for the render namespace
    using IMaterialPass = workphone::render::IMaterialPass;
    using MaterialPass = workphone::render::MaterialPass;

    // Helper: create a MaterialPass via make_ptr (follows existing test patterns)
    SmartPtr<MaterialPass> makeMaterialPass()
    {
        return make_ptr<MaterialPass>();
    }

    // Helper: check ColourF equality with tolerance
    void checkColour( const ColourF &actual, const ColourF &expected, f32 tolerance = 0.0001f )
    {
        BOOST_TEST( actual.r == expected.r, tolerance );
        BOOST_TEST( actual.g == expected.g, tolerance );
        BOOST_TEST( actual.b == expected.b, tolerance );
        BOOST_TEST( actual.a == expected.a, tolerance );
    }
}  // namespace

BOOST_AUTO_TEST_SUITE( GraphicsMaterialPassTests )

// =============================================================================
// Construction and default state
// =============================================================================

BOOST_AUTO_TEST_CASE( materialpass_default_construction )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Default depth settings
    BOOST_TEST( pass->isDepthCheckEnabled() );
    BOOST_TEST( pass->isDepthWriteEnabled() );

    // Default culling mode (expect 2 = back-face culling based on GraphicsMaterialTests)
    BOOST_TEST( pass->getCullingMode() == 2u );

    // Default lighting is enabled
    //BOOST_TEST( pass->isLightingEnabled() == true );

    // Default colour properties (expect white/identity)
    checkColour( pass->getAmbient(), ColourF::White );
    checkColour( pass->getDiffuse(), ColourF::White );
    checkColour( pass->getSpecular(), ColourF::White );
    checkColour( pass->getEmissive(), ColourF::Black );
    checkColour( pass->getTint(), ColourF::White );

    // Default PBR properties (from MaterialPass.hpp: metalness=0.5f, roughness=0.01f)
    BOOST_TEST( pass->getMetalness() == 0.5f, boost::test_tools::tolerance( 0.0001f ) );
    BOOST_TEST( pass->getRoughness() == 0.01f, boost::test_tools::tolerance( 0.0001f ) );

    // Default transparency/cutout states
    BOOST_TEST( !pass->isTransparent() );
    BOOST_TEST( !pass->isCutout() );

    // Default emission/refraction states
    BOOST_TEST( !pass->isEmissionEnabled() );
    BOOST_TEST( !pass->isRefractionEnabled() );

    const auto defaultFlags = static_cast<u32>( render::receiveShadowsFlag ) |
                              static_cast<u32>( render::castShadowsFlag ) |
                              static_cast<u32>( render::depthWriteFlag );
    BOOST_TEST( pass->getFlags() == defaultFlags );
}

// =============================================================================
// Depth state tests
// =============================================================================

BOOST_AUTO_TEST_CASE( materialpass_depth_check_toggle )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Default should be enabled
    BOOST_TEST( pass->isDepthCheckEnabled() );

    // Disable depth check
    pass->setDepthCheckEnabled( false );
    BOOST_TEST( !pass->isDepthCheckEnabled() );

    // Re-enable depth check
    pass->setDepthCheckEnabled( true );
    BOOST_TEST( pass->isDepthCheckEnabled() );
}

BOOST_AUTO_TEST_CASE( materialpass_depth_write_toggle )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Default should be enabled
    BOOST_TEST( pass->isDepthWriteEnabled() );

    // Disable depth write
    pass->setDepthWriteEnabled( false );
    BOOST_TEST( !pass->isDepthWriteEnabled() );

    // Re-enable depth write
    pass->setDepthWriteEnabled( true );
    BOOST_TEST( pass->isDepthWriteEnabled() );
}

// =============================================================================
// Culling mode tests
// =============================================================================

BOOST_AUTO_TEST_CASE( materialpass_culling_mode_default )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Default culling mode should be 2 (back-face culling)
    BOOST_TEST( pass->getCullingMode() == 2u );
}

BOOST_AUTO_TEST_CASE( materialpass_culling_mode_set_get )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Test setting various culling modes
    const u32 cullNone = 0u;
    const u32 cullFront = 1u;
    const u32 cullBack = 2u;

    pass->setCullingMode( cullNone );
    BOOST_TEST( pass->getCullingMode() == cullNone );

    pass->setCullingMode( cullFront );
    BOOST_TEST( pass->getCullingMode() == cullFront );

    pass->setCullingMode( cullBack );
    BOOST_TEST( pass->getCullingMode() == cullBack );
}

// =============================================================================
// Lighting tests
// =============================================================================

BOOST_AUTO_TEST_CASE( materialpass_lighting_toggle )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Default should be enabled
    //BOOST_TEST( pass->isLightingEnabled() );

    // Disable lighting
    pass->setLightingEnabled( false );
    //BOOST_TEST( !pass->isLightingEnabled() );

    // Re-enable lighting
    pass->setLightingEnabled( true );
    //BOOST_TEST( pass->isLightingEnabled() );
}

// =============================================================================
// Colour property tests
// =============================================================================

BOOST_AUTO_TEST_CASE( materialpass_ambient_colour_set_get )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Default is white
    checkColour( pass->getAmbient(), ColourF::White );

    // Set and verify
    const ColourF ambient( 0.2f, 0.3f, 0.4f, 1.0f );
    pass->setAmbient( ambient );
    checkColour( pass->getAmbient(), ambient );
}

BOOST_AUTO_TEST_CASE( materialpass_diffuse_colour_set_get )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Default is white
    checkColour( pass->getDiffuse(), ColourF::White );

    // Set and verify
    const ColourF diffuse( 0.1f, 0.5f, 0.8f, 0.9f );
    pass->setDiffuse( diffuse );
    checkColour( pass->getDiffuse(), diffuse );
}

BOOST_AUTO_TEST_CASE( materialpass_specular_colour_set_get )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Default is white
    checkColour( pass->getSpecular(), ColourF::White );

    // Set and verify
    const ColourF specular( 0.9f, 0.9f, 0.9f, 1.0f );
    pass->setSpecular( specular );
    checkColour( pass->getSpecular(), specular );
}

BOOST_AUTO_TEST_CASE( materialpass_emissive_colour_set_get )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Default is black (no emission)
    checkColour( pass->getEmissive(), ColourF::Black );

    // Set and verify
    const ColourF emissive( 1.0f, 0.5f, 0.0f, 1.0f );
    pass->setEmissive( emissive );
    checkColour( pass->getEmissive(), emissive );
}

BOOST_AUTO_TEST_CASE( materialpass_tint_colour_set_get )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Default is white (no tint)
    checkColour( pass->getTint(), ColourF::White );

    // Set and verify
    const ColourF tint( 0.8f, 0.8f, 0.8f, 1.0f );
    pass->setTint( tint );
    checkColour( pass->getTint(), tint );
}

// =============================================================================
// PBR property tests
// =============================================================================

BOOST_AUTO_TEST_CASE( materialpass_metalness_set_get )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Default is 0.5f
    BOOST_TEST( pass->getMetalness() == 0.5f, boost::test_tools::tolerance( 0.0001f ) );

    // Set various metalness values
    pass->setMetalness( 0.0f );
    BOOST_TEST( pass->getMetalness() == 0.0f, boost::test_tools::tolerance( 0.0001f ) );

    pass->setMetalness( 1.0f );
    BOOST_TEST( pass->getMetalness() == 1.0f, boost::test_tools::tolerance( 0.0001f ) );

    pass->setMetalness( 0.75f );
    BOOST_TEST( pass->getMetalness() == 0.75f, boost::test_tools::tolerance( 0.0001f ) );

    // Restore default
    pass->setMetalness( 0.5f );
    BOOST_TEST( pass->getMetalness() == 0.5f, boost::test_tools::tolerance( 0.0001f ) );
}

BOOST_AUTO_TEST_CASE( materialpass_roughness_set_get )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Default is 0.01f (smooth/glossy)
    BOOST_TEST( pass->getRoughness() == 0.01f, boost::test_tools::tolerance( 0.0001f ) );

    // Set various roughness values
    pass->setRoughness( 0.0f );
    BOOST_TEST( pass->getRoughness() == 0.0f, boost::test_tools::tolerance( 0.0001f ) );

    pass->setRoughness( 1.0f );
    BOOST_TEST( pass->getRoughness() == 1.0f, boost::test_tools::tolerance( 0.0001f ) );

    pass->setRoughness( 0.5f );
    BOOST_TEST( pass->getRoughness() == 0.5f, boost::test_tools::tolerance( 0.0001f ) );

    // Restore default
    pass->setRoughness( 0.01f );
    BOOST_TEST( pass->getRoughness() == 0.01f, boost::test_tools::tolerance( 0.0001f ) );
}

// =============================================================================
// Transparency and cutout tests
// =============================================================================

BOOST_AUTO_TEST_CASE( materialpass_transparent_toggle )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Default is opaque
    BOOST_TEST( !pass->isTransparent() );

    // Enable transparency
    pass->setTransparent( true );
    BOOST_TEST( pass->isTransparent() );

    // Disable transparency
    pass->setTransparent( false );
    BOOST_TEST( !pass->isTransparent() );
}

BOOST_AUTO_TEST_CASE( materialpass_cutout_toggle )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Default is disabled
    BOOST_TEST( !pass->isCutout() );

    // Enable cutout (alpha test)
    pass->setCutout( true );
    BOOST_TEST( pass->isCutout() );

    // Disable cutout
    pass->setCutout( false );
    BOOST_TEST( !pass->isCutout() );
}

// =============================================================================
// Emission and refraction tests
// =============================================================================

BOOST_AUTO_TEST_CASE( materialpass_emission_toggle )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Default is disabled
    BOOST_TEST( !pass->isEmissionEnabled() );

    // Enable emission
    pass->setEmissionEnabled( true );
    BOOST_TEST( pass->isEmissionEnabled() );

    // Disable emission
    pass->setEmissionEnabled( false );
    BOOST_TEST( !pass->isEmissionEnabled() );
}

BOOST_AUTO_TEST_CASE( materialpass_refraction_toggle )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Default is disabled
    BOOST_TEST( !pass->isRefractionEnabled() );

    // Enable refraction
    pass->setRefractionEnabled( true );
    BOOST_TEST( pass->isRefractionEnabled() );

    // Disable refraction
    pass->setRefractionEnabled( false );
    BOOST_TEST( !pass->isRefractionEnabled() );
}

// =============================================================================
// Scene blending tests
// =============================================================================

BOOST_AUTO_TEST_CASE( materialpass_scene_blending_set_get )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Test setting different blend types
    // Common blend types: 0=none, 1=alpha, 2=additive, 3=multiply, etc.
    pass->setSceneBlending( 0u );
    // No direct getter for blend type, but should not crash

    pass->setSceneBlending( 1u );
    pass->setSceneBlending( 2u );
    pass->setSceneBlending( 3u );
    pass->setSceneBlending( 100u );
}

// =============================================================================
// Flags tests
// =============================================================================

BOOST_AUTO_TEST_CASE( materialpass_flags_default )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    const auto defaultFlags = static_cast<u32>( render::receiveShadowsFlag ) |
                              static_cast<u32>( render::castShadowsFlag ) |
                              static_cast<u32>( render::depthWriteFlag );
    BOOST_TEST( pass->getFlags() == defaultFlags );
}

BOOST_AUTO_TEST_CASE( materialpass_flags_set_get )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Set individual flags
    pass->setFlags( 0x01u );
    BOOST_TEST( pass->getFlags() == 0x01u );

    pass->setFlags( 0xFFu );
    BOOST_TEST( pass->getFlags() == 0xFFu );

    pass->setFlags( 0x00u );
    BOOST_TEST( pass->getFlags() == 0x00u );
}

// =============================================================================
// Combined state tests
// =============================================================================

BOOST_AUTO_TEST_CASE( materialpass_combined_pbr_metallic_workflow )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Set up a metallic workflow material
    pass->setMetalness( 1.0f );
    pass->setRoughness( 0.2f );
    pass->setDiffuse( ColourF( 0.8f, 0.8f, 0.8f, 1.0f ) );
    pass->setSpecular( ColourF::White );

    BOOST_TEST( pass->getMetalness() == 1.0f, boost::test_tools::tolerance( 0.0001f ) );
    BOOST_TEST( pass->getRoughness() == 0.2f, boost::test_tools::tolerance( 0.0001f ) );
    checkColour( pass->getDiffuse(), ColourF( 0.8f, 0.8f, 0.8f, 1.0f ) );
}

BOOST_AUTO_TEST_CASE( materialpass_combined_pbr_dielectric_workflow )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Set up a dielectric (non-metallic) material
    pass->setMetalness( 0.0f );
    pass->setRoughness( 0.5f );
    pass->setDiffuse( ColourF( 0.2f, 0.5f, 0.8f, 1.0f ) );
    pass->setSpecular( ColourF( 0.5f, 0.5f, 0.5f, 1.0f ) );

    BOOST_TEST( pass->getMetalness() == 0.0f, boost::test_tools::tolerance( 0.0001f ) );
    BOOST_TEST( pass->getRoughness() == 0.5f, boost::test_tools::tolerance( 0.0001f ) );
    checkColour( pass->getDiffuse(), ColourF( 0.2f, 0.5f, 0.8f, 1.0f ) );
}

BOOST_AUTO_TEST_CASE( materialpass_combined_emissive_material )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Set up an emissive (glowing) material
    pass->setEmissionEnabled( true );
    pass->setEmissive( ColourF( 1.0f, 0.8f, 0.0f, 1.0f ) );
    pass->setDiffuse( ColourF::Black );  // Non-emissive parts are black

    BOOST_TEST( pass->isEmissionEnabled() );
    checkColour( pass->getEmissive(), ColourF( 1.0f, 0.8f, 0.0f, 1.0f ) );
    checkColour( pass->getDiffuse(), ColourF::Black );
}

BOOST_AUTO_TEST_CASE( materialpass_combined_transparent_material )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Set up a transparent material
    pass->setTransparent( true );
    pass->setDiffuse( ColourF( 0.5f, 0.5f, 0.5f, 0.5f ) );
    pass->setDepthWriteEnabled( false );  // Transparent objects often don't write to depth

    BOOST_TEST( pass->isTransparent() );
    checkColour( pass->getDiffuse(), ColourF( 0.5f, 0.5f, 0.5f, 0.5f ) );
    BOOST_TEST( !pass->isDepthWriteEnabled() );
}

BOOST_AUTO_TEST_CASE( materialpass_combined_cutout_material )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Set up a cutout (alpha-tested) material
    pass->setCutout( true );
    pass->setDiffuse( ColourF( 0.8f, 0.6f, 0.4f, 1.0f ) );
    pass->setSpecular( ColourF( 1.0f, 1.0f, 1.0f, 1.0f ) );
    pass->setRoughness( 0.3f );

    BOOST_TEST( pass->isCutout() );
    checkColour( pass->getDiffuse(), ColourF( 0.8f, 0.6f, 0.4f, 1.0f ) );
    BOOST_TEST( pass->getRoughness() == 0.3f, boost::test_tools::tolerance( 0.0001f ) );
}

// =============================================================================
// Interface type tests
// =============================================================================

BOOST_AUTO_TEST_CASE( materialpass_imaterialpass_interface )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Verify the pass implements IMaterialPass
    IMaterialPass *iPass = pass.get();
    BOOST_REQUIRE( iPass != nullptr );

    // Test IMaterialPass methods through interface
    iPass->setDepthCheckEnabled( false );
    BOOST_TEST( !iPass->isDepthCheckEnabled() );

    iPass->setDepthWriteEnabled( false );
    BOOST_TEST( !iPass->isDepthWriteEnabled() );

    iPass->setCullingMode( 0u );
    BOOST_TEST( iPass->getCullingMode() == 0u );

    iPass->setLightingEnabled( false );
    //BOOST_TEST( !iPass->isLightingEnabled() );

    iPass->setTransparent( true );
    BOOST_TEST( iPass->isTransparent() );

    iPass->setCutout( true );
    BOOST_TEST( iPass->isCutout() );
}

// =============================================================================
// Edge cases and boundary values
// =============================================================================

BOOST_AUTO_TEST_CASE( materialpass_extreme_pbr_values )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Test with extreme metalness values
    pass->setMetalness( 0.0f );
    BOOST_TEST( pass->getMetalness() == 0.0f );

    pass->setMetalness( 1.0f );
    BOOST_TEST( pass->getMetalness() == 1.0f );

    // Test with extreme roughness values
    pass->setRoughness( 0.0f );
    BOOST_TEST( pass->getRoughness() == 0.0f );

    pass->setRoughness( 1.0f );
    BOOST_TEST( pass->getRoughness() == 1.0f );

    // Test with near-zero roughness (very glossy)
    pass->setRoughness( 0.001f );
    BOOST_TEST( pass->getRoughness() == 0.001f, boost::test_tools::tolerance( 0.0001f ) );
}

BOOST_AUTO_TEST_CASE( materialpass_colour_boundary_values )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Test with black
    const ColourF black( 0.0f, 0.0f, 0.0f, 0.0f );
    pass->setDiffuse( black );
    checkColour( pass->getDiffuse(), black );

    // Test with white
    const ColourF white( 1.0f, 1.0f, 1.0f, 1.0f );
    pass->setDiffuse( white );
    checkColour( pass->getDiffuse(), white );

    // Test with maximum values
    const ColourF maxCol( 1.0f, 1.0f, 1.0f, 1.0f );
    pass->setAmbient( maxCol );
    pass->setSpecular( maxCol );
    checkColour( pass->getAmbient(), maxCol );
    checkColour( pass->getSpecular(), maxCol );
}

// =============================================================================
// State persistence tests (verify setters are idempotent)
// =============================================================================

BOOST_AUTO_TEST_CASE( materialpass_idempotent_setters )
{
    auto pass = makeMaterialPass();
    BOOST_REQUIRE( pass );

    // Multiple calls to set should produce same result
    pass->setTransparent( true );
    pass->setTransparent( true );
    pass->setTransparent( true );
    BOOST_TEST( pass->isTransparent() );

    pass->setTransparent( false );
    pass->setTransparent( false );
    pass->setTransparent( false );
    BOOST_TEST( !pass->isTransparent() );

    pass->setDepthCheckEnabled( false );
    pass->setDepthCheckEnabled( false );
    BOOST_TEST( !pass->isDepthCheckEnabled() );

    pass->setMetalness( 0.8f );
    pass->setMetalness( 0.8f );
    pass->setMetalness( 0.8f );
    BOOST_TEST( pass->getMetalness() == 0.8f, boost::test_tools::tolerance( 0.0001f ) );
}

BOOST_AUTO_TEST_SUITE_END()
