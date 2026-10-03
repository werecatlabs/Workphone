#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

BOOST_AUTO_TEST_CASE( properties_create )
{
    // Test basic creation
    auto props = workphone::make_ptr<Properties>();
    BOOST_CHECK( props );
    BOOST_CHECK_EQUAL( props->getPropertiesAsArray().size(), 0 );
    BOOST_CHECK_EQUAL( props->getNumChildren(), 0 );
}

BOOST_AUTO_TEST_CASE( properties_add )
{
    auto props = workphone::make_ptr<Properties>();

    // Test adding properties using addProperty method
    props->addProperty( "testString", "value1", "string" );
    BOOST_CHECK( props->hasProperty( "testString" ) );
    BOOST_CHECK_EQUAL( props->getProperty( "testString" ), "value1" );

    // Test adding property with Property object
    Property prop;
    prop.setName( "testInt" );
    prop.setValueAsInt( 42 );
    props->addProperty( prop );
    BOOST_CHECK( props->hasProperty( "testInt" ) );
    BOOST_CHECK_EQUAL( props->getPropertyAsInt( "testInt" ), 42 );

    // Test overwriting existing property via addProperty
    props->addProperty( "testString", "value2", "string" );
    BOOST_CHECK_EQUAL( props->getProperty( "testString" ), "value2" );
}

BOOST_AUTO_TEST_CASE( properties_set )
{
    auto props = workphone::make_ptr<Properties>();

    // Test setting string property
    props->setProperty( "name", "TestName" );
    BOOST_CHECK_EQUAL( props->getProperty( "name" ), "TestName" );

    // Test setting boolean property
    props->setProperty( "enabled", true );
    BOOST_CHECK_EQUAL( props->getPropertyAsBool( "enabled" ), true );

    // Test setting integer property
    props->setProperty( "count", 123 );
    BOOST_CHECK_EQUAL( props->getPropertyAsInt( "count" ), 123 );

    // Test setting unsigned integer property
    props->setProperty( "ucount", static_cast<u32>( 456 ) );
    BOOST_CHECK_EQUAL( props->getPropertyAsInt( "ucount" ), 456 );

    // Test setting float property
    props->setProperty( "rate", 3.14f );
    BOOST_CHECK_CLOSE( props->getPropertyAsFloat( "rate" ), 3.14f, 0.001f );

    // Test setting double property
    props->setProperty( "dratio", 2.718 );
    BOOST_CHECK_CLOSE( props->getPropertyAsFloat( "dratio" ), 2.718f, 0.001f );

    // Test setting Vector2I property
    props->setProperty( "position2i", Vector2I( 10, 20 ) );
    Vector2I vec2i;
    BOOST_CHECK( props->getPropertyValue( "position2i", vec2i ) );
    BOOST_CHECK_EQUAL( vec2i.x, 10 );
    BOOST_CHECK_EQUAL( vec2i.y, 20 );

    // Test setting Vector2F property
    props->setProperty( "position2f", Vector2F( 1.5f, 2.5f ) );
    Vector2F vec2f;
    BOOST_CHECK( props->getPropertyValue( "position2f", vec2f ) );
    BOOST_CHECK_CLOSE( vec2f.x, 1.5f, 0.001f );
    BOOST_CHECK_CLOSE( vec2f.y, 2.5f, 0.001f );

    // Test setting Vector3F property
    props->setProperty( "position", Vector3F( 1.0f, 2.0f, 3.0f ) );
    //BOOST_CHECK_EQUAL( props->getPropertyAsVector3( "position" ), Vector3F( 1.0f, 2.0f, 3.0f ) );

    // Test setting Vector3D property
    props->setProperty( "positionD", Vector3D( 1.0, 2.0, 3.0 ) );
    //BOOST_CHECK_EQUAL( props->getPropertyAsVector3D( "positionD" ), Vector3D( 1.0, 2.0, 3.0 ) );

    // Test setting QuaternionF property
    QuaternionF quat( 1.0f, 0.0f, 0.0f, 0.0f );
    props->setProperty( "rotation", quat );
    QuaternionF quatOut;
    BOOST_CHECK( props->getPropertyValue( "rotation", quatOut ) );

    // Test setting ColourF property
    props->setProperty( "color", ColourF( 1.0f, 0.5f, 0.25f, 1.0f ) );
    ColourF colorOut;
    BOOST_CHECK( props->getPropertyValue( "color", colorOut ) );

    // Test setting Array<String> property
    Array<String> stringArray = { "one", "two", "three" };
    props->setProperty( "items", stringArray );
    Array<String> stringArrayOut;
    BOOST_CHECK( props->getPropertyValue( "items", stringArrayOut ) );
    BOOST_CHECK_EQUAL( stringArrayOut.size(), 3 );

    // Test updating existing property
    props->setProperty( "count", 999 );
    BOOST_CHECK_EQUAL( props->getPropertyAsInt( "count" ), 999 );

    // Test read-only flag
    props->setProperty( "readonly", "immutable", true );
    BOOST_CHECK( props->hasProperty( "readonly" ) );
    const auto &readOnlyProp = props->getPropertyObject( "readonly" );
    BOOST_CHECK_EQUAL( readOnlyProp.isReadOnly(), true );
}

