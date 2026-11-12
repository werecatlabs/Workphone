#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/AI/MeshyClient.hpp>
#include <Workphone/AI/MeshyCredentials.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <algorithm>
#include <chrono>
#include <thread>

#if WP_USE_BOOST
#    include "MeshyValidation.hpp"
#endif

#ifdef _WIN32
#    include <Windows.h>
#    include <winhttp.h>
#    pragma comment( lib, "winhttp.lib" )
#endif

namespace workphone::mesh_generation
{
    namespace
    {
        constexpr auto apiUrl = "https://api.meshy.ai/openapi/v1/image-to-3d";
        constexpr auto assetPrefix = "https://assets.meshy.ai/";
        constexpr auto oversized = "The Meshy response exceeds the supported size limit.";
        constexpr auto unsupportedPlatform = "Meshy HTTPS transport currently requires Windows.";

        struct SecretGuard
        {
            std::string &secret;
            ~SecretGuard()
            {
                volatile char *data = secret.empty() ? nullptr : &secret[0];
                for( std::size_t i = 0; i < secret.size(); ++i ) data[i] = 0;
                secret.clear();
            }
        };

        bool safeAssetUrl( const std::string &url )
        {
            if( url.compare( 0, std::char_traits<char>::length( assetPrefix ), assetPrefix ) != 0 ||
                url.size() > 8192 || url.size() <= std::char_traits<char>::length( assetPrefix ) ) return false;
            for( unsigned char c : url ) if( c <= 32 || c >= 127 || c == '\\' || c == '#' ) return false;
            return true;
        }

        bool safeTaskId( const std::string &id, const std::string &key )
        {
            if( id.empty() || id.size() > 256 || id == "." || id == ".." ||
                ( !key.empty() && id.find( key ) != std::string::npos ) ) return false;
            return std::all_of( id.begin(), id.end(), []( unsigned char c ) { return c > 32 && c < 127; } );
        }

        std::string encodePathSegment( const std::string &value )
        {
            static constexpr char hex[] = "0123456789ABCDEF";
            std::string result;
            for( unsigned char c : value )
            {
                if( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
                    ( c >= '0' && c <= '9' ) || c == '-' || c == '_' ) result += char( c );
                else { result += '%'; result += hex[c >> 4]; result += hex[c & 15]; }
            }
            return result;
        }

        std::string base64( const std::vector<std::uint8_t> &bytes )
        {
            static constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string result;
            result.reserve( ( ( bytes.size() + 2 ) / 3 ) * 4 );
            for( std::size_t i = 0; i < bytes.size(); i += 3 )
            {
                const std::uint32_t packed = ( std::uint32_t( bytes[i] ) << 16 ) |
                    ( i + 1 < bytes.size() ? std::uint32_t( bytes[i + 1] ) << 8 : 0 ) |
                    ( i + 2 < bytes.size() ? bytes[i + 2] : 0 );
                result += alphabet[( packed >> 18 ) & 63];
                result += alphabet[( packed >> 12 ) & 63];
                result += i + 1 < bytes.size() ? alphabet[( packed >> 6 ) & 63] : '=';
                result += i + 2 < bytes.size() ? alphabet[packed & 63] : '=';
            }
            return result;
        }

        std::string httpError( std::uint32_t status )
        {
            switch( status )
            {
            case 400: return "Meshy rejected the image or generation options. Choose another PNG/JPEG image.";
            case 401: return "Meshy authentication failed. Check the Meshy key in File > Settings > Editor.";
            case 402: return "Meshy requires a paid API plan and available credits. Check your Meshy subscription, credit balance and key credit limit.";
            case 403: return "Meshy blocked this request. Check account access or choose a different image.";
            case 404: return "The Meshy task could not be found. Check your Meshy task history.";
            case 429: return "Meshy is rate limiting requests. Wait before trying again; check task history before submitting a new generation.";
            default: return "Meshy request failed (HTTP " + std::to_string( status ) + "). Check Meshy service status and your task history before trying again.";
            }
        }
    }

