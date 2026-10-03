#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

namespace
{
    auto parameterTypeValue( ParameterType type ) -> int
    {
        return static_cast<int>( type );
    }
}  // namespace

BOOST_AUTO_TEST_CASE( parameter_default_and_pointer_values )
{
    Parameter empty;
    BOOST_CHECK_EQUAL( parameterTypeValue( empty.type ),
                       parameterTypeValue( ParameterType::PARAM_TYPE_NULL ) );
    BOOST_CHECK_EQUAL( empty.getPtr(), nullptr );

    int payload = 42;
    Parameter pointerParam( &payload );
    BOOST_CHECK_EQUAL( parameterTypeValue( pointerParam.type ),
                       parameterTypeValue( ParameterType::PARAM_TYPE_PTR ) );
    BOOST_CHECK_EQUAL( pointerParam.getPtr(), &payload );

    pointerParam.setPtr( nullptr );
    BOOST_CHECK_EQUAL( parameterTypeValue( pointerParam.type ),
                       parameterTypeValue( ParameterType::PARAM_TYPE_PTR ) );
    BOOST_CHECK_EQUAL( pointerParam.getPtr(), nullptr );
}

BOOST_AUTO_TEST_CASE( parameter_primitive_round_trips )
{
    Parameter boolParam( true );
    BOOST_CHECK_EQUAL( parameterTypeValue( boolParam.type ),
                       parameterTypeValue( ParameterType::PARAM_TYPE_BOOL ) );
    BOOST_CHECK( boolParam.getBool() );

    Parameter u8Param( static_cast<u8>( 255 ) );
    BOOST_CHECK_EQUAL( parameterTypeValue( u8Param.type ),
                       parameterTypeValue( ParameterType::PARAM_TYPE_U8 ) );
    BOOST_CHECK_EQUAL( u8Param.getU8(), static_cast<u8>( 255 ) );

    Parameter u16Param( static_cast<u16>( 65535 ) );
    BOOST_CHECK_EQUAL( parameterTypeValue( u16Param.type ),
                       parameterTypeValue( ParameterType::PARAM_TYPE_U16 ) );
    BOOST_CHECK_EQUAL( u16Param.getU16(), static_cast<u16>( 65535 ) );

    Parameter s32Param( static_cast<s32>( -12345 ) );
    BOOST_CHECK_EQUAL( parameterTypeValue( s32Param.type ),
                       parameterTypeValue( ParameterType::PARAM_TYPE_S32 ) );
    BOOST_CHECK_EQUAL( s32Param.getS32(), static_cast<s32>( -12345 ) );

    Parameter u32Param( static_cast<u32>( 12345 ) );
    BOOST_CHECK_EQUAL( parameterTypeValue( u32Param.type ),
                       parameterTypeValue( ParameterType::PARAM_TYPE_U32 ) );
    BOOST_CHECK_EQUAL( u32Param.getU32(), static_cast<u32>( 12345 ) );

    Parameter f32Param( 1.25f );
    BOOST_CHECK_EQUAL( parameterTypeValue( f32Param.type ),
                       parameterTypeValue( ParameterType::PARAM_TYPE_F32 ) );
    BOOST_CHECK_CLOSE( f32Param.getF32(), 1.25f, 0.001f );

    constexpr s64 s64Value = 0x123456789ABCDEF0LL;
    Parameter s64Param( s64Value );
    BOOST_CHECK_EQUAL( parameterTypeValue( s64Param.type ),
                       parameterTypeValue( ParameterType::PARAM_TYPE_S64 ) );
    BOOST_CHECK_EQUAL( s64Param.getS64(), s64Value );

    constexpr f64 f64Value = 1.23456789012345;
    Parameter f64Param( f64Value );
    BOOST_CHECK_EQUAL( parameterTypeValue( f64Param.type ),
                       parameterTypeValue( ParameterType::PARAM_TYPE_F64 ) );
    BOOST_CHECK_EQUAL( f64Param.getF64(), f64Value );
}

