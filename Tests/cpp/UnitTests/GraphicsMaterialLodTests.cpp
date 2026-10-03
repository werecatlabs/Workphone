#include "UnitTests.hpp"
#include "GraphicsTestFixture.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Graphics/MaterialTechnique.hpp>
#include <Workphone/Graphics/MaterialPass.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace workphone;
using namespace workphone::render;

namespace
{
    /**
     * @brief Creates a unique material name for testing.
     */
    String makeMaterialName( const char *base )
    {
        static u32 counter = 0;
        return String( base ) + "_" + StringUtil::toString( ++counter );
    }

    /**
     * @brief Creates a new material for testing.
     */
    SmartPtr<Material> createTestMaterial( const String &name )
    {
        auto material = workphone::make_ptr<Material>();
        material->setName( name );
        return material;
    }

    /**
     * @brief Creates a technique with the specified number of passes.
     */
    SmartPtr<MaterialTechnique> createTechniqueWithPasses( u32 numPasses )
    {
        auto technique = workphone::make_ptr<MaterialTechnique>();
        for( u32 i = 0; i < numPasses; ++i )
        {
            technique->createPass();  // createPass() already adds the pass
        }
        return technique;
    }
}  // anonymous namespace

BOOST_FIXTURE_TEST_SUITE( GraphicsMaterialLodTests, GraphicsTestFixture )

//-----------------------------------------------------------------------------
// MaterialTechnique LOD Tests
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( technique_creation_with_default_values )
{
    auto technique = workphone::make_ptr<MaterialTechnique>();

    // Verify default state
    BOOST_TEST( technique->getNumPasses() == 0 );
    BOOST_TEST( technique->getScheme() == 0 );
}

BOOST_AUTO_TEST_CASE( technique_add_and_remove_passes )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping technique pass test" );
        return;
    }

    auto technique = workphone::make_ptr<MaterialTechnique>();

    // Add passes
    auto pass1 = technique->createPass();
    BOOST_REQUIRE( pass1 );
    BOOST_TEST( technique->getNumPasses() == 1 );

    auto pass2 = technique->createPass();
    BOOST_REQUIRE( pass2 );
    BOOST_TEST( technique->getNumPasses() == 2 );

    auto pass3 = technique->createPass();
    BOOST_REQUIRE( pass3 );
    BOOST_TEST( technique->getNumPasses() == 3 );

    // Verify pass retrieval
    auto retrievedPass = technique->getPass( 0 );
    BOOST_TEST( retrievedPass.get() == pass1.get() );

    retrievedPass = technique->getPass( 1 );
    BOOST_TEST( retrievedPass.get() == pass2.get() );

    retrievedPass = technique->getPass( 2 );
    BOOST_TEST( retrievedPass.get() == pass3.get() );

    // Remove a pass
    technique->removePass( pass2 );
    BOOST_TEST( technique->getNumPasses() == 2 );

    // Verify remaining passes
    retrievedPass = technique->getPass( 0 );
    BOOST_TEST( retrievedPass.get() == pass1.get() );
    retrievedPass = technique->getPass( 1 );
    BOOST_TEST( retrievedPass.get() == pass3.get() );

    // Remove all passes
    technique->removePasses();
    BOOST_TEST( technique->getNumPasses() == 0 );
}

BOOST_AUTO_TEST_CASE( technique_scheme_setting )
{
    auto technique = workphone::make_ptr<MaterialTechnique>();

    // Default scheme should be 0
    BOOST_TEST( technique->getScheme() == 0 );

    // Set different scheme values
    hash32 scheme1 = 12345;
    technique->setScheme( scheme1 );
    BOOST_TEST( technique->getScheme() == scheme1 );

    hash32 scheme2 = 0xFFFFFFFF;
    technique->setScheme( scheme2 );
    BOOST_TEST( technique->getScheme() == scheme2 );
}

BOOST_AUTO_TEST_CASE( technique_get_set_passes_array )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping technique passes array test" );
        return;
    }

    auto technique = workphone::make_ptr<MaterialTechnique>();

    // Create passes array
    Array<SmartPtr<IMaterialPass>> passes;
    for( u32 i = 0; i < 3; ++i )
    {
        auto pass = technique->createPass();
        passes.push_back( pass );
    }

    // Set passes via array
    technique->setPasses( passes );
    BOOST_TEST( technique->getNumPasses() == 3 );

    // Get passes and verify
    auto retrievedPasses = technique->getPasses();
    BOOST_TEST( retrievedPasses.size() == 3 );

    for( u32 i = 0; i < 3; ++i )
    {
        BOOST_TEST( retrievedPasses[i].get() == passes[i].get() );
    }

    // Verify array copy is independent (modifying retrieved doesn't affect technique)
    retrievedPasses.clear();
    BOOST_TEST( technique->getNumPasses() == 3 );
}

