#include <Workphone/AI/Trellis2Client.hpp>
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
    constexpr auto secret = "fal-test-secret-never-transmitted";

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
        std::uint32_t status;
        Bytes body;
        bool connected = true;
        Reply( std::uint32_t code, const std::string &text, bool success = true ) :
            status( code ), body( text.begin(), text.end() ), connected( success ) {}
        Reply( std::uint32_t code, Bytes bytes ) : status( code ), body( std::move( bytes ) ) {}
    };

    struct Call { std::string method, url, authorization, body; std::size_t limit; };
    class MockClient : public Trellis2Client
    {
    public:
        mutable std::vector<Call> calls;
        std::vector<Reply> replies;
        mutable unsigned waits = 0, keyLookups = 0;
        bool missingKey = false, throwing = false, waitSuccess = true;
        std::string keyValue = secret, transportError = secret;
        std::atomic_bool *cancelOnCall = nullptr;
        std::size_t cancelCall = 0;
    protected:
        bool resolveApiKey( std::string &key, std::string &error ) const override
        {
            ++keyLookups;
            if( missingKey ) { error = "Add a fal.ai API key in Editor settings."; return false; }
            key = keyValue; error.clear(); return true;
        }
        bool request( const std::string &method, const std::string &url, const std::string &authorization,
                      const std::string &body, std::size_t limit, const std::atomic_bool &,
                      Bytes &response, std::uint32_t &status, std::string &error ) const override
        {
            calls.push_back( {method,url,authorization,body,limit} );
            if( throwing ) throw std::runtime_error( secret );
            require( calls.size() <= replies.size(), "Unexpected transport call or automatic retry" );
            const auto &reply = replies[calls.size() - 1];
            response = reply.body; status = reply.status; error = transportError;
            if( cancelOnCall && calls.size() == cancelCall ) *cancelOnCall = true;
            return reply.connected;
        }
        bool waitForPoll( const std::atomic_bool &cancel ) const override
        { ++waits; return waitSuccess && !cancel.load(); }
    };

    // Only rejected requests are passed to the native transport; these probes must never open a connection.
    class TransportProbe : public Trellis2Client
    {
    public:
        bool rejected( const std::string &method, const std::string &url, const std::string &authorization,
                       const std::string &body = {}, std::size_t limit = maximumGeneratedGlbBytes ) const
        {
            Bytes response;
            std::uint32_t status = 0;
            std::string error;
            std::atomic_bool cancel{false};
            return !Trellis2Client::request( method, url, authorization, body, limit, cancel, response, status, error ) &&
                response.empty() && status == 0;
        }
    };

    constexpr auto endpoint = "https://queue.fal.run/fal-ai/trellis-2";
    constexpr auto complete = R"({"status":"COMPLETED","request_id":"id"})";
    constexpr auto submitted = R"({"request_id":"id","status":"IN_QUEUE"})";
    constexpr auto modelOutput = R"({"model_glb":{"url":"https://v3b.fal.media/files/b/model.glb"}})";

    void failure( const Result &result )
    {
        require( !result.error.empty() && result.glbBytes.empty(), "Failure published mesh data or omitted its explanation" );
        require( result.error.find( secret ) == std::string::npos, "API key or raw provider message escaped" );
    }

    void contracts()
    {
        Request input; input.imageBytes = png(); input.mimeType = "image/png";
        std::atomic_bool cancel{false};
        const auto model = glb();
        MockClient success;
        success.replies = {{202,R"({"request_id":"task/with?opaque=id","status":"IN_QUEUE","status_url":"https://evil.test/status","response_url":"http://127.0.0.1/private","cancel_url":"https://evil.test/cancel"})"},
                           {202,R"({"status":"IN_QUEUE","request_id":"task/with?opaque=id","queue_position":2})"},
                           {202,R"({"status":"IN_PROGRESS","request_id":"task/with?opaque=id"})"},
                           {200,R"({"status":"COMPLETED","request_id":"task/with?opaque=id","response_url":"https://evil.test/result"})"},
                           {200,modelOutput},{200,model}};
        std::vector<Progress> progress;
        auto generated = success.generate( input, 1024, cancel, [&]( const Progress &p ) { progress.push_back( p ); } );
        require( generated.error.empty() && generated.glbBytes == model && !generated.cancelled, "Valid TRELLIS 2 task failed" );
        require( generated.taskId == "task/with?opaque=id" && success.calls.size() == 6 && success.waits == 2, "Unexpected task lifecycle" );
        const auto body = json::parse( success.calls[0].body ).as_object();
        require( success.calls[0].url == endpoint && success.calls[0].method == "POST", "Wrong submission endpoint" );
        require( body.size() == 5 && body.at( "resolution" ).as_int64() == 1024 &&
                 body.at( "decimation_target" ).as_int64() == 10000 && body.at( "texture_size" ).as_int64() == 2048 &&
                 body.at( "remesh" ).as_bool(), "Wrong TRELLIS generation options" );
        require( body.at( "image_url" ).as_string().starts_with( "data:image/png;base64,iVBORw0KGgo" ), "Image was not encoded as a data URI" );
        const auto resultUrl = std::string( endpoint ) + "/requests/task%2Fwith%3Fopaque%3Did";
        for( std::size_t i = 1; i < 4; ++i )
            require( success.calls[i].method == "GET" && success.calls[i].url == resultUrl + "/status", "Unsafe status URL or unescaped task ID" );
        require( success.calls[4].url == resultUrl, "Result URL was not reconstructed from the fixed owner/app" );
        for( std::size_t i = 0; i + 1 < success.calls.size(); ++i )
            require( success.calls[i].authorization == std::string( "Key " ) + secret &&
                     success.calls[i].limit == 1024u * 1024u, "Wrong fal Key authentication or JSON byte limit" );
        require( success.calls.back().authorization.empty() && success.calls.back().limit == maximumGeneratedGlbBytes,
                 "Credentials leaked to the CDN or mesh size was unbounded" );
        require( progress.back().stage == "Mesh ready" && progress.back().percent == 100, "Completion stage missing" );
        for( std::size_t i = 0; i + 1 < progress.size(); ++i )
            require( progress[i].percent == 0 && progress[i].stage.find( secret ) == std::string::npos,
                     "Invented provider percentage or leaked secret in progress" );

        for( const auto resolution : {512u,1024u,1536u} )
            for( const auto count : {5000u,10000u,30000u,100000u,2000000u} )
            {
                MockClient client; client.replies = {{202,submitted},{200,complete},{200,modelOutput},{200,model}};
                auto options = input; options.targetPolycount = count;
                require( client.generate( options, resolution, cancel ).error.empty(), "Supported resolution/vertex count was rejected" );
                const auto payload = json::parse( client.calls[0].body ).as_object();
                require( payload.at( "resolution" ).as_int64() == resolution &&
                         payload.at( "decimation_target" ).as_int64() == count, "Resolution or vertex count ignored" );
            }
        for( const auto host : {"fal.media","v3.fal.media","v3b.fal.media"} )
        {
            MockClient client;
            const auto output = json::serialize( json::object{{"model_glb",json::object{{"url",std::string("https://") + host + "/files/model.glb?Signature=abc"}}}} );
            client.replies = {{202,submitted},{200,complete},{200,output},{200,model}};
            require( client.generate( input, 1024, cancel ).error.empty(), "Documented fal CDN host rejected" );
        }

        for( const auto code : {301u,400u,401u,402u,403u,404u,422u,429u,499u,500u,504u} )
        {
            for( unsigned phase = 0; phase < 4; ++phase )
            {
                MockClient client;
                client.replies = {{202,submitted},{200,complete},{200,modelOutput},{200,model}};
                client.replies[phase] = Reply( code, std::string("{\"detail\":\"") + secret + "\"}" );
                generated = client.generate( input, 1024, cancel ); failure( generated );
                require( client.calls.size() == phase + 1, "Failed request or redirect caused a retry/follow-up" );
                if( code == 402 && phase < 3 ) require( generated.error.find( "credits" ) != std::string::npos, "Credit error confused with rate limiting" );
                if( code == 429 && phase < 3 ) require( generated.error.find( "rate limiting" ) != std::string::npos, "Rate limit error was not distinct" );
            }
        }
        for( const auto response : {"not json","[]","{}","{\"request_id\":null}","{\"request_id\":\"..\"}",
                                    "{\"request_id\":\"a\",\"request_id\":\"b\"}","{\"request_id\":\"bad\\nvalue\"}",
                                    "{\"request_id\":\"fal-test-secret-never-transmitted\"}"} )
        {
            MockClient client; client.replies.emplace_back( 202, response );
            failure( client.generate( input, 1024, cancel ) );
            require( client.calls.size() == 1, "Malformed submission response reached polling" );
        }
        std::string deep = "{\"request_id\":\"id\",\"nested\":";
        deep += std::string( 40, '[' ) + "0" + std::string( 40, ']' ) + "}";
        for( const auto &response : {deep, std::string( 1024u * 1024u + 1, ' ' ),
                json::serialize( json::object{{"request_id",std::string(257,'x')}} )} )
        {
            MockClient client; client.replies.emplace_back( 202, response );
            failure( client.generate( input, 1024, cancel ) );
            require( client.calls.size() == 1, "Unbounded JSON or task ID accepted" );
        }
        for( const auto response : {"{}","[]","not json","{\"status\":\"UNKNOWN\",\"request_id\":\"id\"}",
                    "{\"status\":\"COMPLETED\",\"request_id\":\"different\"}","{\"status\":\"COMPLETED\"}",
                    "{\"status\":\"IN_QUEUE\",\"status\":\"COMPLETED\",\"request_id\":\"id\"}"} )
        {
            MockClient client; client.replies = {{202,submitted},{200,response}};
            failure( client.generate( input, 1024, cancel ) );
            require( client.calls.size() == 2, "Invalid status reached result or kept polling" );
        }
        // fal reports failed/cancelled jobs as COMPLETED too; their result has the actual HTTP failure.
        MockClient remoteFailed;
        remoteFailed.replies = {{202,submitted},{200,R"({"status":"COMPLETED","request_id":"id","error":"fal-test-secret-never-transmitted","error_type":"request_timeout"})"},
                               {504,R"({"detail":"fal-test-secret-never-transmitted"})"}};
        failure( remoteFailed.generate( input, 1024, cancel ) );
        require( remoteFailed.calls.size() == 3, "COMPLETED error did not retrieve its result status" );

        for( const auto response : {"{}","[]","not json","{\"model_glb\":null}",
                   "{\"model_glb\":{\"url\":1}}","{\"data\":{\"model_glb\":{\"url\":\"https://v3b.fal.media/model.glb\"}}}",
                   "{\"model_glb\":{\"url\":\"https://v3b.fal.media/a.glb\",\"url\":\"https://evil.test/a.glb\"}}"} )
        {
            MockClient client; client.replies = {{202,submitted},{200,complete},{200,response}};
            failure( client.generate( input, 1024, cancel ) );
            require( client.calls.size() == 3, "Malformed model output reached download" );
        }
        for( const auto url : {"http://v3b.fal.media/a.glb","https://v3b.fal.media.evil.test/a.glb",
                  "https://v3b.fal.media@evil.test/a.glb","https://evil.test@v3b.fal.media/a.glb",
                  "https://127.0.0.1/a.glb","file:///C:/private","https://v3b.fal.media:443/a.glb",
                  "https://v3b.fal.media/a.glb#fragment","https://v3b.fal.media/","https://v3b.fal.media",
                  "https://v3b.fal.media\\@evil.test/a.glb","https://v3b.fal.media/a b.glb",
                  "https://unlisted.fal.media/a.glb","https://v3b.fal.media/fal-test-secret-never-transmitted.glb"} )
        {
            MockClient client;
            const auto response = json::serialize( json::object{{"model_glb",json::object{{"url",url}}}} );
            client.replies = {{202,submitted},{200,complete},{200,response}};
            failure( client.generate( input, 1024, cancel ) );
            require( client.calls.size() == 3, "Untrusted download URL was requested" );
        }

        MockClient stopped; cancel = true;
        generated = stopped.generate( input, 1024, cancel ); failure( generated );
        require( generated.cancelled && stopped.calls.empty() && stopped.keyLookups == 0, "Pre-cancelled request performed work" );
        cancel = false;
        for( std::size_t phase = 1; phase <= 4; ++phase )
        {
            MockClient client; client.replies = {{202,submitted},{200,complete},{200,modelOutput},{200,model}};
            client.cancelOnCall = &cancel; client.cancelCall = phase;
            generated = client.generate( input, 1024, cancel ); failure( generated );
            require( generated.cancelled && generated.taskId == "id" && client.calls.size() == phase,
                     "Cancellation lost confirmed task ID, kept mesh bytes or sent another request" );
            cancel = false;
        }
        for( const auto stage : {"Submitting image to TRELLIS 2 on fal.ai","Waiting for TRELLIS 2 on fal.ai",
                                 "Retrieving TRELLIS 2 result","Downloading generated mesh","Mesh ready"} )
        {
            MockClient client; client.replies = {{202,submitted},{200,complete},{200,modelOutput},{200,model}};
            generated = client.generate( input, 1024, cancel, [&]( const Progress &p ) { if( p.stage == stage ) cancel = true; } );
            failure( generated ); require( generated.cancelled, "Callback cancellation was ignored" );
            if( client.calls.empty() ) require( generated.taskId.empty(), "Unsubmitted task had an ID" );
            else require( generated.taskId == "id", "Cancellation discarded confirmed ID" );
            cancel = false;
        }
        MockClient queuedCancel;
        queuedCancel.replies = {{202,submitted},{202,R"({"status":"IN_QUEUE","request_id":"id"})"}};
        generated = queuedCancel.generate( input, 1024, cancel, [&]( const Progress &p ) { if( p.stage == "Queued in fal.ai" ) cancel = true; } );
        failure( generated ); require( generated.cancelled && queuedCancel.calls.size() == 2, "Poll wait ignored cancellation" );
        cancel = false;
        MockClient timeout; timeout.replies.emplace_back( 202, submitted );
        for( unsigned i = 0; i < 120; ++i ) timeout.replies.emplace_back( 202, R"({"status":"IN_QUEUE","request_id":"id"})" );
        generated = timeout.generate( input, 1024, cancel ); failure( generated );
        require( timeout.calls.size() == 121 && generated.taskId == "id" && generated.error.find( "20 minutes" ) != std::string::npos,
                 "Queue polling was unbounded or lost task ID" );

        for( unsigned phase = 0; phase < 4; ++phase )
        {
            MockClient client; client.replies = {{202,submitted},{200,complete},{200,modelOutput},{200,model}};
            client.replies[phase] = Reply( 0, "", false );
            failure( client.generate( input, 1024, cancel ) );
            require( client.calls.size() == phase + 1, "Transport failure caused a retry" );
        }
        MockClient unsupported; unsupported.replies.emplace_back( 0, "", false );
        unsupported.transportError = "TRELLIS 2 HTTPS transport currently requires Windows.";
        generated = unsupported.generate( input, 1024, cancel ); failure( generated );
        require( generated.error == unsupported.transportError, "Unsupported platform was misreported as ambiguous submission" );
        MockClient large; large.replies = {{202,submitted},{200,complete},{200,modelOutput},{0,"",false}};
        large.transportError = "The TRELLIS 2 response exceeds the supported size limit.";
        generated = large.generate( input, 1024, cancel ); failure( generated );
        require( generated.error == large.transportError, "Bounded download failure lost size explanation" );
        MockClient throws; throws.throwing = true; failure( throws.generate( input, 1024, cancel ) );
        MockClient noKey; noKey.missingKey = true; failure( noKey.generate( input, 1024, cancel ) );
        require( noKey.calls.empty(), "Missing key reached network" );
        for( const auto &value : {std::string(),std::string("bad\r\nheader"),std::string(4097,'a')} )
        {
            MockClient client; client.keyValue = value; failure( client.generate( input, 1024, cancel ) );
            require( client.calls.empty(), "Malformed key reached network" );
        }
        MockClient invalid;
        for( const auto resolution : {0u,511u,1023u,1537u} ) failure( invalid.generate( input, resolution, cancel ) );
        auto bad = input; bad.targetPolycount = 4999; failure( invalid.generate( bad, 1024, cancel ) );
        bad = input; bad.targetPolycount = 2000001; failure( invalid.generate( bad, 1024, cancel ) );
        bad = input; bad.textured = false; failure( invalid.generate( bad, 1024, cancel ) );
        bad = input; bad.mimeType = "text/plain"; failure( invalid.generate( bad, 1024, cancel ) );
        bad = input; bad.imageBytes[29] ^= 1; failure( invalid.generate( bad, 1024, cancel ) );
        bad = input; bad.imageBytes.clear(); failure( invalid.generate( bad, 1024, cancel ) );
        require( invalid.calls.empty() && invalid.keyLookups == 0, "Invalid input reached credentials or provider" );

        auto external = meshJson(); external["buffers"].as_array()[0].as_object()["uri"] = "file:///C:/private";
        auto truncated = model; truncated.pop_back();
        for( const auto &broken : {Bytes{'b','a','d'},glb( external ),truncated} )
        {
            MockClient client; client.replies = {{202,submitted},{200,complete},{200,modelOutput},{200,broken}};
            generated = client.generate( input, 1024, cancel ); failure( generated );
            require( generated.taskId == "id", "Rejected mesh lost task ID" );
        }

        const TransportProbe transport;
        const auto auth = std::string( "Key " ) + secret;
        require( transport.rejected( "POST", endpoint, std::string("Bearer ") + secret ), "Native transport accepted Bearer authentication" );
        require( transport.rejected( "POST", endpoint, "Key bad\r\nheader" ), "Native transport accepted header injection" );
        require( transport.rejected( "POST", "https://evil.test/fal-ai/trellis-2", auth ), "Native transport accepted external API origin" );
        require( transport.rejected( "GET", "https://queue.fal.run/fal-ai/trellis-2/requests/id/../../private", auth ), "Native transport accepted path traversal" );
        require( transport.rejected( "DELETE", std::string(endpoint) + "/requests/id", auth ), "Native transport allowed remote deletion" );
        require( transport.rejected( "GET", "https://v3b.fal.media/model.glb", auth ), "Native transport accepted CDN credentials" );
        require( transport.rejected( "POST", "https://v3b.fal.media/model.glb", {} ), "Native transport accepted a CDN upload" );
        require( transport.rejected( "GET", "https://v3b.fal.media.evil.test/model.glb", {} ), "Native transport accepted CDN suffix spoofing" );
        require( transport.rejected( "GET", "https://v3b.fal.media/model.glb", {}, "unexpected" ), "Native transport accepted a GET body" );
        require( transport.rejected( "GET", "https://v3b.fal.media/model.glb", {}, {}, maximumGeneratedGlbBytes + 1 ), "Native transport accepted an unbounded response" );
    }
}

int main()
{
    try
    {
        contracts();
        std::cout << "PASS: TRELLIS 2 queue protocol, Key authentication, safe downloads, cancellation, limits and mesh validation\n";
        return 0;
    }
    catch( const std::exception &error )
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}

