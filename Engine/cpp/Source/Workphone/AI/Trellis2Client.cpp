#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/AI/Trellis2Client.hpp>
#include <Workphone/AI/Trellis2Credentials.hpp>
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
        constexpr auto apiUrl = "https://queue.fal.run/fal-ai/trellis-2";
        constexpr auto oversized = "The TRELLIS 2 response exceeds the supported size limit.";
        constexpr auto unsupportedPlatform = "TRELLIS 2 HTTPS transport currently requires Windows.";

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
            if( url.size() > 8192 || url.compare( 0, 8, "https://" ) != 0 ) return false;
            for( unsigned char c : url )
                if( c <= 32 || c >= 127 || c == '\\' || c == '#' ) return false;
            const auto pathStart = url.find( '/', 8 );
            if( pathStart == std::string::npos || pathStart + 1 == url.size() ) return false;
            const auto host = url.substr( 8, pathStart - 8 );
            return host == "fal.media" || host == "v3.fal.media" || host == "v3b.fal.media";
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

        bool safeApiUrl( const std::string &method, const std::string &url )
        {
            if( method == "POST" ) return url == apiUrl;
            if( method != "GET" ) return false;
            const auto prefix = std::string( apiUrl ) + "/requests/";
            if( url.compare( 0, prefix.size(), prefix ) != 0 ) return false;
            auto segment = url.substr( prefix.size() );
            if( segment.size() > 7 && segment.compare( segment.size() - 7, 7, "/status" ) == 0 )
                segment.resize( segment.size() - 7 );
            if( segment.empty() || segment.size() > 768 ) return false;
            const auto hex = []( char c ) { return ( c >= '0' && c <= '9' ) || ( c >= 'A' && c <= 'F' ); };
            for( std::size_t i = 0; i < segment.size(); ++i )
            {
                const auto c = segment[i];
                if( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
                    ( c >= '0' && c <= '9' ) || c == '-' || c == '_' ) continue;
                if( c != '%' || i + 2 >= segment.size() || !hex( segment[i + 1] ) || !hex( segment[i + 2] ) ) return false;
                i += 2;
            }
            return true;
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
            case 400:
            case 422: return "fal.ai rejected the TRELLIS 2 image or options. Choose another PNG/JPEG image or check the generation settings.";
            case 401: return "fal.ai authentication failed. Check the TRELLIS 2 key in File > Settings > Editor.";
            case 402: return "fal.ai requires available API credits. Check your fal.ai credit balance and account billing.";
            case 403: return "fal.ai blocked this TRELLIS 2 request. Check account access, credit balance and billing, or choose a different image.";
            case 404: return "The TRELLIS 2 request or result could not be found. Check your fal.ai request history; results may expire.";
            case 429: return "fal.ai is rate limiting requests. Wait before trying again; check request history before submitting another generation.";
            case 499: return "The TRELLIS 2 task was cancelled by fal.ai. Check your fal.ai request history.";
            default: return "TRELLIS 2 request failed (HTTP " + std::to_string( status ) + "). Check fal.ai service status and request history before trying again.";
            }
        }
    }

    Trellis2Client::~Trellis2Client() = default;

    Result Trellis2Client::generate( const Request &options, std::uint32_t resolution,
                                     const std::atomic_bool &cancel,
                                     const std::function<void( const Progress & )> &progress ) const
    {
        Result result;
#if WP_USE_BOOST
        const auto cancelled = [&]() {
            if( !cancel.load( std::memory_order_relaxed ) ) return false;
            result.glbBytes.clear();
            result.cancelled = true;
            result.error = result.taskId.empty() ? "Image to Mesh stopped. A submitted fal.ai task may continue on your account."
                : "Stopped waiting for TRELLIS 2. The submitted fal.ai task may continue on your account; no mesh was saved.";
            return true;
        };
        try
        {
            if( cancelled() || !validateSourceImage( options.imageBytes.data(), options.imageBytes.size(),
                                                    options.mimeType, result.error ) ) return result;
            if( resolution != 512 && resolution != 1024 && resolution != 1536 )
            { result.error = "Choose a TRELLIS 2 resolution of 512, 1024 or 1536."; return result; }
            if( options.targetPolycount < 5000 || options.targetPolycount > 2000000 )
            { result.error = "Choose a TRELLIS 2 target vertex count between 5000 and 2000000."; return result; }
            if( !options.textured )
            { result.error = "Hosted TRELLIS 2 requires texture generation."; return result; }
            std::string key;
            SecretGuard guard{ key };
            if( !resolveApiKey( key, result.error ) ) return result;
            if( key.empty() || key.size() > 4096 || !std::all_of( key.begin(), key.end(),
                    []( unsigned char c ) { return c > 32 && c < 127; } ) )
            { result.error = "The fal.ai API key is invalid. Check the TRELLIS 2 key in Editor settings."; return result; }
            std::string authorization = "Key " + key;
            SecretGuard authGuard{ authorization };
            const auto report = [&]( const std::string &stage, std::uint32_t percent = 0 ) {
                if( progress ) progress( Progress{ result.taskId, percent, stage } );
            };
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes( 20 );
            detail::json::object payload;
            payload["image_url"] = "data:" + options.mimeType + ";base64," + base64( options.imageBytes );
            payload["resolution"] = resolution;
            payload["decimation_target"] = options.targetPolycount;
            payload["texture_size"] = 2048;
            payload["remesh"] = true;
            std::vector<std::uint8_t> response;
            std::uint32_t status = 0;
            std::string diagnostic;
            report( "Submitting image to TRELLIS 2 on fal.ai" );
            if( cancelled() ) return result;
            // Never retry this POST: a lost response can still represent a billable submitted task.
            if( !request( "POST", apiUrl, authorization, detail::json::serialize( payload ),
                          detail::maximumJsonBytes, cancel, response, status, diagnostic ) )
            {
                if( cancelled() ) return result;
                result.error = diagnostic == unsupportedPlatform ? diagnostic :
                    "Could not confirm the TRELLIS 2 submission. Check your fal.ai request history before retrying to avoid duplicate generations.";
                return result;
            }
            if( status < 200 || status >= 300 )
            { if( !cancelled() ) result.error = httpError( status ); return result; }
            detail::json::value created;
            if( !detail::parseJson( reinterpret_cast<const char *>( response.data() ), response.size(), created ) ||
                !created.is_object() || !safeTaskId( detail::stringField( created.as_object(), "request_id" ), key ) )
            {
                if( !cancelled() ) result.error = "fal.ai returned an invalid task response. Check your fal.ai request history before retrying.";
                return result;
            }
            result.taskId = detail::stringField( created.as_object(), "request_id" );
            // Preserve a confirmed task ID if Stop was pressed while the submission was in flight.
            if( cancelled() ) return result;
            const auto resultUrl = std::string( apiUrl ) + "/requests/" + encodePathSegment( result.taskId );
            const auto statusUrl = resultUrl + "/status";
            report( "Waiting for TRELLIS 2 on fal.ai" );
            for( unsigned poll = 0; poll < 120 && std::chrono::steady_clock::now() < deadline; ++poll )
            {
                if( cancelled() ) return result;
                if( !request( "GET", statusUrl, authorization, {}, detail::maximumJsonBytes,
                              cancel, response, status, diagnostic ) )
                {
                    if( !cancelled() ) result.error = "Could not check the TRELLIS 2 task. It may still be running; check your fal.ai request history before submitting another generation.";
                    return result;
                }
                if( cancelled() ) return result;
                if( status < 200 || status >= 300 ) { result.error = httpError( status ); return result; }
                detail::json::value task;
                if( !detail::parseJson( reinterpret_cast<const char *>( response.data() ), response.size(), task ) || !task.is_object() )
                { result.error = "fal.ai returned an invalid TRELLIS 2 task status."; return result; }
                const auto &fields = task.as_object();
                if( detail::stringField( fields, "request_id" ) != result.taskId )
                { result.error = "fal.ai returned a missing or mismatched TRELLIS 2 task ID."; return result; }
                const auto state = detail::stringField( fields, "status" );
                if( state == "COMPLETED" )
                {
                    // COMPLETED also covers failed/cancelled jobs. The result endpoint supplies their HTTP error.
                    report( "Retrieving TRELLIS 2 result" );
                    if( cancelled() ) return result;
                    if( !request( "GET", resultUrl, authorization, {}, detail::maximumJsonBytes,
                                  cancel, response, status, diagnostic ) )
                    {
                        if( !cancelled() ) result.error = "Could not retrieve the TRELLIS 2 result. Check your fal.ai request history before generating again.";
                        return result;
                    }
                    if( cancelled() ) return result;
                    if( status < 200 || status >= 300 ) { result.error = httpError( status ); return result; }
                    detail::json::value output;
                    if( !detail::parseJson( reinterpret_cast<const char *>( response.data() ), response.size(), output ) || !output.is_object() )
                    { result.error = "fal.ai returned an invalid TRELLIS 2 result."; return result; }
                    const auto model = output.as_object().if_contains( "model_glb" );
                    const auto url = model && model->is_object() ? detail::stringField( model->as_object(), "url" ) : std::string();
                    if( !safeAssetUrl( url ) || url.find( key ) != std::string::npos )
                    { result.error = "fal.ai returned an unsupported mesh download URL."; return result; }
                    report( "Downloading generated mesh" );
                    if( cancelled() ) return result;
                    // The CDN URL is self-authenticating. Never send the fal.ai API key to a media host.
                    if( !request( "GET", url, {}, {}, maximumGeneratedGlbBytes,
                                  cancel, result.glbBytes, status, diagnostic ) )
                    {
                        result.glbBytes.clear();
                        if( !cancelled() ) result.error = diagnostic == oversized ? diagnostic :
                            "Could not download the generated mesh. Check your fal.ai request history to retrieve it.";
                        return result;
                    }
                    if( cancelled() ) return result;
                    if( status < 200 || status >= 300 )
                    { result.glbBytes.clear(); result.error = "The generated mesh download failed. Retrieve the result from your fal.ai request history."; return result; }
                    if( !validateGeneratedGlb( result.glbBytes.data(), result.glbBytes.size(), result.error ) ) result.glbBytes.clear();
                    if( cancelled() ) return result;
                    if( result.error.empty() ) report( "Mesh ready", 100 );
                    if( cancelled() ) return result;
                    return result;
                }
                if( state != "IN_QUEUE" && state != "IN_PROGRESS" )
                { result.error = "fal.ai returned an unknown TRELLIS 2 task status."; return result; }
                report( state == "IN_QUEUE" ? "Queued in fal.ai" : "Generating mesh with TRELLIS 2" );
                if( !waitForPoll( cancel ) )
                {
                    if( !cancelled() ) result.error = "Stopped waiting for TRELLIS 2. Check your fal.ai request history.";
                    return result;
                }
            }
            result.error = "TRELLIS 2 has not finished within 20 minutes. The task may still complete; check your fal.ai request history before generating again.";
        }
        catch( ... )
        {
            result.glbBytes.clear();
            if( !cancelled() ) result.error = "TRELLIS 2 generation failed. Check your configuration and fal.ai request history before trying again.";
        }
#else
        result.error = "Image to Mesh requires a build with Boost enabled.";
#endif
        return result;
    }

    bool Trellis2Client::resolveApiKey( std::string &key, std::string &error ) const
    {
        auto application = core::IApplicationManager::instancePtr();
        String secret, failure;
        const bool found = trellis2_credentials::resolveKey( application && application->isEditor(), secret, failure );
        if( found ) key.assign( secret.data(), secret.size() );
        for( std::size_t i = 0; i < secret.size(); ++i ) static_cast<volatile char &>( secret[i] ) = 0;
        error.assign( failure.data(), failure.size() );
        return found;
    }

    bool Trellis2Client::waitForPoll( const std::atomic_bool &cancel ) const
    {
        for( unsigned tick = 0; tick < 100; ++tick )
        {
            if( cancel.load( std::memory_order_relaxed ) ) return false;
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        }
        return !cancel.load( std::memory_order_relaxed );
    }

    bool Trellis2Client::request( const std::string &method, const std::string &url,
                                  const std::string &authorization, const std::string &body,
                                  std::size_t maximumBytes, const std::atomic_bool &cancel,
                                  std::vector<std::uint8_t> &response, std::uint32_t &status,
                                  std::string &error ) const
    {
        response.clear(); status = 0; error.clear();
#ifdef _WIN32
        const bool api = safeApiUrl( method, url );
        if( ( !api && ( !safeAssetUrl( url ) || !authorization.empty() || method != "GET" ) ) ||
            ( api && ( authorization.size() <= 4 || authorization.compare( 0, 4, "Key " ) != 0 ) ) ||
            ( method == "GET" && !body.empty() ) || body.size() > maximumSourceImageBytes * 2 ||
            maximumBytes > maximumGeneratedGlbBytes || cancel.load( std::memory_order_relaxed ) ) return false;
        for( std::size_t i = 4; i < authorization.size(); ++i )
            if( static_cast<unsigned char>( authorization[i] ) <= 32 || static_cast<unsigned char>( authorization[i] ) >= 127 ) return false;
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
        const auto pathStart = url.find( '/', 8 );
        const auto host = url.substr( 8, pathStart - 8 );
        const std::wstring wideHost( host.begin(), host.end() );
        Handle connection( WinHttpConnect( session, wideHost.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0 ) );
        if( !connection || pathStart == std::string::npos || stopped() ) return false;
        const auto path = url.substr( pathStart );
        const std::wstring widePath( path.begin(), path.end() );
        Handle operation( WinHttpOpenRequest( connection, method == "POST" ? L"POST" : L"GET",
                                             widePath.c_str(), nullptr, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE ) );
        DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        if( !operation || !WinHttpSetOption( operation, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof( policy ) ) ) return false;
        std::wstring headers = L"Content-Type: application/json\r\n";
        if( !authorization.empty() ) headers += L"Authorization: " + std::wstring( authorization.begin(), authorization.end() ) + L"\r\n";
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