//-----------------------------------------------------------------------------
// MaterialPass LOD Property Tests
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( pass_transparency_properties )
{
    auto pass = workphone::make_ptr<MaterialPass>();

    // Default should be non-transparent
    BOOST_TEST( !pass->isTransparent() );

    // Set transparent
    pass->setTransparent( true );
    BOOST_TEST( pass->isTransparent() );

    // Set back to opaque
    pass->setTransparent( false );
    BOOST_TEST( !pass->isTransparent() );
}

BOOST_AUTO_TEST_CASE( pass_cutout_properties )
{
    auto pass = workphone::make_ptr<MaterialPass>();

    // Default should be non-cutout
    BOOST_TEST( !pass->isCutout() );

    // Set cutout
    pass->setCutout( true );
    BOOST_TEST( pass->isCutout() );

    // Set back
    pass->setCutout( false );
    BOOST_TEST( !pass->isCutout() );
}

BOOST_AUTO_TEST_CASE( pass_emission_properties )
{
    auto pass = workphone::make_ptr<MaterialPass>();

    std::cout << "DEBUG pass ptr=" << pass.get() << std::endl;
    std::cout << "DEBUG pass state context=" << pass->getStateContext().get() << std::endl;

    // Default emission state
    BOOST_TEST( !pass->isEmissionEnabled() );

    // Enable emission
    pass->setEmissionEnabled( true );
    BOOST_TEST( pass->isEmissionEnabled() );

    // Disable emission
    pass->setEmissionEnabled( false );
    BOOST_TEST( !pass->isEmissionEnabled() );
}

BOOST_AUTO_TEST_CASE( pass_refraction_properties )
{
    auto pass = workphone::make_ptr<MaterialPass>();

    // Default refraction state
    BOOST_TEST( !pass->isRefractionEnabled() );

    // Enable refraction
    pass->setRefractionEnabled( true );
    BOOST_TEST( pass->isRefractionEnabled() );

    // Disable refraction
    pass->setRefractionEnabled( false );
    BOOST_TEST( !pass->isRefractionEnabled() );
}

BOOST_AUTO_TEST_CASE( pass_flags_management )
{
    auto pass = workphone::make_ptr<MaterialPass>();

    // Default flags
    u32 defaultFlags = pass->getFlags();

    // Set custom flags
    u32 testFlags = transparentFlag | cutoutFlag | emissionEnabledFlag;
    pass->setFlags( testFlags );
    BOOST_TEST( pass->getFlags() == testFlags );

    // Toggle individual flags
    pass->setFlags( pass->getFlags() | doubleSidedFlag );
    BOOST_TEST( ( pass->getFlags() & doubleSidedFlag ) != 0 );

    // Clear flags
    pass->setFlags( 0 );
    BOOST_TEST( pass->getFlags() == 0 );

    // Restore defaults
    pass->setFlags( defaultFlags );
}

BOOST_AUTO_TEST_CASE( pass_depth_check_enabled )
{
    auto pass = workphone::make_ptr<MaterialPass>();

    // Default depth check state (should be enabled by default)
    BOOST_TEST( pass->isDepthCheckEnabled() );

    // Disable depth check
    pass->setDepthCheckEnabled( false );
    BOOST_TEST( !pass->isDepthCheckEnabled() );

    // Re-enable
    pass->setDepthCheckEnabled( true );
    BOOST_TEST( pass->isDepthCheckEnabled() );
}

//-----------------------------------------------------------------------------
// Material LOD Technique Selection Tests
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( material_technique_lifecycle )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    if( !applicationManager || !applicationManager->getGraphicsSystem() )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping material LOD test" );
        return;
    }

    auto material = createTestMaterial( makeMaterialName( "LodTechniqueTest" ) );
    BOOST_REQUIRE( material );

    // Create technique for LOD 0 (high detail)
    auto techniqueLod0 = createTechniqueWithPasses( 1 );
    techniqueLod0->setScheme( 0 );
    material->addTechnique( techniqueLod0 );

    // Create technique for LOD 1 (medium detail)
    auto techniqueLod1 = createTechniqueWithPasses( 1 );
    techniqueLod1->setScheme( 1 );
    material->addTechnique( techniqueLod1 );

    // Create technique for LOD 2 (low detail)
    auto techniqueLod2 = createTechniqueWithPasses( 1 );
    techniqueLod2->setScheme( 2 );
    material->addTechnique( techniqueLod2 );

    // Verify techniques were added
    BOOST_TEST( material->getNumTechniques() == 3 );
}

