#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

#include <array>
#include <limits>

#if WP_GRAPHICS_SYSTEM_CLAW && defined( WP_PLATFORM_WIN32 )
#    include <WPGraphics/ClawRendererDX11.hpp>
#    include <WPGraphics/ClawWindow.hpp>
#    include <WPGraphics/ClawMesh.hpp>
#    include <WPGraphics/ClawScene.hpp>
#    include <WPGraphics/ClawMaterial.hpp>
#    include <Workphone/Graphics/Texture.hpp>
#    include "workphone_graphics_mesh.h"
#    include "workphone_graphics_renderer.h"
#    include "workphone_graphics_renderer_dx11.h"
#    include "workphone_graphics_material.h"
#    include <d3d11.h>
#    include <dxgi.h>
#    include <wrl/client.h>
#endif

using namespace workphone;

namespace
{
    struct FloatFieldCase
    {
        const char *name;
        f32 defaultValue;
    };

    struct UIntFieldCase
    {
        const char *name;
        u32 defaultValue;
    };

    struct BoolFieldCase
    {
        const char *name;
        u32 flag;
        bool defaultValue;
    };

    struct StringFieldCase
    {
        const char *name;
        const char *defaultValue;
        const char *updatedValue;
    };

    constexpr std::array<FloatFieldCase, 38> FloatFields = {
        FloatFieldCase{ "alphaClip", 0.5f },
        FloatFieldCase{ "opacity", 1.0f },
        FloatFieldCase{ "refractionAmount", 0.0f },
        FloatFieldCase{ "refractionIor", 1.45f },
        FloatFieldCase{ "metalness", 0.0f },
        FloatFieldCase{ "roughness", 0.5f },
        FloatFieldCase{ "specular", 0.5f },
        FloatFieldCase{ "normalStrength", 1.0f },
        FloatFieldCase{ "aoStrength", 1.0f },
        FloatFieldCase{ "heightScale", 0.02f },
        FloatFieldCase{ "parallaxSteps", 16.0f },
        FloatFieldCase{ "emissionIntensity", 1.0f },
        FloatFieldCase{ "clearCoat", 0.0f },
        FloatFieldCase{ "clearCoatRoughness", 0.1f },
        FloatFieldCase{ "anisotropy", 0.0f },
        FloatFieldCase{ "uvTilingX", 1.0f },
        FloatFieldCase{ "uvTilingY", 1.0f },
        FloatFieldCase{ "uvOffsetX", 0.0f },
        FloatFieldCase{ "uvOffsetY", 0.0f },
        FloatFieldCase{ "uvRotation", 0.0f },
        FloatFieldCase{ "uvTriplanarScale", 1.0f },
        FloatFieldCase{ "uvAniso", 1.0f },
        FloatFieldCase{ "detailTilingX", 1.0f },
        FloatFieldCase{ "detailTilingY", 1.0f },
        FloatFieldCase{ "detailOffsetX", 0.0f },
        FloatFieldCase{ "detailOffsetY", 0.0f },
        FloatFieldCase{ "detailRotation", 0.0f },
        FloatFieldCase{ "detailStrength", 0.0f },
        FloatFieldCase{ "detailNormalStrength", 1.0f },
        FloatFieldCase{ "maxTextureSize", 2048.0f },
        FloatFieldCase{ "renderQueue", 2000.0f },
        FloatFieldCase{ "sortPriority", 0.0f },
        FloatFieldCase{ "stencilRef", 0.0f },
        FloatFieldCase{ "stencilReadMask", 255.0f },
        FloatFieldCase{ "stencilWriteMask", 255.0f },
        FloatFieldCase{ "layerMaskStrength", 1.0f },
        FloatFieldCase{ "previewExposure", 1.0f },
        FloatFieldCase{ "previewRotation", 0.0f },
    };

    constexpr std::array<UIntFieldCase, 28> UIntFields = {
        UIntFieldCase{ "preset", 0u },          UIntFieldCase{ "renderMode", 0u },
        UIntFieldCase{ "workflow", 0u },        UIntFieldCase{ "cullMode", 2u },
        UIntFieldCase{ "uvSet", 0u },           UIntFieldCase{ "uvProjection", 0u },
        UIntFieldCase{ "uvWrapU", 0u },         UIntFieldCase{ "uvWrapV", 0u },
        UIntFieldCase{ "uvFilter", 1u },        UIntFieldCase{ "detailBlendMode", 0u },
        UIntFieldCase{ "metallicSource", 0u },  UIntFieldCase{ "roughnessSource", 0u },
        UIntFieldCase{ "aoSource", 0u },        UIntFieldCase{ "opacitySource", 0u },
        UIntFieldCase{ "heightSource", 0u },    UIntFieldCase{ "blendMode", 0u },
        UIntFieldCase{ "srcBlend", 0u },        UIntFieldCase{ "dstBlend", 0u },
        UIntFieldCase{ "depthTest", 2u },       UIntFieldCase{ "layerBlend", 0u },
        UIntFieldCase{ "materialVariant", 0u }, UIntFieldCase{ "technique", 0u },
        UIntFieldCase{ "pcCompression", 0u },   UIntFieldCase{ "macCompression", 0u },
        UIntFieldCase{ "iosCompression", 0u },  UIntFieldCase{ "androidCompression", 0u },
        UIntFieldCase{ "previewShape", 0u },    UIntFieldCase{ "previewEnvironment", 0u },
    };

    constexpr std::array<BoolFieldCase, 19> BoolFields = {
        BoolFieldCase{ "transparent", render::transparentFlag, false },
        BoolFieldCase{ "cutout", render::cutoutFlag, false },
        BoolFieldCase{ "emissionEnabled", render::emissionEnabledFlag, false },
        BoolFieldCase{ "refractionEnabled", render::refractionEnabledFlag, false },
        BoolFieldCase{ "doubleSided", render::doubleSidedFlag, false },
        BoolFieldCase{ "receiveShadows", render::receiveShadowsFlag, true },
        BoolFieldCase{ "castShadows", render::castShadowsFlag, true },
        BoolFieldCase{ "generateMipmaps", render::generateMipmapsFlag, false },
        BoolFieldCase{ "srgb", render::srgbFlag, false },
        BoolFieldCase{ "normalMap", render::normalMapFlag, false },
        BoolFieldCase{ "textureStreaming", render::textureStreamingFlag, false },
        BoolFieldCase{ "depthWrite", render::depthWriteFlag, false },
        BoolFieldCase{ "gpuInstancing", render::gpuInstancingFlag, false },
        BoolFieldCase{ "srpBatcher", render::srpBatcherFlag, false },
        BoolFieldCase{ "receiveDecals", render::receiveDecalsFlag, false },
        BoolFieldCase{ "showUvChecker", render::showUvCheckerFlag, false },
        BoolFieldCase{ "showWireframe", render::showWireframeFlag, false },
        BoolFieldCase{ "showTangents", render::showTangentsFlag, false },
        BoolFieldCase{ "showMipLevels", render::showMipLevelsFlag, false },
    };

    constexpr std::array<StringFieldCase, 2> StringFields = {
        StringFieldCase{ "shaderPath", "", "Shaders/PBR/Production" },
        StringFieldCase{ "keywords", "", "NORMAL_MAP;CLEAR_COAT" },
    };

    String makeMaterialName( const String &testName )
    {
        static u32 nextMaterialId = 0u;
        return String( "GraphicsMaterialTests_" ) + testName + "_" +
               StringUtil::toString( ++nextMaterialId );
    }

    SmartPtr<render::IMaterialManager> getMaterialManager()
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE_MESSAGE( applicationManager, "Application manager is required by material tests" );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        BOOST_REQUIRE_MESSAGE( graphicsSystem, "Graphics system is required by material tests" );