BOOST_AUTO_TEST_CASE( properties_remove )
{
    auto props = workphone::make_ptr<Properties>();

    // Add some properties
    props->setProperty( "prop1", "value1" );
    props->setProperty( "prop2", 42 );
    props->setProperty( "prop3", true );

    BOOST_CHECK_EQUAL( props->getPropertiesAsArray().size(), 3 );

    // Test removing existing property
    BOOST_CHECK( props->removeProperty( "prop2" ) );
    BOOST_CHECK( !props->hasProperty( "prop2" ) );
    BOOST_CHECK_EQUAL( props->getPropertiesAsArray().size(), 2 );

    // Test removing non-existent property
    BOOST_CHECK( !props->removeProperty( "nonexistent" ) );
    BOOST_CHECK_EQUAL( props->getPropertiesAsArray().size(), 2 );

    // Test remaining properties are intact
    BOOST_CHECK_EQUAL( props->getProperty( "prop1" ), "value1" );
    BOOST_CHECK_EQUAL( props->getPropertyAsBool( "prop3" ), true );

    // Remove all remaining properties
    BOOST_CHECK( props->removeProperty( "prop1" ) );
    BOOST_CHECK( props->removeProperty( "prop3" ) );
    BOOST_CHECK_EQUAL( props->getPropertiesAsArray().size(), 0 );
}

BOOST_AUTO_TEST_CASE( properties_to_string )
{
    auto input = workphone::make_ptr<Properties>();
    input->setName( "test" );
    input->setProperty( "base_id", 1 );
    input->setProperty( "actorId", 123456 );

    auto child = workphone::make_ptr<Properties>();
    child->setName( "child" );
    child->setProperty( "childValue", 42 );
    input->addChild( child );

    auto dataStr = DataUtil::toString( input.get() );
    BOOST_CHECK( !StringUtil::isNullOrEmpty( dataStr ) );

    auto properties = workphone::make_ptr<Properties>();
    DataUtil::parse( dataStr, properties.get() );
    BOOST_CHECK( properties );

    BOOST_CHECK( properties->getPropertyAsInt( "actorId" ) == 123456 );
    BOOST_CHECK_EQUAL( properties->getNumChildren(), 1 );
    BOOST_CHECK_EQUAL( properties->getChild( 0 )->getName(), "child" );
    BOOST_CHECK_EQUAL( properties->getChild( 0 )->getPropertyAsInt( "childValue" ), 42 );

    properties->setProperty( "actorId", 654321 );
    BOOST_CHECK( properties->getPropertyAsInt( "actorId" ) == 654321 );
}