    bool validateSourceImage( const std::uint8_t *bytes, std::size_t size,
                               const std::string &mimeType, std::string &error )
    {
        error.clear();
#if WP_USE_BOOST
        try
        {
            if( !bytes || size == 0 || size > maximumSourceImageBytes )
                error = "Choose a PNG or JPEG image of at most 20 MiB.";
            else if( !( mimeType == "image/png" ? detail::png( bytes, size ) :
                        mimeType == "image/jpeg" ? detail::jpeg( bytes, size ) : false ) )
                error = "The source must be a valid PNG or JPEG, at least 32 pixels on each side and no more than 178956970 pixels.";
        }
        catch( ... ) { error = "Could not validate the source image."; }
#else
        error = "Image to Mesh requires a build with Boost enabled.";
#endif
        return error.empty();
    }

    bool validateGeneratedGlb( const std::uint8_t *bytes, std::size_t size, std::string &error )
    {
        error.clear();
#if WP_USE_BOOST
        try
        {
            if( !bytes || size > maximumGeneratedGlbBytes || !detail::glb( bytes, size ) )
                error = "Meshy returned an invalid or unsupported GLB mesh. A self-contained triangle mesh of at most 128 MiB is required.";
        }
        catch( ... ) { error = "Could not validate the generated mesh."; }
#else
        error = "Image to Mesh requires a build with Boost enabled.";
#endif
        return error.empty();
    }

    MeshyClient::~MeshyClient() = default;