        auto materialManager = graphicsSystem->getMaterialManager();
        if( !materialManager )
        {
            BOOST_TEST_MESSAGE( "Material manager is not available - skipping material test" );
            return nullptr;
        }
        return materialManager;
    }

    SmartPtr<render::IMaterial> createLoadedMaterial( const String &testName )
    {
        auto materialManager = getMaterialManager();
        auto materialResource = materialManager->create( makeMaterialName( testName ) );
        BOOST_REQUIRE_MESSAGE( materialResource, "Material manager did not create a resource" );

        auto material = workphone::dynamic_pointer_cast<render::IMaterial>( materialResource );
        BOOST_REQUIRE_MESSAGE( material, "Created resource is not an IMaterial" );

        if( !material->isLoaded() )
        {
            material->load( nullptr );
        }

        // Ensure at least one technique exists (create default if needed)
        if( material->getNumTechniques() == 0u )
        {
            auto technique = material->createTechnique();
            BOOST_REQUIRE_MESSAGE( technique, "Failed to create default technique" );
            // Ensure technique has at least one pass
            if( technique->getNumPasses() == 0u )
            {
                auto pass = technique->createPass();
                BOOST_REQUIRE_MESSAGE( pass, "Failed to create default pass" );
            }
        }

        BOOST_REQUIRE_MESSAGE( material->getNumTechniques() > 0u,
                               "A created material must have a default technique" );
        return material;
    }

    Array<SmartPtr<render::IMaterialPass>> getPasses( const SmartPtr<render::IMaterial> &material )
    {
        BOOST_REQUIRE( material );

        auto result = Array<SmartPtr<render::IMaterialPass>>();
        for( const auto &technique : material->getTechniques() )
        {
            BOOST_REQUIRE( technique );
            const auto passes = technique->getPasses();
            BOOST_REQUIRE_MESSAGE( !passes.empty(), "Every material technique must contain a pass" );
            for( const auto &pass : passes )
            {
                BOOST_REQUIRE( pass );
                result.push_back( pass );
            }
        }

        BOOST_REQUIRE( !result.empty() );
        return result;
    }

    void checkColour( const ColourF &actual, const ColourF &expected )
    {
        constexpr auto tolerance = 0.0001f;
        BOOST_TEST( actual.r == expected.r, boost::test_tools::tolerance( tolerance ) );
        BOOST_TEST( actual.g == expected.g, boost::test_tools::tolerance( tolerance ) );
        BOOST_TEST( actual.b == expected.b, boost::test_tools::tolerance( tolerance ) );
        BOOST_TEST( actual.a == expected.a, boost::test_tools::tolerance( tolerance ) );
    }

    void checkVector2( const Vector2F &actual, const Vector2F &expected )
    {
        constexpr auto tolerance = 0.0001f;
        BOOST_TEST( actual.X() == expected.X(), boost::test_tools::tolerance( tolerance ) );
        BOOST_TEST( actual.Y() == expected.Y(), boost::test_tools::tolerance( tolerance ) );
    }
}  // namespace

BOOST_AUTO_TEST_SUITE( GraphicsMaterialTests )

BOOST_AUTO_TEST_CASE( material_pass_state_data_has_documented_defaults )
{
    const auto state = MaterialPassStateData();

    for( const auto &field : FloatFields )
    {
        BOOST_TEST_CONTEXT( field.name )
        {
            auto value = std::numeric_limits<f32>::lowest();
            BOOST_REQUIRE( state.getEditorFloat( field.name, value ) );
            BOOST_TEST( value == field.defaultValue, boost::test_tools::tolerance( 0.0001f ) );
        }
    }

    for( const auto &field : UIntFields )
    {
        BOOST_TEST_CONTEXT( field.name )
        {
            auto value = std::numeric_limits<u32>::max();
            BOOST_REQUIRE( state.getEditorUInt( field.name, value ) );
            BOOST_TEST( value == field.defaultValue );
        }
    }

    for( const auto &field : BoolFields )
    {
        BOOST_TEST_CONTEXT( field.name )
        {
            auto value = !field.defaultValue;
            BOOST_REQUIRE( state.getEditorBool( field.name, value ) );
            BOOST_TEST( value == field.defaultValue );
            BOOST_TEST( state.getFlag( field.flag ) == field.defaultValue );
        }
    }

    for( const auto &field : StringFields )
    {
        BOOST_TEST_CONTEXT( field.name )
        {
            auto value = String( "sentinel" );
            BOOST_REQUIRE( state.getEditorString( field.name, value ) );
            BOOST_TEST( value == field.defaultValue );
        }
    }

    auto materialType = std::numeric_limits<u32>::max();
    BOOST_REQUIRE( state.getEditorUInt( "materialType", materialType ) );
    BOOST_TEST( materialType == static_cast<u32>( MaterialType::Standard ) );
}

BOOST_AUTO_TEST_CASE( material_pass_state_data_float_fields_round_trip_and_export )
{
    auto state = MaterialPassStateData();
    auto exportedFloats = Properties();
    auto exportedUInts = Properties();
    auto exportedBools = Properties();
    auto exportedStrings = Properties();

    for( size_t i = 0; i < FloatFields.size(); ++i )
    {
        const auto &field = FloatFields[i];
        const auto expected = static_cast<f32>( i ) + 10.25f;
        BOOST_TEST_CONTEXT( field.name )
        {
            BOOST_REQUIRE( state.setEditorFloat( field.name, expected ) );

            auto actual = 0.0f;
            BOOST_REQUIRE( state.getEditorFloat( field.name, actual ) );
            BOOST_TEST( actual == expected, boost::test_tools::tolerance( 0.0001f ) );
        }
    }

    state.writeEditorSettings( exportedFloats, exportedUInts, exportedBools, exportedStrings );
    BOOST_TEST( exportedFloats.getPropertiesAsArray().size() == FloatFields.size() );

    for( size_t i = 0; i < FloatFields.size(); ++i )
    {
        const auto &field = FloatFields[i];
        const auto expected = static_cast<f32>( i ) + 10.25f;
        BOOST_TEST_CONTEXT( field.name )
        {
            auto actual = 0.0f;
            BOOST_REQUIRE( exportedFloats.getPropertyValue( field.name, actual ) );
            BOOST_TEST( actual == expected, boost::test_tools::tolerance( 0.0001f ) );
        }
    }
}

BOOST_AUTO_TEST_CASE( material_pass_state_data_uint_fields_and_material_type_are_validated )
{
    auto state = MaterialPassStateData();

    for( size_t i = 0; i < UIntFields.size(); ++i )
    {
        const auto &field = UIntFields[i];
        const auto expected = static_cast<u32>( i ) + 100u;
        BOOST_TEST_CONTEXT( field.name )
        {
            BOOST_REQUIRE( state.setEditorUInt( field.name, expected ) );

            auto actual = 0u;
            BOOST_REQUIRE( state.getEditorUInt( field.name, actual ) );
            BOOST_TEST( actual == expected );
        }
    }

    for( auto i = 0u; i < static_cast<u32>( MaterialType::Count ); ++i )
    {
        BOOST_TEST_CONTEXT( "materialType=" << i )
        {
            BOOST_REQUIRE( state.setEditorUInt( "materialType", i ) );
            auto actual = std::numeric_limits<u32>::max();
            BOOST_REQUIRE( state.getEditorUInt( "materialType", actual ) );
            BOOST_TEST( actual == i );
        }
    }

    const auto lastValidMaterialType = static_cast<u32>( MaterialType::Count ) - 1u;
    BOOST_TEST( !state.setEditorUInt( "materialType", static_cast<u32>( MaterialType::Count ) ) );
    BOOST_TEST( !state.setEditorUInt( "materialType", std::numeric_limits<u32>::max() ) );

    auto preservedMaterialType = 0u;
    BOOST_REQUIRE( state.getEditorUInt( "materialType", preservedMaterialType ) );
    BOOST_TEST( preservedMaterialType == lastValidMaterialType );

    auto floats = Properties();
    auto uints = Properties();
    auto bools = Properties();
    auto strings = Properties();
    state.writeEditorSettings( floats, uints, bools, strings );
    BOOST_TEST( uints.getPropertiesAsArray().size() == UIntFields.size() );
    BOOST_TEST( !uints.hasProperty( "materialType" ) );

    for( size_t i = 0; i < UIntFields.size(); ++i )
    {
        const auto &field = UIntFields[i];
        BOOST_TEST_CONTEXT( field.name )
        {
            auto actual = 0u;
            BOOST_REQUIRE( uints.getPropertyValue( field.name, actual ) );
            BOOST_TEST( actual == static_cast<u32>( i ) + 100u );
        }
    }
}