BOOST_AUTO_TEST_CASE( properties_binary_round_trip_preserves_generic_metadata )
{
    auto input = workphone::make_ptr<Properties>();
    input->setName( "sceneRoot" );

    Property componentValue( "velocity", "1.5 2.5 3.5", "vector3f" );
    componentValue.setReadOnly( true );
    componentValue.setAttribute( "units", "metresPerSecond" );
    componentValue.setAttribute( "editorHint", "direction" );
    input->addProperty( componentValue );

    auto component = workphone::make_ptr<Properties>();
    component->setName( "component" );
    component->setProperty( "componentType", "FutureComponent" );
    component->setProperty( "futureField", "schema-independent value" );
    input->addChild( component );

    auto bytes = PropertiesBinarySerializer::serialize( *input );
    BOOST_REQUIRE( !bytes.empty() );
    BOOST_CHECK( PropertiesBinarySerializer::hasBinaryHeader( bytes.data(), bytes.size() ) );

    Properties output;
    String error;
    BOOST_REQUIRE_MESSAGE( PropertiesBinarySerializer::deserialize( bytes, output, &error ), error );

    BOOST_CHECK_EQUAL( output.getName(), "sceneRoot" );
    BOOST_REQUIRE_EQUAL( output.getNumProperties(), 1u );

    const auto &outputValue = output.getPropertyObject( "velocity" );
    BOOST_CHECK_EQUAL( outputValue.getValue(), "1.5 2.5 3.5" );
    BOOST_CHECK_EQUAL( outputValue.getTypeName(), "vector3f" );
    BOOST_CHECK( outputValue.isReadOnly() );
    BOOST_CHECK_EQUAL( outputValue.getAttribute( "units" ), "metresPerSecond" );
    BOOST_CHECK_EQUAL( outputValue.getAttribute( "editorHint" ), "direction" );

    auto outputComponent = output.getChild( "component" );
    BOOST_REQUIRE( outputComponent );
    BOOST_CHECK_EQUAL( outputComponent->getProperty( "componentType" ), "FutureComponent" );
    BOOST_CHECK_EQUAL( outputComponent->getProperty( "futureField" ), "schema-independent value" );
}

BOOST_AUTO_TEST_CASE( properties_binary_rejects_truncated_data_without_mutating_destination )
{
    Properties input;
    input.setProperty( "source", "complete" );
    auto bytes = PropertiesBinarySerializer::serialize( input );
    BOOST_REQUIRE_GT( bytes.size(), 1u );
    bytes.pop_back();

    Properties output;
    output.setName( "unchanged" );
    output.setProperty( "sentinel", 42 );

    String error;
    BOOST_CHECK( !PropertiesBinarySerializer::deserialize( bytes, output, &error ) );
    BOOST_CHECK( !error.empty() );
    BOOST_CHECK_EQUAL( output.getName(), "unchanged" );
    BOOST_CHECK_EQUAL( output.getPropertyAsInt( "sentinel" ), 42 );
}

BOOST_AUTO_TEST_CASE( properties_default_values )
{
    auto props = workphone::make_ptr<Properties>();

    // Test default values for non-existent properties
    BOOST_CHECK_EQUAL( props->getProperty( "missing", "default" ), "default" );
    BOOST_CHECK_EQUAL( props->getPropertyAsBool( "missing", true ), true );
    BOOST_CHECK_EQUAL( props->getPropertyAsInt( "missing", -1 ), -1 );
    BOOST_CHECK_CLOSE( props->getPropertyAsFloat( "missing", 9.99f ), 9.99f, 0.001f );
    //BOOST_CHECK_EQUAL( props->getPropertyAsVector3( "missing", Vector3F( 5.0f, 6.0f, 7.0f ) ),
    //                   Vector3F( 5.0f, 6.0f, 7.0f ) );
}

