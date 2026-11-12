#include <Workphone/AI/MeshyClient.hpp>
#include <boost/json.hpp>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace
{
    using namespace workphone::mesh_generation;
    using Bytes = std::vector<std::uint8_t>;
    namespace json = boost::json;
    constexpr auto secret = "msy-test-secret-never-transmitted";

    void require( bool condition, const char *message )
    { if( !condition ) throw std::runtime_error( message ); }

    void little( Bytes &bytes, std::uint32_t value )
    { for( unsigned shift = 0; shift < 32; shift += 8 ) bytes.push_back( std::uint8_t( value >> shift ) ); }
    void big( Bytes &bytes, std::uint32_t value )
    { for( int shift = 24; shift >= 0; shift -= 8 ) bytes.push_back( std::uint8_t( value >> shift ) ); }

    std::uint32_t crc32( const std::uint8_t *bytes, std::size_t size )
    {
        std::uint32_t crc = 0xffffffffu;
        for( std::size_t i = 0; i < size; ++i )
        {
            crc ^= bytes[i];
            for( unsigned bit = 0; bit < 8; ++bit ) crc = ( crc >> 1 ) ^ ( 0xedb88320u & ( 0u - ( crc & 1u ) ) );
        }
        return crc ^ 0xffffffffu;
    }

    void chunk( Bytes &bytes, const char *type, const Bytes &payload )
    {
        big( bytes, static_cast<std::uint32_t>( payload.size() ) );
        const auto start = bytes.size();
        bytes.insert( bytes.end(), type, type + 4 );
        bytes.insert( bytes.end(), payload.begin(), payload.end() );
        big( bytes, crc32( bytes.data() + start, bytes.size() - start ) );
    }

    Bytes png()
    {
        Bytes bytes{137,80,78,71,13,10,26,10};
        Bytes header; big( header, 32 ); big( header, 32 );
        header.insert( header.end(), {8,2,0,0,0} );
        chunk( bytes, "IHDR", header );
        Bytes raw( 32 * ( 1 + 32 * 3 ), 0 );
        const auto count = static_cast<std::uint16_t>( raw.size() );
        Bytes data{0x78,0x01,0x01,std::uint8_t(count),std::uint8_t(count >> 8),
                   std::uint8_t(~count),std::uint8_t((~count) >> 8)};
        data.insert( data.end(), raw.begin(), raw.end() );
        big( data, ( std::uint32_t( raw.size() ) << 16 ) | 1 );
        chunk( bytes, "IDAT", data ); chunk( bytes, "IEND", {} );
        return bytes;
    }

    json::object meshJson()
    {
        return json::parse( R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":36}],"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36}],"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"}],"meshes":[{"primitives":[{"attributes":{"POSITION":0}}]}],"nodes":[{"mesh":0}],"scenes":[{"nodes":[0]}],"scene":0})" ).as_object();
    }

    Bytes glb( const json::object &object = meshJson(), Bytes binary = {} )
    {
        if( binary.empty() )
        {
            const float vertices[] = {0,0,0, 1,0,0, 0,1,0};
            binary.resize( sizeof( vertices ) );
            std::memcpy( binary.data(), vertices, sizeof( vertices ) );
        }
        while( binary.size() % 4 ) binary.push_back( 0 );
        auto text = json::serialize( object );
        while( text.size() % 4 ) text += ' ';
        Bytes output; little( output, 0x46546c67 ); little( output, 2 );
        little( output, static_cast<std::uint32_t>( 28 + text.size() + binary.size() ) );
        little( output, static_cast<std::uint32_t>( text.size() ) ); little( output, 0x4e4f534a );
        output.insert( output.end(), text.begin(), text.end() );
        little( output, static_cast<std::uint32_t>( binary.size() ) ); little( output, 0x004e4942 );
        output.insert( output.end(), binary.begin(), binary.end() );
        return output;
    }

    struct Reply
    {
        std::uint32_t status = 200;
        Bytes body;
        bool connected = true;
        Reply( std::uint32_t code, const std::string &text, bool success = true ) :
            status( code ), body( text.begin(), text.end() ), connected( success ) {}
        Reply( std::uint32_t code, Bytes bytes ) : status( code ), body( std::move( bytes ) ) {}
    };

    struct Call { std::string method, url, bearer, body; std::size_t limit; };
    class MockClient : public MeshyClient
    {
    public:
        mutable std::vector<Call> calls;
        std::vector<Reply> replies;
        mutable unsigned waits = 0;
        bool missingKey = false, throwing = false;
        std::string transportError = secret;
        std::atomic_bool *cancelDuringPost = nullptr;
    protected:
        bool resolveApiKey( std::string &key, std::string &error ) const override
        {
            if( missingKey ) { error = "Add a Meshy API key in Editor settings."; return false; }
            key = secret; error.clear(); return true;
        }
        bool request( const std::string &method, const std::string &url, const std::string &bearer,
                      const std::string &body, std::size_t limit, const std::atomic_bool &,
                      Bytes &response, std::uint32_t &status, std::string &error ) const override
        {
            calls.push_back( {method,url,bearer,body,limit} );
            if( throwing ) throw std::runtime_error( secret );
            require( calls.size() <= replies.size(), "Unexpected transport call or retry" );
            const auto &reply = replies[calls.size() - 1];
            response = reply.body; status = reply.status; error = transportError;
            if( method == "POST" && cancelDuringPost ) *cancelDuringPost = true;
            return reply.connected;
        }
        bool waitForPoll( const std::atomic_bool &cancel ) const override
        { ++waits; return !cancel.load(); }
    };

    void failure( const Result &result )
    {
        require( !result.error.empty() && result.glbBytes.empty(), "Failure published mesh data or omitted its explanation" );
        require( result.error.find( secret ) == std::string::npos, "API key or raw provider message escaped" );
    }

    void contracts()
    {
        Request request; request.imageBytes = png(); request.mimeType = "image/png";
        std::atomic_bool cancel{false};
        std::string diagnostic;
        require( validateSourceImage( request.imageBytes.data(), request.imageBytes.size(), request.mimeType, diagnostic ), diagnostic.c_str() );
        const auto model = glb();
        require( validateGeneratedGlb( model.data(), model.size(), diagnostic ), diagnostic.c_str() );
        MockClient success;
        success.replies = { {202,R"({"result":"task/with?opaque=id"})"},
                            {200,R"({"status":"PENDING","progress":0})"},
                            {200,R"({"status":"IN_PROGRESS","progress":42})"},
                            {200,R"({"status":"SUCCEEDED","model_urls":{"glb":"https://assets.meshy.ai/generated/model.glb?Expires=123&Signature=abc"}})"},
                            {200,model} };
        std::vector<Progress> progress;
        auto generated = success.generate( request, cancel, [&]( const Progress &p ) { progress.push_back( p ); } );
        require( generated.error.empty() && generated.glbBytes == model && !generated.cancelled, "Valid Meshy task failed" );
        require( success.calls.size() == 5 && success.waits == 2, "Unexpected polling lifecycle" );
        const auto body = json::parse( success.calls[0].body ).as_object();
        require( body.at( "ai_model" ) == "meshy-7.1" && body.at( "topology" ) == "triangle" &&
                 body.at( "target_polycount" ).as_int64() == 10000 && body.at( "should_remesh" ).as_bool() &&
                 body.at( "should_texture" ).as_bool() && body.at( "target_formats" ).as_array()[0] == "glb", "Wrong generation options" );
        require( body.at( "image_url" ).as_string().starts_with( "data:image/png;base64,iVBORw0KGgo" ), "Image was not encoded as an upload data URI" );
        require( success.calls[1].url.find( "/task%2Fwith%3Fopaque%3Did" ) != std::string::npos, "Opaque task ID was not escaped as one path segment" );
        require( success.calls[0].bearer == secret && success.calls[1].bearer == secret && success.calls.back().bearer.empty(), "API key leaked to the download host or was omitted from API calls" );
        require( success.calls.back().limit == maximumGeneratedGlbBytes && progress.back().stage == "Mesh ready", "Download limit/progress contract failed" );

        MockClient untextured; untextured.replies = success.replies;
        auto options = request; options.textured = false; options.targetPolycount = 25000;
        require( untextured.generate( options, cancel ).error.empty(), "Untextured generation failed" );
        const auto untexturedBody = json::parse( untextured.calls[0].body ).as_object();
        require( !untexturedBody.at( "should_texture" ).as_bool() &&
                 untexturedBody.at( "target_polycount" ).as_int64() == 25000, "Mesh options were ignored" );

        for( const auto code : {400u,401u,402u,403u,404u,429u,500u} )
        {
            MockClient client; client.replies.emplace_back( code, std::string( "{\"message\":\"" ) + secret + "\"}" );
            generated = client.generate( request, cancel ); failure( generated );
            require( client.calls.size() == 1, "Failed POST was automatically retried" );
            if( code == 402 ) require( generated.error.find( "credits" ) != std::string::npos, "Billing failure was confused with rate limiting" );
        }
        for( const auto response : {"not json", "[]", "{}", "{\"result\":null}",
                                    "{\"result\":\"a\",\"result\":\"b\"}", "{\"result\":\"..\"}"} )
        {
            MockClient client; client.replies.emplace_back( 202, response );
            failure( client.generate( request, cancel ) );
            require( client.calls.size() == 1, "Invalid task response reached polling" );
        }
        for( const auto url : {"http://assets.meshy.ai/a.glb", "https://assets.meshy.ai.evil.test/a.glb",
                              "https://assets.meshy.ai@evil.test/a.glb", "https://127.0.0.1/a.glb",
                              "file:///C:/private", "https://assets.meshy.ai:8443/a.glb", "https://assets.meshy.ai/a.glb#x"} )
        {
            MockClient client;
            client.replies = {{202,R"({"result":"id"})"},
                              {200,std::string("{\"status\":\"SUCCEEDED\",\"model_urls\":{\"glb\":\"") + url + "\"}}"}};
            failure( client.generate( request, cancel ) );
            require( client.calls.size() == 2, "Untrusted download URL was requested" );
        }
        for( const auto response : {"{}", "{\"status\":\"UNKNOWN\"}", "{\"status\":\"FAILED\",\"task_error\":{\"message\":\"msy-test-secret-never-transmitted\"}}",
                                    "{\"status\":\"CANCELED\"}", "{\"id\":\"other\",\"status\":\"PENDING\"}",
                                    "{\"status\":\"IN_PROGRESS\",\"progress\":101}", "{\"status\":\"IN_PROGRESS\",\"progress\":-1}"} )
        {
            MockClient client; client.replies = {{202,R"({"result":"id"})"},{200,response}};
            failure( client.generate( request, cancel ) );
            require( client.calls.size() == 2, "Failed or malformed task kept polling" );
        }
        MockClient stopped; cancel = true;
        generated = stopped.generate( request, cancel ); failure( generated );
        require( generated.cancelled && stopped.calls.empty(), "Pre-cancelled generation submitted an image" );
        cancel = false;
        stopped.replies = {{202,R"({"result":"id"})"},{200,R"({"status":"IN_PROGRESS","progress":8})"}};
        generated = stopped.generate( request, cancel, [&]( const Progress &p ) { if( p.percent == 8 ) cancel = true; } );
        failure( generated );
        require( generated.cancelled && generated.taskId == "id" && stopped.calls.size() == 2, "Cancellation lost task identity or sent another request" );
        cancel = false;
        MockClient stoppedDuringPost;
        stoppedDuringPost.replies = {{202,R"({"result":"confirmed-task"})"}};
        stoppedDuringPost.cancelDuringPost = &cancel;
        generated = stoppedDuringPost.generate( request, cancel ); failure( generated );
        require( generated.cancelled && generated.taskId == "confirmed-task" && stoppedDuringPost.calls.size() == 1,
                 "Cancellation during submission lost the confirmed task ID or began polling" );
        cancel = false;
        MockClient stoppedAfterValidation; stoppedAfterValidation.replies = success.replies;
        generated = stoppedAfterValidation.generate( request, cancel,
            [&]( const Progress &p ) { if( p.stage == "Mesh ready" ) cancel = true; } );
        failure( generated );
        require( generated.cancelled && !generated.taskId.empty(), "Late cancellation returned publishable mesh bytes" );
        cancel = false;
        MockClient timeout; timeout.replies.emplace_back( 202,R"({"result":"id"})" );
        for( unsigned i = 0; i < 120; ++i ) timeout.replies.emplace_back( 200,R"({"status":"PENDING"})" );
        generated = timeout.generate( request, cancel ); failure( generated );
        require( timeout.calls.size() == 121 && generated.error.find( "20 minutes" ) != std::string::npos, "Polling was not bounded" );
        MockClient offline; offline.replies.emplace_back( 0, "", false );
        failure( offline.generate( request, cancel ) );
        require( offline.calls.size() == 1, "Ambiguous submission was retried" );
        MockClient unsupported; unsupported.replies.emplace_back( 0, "", false );
        unsupported.transportError = "Meshy HTTPS transport currently requires Windows.";
        generated = unsupported.generate( request, cancel ); failure( generated );
        require( generated.error == unsupported.transportError, "Unsupported platform was reported as an ambiguous submission" );
        MockClient throws; throws.throwing = true; failure( throws.generate( request, cancel ) );
        MockClient noKey; noKey.missingKey = true; failure( noKey.generate( request, cancel ) );
        require( noKey.calls.empty(), "Missing key reached network" );

        MockClient invalid;
        auto bad = request; bad.targetPolycount = 99; failure( invalid.generate( bad, cancel ) );
        bad = request; bad.targetPolycount = 300001; failure( invalid.generate( bad, cancel ) );
        bad = request; bad.mimeType = "text/plain"; failure( invalid.generate( bad, cancel ) );
        bad = request; bad.imageBytes[29] ^= 1; failure( invalid.generate( bad, cancel ) );
        bad = request; bad.imageBytes.clear(); failure( invalid.generate( bad, cancel ) );
        require( invalid.calls.empty(), "Invalid input reached provider" );
        require( !validateSourceImage( nullptr, maximumSourceImageBytes + 1, "image/png", diagnostic ), "Oversized image accepted" );
        require( !validateGeneratedGlb( nullptr, maximumGeneratedGlbBytes + 1, diagnostic ), "Oversized GLB accepted" );

        auto object = meshJson(); object["buffers"].as_array()[0].as_object()["uri"] = "file:///C:/private";
        auto broken = glb( object ); require( !validateGeneratedGlb( broken.data(), broken.size(), diagnostic ), "GLB external URI accepted" );
        object = meshJson(); object["bufferViews"].as_array()[0].as_object()["byteLength"] = 999;
        broken = glb( object ); require( !validateGeneratedGlb( broken.data(), broken.size(), diagnostic ), "GLB out-of-bounds view accepted" );
        object = meshJson(); object["accessors"].as_array()[0].as_object()["count"] = 999;
        broken = glb( object ); require( !validateGeneratedGlb( broken.data(), broken.size(), diagnostic ), "GLB out-of-bounds accessor accepted" );
        object = meshJson(); object["meshes"] = json::array(); broken = glb( object );
        require( !validateGeneratedGlb( broken.data(), broken.size(), diagnostic ), "GLB without geometry accepted" );
        broken = model; broken.back() = 0x7f; broken[broken.size() - 2] = 0xc0;
        require( !validateGeneratedGlb( broken.data(), broken.size(), diagnostic ), "GLB non-finite position accepted" );
        broken = model; broken.pop_back();
        require( !validateGeneratedGlb( broken.data(), broken.size(), diagnostic ), "Truncated GLB accepted" );

        object = meshJson();
        object["buffers"].as_array()[0].as_object()["byteLength"] = 42;
        object["bufferViews"].as_array().push_back( json::object{{"buffer",0},{"byteOffset",36},{"byteLength",6}} );
        object["accessors"].as_array().push_back( json::object{{"bufferView",1},{"componentType",5123},{"count",3},{"type","SCALAR"}} );
        object["meshes"].as_array()[0].as_object()["primitives"].as_array()[0].as_object()["indices"] = 1;
        const float vertices[] = {0,0,0, 1,0,0, 0,1,0};
        Bytes indexed( sizeof( vertices ) ); std::memcpy( indexed.data(), vertices, sizeof( vertices ) );
        indexed.insert( indexed.end(), {0,0,1,0,2,0} );
        auto indexedModel = glb( object, indexed );
        require( validateGeneratedGlb( indexedModel.data(), indexedModel.size(), diagnostic ), "Valid indexed triangle was rejected" );
        indexed[40] = 3; indexedModel = glb( object, indexed );
        require( !validateGeneratedGlb( indexedModel.data(), indexedModel.size(), diagnostic ), "GLB index beyond vertex count accepted" );
        MockClient brokenDownload;
        brokenDownload.replies = {{202,R"({"result":"id"})"},{200,R"({"status":"SUCCEEDED","model_urls":{"glb":"https://assets.meshy.ai/model.glb"}})"},{200,broken}};
        failure( brokenDownload.generate( request, cancel ) );
    }
}

int main()
{
    try
    {
        contracts();
        std::cout << "PASS: Meshy requests, task polling, credential isolation, cancellation, limits and self-contained mesh validation\n";
        return 0;
    }
    catch( const std::exception &error )
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