BOOST_AUTO_TEST_CASE( material_pass_state_data_bool_fields_use_independent_flags_and_export )
{
    auto state = MaterialPassStateData();

    for( const auto &field : BoolFields )
    {
        BOOST_TEST_CONTEXT( field.name )
        {
            const auto expected = !field.defaultValue;
            BOOST_REQUIRE( state.setEditorBool( field.name, expected ) );

            auto actual = field.defaultValue;
            BOOST_REQUIRE( state.getEditorBool( field.name, actual ) );
            BOOST_TEST( actual == expected );
            BOOST_TEST( state.getFlag( field.flag ) == expected );
        }
    }

    auto floats = Properties();
    auto uints = Properties();
    auto bools = Properties();
    auto strings = Properties();
    state.writeEditorSettings( floats, uints, bools, strings );
    BOOST_TEST( bools.getPropertiesAsArray().size() == BoolFields.size() );

    for( const auto &field : BoolFields )
    {
        BOOST_TEST_CONTEXT( field.name )
        {
            auto actual = field.defaultValue;
            BOOST_REQUIRE( bools.getPropertyValue( field.name, actual ) );
            BOOST_TEST( actual == !field.defaultValue );
        }
    }
}

BOOST_AUTO_TEST_CASE( material_pass_state_data_string_fields_round_trip_and_export )
{
    auto state = MaterialPassStateData();

    for( const auto &field : StringFields )
    {
        BOOST_TEST_CONTEXT( field.name )
        {
            BOOST_REQUIRE( state.setEditorString( field.name, field.updatedValue ) );

            auto actual = String();
            BOOST_REQUIRE( state.getEditorString( field.name, actual ) );
            BOOST_TEST( actual == field.updatedValue );
        }
    }

    BOOST_REQUIRE( state.setEditorString( "keywords", "" ) );
    auto emptyValue = String( "sentinel" );
    BOOST_REQUIRE( state.getEditorString( "keywords", emptyValue ) );
    BOOST_TEST( emptyValue.empty() );

    auto floats = Properties();
    auto uints = Properties();
    auto bools = Properties();
    auto strings = Properties();
    state.writeEditorSettings( floats, uints, bools, strings );
    BOOST_TEST( strings.getPropertiesAsArray().size() == StringFields.size() );

    for( const auto &field : StringFields )
    {
        BOOST_TEST_CONTEXT( field.name )
        {
            auto actual = String();
            BOOST_REQUIRE( strings.getPropertyValue( field.name, actual ) );
            const auto expected = String( field.name ) == "keywords" ? "" : field.updatedValue;
            BOOST_TEST( actual == expected );
        }
    }
}

BOOST_AUTO_TEST_CASE( material_pass_state_data_rejects_unknown_fields_without_mutation )
{
    auto state = MaterialPassStateData();

    BOOST_TEST( !state.setEditorFloat( "unknown", 1.0f ) );
    BOOST_TEST( !state.setEditorUInt( "unknown", 1u ) );
    BOOST_TEST( !state.setEditorBool( "unknown", true ) );
    BOOST_TEST( !state.setEditorString( "unknown", "value" ) );
    BOOST_TEST( !state.setEditorFloat( "Opacity", 0.5f ) );

    auto floatValue = 12.5f;
    auto uintValue = 42u;
    auto boolValue = true;
    auto stringValue = String( "sentinel" );
    BOOST_TEST( !state.getEditorFloat( "unknown", floatValue ) );
    BOOST_TEST( !state.getEditorUInt( "unknown", uintValue ) );
    BOOST_TEST( !state.getEditorBool( "unknown", boolValue ) );
    BOOST_TEST( !state.getEditorString( "unknown", stringValue ) );
    BOOST_TEST( floatValue == 12.5f );
    BOOST_TEST( uintValue == 42u );
    BOOST_TEST( boolValue );
    BOOST_TEST( stringValue == "sentinel" );

    auto opacity = 0.0f;
    BOOST_REQUIRE( state.getEditorFloat( "opacity", opacity ) );
    BOOST_TEST( opacity == 1.0f );
}