BOOST_AUTO_TEST_CASE( material_lod_technique_properties )
{
    auto material = createTestMaterial( makeMaterialName( "LodPropsTest" ) );
    BOOST_REQUIRE( material );

    // Create technique
    auto technique = createTechniqueWithPasses( 2 );
    technique->setScheme( 100 );

    // Verify technique properties
    BOOST_TEST( technique->getNumPasses() == 2 );
    BOOST_TEST( technique->getScheme() == 100 );

    // Add technique to material
    material->addTechnique( technique );
    BOOST_TEST( material->getNumTechniques() == 1 );
}

BOOST_AUTO_TEST_CASE( material_technique_pass_properties )
{
    auto technique = createTechniqueWithPasses( 3 );
    BOOST_REQUIRE( technique );
    BOOST_TEST( technique->getNumPasses() == 3 );

    // Access and verify each pass
    for( u32 i = 0; i < 3; ++i )
    {
        auto pass = technique->getPass( i );
        BOOST_REQUIRE( pass );

        // Test pass properties
        pass->setTransparent( i == 0 );
        BOOST_TEST( pass->isTransparent() == ( i == 0 ) );

        pass->setCutout( i == 1 );
        BOOST_TEST( pass->isCutout() == ( i == 1 ) );

        pass->setEmissionEnabled( i == 2 );
        BOOST_TEST( pass->isEmissionEnabled() == ( i == 2 ) );
    }
}

//-----------------------------------------------------------------------------
// Material Technique Serialization Tests
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( technique_properties_serialization )
{
    auto technique = createTechniqueWithPasses( 2 );
    technique->setScheme( 42 );

    // Get properties
    auto properties = technique->getProperties();
    BOOST_REQUIRE( properties );

    // Create new technique from properties
    auto newTechnique = workphone::make_ptr<MaterialTechnique>();
    newTechnique->setProperties( properties );

    // Verify scheme was preserved
    BOOST_TEST( newTechnique->getScheme() == 42 );
}

BOOST_AUTO_TEST_CASE( technique_child_objects )
{
    auto technique = createTechniqueWithPasses( 3 );

    // Get child objects (should include passes)
    auto childObjects = technique->getChildObjects();

    // Should contain the passes
    BOOST_TEST( childObjects.size() >= 3 );
}

//-----------------------------------------------------------------------------
// LOD Material Pass Count Tests
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( lod_material_pass_count_by_level )
{
    // LOD 0 (closest) - highest detail, most passes
    auto techniqueLod0 = createTechniqueWithPasses( 4 );
    BOOST_TEST( techniqueLod0->getNumPasses() == 4 );

    // LOD 1 - medium detail
    auto techniqueLod1 = createTechniqueWithPasses( 2 );
    BOOST_TEST( techniqueLod1->getNumPasses() == 2 );

    // LOD 2 (farthest) - lowest detail, minimum passes
    auto techniqueLod2 = createTechniqueWithPasses( 1 );
    BOOST_TEST( techniqueLod2->getNumPasses() == 1 );
}

BOOST_AUTO_TEST_CASE( lod_material_pass_flags_vary_by_level )
{
    // LOD 0 - full detail, all features
    auto passLod0 = workphone::make_ptr<MaterialPass>();
    passLod0->setTransparent( true );
    passLod0->setEmissionEnabled( true );
    passLod0->setRefractionEnabled( true );
    BOOST_TEST( passLod0->isTransparent() );
    BOOST_TEST( passLod0->isEmissionEnabled() );
    BOOST_TEST( passLod0->isRefractionEnabled() );

    // LOD 2 - minimal detail, reduced features
    auto passLod2 = workphone::make_ptr<MaterialPass>();
    passLod2->setTransparent( false );
    passLod2->setEmissionEnabled( false );
    passLod2->setRefractionEnabled( false );
    BOOST_TEST( !passLod2->isTransparent() );
    BOOST_TEST( !passLod2->isEmissionEnabled() );
    BOOST_TEST( !passLod2->isRefractionEnabled() );
}

//-----------------------------------------------------------------------------
// Technique Pass Removal Edge Cases
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( remove_nonexistent_pass_has_no_effect )
{
    auto technique = createTechniqueWithPasses( 2 );
    BOOST_TEST( technique->getNumPasses() == 2 );

    auto nonexistentPass = workphone::make_ptr<MaterialPass>();
    technique->removePass( nonexistentPass );

    // Should remain unchanged
    BOOST_TEST( technique->getNumPasses() == 2 );
}

BOOST_AUTO_TEST_CASE( remove_same_pass_twice )
{
    auto technique = createTechniqueWithPasses( 3 );
    auto pass = technique->getPass( 1 );

    // Remove once
    technique->removePass( pass );
    BOOST_TEST( technique->getNumPasses() == 2 );

    // Remove same pass again (should have no effect)
    technique->removePass( pass );
    BOOST_TEST( technique->getNumPasses() == 2 );
}