    Result MeshyClient::generate( const Request &options, const std::atomic_bool &cancel,
                                   const std::function<void( const Progress & )> &progress ) const
    {
        Result result;
#if WP_USE_BOOST
        const auto cancelled = [&]() {
            if( !cancel.load( std::memory_order_relaxed ) ) return false;
            result.glbBytes.clear();
            result.cancelled = true;
            result.error = result.taskId.empty() ? "Image to Mesh stopped. A submitted Meshy task may continue on your account."
                : "Stopped waiting for Meshy. The submitted task may continue on your account; no mesh was saved.";
            return true;
        };
        try
        {
            if( cancelled() || !validateSourceImage( options.imageBytes.data(), options.imageBytes.size(),
                                                    options.mimeType, result.error ) ) return result;
            if( options.targetPolycount < 100 || options.targetPolycount > 300000 )
            {
                result.error = "Choose a target polygon count between 100 and 300000.";
                return result;
            }
            std::string key;
            SecretGuard guard{ key };
            if( !resolveApiKey( key, result.error ) ) return result;
            const auto report = [&]( const std::string &stage, std::uint32_t percent ) {
                if( progress ) progress( Progress{ result.taskId, percent, stage } );
            };
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes( 20 );
            detail::json::object payload;
            payload["image_url"] = "data:" + options.mimeType + ";base64," + base64( options.imageBytes );
            payload["ai_model"] = "meshy-7.1";
            payload["should_remesh"] = true;
            payload["topology"] = "triangle";
            payload["target_polycount"] = options.targetPolycount;
            payload["should_texture"] = options.textured;
            payload["target_formats"] = detail::json::array{ "glb" };
            std::vector<std::uint8_t> response;
            std::uint32_t status = 0;
            std::string diagnostic;
            report( "Submitting image to Meshy", 0 );
            if( cancelled() ) return result;
            if( !request( "POST", apiUrl, key, detail::json::serialize( payload ), detail::maximumJsonBytes,
                          cancel, response, status, diagnostic ) )
            {
                if( cancelled() ) return result;
                result.error = diagnostic == unsupportedPlatform ? diagnostic :
                    "Could not confirm the Meshy submission. Check your Meshy task history before retrying to avoid duplicate generations.";
                return result;
            }
            if( status < 200 || status >= 300 )
            {
                if( !cancelled() ) result.error = httpError( status );
                return result;
            }
            detail::json::value created;
            if( !detail::parseJson( reinterpret_cast<const char *>( response.data() ), response.size(), created ) ||
                !created.is_object() || !safeTaskId( detail::stringField( created.as_object(), "result" ), key ) )
            {
                if( !cancelled() ) result.error = "Meshy returned an invalid task response. Check your Meshy task history before retrying.";
                return result;
            }
            result.taskId = detail::stringField( created.as_object(), "result" );
            // Keep a confirmed submission ID even if Stop was pressed during upload.
            if( cancelled() ) return result;
            const auto taskUrl = std::string( apiUrl ) + "/" + encodePathSegment( result.taskId );
            report( "Waiting for Meshy", 0 );
            for( unsigned poll = 0; poll < 120 && std::chrono::steady_clock::now() < deadline; ++poll )
            {
                if( cancelled() ) return result;
                if( !request( "GET", taskUrl, key, {}, detail::maximumJsonBytes, cancel, response, status, diagnostic ) )
                {
                    if( cancelled() ) return result;
                    result.error = "Could not check the Meshy task. It may still be running; check your Meshy task history before submitting another generation.";
                    return result;
                }
                if( cancelled() ) return result;
                if( status < 200 || status >= 300 ) { result.error = httpError( status ); return result; }
                detail::json::value task;
                if( !detail::parseJson( reinterpret_cast<const char *>( response.data() ), response.size(), task ) || !task.is_object() )
                { result.error = "Meshy returned an invalid task status."; return result; }
                const auto &fields = task.as_object();
                if( const auto id = fields.if_contains( "id" ); id && ( !id->is_string() || id->as_string() != result.taskId ) )
                { result.error = "Meshy returned a mismatched task status."; return result; }
                const auto state = detail::stringField( fields, "status" );
                if( state == "SUCCEEDED" )
                {
                    const auto urls = fields.if_contains( "model_urls" );
                    const auto url = urls && urls->is_object() ? detail::stringField( urls->as_object(), "glb" ) : std::string();
                    if( !safeAssetUrl( url ) ) { result.error = "Meshy returned an unsupported mesh download URL."; return result; }
                    report( "Downloading generated mesh", 100 );
                    if( cancelled() ) return result;
                    // Signed asset URLs authenticate themselves. Never send the API key to this host.
                    if( !request( "GET", url, {}, {}, maximumGeneratedGlbBytes, cancel, result.glbBytes, status, diagnostic ) )
                    {
                        result.glbBytes.clear();
                        if( cancelled() ) return result;
                        result.error = diagnostic == oversized ? diagnostic : "Could not download the generated mesh. Check your Meshy task history to retrieve it.";
                        return result;
                    }
                    if( cancelled() ) return result;
                    if( status < 200 || status >= 300 )
                    { result.glbBytes.clear(); result.error = "The generated mesh download failed. Retrieve the result from your Meshy task history."; return result; }
                    if( !validateGeneratedGlb( result.glbBytes.data(), result.glbBytes.size(), result.error ) ) result.glbBytes.clear();
                    if( cancelled() ) return result;
                    if( result.error.empty() ) report( "Mesh ready", 100 );
                    if( cancelled() ) return result;
                    return result;
                }
                if( state == "FAILED" || state == "CANCELED" )
                { result.error = "Meshy could not finish this task. Review the task in Meshy or choose another image."; return result; }
                if( state != "PENDING" && state != "IN_PROGRESS" )
                { result.error = "Meshy returned an unknown task status."; return result; }
                std::uint64_t percent = 0;
                if( !detail::optionalUnsigned( fields, "progress", percent ) || percent > 100 )
                { result.error = "Meshy returned invalid task progress."; return result; }
                report( state == "PENDING" ? "Queued in Meshy" : "Generating mesh in Meshy", static_cast<std::uint32_t>( percent ) );
                if( !waitForPoll( cancel ) )
                {
                    if( !cancelled() ) result.error = "Stopped waiting for the Meshy task. Check your Meshy task history.";
                    return result;
                }
            }
            result.error = "Meshy has not finished within 20 minutes. The task may still complete; check your Meshy task history before generating again.";
        }
        catch( ... )
        {
            result.glbBytes.clear();
            if( !cancelled() ) result.error = "Image to Mesh failed. Check your configuration and Meshy task history before trying again.";
        }
#else
        result.error = "Image to Mesh requires a build with Boost enabled.";
#endif
        return result;
    }

    bool MeshyClient::resolveApiKey( std::string &key, std::string &error ) const
    {
        auto application = core::IApplicationManager::instancePtr();
        String secret, failure;
        const bool found = meshy_credentials::resolveKey( application && application->isEditor(), secret, failure );
        if( found ) key.assign( secret.data(), secret.size() );
        for( std::size_t i = 0; i < secret.size(); ++i ) static_cast<volatile char &>( secret[i] ) = 0;
        error.assign( failure.data(), failure.size() );
        return found;
    }