BOOST_AUTO_TEST_CASE( created_material_has_a_valid_owned_graph_and_safe_bounds )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    if( !applicationManager || !applicationManager->getGraphicsSystem() )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping material test" );
        return;
    }
    auto material = createLoadedMaterial( "Graph" );
    const auto techniques = material->getTechniques();

    BOOST_TEST( material->getNumTechniques() == techniques.size() );
    BOOST_TEST( !material->getTechnique( material->getNumTechniques() ) );
    BOOST_TEST( !material->getTechnique( std::numeric_limits<u32>::max() ) );

    const auto children = material->getChildObjects();
    BOOST_TEST( children.size() == techniques.size() );

    for( size_t techniqueIndex = 0; techniqueIndex < techniques.size(); ++techniqueIndex )
    {
        const auto &technique = techniques[techniqueIndex];
        BOOST_TEST_CONTEXT( "technique " << techniqueIndex )
        {
            BOOST_REQUIRE( technique );
            BOOST_TEST( technique->getMaterial().get() == material.get() );
            BOOST_TEST( children[techniqueIndex].get() == technique.get() );

            const auto passes = technique->getPasses();
            BOOST_TEST( technique->getNumPasses() == passes.size() );
            BOOST_TEST( !technique->getPass( technique->getNumPasses() ) );
            BOOST_TEST( !technique->getPass( std::numeric_limits<u32>::max() ) );

            for( size_t passIndex = 0; passIndex < passes.size(); ++passIndex )
            {
                BOOST_TEST_CONTEXT( "pass " << passIndex )
                {
                    BOOST_REQUIRE( passes[passIndex] );
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE( material_techniques_and_texture_units_support_add_remove )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    if( !applicationManager || !applicationManager->getGraphicsSystem() )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping material test" );
        return;
    }
    auto material = createLoadedMaterial( "GraphMutation" );
    const auto originalTechniqueCount = material->getNumTechniques();

    auto technique = material->createTechnique();
    BOOST_REQUIRE( technique );
    BOOST_TEST( technique->getMaterial().get() == material.get() );
    BOOST_TEST( material->getNumTechniques() == originalTechniqueCount + 1u );
    BOOST_TEST( material->getTechnique( originalTechniqueCount ).get() == technique.get() );

    const auto originalPassCount = technique->getNumPasses();
    auto pass = technique->createPass();
    BOOST_REQUIRE( pass );
    BOOST_TEST( technique->getNumPasses() == originalPassCount + 1u );

    const auto originalTextureCount = pass->getNumTexturesNodes();
    auto textureUnit = pass->createTextureUnit();
    BOOST_REQUIRE( textureUnit );

    // Some renderer backends return an unattached unit and expose attachment through the
    // separate addTextureUnit API, while the core implementation attaches during creation.
    if( pass->getNumTexturesNodes() == originalTextureCount )
    {
        pass->addTextureUnit( textureUnit );
    }
    if( !textureUnit->getMaterial() )
    {
        textureUnit->setMaterial( material );
    }

    BOOST_TEST( textureUnit->getMaterial().get() == material.get() );
    BOOST_TEST( pass->getNumTexturesNodes() == originalTextureCount + 1u );

    const auto textureName = String( "Textures/Material Test/albedo.png" );
    textureUnit->setTextureName( textureName );
    BOOST_TEST( textureUnit->getTextureName() == textureName );

    pass->removeTextureUnit( textureUnit );
    BOOST_TEST( pass->getNumTexturesNodes() == originalTextureCount );
    pass->removeTextureUnit( textureUnit );
    BOOST_TEST( pass->getNumTexturesNodes() == originalTextureCount );

    material->removeTechnique( technique );
    BOOST_TEST( material->getNumTechniques() == originalTechniqueCount );
    material->removeTechnique( technique );
    material->removeTechnique( nullptr );
    BOOST_TEST( material->getNumTechniques() == originalTechniqueCount );
}

BOOST_AUTO_TEST_CASE( material_pbr_and_colour_values_propagate_to_every_pass )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    if( !applicationManager || !applicationManager->getGraphicsSystem() )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping material test" );
        return;
    }
    auto material = createLoadedMaterial( "PbrPropagation" );
    const auto passes = getPasses( material );

    const auto diffuse = ColourF( 0.15f, 0.25f, 0.35f, 0.45f );
    const auto specular = ColourF( 0.55f, 0.65f, 0.75f, 0.85f );
    const auto emissive = ColourF( 0.05f, 0.10f, 0.20f, 1.0f );
    material->setDiffuse( diffuse );
    material->setSpecular( specular );
    material->setEmissive( emissive );
    material->setMetalness( 0.37f );
    material->setRoughness( 0.63f );

    checkColour( material->getDiffuse(), diffuse );
    checkColour( material->getSpecular(), specular );
    checkColour( material->getEmissive(), emissive );
    BOOST_TEST( material->getMetalness() == 0.37f, boost::test_tools::tolerance( 0.0001f ) );
    BOOST_TEST( material->getRoughness() == 0.63f, boost::test_tools::tolerance( 0.0001f ) );

    for( const auto &pass : passes )
    {
        checkColour( pass->getDiffuse(), diffuse );
        checkColour( pass->getSpecular(), specular );
        checkColour( pass->getEmissive(), emissive );
        BOOST_TEST( pass->getMetalness() == 0.37f, boost::test_tools::tolerance( 0.0001f ) );
        BOOST_TEST( pass->getRoughness() == 0.63f, boost::test_tools::tolerance( 0.0001f ) );
    }

    material->setMetalness( 0.0f );
    material->setRoughness( 1.0f );
    BOOST_TEST( material->getMetalness() == 0.0f );
    BOOST_TEST( material->getRoughness() == 1.0f );

    material->setSpecularAmount( 0.42f );
    BOOST_TEST( material->getSpecularAmount() == 0.42f, boost::test_tools::tolerance( 0.0001f ) );
    for( const auto &pass : passes )
    {
        checkColour( pass->getSpecular(), ColourF( 0.42f, 0.42f, 0.42f, 1.0f ) );
    }
}

BOOST_AUTO_TEST_CASE( material_render_modes_keep_surface_flags_and_blending_consistent )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    if( !applicationManager || !applicationManager->getGraphicsSystem() )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping material test" );
        return;
    }
    struct RenderModeCase
    {
        u32 mode;
        bool transparent;
        bool cutout;
        u32 blendMode;
    };

    constexpr std::array<RenderModeCase, 7> renderModes = {
        RenderModeCase{ 0u, false, false, 0u }, RenderModeCase{ 1u, false, true, 0u },
        RenderModeCase{ 2u, true, false, 1u },  RenderModeCase{ 3u, true, false, 1u },
        RenderModeCase{ 4u, true, false, 3u },  RenderModeCase{ 5u, true, false, 4u },
        RenderModeCase{ 6u, true, false, 2u },
    };

    auto material = createLoadedMaterial( "RenderModes" );
    const auto passes = getPasses( material );

    for( const auto &testCase : renderModes )
    {
        BOOST_TEST_CONTEXT( "renderMode=" << testCase.mode )
        {
            material->setRenderMode( testCase.mode );
            BOOST_TEST( material->getRenderMode() == testCase.mode );
            BOOST_TEST( material->isTransparent() == testCase.transparent );
            BOOST_TEST( material->isCutout() == testCase.cutout );
            BOOST_TEST( material->getBlendMode() == testCase.blendMode );
            for( const auto &pass : passes )
            {
                BOOST_TEST( pass->isTransparent() == testCase.transparent );
                BOOST_TEST( pass->isCutout() == testCase.cutout );
            }
        }
    }

    material->setRenderMode( std::numeric_limits<u32>::max() );
    BOOST_TEST( material->getRenderMode() == 0u );
    BOOST_TEST( !material->isTransparent() );
    BOOST_TEST( !material->isCutout() );
    BOOST_TEST( material->getBlendMode() == 0u );

    material->setCutout( true );
    material->setTransparent( true );
    BOOST_TEST( material->isTransparent() );
    BOOST_TEST( material->isCutout() );
    material->setCutout( false );
    BOOST_TEST( material->isTransparent() );
    BOOST_TEST( !material->isCutout() );
    material->setTransparent( false );
    BOOST_TEST( !material->isTransparent() );
    BOOST_TEST( !material->isCutout() );
}

BOOST_AUTO_TEST_CASE( material_depth_and_culling_controls_propagate_to_passes )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    if( !applicationManager || !applicationManager->getGraphicsSystem() )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping material test" );
        return;
    }
    auto material = createLoadedMaterial( "DepthAndCulling" );
    const auto passes = getPasses( material );

    material->setDepthWrite( false );
    BOOST_TEST( !material->getDepthWrite() );
    for( const auto &pass : passes )
    {
        BOOST_TEST( !pass->isDepthWriteEnabled() );
    }

    material->setDepthWrite( true );
    BOOST_TEST( material->getDepthWrite() );
    for( const auto &pass : passes )
    {
        BOOST_TEST( pass->isDepthWriteEnabled() );
    }

    material->setDepthTest( 0u );
    BOOST_TEST( material->getDepthTest() == 0u );
    for( const auto &pass : passes )
    {
        BOOST_TEST( !pass->isDepthCheckEnabled() );
    }

    material->setDepthTest( 2u );
    BOOST_TEST( material->getDepthTest() == 2u );
    for( const auto &pass : passes )
    {
        BOOST_TEST( pass->isDepthCheckEnabled() );
    }

    material->setCullMode( 3u );
    BOOST_TEST( material->getCullMode() == 3u );
    for( const auto &pass : passes )
    {
        BOOST_TEST( pass->getCullingMode() == 3u );
    }

    material->setCullMode( 2u );
    BOOST_TEST( material->getCullMode() == 2u );
    for( const auto &pass : passes )
    {
        BOOST_TEST( pass->getCullingMode() == 2u );
    }

    material->setEditorBool( "doubleSided", false );
    for( const auto &pass : passes )
    {
        BOOST_TEST( pass->getCullingMode() == 2u );
    }
}