BOOST_AUTO_TEST_CASE( properties_children )
{
    auto parent = workphone::make_ptr<Properties>();
    parent->setName( "parent" );

    auto child1 = workphone::make_ptr<Properties>();
    child1->setName( "child1" );
    child1->setProperty( "id", 1 );

    auto child2 = workphone::make_ptr<Properties>();
    child2->setName( "child2" );
    child2->setProperty( "id", 2 );

    auto child3 = workphone::make_ptr<Properties>();
    child3->setName( "child1" );  // Same name as child1
    child3->setProperty( "id", 3 );

    // Test adding children
    parent->addChild( child1 );
    parent->addChild( child2 );
    parent->addChild( child3 );

    BOOST_CHECK_EQUAL( parent->getNumChildren(), 3 );

    // Test getting children
    auto children = parent->getChildren();
    BOOST_CHECK_EQUAL( children.size(), 3 );

    // Test getting child by index
    auto childByIndex = parent->getChild( 0 );
    BOOST_CHECK( childByIndex );
    BOOST_CHECK_EQUAL( childByIndex->getName(), "child1" );

    // Test getting child by name (returns first match)
    auto childByName = parent->getChild( "child2" );
    BOOST_CHECK( childByName );
    BOOST_CHECK_EQUAL( childByName->getPropertyAsInt( "id" ), 2 );

    // Test getting children by name (returns all matches)
    auto childrenByName = parent->getChildrenByName( "child1" );
    BOOST_CHECK_EQUAL( childrenByName.size(), 2 );

    // Test hasChild
    BOOST_CHECK( parent->hasChild( "child1" ) );
    BOOST_CHECK( parent->hasChild( "child2" ) );
    BOOST_CHECK( !parent->hasChild( "nonexistent" ) );

    // Test removing child
    parent->removeChild( "child2" );
    BOOST_CHECK_EQUAL( parent->getNumChildren(), 2 );
    BOOST_CHECK( !parent->hasChild( "child2" ) );
}

BOOST_AUTO_TEST_CASE( properties_clear_all )
{
    auto parent = workphone::make_ptr<Properties>();
    parent->setProperty( "parentProp", "value" );

    auto child = workphone::make_ptr<Properties>();
    child->setName( "child" );
    child->setProperty( "childProp", 42 );

    parent->addChild( child );

    // Test clearAll without cascade
    parent->clearAll( false );
    BOOST_CHECK_EQUAL( parent->getPropertiesAsArray().size(), 0 );
    BOOST_CHECK_EQUAL( child->getPropertiesAsArray().size(), 1 );  // Child still has properties

    // Test clearAll with cascade
    parent->setProperty( "parentProp", "value" );
    parent->clearAll( true );
    BOOST_CHECK_EQUAL( parent->getPropertiesAsArray().size(), 0 );
    BOOST_CHECK_EQUAL( child->getPropertiesAsArray().size(), 0 );  // Child properties cleared
}

BOOST_AUTO_TEST_CASE( properties_copy_constructor )
{
    auto original = workphone::make_ptr<Properties>();
    original->setName( "original" );
    original->setProperty( "name", "test" );
    original->setProperty( "count", 100 );
    original->setProperty( "enabled", true );

    auto child = workphone::make_ptr<Properties>();
    child->setName( "child" );
    child->setProperty( "childProp", "value" );
    original->addChild( child );

    // Create copy
    Properties copy( *original );

    // Verify properties are copied
    BOOST_CHECK_EQUAL( copy.getProperty( "name" ), "test" );
    BOOST_CHECK_EQUAL( copy.getPropertyAsInt( "count" ), 100 );
    BOOST_CHECK_EQUAL( copy.getPropertyAsBool( "enabled" ), true );

    // Verify children are copied
    BOOST_CHECK_EQUAL( copy.getNumChildren(), 1 );
}

BOOST_AUTO_TEST_CASE( properties_assignment_operator )
{
    auto original = workphone::make_ptr<Properties>();
    original->setProperty( "name", "original" );
    original->setProperty( "value", 42 );

    auto assigned = workphone::make_ptr<Properties>();
    assigned->setProperty( "other", "data" );

    // Perform assignment
    *assigned = *original;

    // Verify properties are copied
    BOOST_CHECK_EQUAL( assigned->getProperty( "name" ), "original" );
    BOOST_CHECK_EQUAL( assigned->getPropertyAsInt( "value" ), 42 );

    // Verify old properties are replaced
    BOOST_CHECK( !assigned->hasProperty( "other" ) );
}

BOOST_AUTO_TEST_CASE( properties_property_value_equals )
{
    using namespace workphone;

    auto props = workphone::make_ptr<Properties>();
    props->setProperty( "status", "active" );
    props->setProperty( "count", 5 );

    // Test propertyValueEquals
    BOOST_CHECK( props->propertyValueEquals( "status", "active" ) );
    BOOST_CHECK( !props->propertyValueEquals( "status", "inactive" ) );
    BOOST_CHECK( !props->propertyValueEquals( "nonexistent", "active" ) );
}