BOOST_AUTO_TEST_CASE( parameter_string_array_and_object_values )
{
    Parameter stringParam( String( "engine" ) );
    BOOST_CHECK_EQUAL( parameterTypeValue( stringParam.type ),
                       parameterTypeValue( ParameterType::PARAM_TYPE_STR ) );
    BOOST_CHECK_EQUAL( stringParam.getStr(), "engine" );

    Array<Parameter> values;
    values.emplace_back( static_cast<s32>( 7 ) );
    values.emplace_back( false );

    Parameter arrayParam( values );
    BOOST_CHECK_EQUAL( parameterTypeValue( arrayParam.type ),
                       parameterTypeValue( ParameterType::PARAM_TYPE_ARRAY ) );
    BOOST_REQUIRE_EQUAL( arrayParam.getArray().size(), 2 );
    BOOST_CHECK_EQUAL( arrayParam.getArray()[0].getS32(), 7 );
    BOOST_CHECK( !arrayParam.getArray()[1].getBool() );

    auto object = workphone::make_ptr<ISharedObject>();
    Parameter objectParam( object );
    BOOST_CHECK_EQUAL( parameterTypeValue( objectParam.type ),
                       parameterTypeValue( ParameterType::PARAM_TYPE_OBJECT ) );
    BOOST_CHECK_EQUAL( objectParam.getObject(), object );
}

BOOST_AUTO_TEST_CASE( parameter_preserves_native_event_hash )
{
    const auto eventHash = IEvent::handleValueChanged;
    Parameter eventHashParam( eventHash );

#if WP_USE_HASH32
    BOOST_CHECK_EQUAL( parameterTypeValue( eventHashParam.type ),
                       parameterTypeValue( ParameterType::PARAM_TYPE_S32 ) );
    BOOST_CHECK_EQUAL( eventHashParam.getS32(), eventHash );
#else
    BOOST_CHECK_EQUAL( parameterTypeValue( eventHashParam.type ),
                       parameterTypeValue( ParameterType::PARAM_TYPE_S64 ) );
    BOOST_CHECK_EQUAL( eventHashParam.getS64(), eventHash );
    BOOST_CHECK_NE( eventHashParam.getS64(), static_cast<s64>( static_cast<u32>( eventHash ) ) );
#endif
}

BOOST_AUTO_TEST_CASE( parameter_equality_uses_active_storage )
{
    Parameter stringA( static_cast<s32>( 77 ) );
    stringA.setStr( "alpha" );

    Parameter stringB( static_cast<s32>( 77 ) );
    stringB.setStr( "beta" );

    BOOST_CHECK( !( stringA == stringB ) );

    stringB.setStr( "alpha" );
    BOOST_CHECK( stringA == stringB );

    Array<Parameter> arrayA;
    arrayA.emplace_back( static_cast<s32>( 1 ) );

    Array<Parameter> arrayB;
    arrayB.emplace_back( static_cast<s32>( 2 ) );

    Parameter arrayParamA( static_cast<s32>( 99 ) );
    arrayParamA.setArray( arrayA );

    Parameter arrayParamB( static_cast<s32>( 99 ) );
    arrayParamB.setArray( arrayB );

    BOOST_CHECK( !( arrayParamA == arrayParamB ) );

    arrayParamB.setArray( arrayA );
    BOOST_CHECK( arrayParamA == arrayParamB );

    int legacyPayload = 0;
    auto objectA = workphone::make_ptr<ISharedObject>();
    auto objectB = workphone::make_ptr<ISharedObject>();

    Parameter objectParamA( &legacyPayload );
    objectParamA.setObject( objectA );

    Parameter objectParamB( &legacyPayload );
    objectParamB.setObject( objectB );

    BOOST_CHECK( !( objectParamA == objectParamB ) );

    objectParamB.setObject( objectA );
    BOOST_CHECK( objectParamA == objectParamB );
}