BOOST_AUTO_TEST_CASE( material_editor_controls_round_trip_and_normalize_invalid_enums )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    if( !applicationManager || !applicationManager->getGraphicsSystem() )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping material test" );
        return;
    }
    auto material = createLoadedMaterial( "EditorControls" );

    material->setNormalStrength( 0.75f );
    material->setDetailNormalStrength( 1.25f );
    material->setAlphaClip( 0.33f );
    BOOST_TEST( material->getNormalStrength() == 0.75f, boost::test_tools::tolerance( 0.0001f ) );
    BOOST_TEST( material->getDetailNormalStrength() == 1.25f, boost::test_tools::tolerance( 0.0001f ) );
    BOOST_TEST( material->getAlphaClip() == 0.33f, boost::test_tools::tolerance( 0.0001f ) );

    for( auto workflow = 0u; workflow <= 3u; ++workflow )
    {
        material->setWorkflow( workflow );
        BOOST_TEST( material->getWorkflow() == workflow );
    }
    material->setWorkflow( std::numeric_limits<u32>::max() );
    BOOST_TEST( material->getWorkflow() == 0u );

    for( auto projection = 0u; projection <= 6u; ++projection )
    {
        material->setUVProjection( projection );
        BOOST_TEST( material->getUVProjection() == projection );
    }
    material->setUVProjection( 7u );
    BOOST_TEST( material->getUVProjection() == 0u );

    for( auto uvSet = 0u; uvSet <= 3u; ++uvSet )
    {
        material->setUVSet( uvSet );
        BOOST_TEST( material->getUVSet() == uvSet );
    }
    material->setUVSet( 4u );
    BOOST_TEST( material->getUVSet() == 0u );

    material->setEditorFloat( "notAFloat", 9.0f );
    material->setEditorUInt( "notAUInt", 9u );
    material->setEditorBool( "notABool", true );
    material->setEditorString( "notAString", "value" );
    BOOST_TEST( material->getEditorFloat( "notAFloat", 1.25f ) == 1.25f );
    BOOST_TEST( material->getEditorUInt( "notAUInt", 27u ) == 27u );
    BOOST_TEST( material->getEditorBool( "notABool", true ) );
    BOOST_TEST( material->getEditorString( "notAString", "fallback" ) == "fallback" );
}

BOOST_AUTO_TEST_CASE( material_uv_and_opacity_controls_handle_boundaries )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    if( !applicationManager || !applicationManager->getGraphicsSystem() )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping material test" );
        return;
    }
    auto material = createLoadedMaterial( "UvAndOpacity" );

    const auto tiling = Vector2F( 2.5f, 0.25f );
    const auto offset = Vector2F( -0.5f, 1.5f );
    material->setUVTiling( tiling );
    material->setUVOffset( offset );
    material->setUVRotation( -45.0f );
    checkVector2( material->getUVTiling(), tiling );
    checkVector2( material->getUVOffset(), offset );
    BOOST_TEST( material->getUVRotation() == -45.0f );

    material->setTriplanarScale( 2.75f );
    BOOST_TEST( material->getTriplanarScale() == 2.75f, boost::test_tools::tolerance( 0.0001f ) );
    material->setTriplanarScale( 0.0f );
    BOOST_TEST( material->getTriplanarScale() == 0.0001f, boost::test_tools::tolerance( 0.000001f ) );
    material->setTriplanarScale( -100.0f );
    BOOST_TEST( material->getTriplanarScale() == 0.0001f, boost::test_tools::tolerance( 0.000001f ) );
    material->setTriplanarScale( std::numeric_limits<f32>::quiet_NaN() );
    BOOST_TEST( material->getTriplanarScale() == 1.0f );
    material->setTriplanarScale( std::numeric_limits<f32>::infinity() );
    BOOST_TEST( material->getTriplanarScale() == 1.0f );

    material->setOpacity( 0.4f );
    BOOST_TEST( material->getOpacity() == 0.4f, boost::test_tools::tolerance( 0.0001f ) );
    BOOST_TEST( material->getDiffuse().a == 0.4f, boost::test_tools::tolerance( 0.0001f ) );
    BOOST_TEST( material->isTransparent() );

    material->setOpacity( -1.0f );
    BOOST_TEST( material->getOpacity() == 0.0f );
    BOOST_TEST( material->getDiffuse().a == 0.0f );
    material->setOpacity( 2.0f );
    BOOST_TEST( material->getOpacity() == 1.0f );
    BOOST_TEST( material->getDiffuse().a == 1.0f );
}

BOOST_AUTO_TEST_CASE( material_properties_accept_partial_symbolic_and_numeric_updates )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    if( !applicationManager || !applicationManager->getGraphicsSystem() )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping material test" );
        return;
    }
    auto material = createLoadedMaterial( "Properties" );
    const auto originalName = material->getName();

    material->setProperties( nullptr );
    BOOST_TEST( material->getName() == originalName );

    auto properties = workphone::make_ptr<Properties>();
    for( auto i = 0u; i < static_cast<u32>( MaterialType::Count ); ++i )
    {
        const auto expected = static_cast<MaterialType>( i );
        const auto symbolicValue = render::GraphicsUtil::getMaterialType( expected );
        properties->setProperty( "Material Type", symbolicValue );
        material->setProperties( properties );
        BOOST_TEST_CONTEXT( symbolicValue )
        {
            BOOST_TEST( static_cast<u32>( material->getMaterialType() ) == i );
            BOOST_TEST( material->getName() == originalName );
        }

        properties->setProperty( "Material Type", StringUtil::toString( i ) );
        material->setProperties( properties );
        BOOST_TEST_CONTEXT( "numeric material type " << i )
        {
            BOOST_TEST( static_cast<u32>( material->getMaterialType() ) == i );
        }
    }

    properties->setProperty( "Material Type", " standardtriplanar " );
    material->setProperties( properties );
    BOOST_TEST( static_cast<u32>( material->getMaterialType() ) ==
                static_cast<u32>( MaterialType::StandardTriPlanar ) );

    const std::array<const char *, 7> invalidValues = {
        "", "   ", "-1", "2.5", "+", "not-a-material-type", "4294967295",
    };
    for( const auto *invalidValue : invalidValues )
    {
        BOOST_TEST_CONTEXT( "invalid material type '" << invalidValue << "'" )
        {
            properties->setProperty( "Material Type", invalidValue );
            material->setProperties( properties );
            BOOST_TEST( static_cast<u32>( material->getMaterialType() ) ==
                        static_cast<u32>( MaterialType::StandardTriPlanar ) );
        }
    }

    material->setMaterialType( static_cast<MaterialType>( MaterialType::Count ) );
    BOOST_TEST( static_cast<u32>( material->getMaterialType() ) ==
                static_cast<u32>( MaterialType::StandardTriPlanar ) );

    auto exported = material->getProperties();
    BOOST_REQUIRE( exported );
    auto exportedMaterialType = String();
    BOOST_REQUIRE( exported->getPropertyValue( "Material Type", exportedMaterialType ) );
    BOOST_TEST( exportedMaterialType == "StandardTriPlanar" );

    const auto &materialTypeProperty = exported->getPropertyObject( "Material Type" );
    BOOST_TEST( materialTypeProperty.getTypeName() == "enum" );
    const auto enumValues = materialTypeProperty.getAttribute( "enum" );
    BOOST_TEST( enumValues.find( "; " ) == String::npos );

    auto parsedEnumValues = Array<String>();
    StringUtil::parseArray( enumValues, parsedEnumValues );
    BOOST_TEST( parsedEnumValues.size() == static_cast<size_t>( MaterialType::Count ) );
    for( size_t i = 0; i < parsedEnumValues.size(); ++i )
    {
        BOOST_TEST( parsedEnumValues[i] ==
                    render::GraphicsUtil::getMaterialType( static_cast<MaterialType>( i ) ) );
    }
}