BOOST_AUTO_TEST_CASE( properties_set_property_type )
{
    auto props = workphone::make_ptr<Properties>();
    props->setProperty( "value", "123", "string" );

    // Test setting property type
    BOOST_CHECK( props->setPropertyType( "value", "int" ) );

    s32 intValue = 0;
    BOOST_CHECK( props->getPropertyValue( "value", intValue ) );
    BOOST_CHECK_EQUAL( intValue, 123 );

    // Test setting type of non-existent property
    BOOST_CHECK( !props->setPropertyType( "nonexistent", "int" ) );
}

BOOST_AUTO_TEST_CASE( property_set_value_keeps_typed_data_in_sync )
{
    Property constructedInt( "count", "123", "int" );
    BOOST_CHECK_EQUAL( constructedInt.getValueAsInt(), 123 );

    Property constructedFloat( "ratio", "2.5", "double" );
    BOOST_CHECK_CLOSE( constructedFloat.getValueAsFloat(), 2.5f, 0.001f );

    Property delayedUInt;
    delayedUInt.setValue( "456" );
    delayedUInt.setType( ParameterType::PARAM_TYPE_U32 );
    BOOST_CHECK_EQUAL( delayedUInt.getValueAsUInt(), static_cast<u32>( 456 ) );

    delayedUInt.setValue( "789" );
    BOOST_CHECK_EQUAL( delayedUInt.getValueAsUInt(), static_cast<u32>( 789 ) );

    Property delayedBool;
    delayedBool.setValue( "true" );
    delayedBool.setTypeName( "bool" );
    BOOST_CHECK( delayedBool.getValueAsBool() );

    delayedBool.setValue( "" );
    BOOST_CHECK( !delayedBool.getValueAsBool() );

    Property button;
    button.setType( ParameterType::PARAM_TYPE_BUTTON );
    button.setValue( "yes" );
    BOOST_CHECK( button.getValueAsButton() );

    Property smallSigned;
    smallSigned.setType( ParameterType::PARAM_TYPE_S8 );
    smallSigned.setValue( "-8" );
    BOOST_CHECK_EQUAL( smallSigned.getValueAsInt(), -8 );

    Property smallUnsigned;
    smallUnsigned.setType( ParameterType::PARAM_TYPE_U16 );
    smallUnsigned.setValue( "65535" );
    BOOST_CHECK_EQUAL( smallUnsigned.getValueAsUInt(), static_cast<u32>( 65535 ) );
}

BOOST_AUTO_TEST_CASE( properties_string_value_type_parses_cached_data )
{
    auto props = workphone::make_ptr<Properties>();

    props->setProperty( "freshInt", "321", "int" );
    s32 intValue = 0;
    BOOST_CHECK( props->getPropertyValue( "freshInt", intValue ) );
    BOOST_CHECK_EQUAL( intValue, 321 );

    props->setProperty( "freshUInt", "654", "u32" );
    u32 uintValue = 0;
    BOOST_CHECK( props->getPropertyValue( "freshUInt", uintValue ) );
    BOOST_CHECK_EQUAL( uintValue, static_cast<u32>( 654 ) );

    props->setProperty( "flag", "false", "bool" );
    bool boolValue = true;
    BOOST_CHECK( props->getPropertyValue( "flag", boolValue ) );
    BOOST_CHECK( !boolValue );

    props->setProperty( "flag", "true", "bool" );
    BOOST_CHECK( props->getPropertyValue( "flag", boolValue ) );
    BOOST_CHECK( boolValue );
}

BOOST_AUTO_TEST_CASE( properties_enum )
{
    auto props = workphone::make_ptr<Properties>();

    Array<String> enumValues = { "Low", "Medium", "High" };

    // Test setting enum by value
    props->setPropertyAsEnum( "quality", 1, enumValues );
    BOOST_CHECK( props->hasProperty( "quality" ) );
    BOOST_CHECK_EQUAL( props->getPropertyAsInt( "quality" ), 1 );

    // Test setting enum by string value
    props->setPropertyAsEnum( "priority", "2", enumValues );
    BOOST_CHECK_EQUAL( props->getPropertyAsInt( "priority" ), 2 );
}