BOOST_AUTO_TEST_CASE( get_pass_out_of_bounds )
{
    auto technique = workphone::make_ptr<MaterialTechnique>();

    // Empty technique - pass access should be handled gracefully
    auto pass = technique->getPass( 0 );
    // Behavior depends on implementation - may return null or be undefined

    (void)technique->createPass();  // createPass() already adds the pass
    BOOST_TEST( technique->getNumPasses() == 1 );

    // Valid access
    pass = technique->getPass( 0 );
    BOOST_TEST( pass != nullptr );
}

//-----------------------------------------------------------------------------
// Technique State Management Tests
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( technique_state_message_handling )
{
    auto technique = createTechniqueWithPasses( 1 );
    BOOST_REQUIRE( technique );

    // Test state message handling (should not crash)
    auto handled = technique->handleStateMessage( nullptr );
    // Result depends on implementation

    auto state = SmartPtr<IState>();
    handled = technique->handleStateChanged( state );
    // Result depends on implementation
}

BOOST_AUTO_TEST_CASE( technique_to_data_serialization )
{
    auto technique = createTechniqueWithPasses( 2 );
    technique->setScheme( 999 );

    // Serialize to data
    auto data = technique->toData();
    BOOST_REQUIRE( data );

    // Create new technique from data
    auto restoredTechnique = workphone::make_ptr<MaterialTechnique>();
    restoredTechnique->fromData( data );

    // Verify scheme was restored
    BOOST_TEST( restoredTechnique->getScheme() == 999 );
    BOOST_TEST( restoredTechnique->getNumPasses() == technique->getNumPasses() );
}

//-----------------------------------------------------------------------------
// Multi-LOD Material System Integration
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( multi_lod_material_creation )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    if( !applicationManager || !applicationManager->getGraphicsSystem() )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping material LOD test" );
        return;
    }

    auto material = createTestMaterial( makeMaterialName( "MultiLodTest" ) );
    BOOST_REQUIRE( material );

    // Create LOD levels from high to low detail
    const u32 lodLevels = 4;
    for( u32 i = 0; i < lodLevels; ++i )
    {
        // Higher LOD levels (higher index) have fewer passes
        u32 numPasses = lodLevels - i;
        auto technique = createTechniqueWithPasses( numPasses );
        technique->setScheme( i );

        material->addTechnique( technique );
    }

    BOOST_TEST( material->getNumTechniques() == lodLevels );
}

BOOST_AUTO_TEST_CASE( lod_material_properties_preservation )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    if( !applicationManager || !applicationManager->getGraphicsSystem() )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping material LOD test" );
        return;
    }

    auto material = createTestMaterial( makeMaterialName( "LodPropsPreserveTest" ) );
    BOOST_REQUIRE( material );

    // Set material properties
    material->setMaterialType( MaterialType::Standard );
    material->setOpacity( 0.8f );
    material->setMetalness( 0.5f );
    material->setRoughness( 0.3f );

    // Create and add technique
    auto technique = createTechniqueWithPasses( 1 );
    material->addTechnique( technique );

    // Verify properties are still set correctly
    BOOST_TEST( static_cast<u32>( material->getMaterialType() ) ==
                static_cast<u32>( MaterialType::Standard ) );
    BOOST_TEST( material->getOpacity() == 0.8f, boost::test_tools::tolerance( 0.001f ) );
    BOOST_TEST( material->getMetalness() == 0.5f, boost::test_tools::tolerance( 0.001f ) );
    BOOST_TEST( material->getRoughness() == 0.3f, boost::test_tools::tolerance( 0.001f ) );
}

//-----------------------------------------------------------------------------
// Stress Tests for LOD Techniques
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( technique_with_many_passes )
{
    auto technique = workphone::make_ptr<MaterialTechnique>();

    const u32 manyPasses = 100;
    for( u32 i = 0; i < manyPasses; ++i )
    {
        (void)technique->createPass();  // createPass() already adds the pass
    }

    BOOST_TEST( technique->getNumPasses() == manyPasses );

    // Verify all passes are accessible
    for( u32 i = 0; i < manyPasses; ++i )
    {
        auto pass = technique->getPass( i );
        BOOST_TEST( pass != nullptr );
    }

    // Clear all passes
    technique->removePasses();
    BOOST_TEST( technique->getNumPasses() == 0 );
}

BOOST_AUTO_TEST_CASE( rapid_technique_creation_and_deletion )
{
    const u32 iterations = 50;

    for( u32 i = 0; i < iterations; ++i )
    {
        auto technique = createTechniqueWithPasses( 3 );
        BOOST_TEST( technique->getNumPasses() == 3 );

        // Technique will be destroyed when smart pointer goes out of scope
    }

    // If we reach here without crashes, the test passes
    BOOST_TEST_MESSAGE( "Created and destroyed " << iterations << " techniques without issues" );
}

BOOST_AUTO_TEST_SUITE_END()