BOOST_AUTO_TEST_CASE( cloned_material_preserves_serialized_state_and_is_independent )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    if( !applicationManager || !applicationManager->getGraphicsSystem() )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping material test" );
        return;
    }
    auto material = createLoadedMaterial( "CloneSource" );
    material->setMaterialType( MaterialType::StandardTriPlanar );
    material->setRenderMode( 4u );
    material->setWorkflow( 3u );
    material->setDepthWrite( false );
    material->setNormalStrength( 0.65f );
    material->setUVTiling( Vector2F( 3.0f, 4.0f ) );
    material->setUVOffset( Vector2F( 0.1f, 0.2f ) );
    material->setUVRotation( 22.5f );
    material->setTriplanarScale( 1.75f );
    material->setEditorBool( "showWireframe", true );
    material->setEditorString( "shaderPath", "Shaders/Clone/PBR" );
    material->setDiffuse( ColourF( 0.2f, 0.3f, 0.4f, 0.7f ) );
    material->setMetalness( 0.25f );
    material->setRoughness( 0.75f );
    material->setOpacity( 0.7f );

    auto materialManager = getMaterialManager();
    const auto cloneName = makeMaterialName( "CloneTarget" );
    auto clone = materialManager->cloneMaterial( material, cloneName );
    BOOST_REQUIRE( clone );

    BOOST_TEST( clone.get() != material.get() );
    BOOST_TEST( clone->getName() == cloneName );
    BOOST_REQUIRE( clone->getParentPrototype() );
    BOOST_TEST( clone->getParentPrototype().get() == material.get() );
    BOOST_TEST( static_cast<u32>( clone->getMaterialType() ) ==
                static_cast<u32>( MaterialType::StandardTriPlanar ) );

    clone->setWorkflow( 1u );
    clone->setMetalness( 0.9f );
    BOOST_TEST( material->getWorkflow() == 3u );
    BOOST_TEST( material->getMetalness() == 0.25f, boost::test_tools::tolerance( 0.0001f ) );
}

BOOST_AUTO_TEST_CASE( material_texture_unit_resolves_path_and_clears_binding )
{
    TestGuard guard;
    guard.setupThread();
    auto unit = guard.factoryManager->make_object<render::IMaterialTexture>();
    BOOST_REQUIRE( unit );
    auto data = workphone::make_ptr<Properties>();
    data->setProperty( render::IMaterialTexture::texturePathStr, String( "panel.png" ) );
    unit->fromData( data );
    BOOST_REQUIRE( unit->getTexture() );
    BOOST_TEST( unit->getTexture()->isLoaded() );

    unit->setTextureName( String() );
    BOOST_TEST( !unit->getTexture() );
    BOOST_TEST( unit->getTextureName().empty() );
}

BOOST_AUTO_TEST_SUITE_END()

#if WP_GRAPHICS_SYSTEM_CLAW && defined( WP_PLATFORM_WIN32 )
BOOST_AUTO_TEST_CASE( claw_new_material_accepts_edits_and_exports_current_pass_state )
{
    TestGuard guard;
    auto material = dynamic_pointer_cast<render::ClawMaterial>(
        getMaterialManager()->create( makeMaterialName( "NewMaterial" ) ) );
    BOOST_REQUIRE( material );
    material->load( nullptr );
    BOOST_REQUIRE( material->isLoaded() );
    BOOST_REQUIRE_EQUAL( material->getNumTechniques(), 1u );
    BOOST_REQUIRE_EQUAL( material->getTechnique( 0 )->getNumPasses(), 1u );
    material->setDiffuse( ColourF::Red );
    material->setMetalness( 0.25f );
    material->setRoughness( 0.75f );
    material->setNormalStrength( 0.4f );
    material->setRenderMode( 4u );
    material->setCullMode( 3u );
    auto native = material->getNativeMaterial();
    const auto nativeColour = wp_graphics_material_get_diffuse( native );
    checkColour( ColourF( nativeColour.r, nativeColour.g, nativeColour.b, nativeColour.a ), ColourF::Red );
    BOOST_TEST( wp_graphics_material_get_metalness( native ) == 0.25f );
    BOOST_TEST( wp_graphics_material_get_normal_scale( native ) == 0.4f );
    BOOST_TEST( wp_graphics_material_get_blend_mode( native ) == WORKPHONE_BLEND_MODE_ADDITIVE );
    BOOST_TEST( wp_graphics_material_get_cull_mode( native ) == WORKPHONE_CULL_MODE_FRONT );

    // The property grid must not export native defaults over freshly edited pass values.
    auto pass = material->getTechnique( 0 )->getPass( 0 );
    pass->setRoughness( 0.35f );
    BOOST_TEST( pass->getRoughness() == 0.35f );
    BOOST_TEST( material->getRoughness() == 0.35f );
    auto properties = material->getProperties();
    f32 roughness = 0.0f;
    BOOST_REQUIRE( properties->getPropertyValue( "roughness", roughness ) );
    BOOST_TEST( roughness == 0.35f );
    material->setProperties( properties );
    checkColour( material->getDiffuse(), ColourF::Red );
    BOOST_TEST( material->getMetalness() == 0.25f );
    BOOST_TEST( material->getRoughness() == 0.35f );
    auto aoTexture = guard.resourceDatabase->loadResourceByType<render::ITexture>( "panel.png" );
    BOOST_REQUIRE( aoTexture );
    material->setTexture( aoTexture, 22u );
    BOOST_REQUIRE( material->getTexture( 22u ) );
    auto restored = make_ptr<render::ClawMaterial>();
    restored->fromData( material->toData() );
    BOOST_REQUIRE_EQUAL( restored->getTechnique( 0 )->getPass( 0 )->getTextureUnits().size(), 23u );
    BOOST_REQUIRE( restored->getTexture( 22u ) );
    restored->load( nullptr );
    BOOST_REQUIRE_EQUAL( restored->getNumTechniques(), 1u );
    BOOST_REQUIRE_EQUAL( restored->getTechnique( 0 )->getNumPasses(), 1u );
    checkColour( restored->getDiffuse(), ColourF::Red );
    BOOST_TEST( wp_graphics_material_get_roughness( restored->getNativeMaterial() ) == 0.35f );
    BOOST_REQUIRE( restored->getTexture( 22u ) );
    BOOST_TEST( restored->getTexture( 22u )->getName() == aoTexture->getName() );
}