BOOST_AUTO_TEST_CASE( properties_button )
{
    auto props = workphone::make_ptr<Properties>();

    // Test setting button property
    props->setButtonPressed( "clickMe" );
    BOOST_CHECK( props->hasProperty( "clickMe" ) );
    BOOST_CHECK( !props->isButtonPressed( "clickMe" ) );

    // Test button pressed state
    props->setButtonPressed( "clickMe", true );
    BOOST_CHECK( props->isButtonPressed( "clickMe" ) );

    props->setButtonPressed( "clickMe", false );
    BOOST_CHECK( !props->isButtonPressed( "clickMe" ) );

    // Test non-existent button
    BOOST_CHECK( !props->isButtonPressed( "nonexistent" ) );
}

BOOST_AUTO_TEST_CASE( properties_edge_cases )
{
    auto props = workphone::make_ptr<Properties>();

    // Test empty property name
    props->setProperty( "", "value" );
    BOOST_CHECK( props->hasProperty( "" ) );
    BOOST_CHECK_EQUAL( props->getProperty( "" ), "value" );

    // Test empty property value
    props->setProperty( "empty", "" );
    BOOST_CHECK_EQUAL( props->getProperty( "empty" ), "" );

    // Test special characters in property names
    props->setProperty( "prop@#$", "special" );
    auto value = props->getProperty( "prop@#$" );
    BOOST_CHECK_EQUAL( value, String( "special" ) );

    // Test very long property value
    String longValue( 100, 'x' );
    props->setProperty( "long", longValue );
    BOOST_CHECK_EQUAL( props->getProperty( "long" ).length(), 100 );

    // Test numeric edge values
    props->setProperty( "maxInt", std::numeric_limits<s32>::max() );
    BOOST_CHECK_EQUAL( props->getPropertyAsInt( "maxInt" ), std::numeric_limits<s32>::max() );

    props->setProperty( "minInt", std::numeric_limits<s32>::min() );
    BOOST_CHECK_EQUAL( props->getPropertyAsInt( "minInt" ), std::numeric_limits<s32>::min() );

    // Test zero values
    props->setProperty( "zeroInt", 0 );
    props->setProperty( "zeroFloat", 0.0f );
    BOOST_CHECK_EQUAL( props->getPropertyAsInt( "zeroInt" ), 0 );
    BOOST_CHECK_CLOSE( props->getPropertyAsFloat( "zeroFloat" ), 0.0f, 0.001f );

    // Test negative values
    props->setProperty( "negativeInt", -42 );
    props->setProperty( "negativeFloat", -3.14f );
    BOOST_CHECK_EQUAL( props->getPropertyAsInt( "negativeInt" ), -42 );
    BOOST_CHECK_CLOSE( props->getPropertyAsFloat( "negativeFloat" ), -3.14f, 0.001f );
}

BOOST_AUTO_TEST_CASE( properties_array_operations )
{
    auto props = workphone::make_ptr<Properties>();
    props->setProperty( "prop1", "value1" );
    props->setProperty( "prop2", 42 );
    props->setProperty( "prop3", true );

    // Test getPropertiesAsArray
    auto propArray = props->getPropertiesAsArray();
    BOOST_CHECK_EQUAL( propArray.size(), 3 );

    // Test setPropertiesAsArray
    Array<Property> newProps;
    Property p1;
    p1.setName( "newProp1" );
    p1.setValue( "newValue1" );
    newProps.push_back( p1 );

    Property p2;
    p2.setName( "newProp2" );
    p2.setValueAsInt( 99 );
    newProps.push_back( p2 );

    props->setPropertiesAsArray( newProps );
    BOOST_CHECK_EQUAL( props->getPropertiesAsArray().size(), 2 );
    BOOST_CHECK( props->hasProperty( "newProp1" ) );
    BOOST_CHECK( props->hasProperty( "newProp2" ) );
    BOOST_CHECK( !props->hasProperty( "prop1" ) );
}

