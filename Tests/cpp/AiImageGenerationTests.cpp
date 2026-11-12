#include <Workphone/Workphone.hpp>
#include <Workphone/AI/AiManager.hpp>
#include <boost/json.hpp>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    using namespace workphone;
    using Bytes = std::vector<unsigned char>;

    void require( bool value, const char *message )
    {
        if( !value ) throw std::runtime_error( message );
    }

    struct EnvironmentGuard
    {
        std::string name, previous;
        bool existed;

        EnvironmentGuard( const char *key, const char *value ) : name( key )
        {
            const auto old = std::getenv( key );
            existed = old != nullptr;
            if( old ) previous = old;
            set( value );
        }
        void set( const char *value ) const
        {
#ifdef _WIN32
            _putenv_s( name.c_str(), value ? value : "" );
#else
            if( value ) setenv( name.c_str(), value, 1 );
            else unsetenv( name.c_str() );
#endif
        }
        ~EnvironmentGuard() { set( existed ? previous.c_str() : nullptr ); }
    };

    void bigEndian( Bytes &output, unsigned value )
    {
        for( int shift = 24; shift >= 0; shift -= 8 )
            output.push_back( static_cast<unsigned char>( value >> shift ) );
    }

    unsigned crc32( const unsigned char *data, std::size_t size )
    {
        unsigned crc = 0xffffffffu;
        for( std::size_t index = 0; index < size; ++index )
        {
            crc ^= data[index];
            for( unsigned bit = 0; bit < 8; ++bit )
                crc = ( crc >> 1 ) ^ ( 0xedb88320u & ( 0u - ( crc & 1u ) ) );
        }
        return crc ^ 0xffffffffu;
    }

    void chunk( Bytes &output, const char *type, const Bytes &data )
    {
        bigEndian( output, static_cast<unsigned>( data.size() ) );
        const auto start = output.size();
        output.insert( output.end(), type, type + 4 );
        output.insert( output.end(), data.begin(), data.end() );
        bigEndian( output, crc32( output.data() + start, data.size() + 4 ) );
    }

    // Stored DEFLATE blocks keep the fixture independent of any encoder library and
    // deliberately produce a valid image larger than the text provider's 1 MiB limit.
    Bytes png( unsigned width, unsigned height, bool invalidFilter = false )
    {
        Bytes pixels( ( width + 1 ) * height, 127 );
        for( unsigned y = 0; y < height; ++y )
            pixels[y * ( width + 1 )] = invalidFilter ? 5 : 0;
        Bytes compressed{ 0x78, 0x01 };
        for( std::size_t offset = 0; offset < pixels.size(); )
        {
            const auto length = static_cast<unsigned>(
                std::min<std::size_t>( 65535, pixels.size() - offset ) );
            compressed.push_back( offset + length == pixels.size() ? 1 : 0 );
            compressed.push_back( static_cast<unsigned char>( length ) );
            compressed.push_back( static_cast<unsigned char>( length >> 8 ) );
            compressed.push_back( static_cast<unsigned char>( ~length ) );
            compressed.push_back( static_cast<unsigned char>( ~length >> 8 ) );
            compressed.insert( compressed.end(), pixels.begin() + offset,
                               pixels.begin() + offset + length );
            offset += length;
        }
        unsigned a = 1, b = 0;
        for( auto pixel : pixels ) { a = ( a + pixel ) % 65521; b = ( b + a ) % 65521; }
        bigEndian( compressed, ( b << 16 ) | a );
        Bytes output{ 137, 80, 78, 71, 13, 10, 26, 10 };
        Bytes header;
        bigEndian( header, width );
        bigEndian( header, height );
        header.insert( header.end(), { 8, 0, 0, 0, 0 } );
        chunk( output, "IHDR", header );
        chunk( output, "IDAT", compressed );
        chunk( output, "IEND", {} );
        return output;
    }

    std::string base64( const Bytes &bytes )
    {
        constexpr auto alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string output;
        output.reserve( ( bytes.size() + 2 ) / 3 * 4 );
        for( std::size_t i = 0; i < bytes.size(); i += 3 )
        {
            const unsigned value = ( unsigned( bytes[i] ) << 16 ) |
                ( i + 1 < bytes.size() ? unsigned( bytes[i + 1] ) << 8 : 0 ) |
                ( i + 2 < bytes.size() ? unsigned( bytes[i + 2] ) : 0 );
            output += alphabet[( value >> 18 ) & 63];
            output += alphabet[( value >> 12 ) & 63];
            output += i + 1 < bytes.size() ? alphabet[( value >> 6 ) & 63] : '=';
            output += i + 2 < bytes.size() ? alphabet[value & 63] : '=';
        }
        return output;
    }

    class MockAiManager : public AiManager
    {
    public:
        mutable String capturedImageRequest, capturedTextRequest;
        mutable unsigned imageCalls = 0, textCalls = 0, actionCalls = 0;
        String response = "{}";
        u32 status = 200;
        u32 textStatus = 200;
        String textResponse = R"({"status":"completed","output":[{"type":"message","role":"assistant","status":"completed","content":[{"type":"output_text","text":"{\"message\":\"Ready\",\"actions\":[]}"}]}]})";
        bool connected = true, throwTransport = false;

        void image( const Bytes &bytes )
        {
            response = ( "{\"data\":[{\"b64_json\":\"" + base64( bytes ) + "\"}]}" ).c_str();
        }

        String processResponseWithFeedback( const String & ) const override
        {
            ++actionCalls;
            return "text actions processed";
        }

    protected:
        bool requestOpenAIImage( const String &body, String &output, u32 &code,
                                 String &error ) const override
        {
            ++imageCalls;
            capturedImageRequest = body;
            if( throwTransport ) throw std::runtime_error( "sensitive-transport-detail" );
            output = response;
            code = status;
            error.clear();
            return connected;
        }

        bool requestOpenAI( const String &body, String &output, u32 &code, String & ) const override
        {
            ++textCalls;
            capturedTextRequest = body;
            code = textStatus;
            output = textResponse;
            return true;
        }
    };

    void checkFailure( const IAiManager::ImageGenerationResult &result )
    {
        require( result.pngBytes.empty(), "Failed generation returned image bytes" );
        require( !result.error.empty(), "Failed generation had no explanation" );
        require( result.error.find( "sensitive-transport-detail" ) == String::npos,
                 "Provider details leaked into the error" );
        require( result.error.find( "test-secret-never-transmitted" ) == String::npos,
                 "API key leaked into the error" );
    }

    void contracts()
    {
        EnvironmentGuard model( "OPENAI_IMAGE_MODEL", nullptr );
        EnvironmentGuard apiKey( "OPENAI_API_KEY", "test-secret-never-transmitted" );
        MockAiManager manager;
        IAiManager::ImageGenerationRequest request;
        request.prompt = "A \"blue\" tile\nwith stars and a caf\xc3\xa9 sign";
        const auto validPng = png( 1024, 1024 );
        require( validPng.size() > 1024 * 1024, "Fixture must exceed the old response size limit" );
        manager.image( validPng );
        auto generated = manager.generateImage( request );
        require( generated.error.empty(), generated.error.c_str() );
        require( generated.pngBytes.size() == validPng.size() &&
                 std::equal( validPng.begin(), validPng.end(), generated.pngBytes.begin() ),
                 "Valid PNG was altered or rejected" );
        auto body = boost::json::parse( manager.capturedImageRequest.c_str() ).as_object();
        require( body.at( "prompt" ).as_string() == request.prompt.c_str(),
                 "Prompt was not JSON escaped and preserved" );
        require( body.at( "model" ).as_string() == "gpt-image-2.5-flare", "Wrong default image model" );
        require( body.at( "n" ).as_int64() == 1 && body.at( "output_format" ).as_string() == "png",
                 "Generation must request exactly one PNG" );
        require( !body.if_contains( "response_format" ), "GPT Image request used legacy response_format" );
        require( manager.actionCalls == 0 && manager.textCalls == 0,
                 "Image generation invoked text actions" );

        model.set( "custom-image-model" );
        request.size = "1536x1024";
        request.quality = "high";
        manager.image( png( 1536, 1024 ) );
        generated = manager.generateImage( request );
        require( generated.error.empty(), generated.error.c_str() );
        body = boost::json::parse( manager.capturedImageRequest.c_str() ).as_object();
        require( body.at( "model" ).as_string() == "custom-image-model" &&
                 body.at( "size" ).as_string() == "1536x1024" &&
                 body.at( "quality" ).as_string() == "high", "Image options were ignored" );

        request.size = "1024x1536";
        request.quality = "low";
        manager.image( png( 1024, 1536 ) );
        require( manager.generateImage( request ).error.empty(), "Portrait PNG was rejected" );
        request.size = "1024x1024";
        request.quality = "medium";
        const auto calls = manager.imageCalls;
        for( const auto &prompt : { String(), String( " \t\r\n" ), String( 32001, 'a' ) } )
        {
            auto invalid = request;
            invalid.prompt = prompt;
            checkFailure( manager.generateImage( invalid ) );
        }
        auto invalid = request;
        invalid.size = "99999x99999";
        checkFailure( manager.generateImage( invalid ) );
        invalid = request;
        invalid.quality = "ultra";
        checkFailure( manager.generateImage( invalid ) );
        require( manager.imageCalls == calls, "Invalid input reached the provider" );
        for( const auto &invalidModel : { std::string( " \t" ), std::string( "bad\r\nmodel" ),
                                          std::string( 129, 'm' ) } )
        {
            model.set( invalidModel.c_str() );
            checkFailure( manager.generateImage( request ) );
        }
        require( manager.imageCalls == calls, "Invalid model reached the provider" );
        model.set( nullptr );

        // These real transport preflight failures return before any connection.
        AiManager realManager;
        apiKey.set( nullptr );
        generated = realManager.generateImage( request );
        checkFailure( generated );
        require( generated.error.find( "OPENAI_API_KEY" ) != String::npos,
                 "Missing credential did not explain how to configure image generation" );
        apiKey.set( "invalid\r\nkey" );
        checkFailure( realManager.generateImage( request ) );
        apiKey.set( "test-secret-never-transmitted" );

        for( const auto status : { 400u, 401u, 403u, 429u, 500u } )
        {
            manager.status = status;
            manager.response = R"({"error":{"message":"test-secret-never-transmitted"}})";
            checkFailure( manager.generateImage( request ) );
        }

        manager.status = 429;
        struct LimitCase
        {
            const char *code;
            const char *type;
            const char *expected;
        };
        for( const auto &limit : {
                 LimitCase{ "credit_balance_exhausted", "insufficient_quota", "credits are exhausted" },
                 LimitCase{ "organization_spend_limit_exceeded", "insufficient_quota", "organization spending limit" },
                 LimitCase{ "project_spend_limit_exceeded", "insufficient_quota", "project spending limit" },
                 LimitCase{ "organization_usage_limit_exceeded", "insufficient_quota", "organization usage limit" },
                 LimitCase{ "insufficient_quota", "", "quota is exhausted" },
                 LimitCase{ "", "insufficient_quota", "quota is exhausted" },
                 LimitCase{ "rate_limit_exceeded", "", "Wait before trying again" },
                 LimitCase{ "slow_down", "rate_limit_error", "Wait before trying again" },
                 LimitCase{ "", "rate_limit_error", "Wait before trying again" },
                 LimitCase{ "", "rate_limit_exceeded", "Wait before trying again" } } )
        {
            boost::json::object failure;
            failure["code"] = limit.code;
            failure["type"] = limit.type;
            failure["message"] = "test-secret-never-transmitted sensitive-transport-detail";
            manager.response = boost::json::serialize( boost::json::object{ { "error", failure } } );
            const auto before = manager.imageCalls;
            generated = manager.generateImage( request );
            checkFailure( generated );
            require( generated.error.find( limit.expected ) != String::npos,
                     "Rate limit and billing failures need different recovery guidance" );
            require( manager.imageCalls == before + 1, "Limit failure was automatically retried" );
            manager.textStatus = 429;
            manager.textResponse = manager.response;
            require( manager.query( "Limit test", IAiManager::Provider::OpenAI ) == generated.error,
                     "Text and image requests report limits differently" );
        }
        require( manager.actionCalls == 0, "Limit errors applied engine actions" );
        manager.textStatus = 200;

        for( const auto &response : {
                 String( "not json" ), String( "[]" ), String( "{\"error\":null}" ),
                 String( R"({"error":{"code":7,"type":[]}})" ),
                 String( R"({"error":{"code":"sensitive-transport-detail","type":"test-secret-never-transmitted"}})" ),
                 String( R"({"error":{"code":"insufficient_quota","code":"rate_limit_exceeded"}})" ),
                 String( R"({"error":{"code":"insufficient_quota"}})" ) + String( 64 * 1024, ' ' ) } )
        {
            manager.response = response;
            generated = manager.generateImage( request );
            checkFailure( generated );
            require( generated.error.find( "specific cause could not be identified" ) != String::npos,
                     "Unknown or malformed error was misclassified" );
        }
        manager.status = 200;
        manager.connected = false;
        checkFailure( manager.generateImage( request ) );
        manager.connected = true;
        manager.throwTransport = true;
        checkFailure( manager.generateImage( request ) );
        manager.throwTransport = false;
        for( const auto response : {
                 "not json", "[]", "{}", "{\"data\":[]}", "{\"data\":[null]}",
                 "{\"data\":[{\"url\":\"https://untrusted.invalid/image.png\"}]}",
                 "{\"data\":[{\"b64_json\":17}]}", "{\"data\":[{\"b64_json\":\"\"}]}",
                 "{\"data\":[{\"b64_json\":\"%%%%\"}]}",
                 "{\"data\":[{\"b64_json\":\"abc\"}]}",
                 "{\"data\":[{\"b64_json\":\"ab=c\"}]}",
                 "{\"data\":[{\"b64_json\":\"a===\"}]}",
                 "{\"data\":[{\"b64_json\":\"aGVsbG8=\"}]}",
                 "{\"error\":{\"message\":\"sensitive-transport-detail\"}}" } )
        {
            manager.response = response;
            checkFailure( manager.generateImage( request ) );
        }
        manager.image( png( 1, 1 ) );
        checkFailure( manager.generateImage( request ) );
        auto corrupt = validPng;
        corrupt[29] ^= 1;
        manager.image( corrupt );
        checkFailure( manager.generateImage( request ) );
        corrupt = validPng;
        corrupt.resize( corrupt.size() - 12 );
        manager.image( corrupt );
        checkFailure( manager.generateImage( request ) );
        manager.image( png( 1024, 1024, true ) );
        checkFailure( manager.generateImage( request ) );
        const auto encoded = base64( validPng );
        manager.response = ( "{\"data\":[{\"b64_json\":\"" + encoded +
                             "\",\"b64_json\":\"" + encoded + "\"}]}" ).c_str();
        checkFailure( manager.generateImage( request ) );
        manager.response = ( "{\"data\":[{\"b64_json\":\"" + encoded +
                             "\"},{\"b64_json\":\"" + encoded + "\"}]}" ).c_str();
        checkFailure( manager.generateImage( request ) );
        manager.response = String( 32 * 1024 * 1024 + 1, ' ' );
        checkFailure( manager.generateImage( request ) );

        manager.textResponse = R"({"status":"completed","output":[{"type":"message","role":"assistant","status":"completed","content":[{"type":"output_text","text":"{\"message\":\"Ready\",\"actions\":[]}"}]}]})";
        const auto textCalls = manager.textCalls;
        require( manager.query( "Keep existing text flow", IAiManager::Provider::OpenAI ) ==
                     "text actions processed", "Existing text provider seam regressed" );
        require( manager.textCalls == textCalls + 1 && manager.actionCalls == 1,
                 "Text and image provider flows are not independent" );
    }
}

int main()
{
    workphone::TypeManager types;
    types.load();
    workphone::TypeManager::setInstance( &types );
    int result = 0;
    try
    {
        contracts();
        std::cout << "PASS: image request, PNG decoding, validation and provider error contracts\n";
    }
    catch( const std::exception &error )
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        result = 1;
    }
    types.unload();
    workphone::TypeManager::setInstance( nullptr );
    return result;
}