BOOST_AUTO_TEST_CASE( claw_scene_renders_assigned_mesh_material_colour )
{
    TestGuard guard;
    auto scene = guard.graphicsSystem->addGraphicsScene( "DefaultScene", "MaterialColourTest" );
    BOOST_REQUIRE( scene );
    guard.addCleanup( [graphics = guard.graphicsSystem, scene]() mutable {
        graphics->clearObjectQueues();
        graphics->removeGraphicsScene( scene );
    } );
    auto mesh = dynamic_pointer_cast<render::ClawMesh>(
        scene->addGraphicsObjectByType<render::IGraphicsMesh>() );
    BOOST_REQUIRE( mesh );
    mesh->load( nullptr );
    mesh->setVisible( true );
    mesh->setVisibilityFlags( 0xFFFFFFFFu );
    auto node = scene->addSceneNode( "MaterialColourNode" );
    BOOST_REQUIRE( node );
    node->load( nullptr );
    node->attachObject( mesh );

    const wp_graphics_mesh_vertex_ptc vertices[] = {
        { { -0.75f, -0.75f, 0.5f }, { 0.0f, 1.0f }, 0xFFFFFFFFu },
        { { 0.75f, -0.75f, 0.5f }, { 1.0f, 1.0f }, 0xFFFFFFFFu },
        { { 0.0f, 0.75f, 0.5f }, { 0.5f, 0.0f }, 0xFFFFFFFFu }
    };
    BOOST_REQUIRE( wp_graphics_mesh_set_vertices( mesh->getNativeMesh(), WORKPHONE_VERTEX_FORMAT_PTC,
                                                   vertices, 3 ) );
    const wp_u32 indices[] = { 0u, 1u, 2u };
    BOOST_REQUIRE( wp_graphics_mesh_set_indices_u32( mesh->getNativeMesh(), indices, 3 ) );
    auto material = createLoadedMaterial( "SceneColour" );
    material->setEditorBool( "doubleSided", true );
    material->setSpecular( ColourF::Black );
    material->setEmissionEnabled( true );
    mesh->setMaterial( material, 0 );

    // An unshown Win32 window gives DX11 a swap chain without opening a UI.
    auto windowHandle = CreateWindowExW( 0, L"STATIC", L"Material render test", WS_POPUP,
                                         0, 0, 64, 64, nullptr, nullptr, GetModuleHandleW( nullptr ),
                                         nullptr );
    BOOST_REQUIRE( windowHandle );
    guard.addCleanup( [windowHandle]() { DestroyWindow( windowHandle ); } );
    struct TestWindow : render::ClawWindow
    {
        HWND handle = nullptr;
        Vector2I getSize() const override { return Vector2I( 64, 64 ); }
        void getWindowHandle( void *data ) override { *static_cast<void **>( data ) = handle; }
    };
    auto window = make_ptr<TestWindow>();
    window->handle = windowHandle;
    auto renderer = make_ptr<render::ClawRendererDX11>();
    renderer->load( window );
    BOOST_REQUIRE( renderer->getNativeRenderer() );
    renderer->beginRender();
    auto dx11 = wp_renderer_get_dx11( renderer->getNativeRenderer() );
    auto device = static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( dx11 ) );
    auto context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( dx11 ) );
    auto swapChain = static_cast<IDXGISwapChain *>( wp_renderer_dx11_get_swap_chain( dx11 ) );
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    BOOST_REQUIRE( SUCCEEDED( swapChain->GetBuffer( 0, IID_PPV_ARGS( &backBuffer ) ) ) );
    D3D11_TEXTURE2D_DESC desc;
    backBuffer->GetDesc( &desc );
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> readback;
    BOOST_REQUIRE( SUCCEEDED( device->CreateTexture2D( &desc, nullptr, &readback ) ) );
    struct TestTexture : render::Texture
    {
        void *view = nullptr;
        ~TestTexture() override { wp_renderer_dx11_destroy_texture_native( view ); }
        void getTextureFinal( void **data ) const override { *data = view; }
    };
    auto texture = make_ptr<TestTexture>();
    const u8 bluePixel[] = { 255, 0, 0, 255 };
    texture->view = wp_renderer_dx11_create_texture_native(
        dx11, bluePixel, 1, 1, WORKPHONE_PIXEL_FORMAT_BGRA8 );
    BOOST_REQUIRE( texture->view );
    for( const auto colour : { ColourF::Red, ColourF::Green, ColourF::White } )
    {
        material->setDiffuse( colour );
        material->setEmissive( colour == ColourF::White ? ColourF::Black : colour );
        if( colour == ColourF::White )
            material->setTexture( texture, 0 );
        checkColour( mesh->getMaterial( 0 )->getDiffuse(), colour );
        renderer->clear( ColourF::Black );
        dynamic_pointer_cast<render::ClawScene>( scene )->render(
            static_cast<render::IRenderer *>( renderer.get() ) );
        context->CopyResource( readback.Get(), backBuffer.Get() );
        D3D11_MAPPED_SUBRESOURCE pixels;
        BOOST_REQUIRE( SUCCEEDED( context->Map( readback.Get(), 0, D3D11_MAP_READ, 0, &pixels ) ) );
        const auto pixel = static_cast<const u8 *>( pixels.pData ) + 32 * pixels.RowPitch + 32 * 4;
        const auto red = pixel[0];
        const auto green = pixel[1];
        const auto blue = pixel[2];
        context->Unmap( readback.Get(), 0 );
        if( colour == ColourF::Red )
            BOOST_TEST( static_cast<int>( red ) > static_cast<int>( green ) + 20 );
        else if( colour == ColourF::Green )
            BOOST_TEST( static_cast<int>( green ) > static_cast<int>( red ) + 20 );
        else
        {
            BOOST_TEST( static_cast<int>( blue ) > static_cast<int>( red ) + 20 );
            BOOST_TEST( static_cast<int>( blue ) > static_cast<int>( green ) + 20 );
        }
        if( colour != ColourF::White )
            BOOST_TEST( static_cast<int>( blue ) < 20 );
    }
    const auto drawPixel = [&]( ColourF clear = ColourF::Black,
                                const Matrix4F &transform = Matrix4F::identity() ) {
        renderer->clear( clear );
        renderer->renderMesh( mesh.get(), transform );
        context->CopyResource( readback.Get(), backBuffer.Get() );
        D3D11_MAPPED_SUBRESOURCE pixels;
        BOOST_REQUIRE( SUCCEEDED( context->Map( readback.Get(), 0, D3D11_MAP_READ, 0, &pixels ) ) );
        const auto pixel = static_cast<const u8 *>( pixels.pData ) + 32 * pixels.RowPitch + 32 * 4;
        const auto result = std::array<int, 3>{ pixel[0], pixel[1], pixel[2] };
        context->Unmap( readback.Get(), 0 );
        return result;
    };
    material->setMaterialType( MaterialType::UI );
    material->setTexture( SmartPtr<render::ITexture>(), 0u );
    material->setDiffuse( ColourF::Black );
    material->setEmissive( ColourF::Red );
    material->setEditorFloat( "emissionIntensity", 1.0f );
    BOOST_TEST( drawPixel()[0] > 200 );
    material->setEditorFloat( "emissionIntensity", 0.0f );
    BOOST_TEST( drawPixel()[0] == 0 );
    material->setEditorFloat( "emissionIntensity", 1.0f );
    material->setEmissionEnabled( false );
    BOOST_TEST( drawPixel()[0] == 0 );

    material->setDiffuse( ColourF( 1.0f, 0.0f, 0.0f, 0.25f ) );
    material->setRenderMode( 1u );
    material->setAlphaClip( 0.5f );
    BOOST_TEST( drawPixel()[0] == 0 );
    material->setAlphaClip( 0.1f );
    BOOST_TEST( drawPixel()[0] > 200 );
    material->setRenderMode( 3u );
    material->setOpacity( 0.25f );
    material->setDepthWrite( false );
    const auto alphaPixel = drawPixel();
    BOOST_TEST( alphaPixel[0] > 50 );
    BOOST_TEST( alphaPixel[0] < 80 );
    material->setRenderMode( 6u );
    const auto premultipliedPixel = drawPixel();
    BOOST_TEST( std::abs( premultipliedPixel[0] - alphaPixel[0] ) <= 1 );
    material->setRenderMode( 4u );
    const auto additivePixel = drawPixel( ColourF::Green );
    BOOST_TEST( additivePixel[0] > 50 );
    BOOST_TEST( additivePixel[1] > 200 );

    material->setRenderMode( 0u );
    material->setOpacity( 1.0f );
    material->setDepthTest( 0u );
    BOOST_TEST( drawPixel()[0] == 0 );
    material->setDepthTest( 2u );
    BOOST_TEST( drawPixel()[0] > 200 );
    material->setEditorBool( "doubleSided", false );
    material->setCullMode( 2u );
    const auto backCulled = drawPixel()[0];
    material->setCullMode( 3u );
    const auto frontCulled = drawPixel()[0];
    BOOST_TEST( ( backCulled > 200 ) != ( frontCulled > 200 ) );
    material->setCullMode( 1u );
    BOOST_TEST( drawPixel()[0] > 200 );

    // Texture transforms and addressing must be evaluated for each draw.
    auto stripe = make_ptr<TestTexture>();
    const u8 stripePixels[] = { 255, 0, 0, 255, 0, 0, 255, 255 };
    stripe->view = wp_renderer_dx11_create_texture_native( dx11, stripePixels, 2, 1, WORKPHONE_PIXEL_FORMAT_BGRA8 );
    BOOST_REQUIRE( stripe->view );
    material->setTexture( stripe, 0u );
    material->setDiffuse( ColourF::White );
    material->setUVTiling( Vector2F( 0.0f, 0.0f ) );
    material->setUVOffset( Vector2F( 0.0f, 0.0f ) );
    material->setEditorUInt( "uvFilter", 0u );
    BOOST_TEST( drawPixel()[2] > 200 );
    material->setUVOffset( Vector2F( 0.75f, 0.0f ) );
    BOOST_TEST( drawPixel()[0] > 200 );
    material->setUVOffset( Vector2F( 1.0f, 0.0f ) );
    material->setEditorUInt( "uvWrapU", 0u );
    BOOST_TEST( drawPixel()[2] > 200 );
    material->setEditorUInt( "uvWrapU", 1u );
    BOOST_TEST( drawPixel()[0] > 200 );

    // Generated projections must sample coordinates independent of mesh UVs.
    auto quadrants = make_ptr<TestTexture>();
    const u8 quadrantPixels[] = { 255, 0, 0, 255, 0, 0, 255, 255,
                                  0, 255, 0, 255, 255, 255, 255, 255 };
    quadrants->view = wp_renderer_dx11_create_texture_native(
        dx11, quadrantPixels, 2, 2, WORKPHONE_PIXEL_FORMAT_BGRA8 );
    BOOST_REQUIRE( quadrants->view );
    material->setTexture( quadrants, 0u );
    material->setUVTiling( Vector2F( 1.0f, 1.0f ) );
    material->setUVOffset( Vector2F( 0.495f, 0.25f ) );
    material->setEditorUInt( "uvWrapU", 0u );
    material->setEditorUInt( "uvWrapV", 0u );
    const auto checkProjectedPixel = [&]( u32 projection, const std::array<int, 3> &expected,
                                          const Matrix4F &transform = Matrix4F::identity() ) {
        material->setUVProjection( projection );
        const auto pixel = drawPixel( ColourF::Black, transform );
        for( size_t channel = 0; channel < expected.size(); ++channel )
            BOOST_TEST_CONTEXT( "Projection " << projection << ", channel " << channel )
                BOOST_TEST( std::abs( pixel[channel] - expected[channel] ) < 5 );
    };
    checkProjectedPixel( 0u, { 0, 255, 0 } ); // Mesh UV.
    checkProjectedPixel( 3u, { 255, 255, 255 } ); // World XZ.
    checkProjectedPixel( 4u, { 255, 0, 0 } ); // World XY.
    checkProjectedPixel( 5u, { 0, 255, 0 } ); // World YZ.
    checkProjectedPixel( 1u, { 255, 0, 0 } ); // Box projection chooses this face's XY plane.
    material->setTriplanarScale( 2.0f );
    checkProjectedPixel( 3u, { 255, 0, 0 } );
    material->setTriplanarScale( 1.0f );
    auto translated = Matrix4F::identity();
    translated.makeTransform( Vector3F( 0.3f, 0.0f, 0.0f ), Vector3F( 1.0f, 1.0f, 1.0f ),
                               QuaternionF::identity() );
    checkProjectedPixel( 1u, { 255, 0, 0 }, translated ); // World mapping stays anchored.
    checkProjectedPixel( 2u, { 0, 0, 255 }, translated ); // Object mapping follows the mesh.

    auto constantUvVertices = std::array<wp_graphics_mesh_vertex_ptc, 3>{ vertices[0], vertices[1], vertices[2] };
    for( auto &vertex : constantUvVertices ) vertex.uv = { 0.1f, 0.1f };
    BOOST_REQUIRE( wp_graphics_mesh_set_vertices( mesh->getNativeMesh(), WORKPHONE_VERTEX_FORMAT_PTC,
                                                 constantUvVertices.data(), 3 ) );
    material->setUVOffset( Vector2F::zero() );
    checkProjectedPixel( 0u, { 0, 0, 255 } );
    checkProjectedPixel( 6u, { 255, 255, 255 } );
    BOOST_REQUIRE( wp_graphics_mesh_set_vertices( mesh->getNativeMesh(), WORKPHONE_VERTEX_FORMAT_PTC,
                                                 vertices, 3 ) );
    material->setUVProjection( 0u );

    // Non-albedo texture slots must reach the shader and clear when unassigned.
    material->setTexture( texture, 13u );
    material->setTexture( SmartPtr<render::ITexture>(), 0u );
    material->setDiffuse( ColourF::Black );
    material->setEmissive( ColourF::White );
    material->setEmissionEnabled( true );
    BOOST_TEST( drawPixel()[2] > 200 );
    BOOST_TEST( drawPixel()[0] == 0 );
    material->setTexture( SmartPtr<render::ITexture>(), 13u );
    BOOST_TEST( drawPixel()[0] > 200 );

    // Normal strength and AO are checked against rendered pixels, not just getters.
    material->setMaterialType( MaterialType::Standard );
    material->setEmissionEnabled( false );
    material->setDiffuse( ColourF::White );
    material->setUVTiling( Vector2F( 1.0f, 1.0f ) );
    material->setUVOffset( Vector2F::zero() );
    material->setMetalness( 0.0f );
    material->setSpecular( ColourF::Black );
    renderer->setSceneLighting( ColourF( 0.2f, 0.2f, 0.2f, 1.0f ),
        Vector3F( 0.0f, 0.0f, backCulled > 200 ? -1.0f : 1.0f ), ColourF::White, 2.0f );
    auto normalMap = make_ptr<TestTexture>();
    const u8 normalPixel[] = { 128, 128, 255, 255 };
    normalMap->view = wp_renderer_dx11_create_texture_native( dx11, normalPixel, 1, 1, WORKPHONE_PIXEL_FORMAT_BGRA8 );
    BOOST_REQUIRE( normalMap->view );
    material->setTexture( normalMap, 1u );
    material->setNormalStrength( 0.0f );
    const auto flatNormalPixel = drawPixel()[0];
    material->setNormalStrength( 1.0f );
    BOOST_TEST( flatNormalPixel > drawPixel()[0] + 15 );
    material->setTexture( SmartPtr<render::ITexture>(), 1u );
    renderer->setSceneLighting( ColourF( 0.2f, 0.2f, 0.2f, 1.0f ), Vector3F( 0, 0, -1 ), ColourF::White, 0.0f );
    auto blackMap = make_ptr<TestTexture>();
    const u8 blackPixel[] = { 0, 0, 0, 255 };
    blackMap->view = wp_renderer_dx11_create_texture_native( dx11, blackPixel, 1, 1, WORKPHONE_PIXEL_FORMAT_BGRA8 );
    BOOST_REQUIRE( blackMap->view );
    material->setTexture( blackMap, 22u );
    material->setEditorFloat( "aoStrength", 1.0f );
    BOOST_TEST( drawPixel()[0] == 0 );
    material->setEditorFloat( "aoStrength", 0.0f );
    BOOST_TEST( drawPixel()[0] > 50 );
    material->setTexture( blackMap, 24u );
    material->setEditorUInt( "opacitySource", 1u );
    material->setRenderMode( 1u );
    material->setAlphaClip( 0.5f );
    BOOST_TEST( drawPixel()[0] == 0 );
    material->setTexture( SmartPtr<render::ITexture>(), 24u );
    BOOST_TEST( drawPixel()[0] > 50 );
    renderer->unload( nullptr );
}
#endif