BOOST_AUTO_TEST_CASE( parameter_equality_handles_edge_types )
{
    Parameter pointerA;
    Parameter pointerB;
    BOOST_CHECK( pointerA == pointerB );

    int firstPayload = 1;
    int secondPayload = 2;
    pointerA.setPtr( &firstPayload );
    pointerB.setPtr( &firstPayload );
    BOOST_CHECK( pointerA == pointerB );

    pointerB.setPtr( &secondPayload );
    BOOST_CHECK( !( pointerA == pointerB ) );

    Parameter stringParam( String( "42" ) );
    Parameter intParam( static_cast<s32>( 42 ) );
    BOOST_CHECK( !( stringParam == intParam ) );

    Parameter vectorA( static_cast<s32>( 5 ) );
    vectorA.setVector2( Vector2F( 1.0f, 2.0f ) );

    Parameter vectorB( static_cast<s32>( 5 ) );
    vectorB.setVector2( Vector2F( 1.0f, 3.0f ) );

    BOOST_CHECK( !( vectorA == vectorB ) );

    vectorB.setVector2( Vector2F( 1.0f, 2.0f ) );
    BOOST_CHECK( vectorA == vectorB );

    Parameter charPtrA;
    Parameter charPtrB;
    charPtrA.setCharPtr( nullptr );
    charPtrB.setCharPtr( nullptr );
    BOOST_CHECK( charPtrA == charPtrB );
}

BOOST_AUTO_TEST_CASE( parameter_vector_helpers_resize_and_preserve_values )
{
    Parameter vector2Param;
    vector2Param.setVector2( Vector2F( 2.5f, -4.0f ) );
    BOOST_REQUIRE_EQUAL( vector2Param.getArray().size(), 2 );

    auto vector2 = vector2Param.getVector2();
    BOOST_CHECK_CLOSE( vector2.x, 2.5f, 0.001f );
    BOOST_CHECK_CLOSE( vector2.y, -4.0f, 0.001f );

    Parameter vector3Param;
    vector3Param.setVector3( Vector3<real_Num>( 1.0f, 2.0f, 3.0f ) );
    BOOST_REQUIRE_EQUAL( vector3Param.getArray().size(), 3 );

    auto vector3 = vector3Param.getVector3();
    BOOST_CHECK_CLOSE( static_cast<f32>( vector3.x ), 1.0f, 0.001f );
    BOOST_CHECK_CLOSE( static_cast<f32>( vector3.y ), 2.0f, 0.001f );
    BOOST_CHECK_CLOSE( static_cast<f32>( vector3.z ), 3.0f, 0.001f );

    Parameter quatParam;
    quatParam.setQuaternion( Quaternion<real_Num>( 1.0f, 0.0f, 0.5f, -0.5f ) );
    BOOST_REQUIRE_EQUAL( quatParam.getArray().size(), 4 );

    auto quaternion = quatParam.getQuaternion();
    BOOST_CHECK_CLOSE( static_cast<f32>( quaternion[0] ), 1.0f, 0.001f );
    BOOST_CHECK_CLOSE( static_cast<f32>( quaternion[1] ), 0.0f, 0.001f );
    BOOST_CHECK_CLOSE( static_cast<f32>( quaternion[2] ), 0.5f, 0.001f );
    BOOST_CHECK_CLOSE( static_cast<f32>( quaternion[3] ), -0.5f, 0.001f );
}

BOOST_AUTO_TEST_CASE( atomic_float_exchange_compare_and_arithmetic )
{
    atomic_f32 value( 10.0f );

    BOOST_CHECK_CLOSE( value.fetch_and_store( 12.5f ), 10.0f, 0.001f );
    BOOST_CHECK_CLOSE( static_cast<f32>( value ), 12.5f, 0.001f );

    BOOST_CHECK_CLOSE( value.compare_and_swap( 20.0f, 12.5f ), 12.5f, 0.001f );
    BOOST_CHECK_CLOSE( static_cast<f32>( value ), 20.0f, 0.001f );

    BOOST_CHECK_CLOSE( value.compare_and_swap( 30.0f, 12.5f ), 20.0f, 0.001f );
    BOOST_CHECK_CLOSE( static_cast<f32>( value ), 20.0f, 0.001f );

    BOOST_CHECK_CLOSE( value += 2.0f, 22.0f, 0.001f );
    BOOST_CHECK_CLOSE( value -= 5.0f, 17.0f, 0.001f );
    BOOST_CHECK_CLOSE( value *= 2.0f, 34.0f, 0.001f );
    BOOST_CHECK_CLOSE( value /= 4.0f, 8.5f, 0.001f );

    BOOST_CHECK_CLOSE( value.fetch_and_add( 1.5f ), 8.5f, 0.001f );
    BOOST_CHECK_CLOSE( static_cast<f32>( value ), 10.0f, 0.001f );
}