    bool MeshyClient::waitForPoll( const std::atomic_bool &cancel ) const
    {
        for( unsigned tick = 0; tick < 100; ++tick )
        {
            if( cancel.load( std::memory_order_relaxed ) ) return false;
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        }
        return !cancel.load( std::memory_order_relaxed );
    }

    bool MeshyClient::request( const std::string &method, const std::string &url,
                                const std::string &bearer, const std::string &body,
                                std::size_t maximumBytes, const std::atomic_bool &cancel,
                                std::vector<std::uint8_t> &response, std::uint32_t &status,
                                std::string &error ) const
    {
        response.clear(); status = 0; error.clear();
#ifdef _WIN32
        const std::string endpoint = apiUrl;
        const bool api = url == endpoint || url.compare( 0, endpoint.size() + 1, endpoint + "/" ) == 0;
        if( ( !api && ( !safeAssetUrl( url ) || !bearer.empty() || method != "GET" ) ) ||
            ( api && bearer.empty() ) || ( method != "POST" && method != "GET" ) ||
            body.size() > maximumSourceImageBytes * 2 || cancel.load( std::memory_order_relaxed ) ) return false;
        for( unsigned char c : bearer ) if( c <= 32 || c >= 127 ) return false;
        struct Handle
        {
            HINTERNET value = nullptr;
            explicit Handle( HINTERNET handle ) : value( handle ) {}
            ~Handle() { if( value ) WinHttpCloseHandle( value ); }
            operator HINTERNET() const { return value; }
            explicit operator bool() const { return value != nullptr; }
        };
        const auto started = std::chrono::steady_clock::now();
        const auto stopped = [&]() { return cancel.load( std::memory_order_relaxed ) ||
            std::chrono::steady_clock::now() - started > std::chrono::seconds( 30 ); };
        Handle session( WinHttpOpen( L"Workphone ImageToMesh", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 ) );
        if( !session || !WinHttpSetTimeouts( session, 10000, 10000, 10000, 10000 ) ) return false;
        const wchar_t *host = api ? L"api.meshy.ai" : L"assets.meshy.ai";
        Handle connection( WinHttpConnect( session, host, INTERNET_DEFAULT_HTTPS_PORT, 0 ) );
        const auto pathStart = url.find( '/', 8 );
        if( !connection || pathStart == std::string::npos || stopped() ) return false;
        const auto path = url.substr( pathStart );
        const std::wstring widePath( path.begin(), path.end() );
        Handle operation( WinHttpOpenRequest( connection, method == "POST" ? L"POST" : L"GET",
                                             widePath.c_str(), nullptr, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE ) );
        DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        if( !operation || !WinHttpSetOption( operation, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof( policy ) ) ) return false;
        std::wstring headers = L"Content-Type: application/json\r\n";
        if( !bearer.empty() ) headers += L"Authorization: Bearer " + std::wstring( bearer.begin(), bearer.end() ) + L"\r\n";
        struct WipeHeaders
        {
            std::wstring &value;
            ~WipeHeaders() { if( !value.empty() ) SecureZeroMemory( &value[0], value.size() * sizeof( wchar_t ) ); }
        } wipe{ headers };
        if( stopped() || !WinHttpSendRequest( operation, headers.c_str(), static_cast<DWORD>( headers.size() ),
                                             body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char *>( body.data() ),
                                             static_cast<DWORD>( body.size() ), static_cast<DWORD>( body.size() ), 0 ) ||
            stopped() || !WinHttpReceiveResponse( operation, nullptr ) ) return false;
        DWORD code = 0, codeSize = sizeof( code );
        if( !WinHttpQueryHeaders( operation, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                  WINHTTP_HEADER_NAME_BY_INDEX, &code, &codeSize, WINHTTP_NO_HEADER_INDEX ) ) return false;
        status = code;
        const auto limit = code >= 200 && code < 300 ? maximumBytes : std::min<std::size_t>( maximumBytes, 64u * 1024u );
        while( !stopped() )
        {
            std::uint8_t block[16384];
            DWORD read = 0;
            if( !WinHttpReadData( operation, block, sizeof( block ), &read ) ) return false;
            if( !read ) return true;
            if( response.size() > limit || read > limit - response.size() )
            { response.clear(); error = oversized; return false; }
            response.insert( response.end(), block, block + read );
        }
        return false;
#else
        error = unsupportedPlatform;
        return false;
#endif
    }
}