BOOST_AUTO_TEST_CASE( properties_get_property_value )
{
    auto props = workphone::make_ptr<Properties>();
    props->setProperty( "string", "test" );
    props->setProperty( "bool", true );
    props->setProperty( "int", 42 );
    props->setProperty( "float", 3.14f );

    // Test getPropertyValue for various types
    String stringVal;
    BOOST_CHECK( props->getPropertyValue( "string", stringVal ) );
    BOOST_CHECK_EQUAL( stringVal, "test" );

    bool boolVal;
    BOOST_CHECK( props->getPropertyValue( "bool", boolVal ) );
    BOOST_CHECK_EQUAL( boolVal, true );

    s32 intVal;
    BOOST_CHECK( props->getPropertyValue( "int", intVal ) );
    BOOST_CHECK_EQUAL( intVal, 42 );

    f32 floatVal;
    BOOST_CHECK( props->getPropertyValue( "float", floatVal ) );
    BOOST_CHECK_CLOSE( floatVal, 3.14f, 0.001f );

    // Test getPropertyValue for non-existent property
    String missingVal;
    BOOST_CHECK( !props->getPropertyValue( "nonexistent", missingVal ) );
}

BOOST_AUTO_TEST_CASE( properties_transform )
{
    auto props = workphone::make_ptr<Properties>();

    // Test Transform3F
    Transform3F transform;
    transform.setPosition( Vector3F( 1.0f, 2.0f, 3.0f ) );
    transform.setRotation( Vector3F( 0.0f, 90.0f, 0.0f ) );
    transform.setScale( Vector3F( 2.0f, 2.0f, 2.0f ) );

    props->setProperty( "transform", transform );
    BOOST_CHECK( props->hasProperty( "transform" ) );

    Transform3F transformOut;
    BOOST_CHECK( props->getPropertyValue( "transform", transformOut ) );
    //BOOST_CHECK_EQUAL( transformOut.getPosition(), Vector3F( 1.0f, 2.0f, 3.0f ) );
    //BOOST_CHECK_EQUAL( transformOut.getScale(), Vector3F( 2.0f, 2.0f, 2.0f ) );
}

BOOST_AUTO_TEST_CASE( properties_aabb )
{
    auto props = workphone::make_ptr<Properties>();

    // Test AABB3F
    AABB3F aabb;
    aabb.setMinimum( Vector3F( -1.0f, -1.0f, -1.0f ) );
    aabb.setMaximum( Vector3F( 1.0f, 1.0f, 1.0f ) );

    props->setProperty( "bounds", aabb );
    BOOST_CHECK( props->hasProperty( "bounds" ) );

    AABB3F aabbOut;
    BOOST_CHECK( props->getPropertyValue( "bounds", aabbOut ) );
    //BOOST_CHECK_EQUAL( aabbOut.getMinimum(), Vector3F( -1.0f, -1.0f, -1.0f ) );
    //BOOST_CHECK_EQUAL( aabbOut.getMaximum(), Vector3F( 1.0f, 1.0f, 1.0f ) );
}

BOOST_AUTO_TEST_CASE( properties_multiple_properties_same_type )
{
    auto props = workphone::make_ptr<Properties>();

    // Add multiple properties of same type
    for( int i = 0; i < 100; ++i )
    {
        auto name = "prop" + StringUtil::toString( i );
        props->setProperty( name, i );
    }

    BOOST_CHECK_EQUAL( props->getPropertiesAsArray().size(), 100 );

    // Verify all properties
    for( int i = 0; i < 100; ++i )
    {
        auto name = "prop" + StringUtil::toString( i );
        BOOST_CHECK_EQUAL( props->getPropertyAsInt( name ), i );
    }
}

BOOST_AUTO_TEST_CASE( properties_overwrite_different_types )
{
    auto props = workphone::make_ptr<Properties>();

    // Set property as string
    props->setProperty( "value", "text" );
    BOOST_CHECK_EQUAL( props->getProperty( "value" ), "text" );

    // Overwrite with int
    props->setProperty( "value", 42 );
    BOOST_CHECK_EQUAL( props->getPropertyAsInt( "value" ), 42 );

    // Overwrite with float
    props->setProperty( "value", 3.14f );
    BOOST_CHECK_CLOSE( props->getPropertyAsFloat( "value" ), 3.14f, 0.001f );

    // Overwrite with bool
    props->setProperty( "value", true );
    BOOST_CHECK_EQUAL( props->getPropertyAsBool( "value" ), true );
}
