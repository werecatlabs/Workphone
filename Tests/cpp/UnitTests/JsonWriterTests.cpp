#include "UnitTests.hpp"
#include <Workphone/Core/JsonParser.hpp>
#include <Workphone/Core/JsonWriter.hpp>
#include <boost/test/unit_test.hpp>
#include <limits>

using namespace workphone;

BOOST_AUTO_TEST_CASE( json_writer_writes_nested_document )
{
    StringOutput output;
    JsonWriter writer( output );

    writer.beginObject();
    writer.key( "name" );
    writer.string( "workphone" );
    writer.key( "flags" );
    writer.beginArray();
    writer.boolean( true );
    writer.boolean( false );
    writer.null();
    writer.endArray();
    writer.key( "nested" );
    writer.beginObject();
    writer.key( "answer" );
    writer.number( 42.0 );
    writer.key( "text" );
    writer.string( "quote \" slash \\ newline\n" );
    writer.endObject();
    writer.endObject();

    writer.finish();

    BOOST_CHECK( writer.isComplete() );
    BOOST_CHECK_EQUAL( output.buffer,
                       "{\"name\":\"workphone\",\"flags\":[true,false,null],\"nested\":{\"answer\":42,"
                       "\"text\":\"quote \\\" "
                       "slash \\\\ newline\\n\"}}" );

    JsonParser parser( output.buffer.c_str(), static_cast<u32>( output.buffer.size() ) );
    auto parsed = parser.parse();
    BOOST_CHECK( std::holds_alternative<JsonObject>( parsed ) );
}

BOOST_AUTO_TEST_CASE( json_writer_escapes_control_characters )
{
    StringOutput output;
    JsonWriter writer( output );

    String value;
    value += static_cast<c8>( 0x01 );
    value += '\b';
    value += '\f';
    value += '\n';
    value += '\r';
    value += '\t';

    writer.string( value );
    writer.finish();

    BOOST_CHECK_EQUAL( output.buffer, "\"\\u0001\\b\\f\\n\\r\\t\"" );
}

BOOST_AUTO_TEST_CASE( json_writer_rejects_multiple_root_values )
{
    StringOutput output;
    JsonWriter writer( output );

    writer.string( "first" );
    BOOST_CHECK_THROW( writer.string( "second" ), std::runtime_error );

    writer.finish();
    BOOST_CHECK_EQUAL( output.buffer, "\"first\"" );
}

BOOST_AUTO_TEST_CASE( json_writer_rejects_object_value_without_key )
{
    StringOutput output;
    JsonWriter writer( output );

    writer.beginObject();
    BOOST_CHECK_THROW( writer.string( "orphan" ), std::runtime_error );
    writer.endObject();
    writer.finish();

    BOOST_CHECK_EQUAL( output.buffer, "{}" );
}

BOOST_AUTO_TEST_CASE( json_writer_rejects_dangling_object_key )
{
    StringOutput output;
    JsonWriter writer( output );

    writer.beginObject();
    writer.key( "missing" );

    BOOST_CHECK_THROW( writer.key( "next" ), std::runtime_error );
    BOOST_CHECK_THROW( writer.endObject(), std::runtime_error );
    BOOST_CHECK_THROW( writer.finish(), std::runtime_error );
    BOOST_CHECK( !writer.isComplete() );
}

BOOST_AUTO_TEST_CASE( json_writer_rejects_mismatched_container_close )
{
    StringOutput output;
    JsonWriter writer( output );

    writer.beginArray();
    BOOST_CHECK_THROW( writer.endObject(), std::runtime_error );
    writer.endArray();
    writer.finish();

    BOOST_CHECK_EQUAL( output.buffer, "[]" );
}

BOOST_AUTO_TEST_CASE( json_writer_rejects_non_finite_numbers_without_changing_state )
{
    StringOutput output;
    JsonWriter writer( output );

    BOOST_CHECK_THROW( writer.number( std::numeric_limits<f64>::infinity() ), std::invalid_argument );
    BOOST_CHECK_THROW( writer.number( -std::numeric_limits<f64>::infinity() ), std::invalid_argument );
    BOOST_CHECK_THROW( writer.number( std::numeric_limits<f64>::quiet_NaN() ), std::invalid_argument );
    BOOST_CHECK( !writer.isComplete() );

    writer.number( 1.0 );
    writer.finish();

    BOOST_CHECK_EQUAL( output.buffer, "1" );
}

BOOST_AUTO_TEST_CASE( json_writer_rejects_invalid_utf8_without_output )
{
    StringOutput output;
    JsonWriter writer( output );

    String invalid;
    invalid += static_cast<c8>( 0xC0 );

    BOOST_CHECK_THROW( writer.string( invalid ), std::invalid_argument );
    BOOST_CHECK_EQUAL( output.buffer, "" );
    BOOST_CHECK( !writer.isComplete() );
}

BOOST_AUTO_TEST_CASE( json_writer_finish_reports_incomplete_documents )
{
    StringOutput emptyOutput;
    JsonWriter emptyWriter( emptyOutput );
    BOOST_CHECK_THROW( emptyWriter.finish(), std::runtime_error );

    StringOutput arrayOutput;
    JsonWriter arrayWriter( arrayOutput );
    arrayWriter.beginArray();
    arrayWriter.boolean( true );

    BOOST_CHECK_THROW( arrayWriter.finish(), std::runtime_error );
    BOOST_CHECK( !arrayWriter.isComplete() );
}
