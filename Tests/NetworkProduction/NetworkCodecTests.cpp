#include <Workphone/Interface/Net/NetworkCodec.hpp>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#define CHECK( condition )                                                      \
    do                                                                          \
    {                                                                           \
        if( !( condition ) )                                                    \
        {                                                                       \
            std::fprintf( stderr, "FAIL line %d: %s\n", __LINE__, #condition ); \
            return 1;                                                           \
        }                                                                       \
    } while( false )

template <class Error, class Fn>
bool throws( Fn fn )
{
    try
    {
        fn();
    }
    catch( const Error & )
    {
        return true;
    }
    return false;
}

int main()
{
    using namespace workphone::network;
    std::vector<unsigned char> bytes;
    write( bytes, std::uint16_t( 0x1234 ) );
    write( bytes, std::int32_t( -2147483647 - 1 ) );
    write( bytes, 1.0f );
    const std::vector<unsigned char> golden = { 0x34, 0x12, 0, 0, 0, 0x80, 0, 0, 0x80, 0x3f };
    CHECK( bytes == golden );
    size_t cursor = 0;
    std::uint16_t small = 0;
    std::int32_t signedValue = 0;
    float number = 0;
    read( bytes, cursor, small );
    read( bytes, cursor, signedValue );
    read( bytes, cursor, number );
    CHECK( small == 0x1234 && signedValue == ( -2147483647 - 1 ) && number == 1.0f );
    CHECK( cursor == bytes.size() );
    CHECK( throws<std::out_of_range>( [&] { read( bytes, cursor, signedValue ); } ) );
    CHECK( cursor == bytes.size() && signedValue == ( -2147483647 - 1 ) );
    CHECK(
        throws<std::out_of_range>( [&] { requireBytes( 1, std::numeric_limits<size_t>::max(), 8 ); } ) );
    CHECK(
        throws<std::out_of_range>( [&] { requireBytes( std::numeric_limits<size_t>::max(), 0, 8 ); } ) );

    for( int n = -128; n <= 127; ++n )
    {
        bytes.clear();
        write( bytes, static_cast<std::int8_t>( n ) );
        cursor = 0;
        std::int8_t value = 0;
        read( bytes, cursor, value );
        CHECK( value == n );
    }
    bytes.clear();
    writeString( bytes, std::string( "a\0b", 3 ) );
    std::string string = "unchanged";
    cursor = 0;
    readString( bytes, cursor, string );
    CHECK( string == std::string( "a\0b", 3 ) );
    bytes.pop_back();
    cursor = 0;
    string = "unchanged";
    CHECK( throws<std::out_of_range>( [&] { readString( bytes, cursor, string ); } ) );
    CHECK( cursor == 0 && string == "unchanged" );
    bytes = { 0xff, 0xff, 0xff, 0xff };
    CHECK( throws<std::length_error>( [&] { readString( bytes, cursor, string ); } ) );
    CHECK( cursor == 0 && string == "unchanged" );
    const auto previous = bytes;
    CHECK( throws<std::length_error>(
        [&] { writeString( bytes, std::string( MaxStringBytes + 1, 'x' ) ); } ) );
    CHECK( bytes == previous );

    bytes = { 2 };
    cursor = 0;
    bool flag = false;
    CHECK( throws<std::invalid_argument>( [&] { readBool( bytes, cursor, flag ); } ) );
    CHECK( cursor == 0 && !flag );
    CHECK( throws<std::invalid_argument>( [&] { assignBytes( bytes, nullptr, 1 ); } ) );
    CHECK( bytes == std::vector<unsigned char>( { 2 } ) );
    assignBytes( bytes, bytes.data(), bytes.size() );
    CHECK( bytes.size() == 1 && bytes[0] == 2 );
    appendBytes( bytes, bytes.data(), bytes.size() );
    CHECK( bytes == std::vector<unsigned char>( { 2, 2 } ) );
    CHECK( throws<std::out_of_range>( [&] { appendBytes( bytes, bytes.data() + 1, 2 ); } ) );
    CHECK( bytes.size() == 2 );
    CHECK(
        throws<std::length_error>( [&] { assignBytes( bytes, bytes.data(), MaxMessageBytes + 1 ); } ) );
    CHECK( bytes.size() == 2 );
    assignBytes( bytes, nullptr, 0 );
    CHECK( bytes.empty() );
    bytes.resize( MaxMessageBytes );
    CHECK( throws<std::length_error>( [&] { write( bytes, std::uint8_t( 1 ) ); } ) );
    CHECK( bytes.size() == MaxMessageBytes );
    std::puts(
        "PASS: portable codec golden bytes, numeric values, atomic failures and capacity bounds" );
    return 0;
}
