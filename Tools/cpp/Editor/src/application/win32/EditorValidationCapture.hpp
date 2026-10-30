#pragma once

#include <Workphone/Workphone.hpp>
#include <boost/json.hpp>
#include <Windows.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace workphone::editor
{
    // Opt-in observation of this Editor's rendered client area. Requests contain
    // an output token only: no commands, input injection or terrain mutations.
    class EditorValidationCapture
    {
    public:
        void configure( const std::filesystem::path &directory )
        {
            if( directory.empty() ) return;
            if( !directory.is_absolute() || !std::filesystem::is_directory( directory ) ||
                std::filesystem::is_symlink( std::filesystem::symlink_status( directory ) ) )
                throw std::runtime_error( "Validation capture requires an existing absolute directory" );
            m_directory = std::filesystem::canonical( directory );
        }

        void poll()
        {
            if( m_directory.empty() || Thread::getCurrentTask() != TaskId::Render ) return;
            ++m_frame;
            std::array<char, 66> request{};
            std::ifstream input( m_directory / "capture.request", std::ios::binary );
            if( !input ) return;
            input.read( request.data(), request.size() );
            const auto count = input.gcount();
            if( count < 1 || count > 64 ) return;
            const std::string token( request.data(), size_t( count ) );
            if( token == m_lastToken || !std::all_of( token.begin(), token.end(), []( unsigned char c ) {
                    return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
                           ( c >= '0' && c <= '9' ) || c == '_' || c == '-';
                } ) ) return;
            const auto metadata = m_directory / ( token + ".json" );
            if( std::filesystem::exists( metadata ) )
            {
                m_lastToken = token;
                return;
            }
            if( m_pendingToken == token && !m_pendingMetadata.empty() )
            {
                publishMetadata( token, metadata );
                return;
            }
            boost::json::object record{ { "schema", "workphone.editor-native-capture.v1" },
                { "token", token }, { "processId", GetCurrentProcessId() },
                { "renderSequence", m_frame }, { "captureSource", "Editor default-window back buffer" },
                { "interactiveInputVerified", false } };
            try
            {
                if( std::filesystem::canonical( m_directory ) != m_directory )
                    throw std::runtime_error( "Capture directory changed" );
                auto app = core::IApplicationManager::instancePtr();
                auto graphics = app ? app->getGraphicsSystem() : nullptr;
                auto window = graphics ? graphics->getDefaultWindow() : nullptr;
                if( !window ) throw std::runtime_error( "Editor window is unavailable" );
                const auto size = window->getSize();
                if( size.x <= 0 || size.y <= 0 || size.x > 16384 || size.y > 16384 ||
                    uint64_t( size.x ) * size.y > 32ull * 1024 * 1024 )
                    throw std::runtime_error( "Editor capture dimensions exceed limits" );
                void *handle = nullptr;
                window->getWindowHandle( &handle );
                const auto hwnd = static_cast<HWND>( handle );
                DWORD process = 0;
                GetWindowThreadProcessId( hwnd, &process );
                if( !hwnd || process != GetCurrentProcessId() )
                    throw std::runtime_error( "Capture window does not belong to this Editor" );
                RECT bounds{}, client{};
                POINT origin{};
                if( !GetWindowRect( hwnd, &bounds ) || !GetClientRect( hwnd, &client ) ||
                    !ClientToScreen( hwnd, &origin ) )
                    throw std::runtime_error( "Editor window geometry is unavailable" );
                using GetDpi = UINT ( WINAPI * )( HWND );
                const auto getDpi = reinterpret_cast<GetDpi>(
                    GetProcAddress( GetModuleHandleW( L"user32.dll" ), "GetDpiForWindow" ) );
                const auto dpi = getDpi ? getDpi( hwnd ) : 0;
                record["windowHandle"] = uint64_t( reinterpret_cast<uintptr_t>( hwnd ) );
                record["windowRect"] = boost::json::array{ bounds.left, bounds.top, bounds.right, bounds.bottom };
                record["clientRect"] = boost::json::array{ client.left, client.top, client.right, client.bottom };
                record["clientScreenOrigin"] = boost::json::array{ origin.x, origin.y };
                record["clientOffsetInWindow"] = boost::json::array{ origin.x - bounds.left, origin.y - bounds.top };
                record["foregroundWindowMatches"] = GetForegroundWindow() == hwnd;
                POINT cursor{};
                if( GetCursorPos( &cursor ) && ScreenToClient( hwnd, &cursor ) )
                {
                    const bool inside = cursor.x >= 0 && cursor.y >= 0 &&
                        cursor.x < client.right && cursor.y < client.bottom;
                    record["cursorInsideClient"] = inside;
                    if( inside ) record["cursorClientPosition"] = boost::json::array{ cursor.x, cursor.y };
                }
                record["dpi"] = dpi;
                record["width"] = size.x;
                record["height"] = size.y;
                record["clientDimensionsMatch"] = size.x == client.right - client.left && size.y == client.bottom - client.top;
                std::vector<uint8_t> pixels( size_t( size.x ) * size.y * 4 );
                if( !window->tryCopyContentsToMemory( pixels.data(), u32( pixels.size() ) ) )
                    throw std::runtime_error( "Editor back-buffer readback is unavailable" );
                const auto bitmap = m_directory / ( token + ".bmp" );
                writeBitmap( bitmap, size.x, size.y, pixels );
                record["image"] = bitmap.generic_string();
                record["success"] = true;
            }
            catch( const std::exception &error )
            {
                record["success"] = false;
                record["error"] = error.what();
            }
            SYSTEMTIME time{};
            GetSystemTime( &time );
            std::ostringstream utc;
            utc << std::setfill( '0' ) << std::setw( 4 ) << time.wYear << '-' << std::setw( 2 ) << time.wMonth
                << '-' << std::setw( 2 ) << time.wDay << 'T' << std::setw( 2 ) << time.wHour << ':'
                << std::setw( 2 ) << time.wMinute << ':' << std::setw( 2 ) << time.wSecond << '.'
                << std::setw( 3 ) << time.wMilliseconds << 'Z';
            record["capturedAtUtc"] = utc.str();
            m_pendingToken = token;
            m_pendingMetadata = boost::json::serialize( record );
            publishMetadata( token, metadata );
        }

    private:
        void publishMetadata( const std::string &token, const std::filesystem::path &file )
        {
            try
            {
                writeFresh( file, m_pendingMetadata );
                m_lastToken = token;
                m_pendingToken.clear();
                m_pendingMetadata.clear();
                m_reportedMetadataErrorToken.clear();
            }
            catch( const std::exception &error )
            {
                // Preserve the original pixels/timestamp and retry only their
                // acknowledgement. Observation failures must not escape update().
                if( m_reportedMetadataErrorToken != token )
                {
                    WP_LOG_ERROR( String( "Editor validation capture metadata failed for " ) +
                                  token.c_str() + ": " + error.what() );
                    m_reportedMetadataErrorToken = token;
                }
            }
        }

        static void writeFresh( const std::filesystem::path &file, const std::string &bytes )
        {
            auto temporary = file;
            temporary += ".tmp";
            if( std::filesystem::exists( file ) || std::filesystem::exists( temporary ) )
                throw std::runtime_error( "Capture output already exists" );
            std::ofstream output( temporary, std::ios::binary );
            output.write( bytes.data(), std::streamsize( bytes.size() ) );
            output.close();
            if( !output ) throw std::runtime_error( "Cannot write capture output" );
            std::filesystem::rename( temporary, file );
        }

        static void writeBitmap( const std::filesystem::path &file, int width, int height,
                                 std::vector<uint8_t> &rgba )
        {
            BITMAPFILEHEADER header{};
            BITMAPINFOHEADER info{};
            header.bfType = 0x4d42;
            header.bfOffBits = sizeof( header ) + sizeof( info );
            header.bfSize = DWORD( header.bfOffBits + rgba.size() );
            info.biSize = sizeof( info );
            info.biWidth = width;
            info.biHeight = -height;
            info.biPlanes = 1;
            info.biBitCount = 32;
            info.biCompression = BI_RGB;
            for( size_t offset = 0; offset < rgba.size(); offset += 4 )
            {
                std::swap( rgba[offset], rgba[offset + 2] );
                rgba[offset + 3] = 255;
            }
            std::string bytes( sizeof( header ) + sizeof( info ), '\0' );
            std::memcpy( bytes.data(), &header, sizeof( header ) );
            std::memcpy( bytes.data() + sizeof( header ), &info, sizeof( info ) );
            bytes.append( reinterpret_cast<const char *>( rgba.data() ), rgba.size() );
            writeFresh( file, bytes );
        }

        std::filesystem::path m_directory;
        std::string m_lastToken;
        std::string m_pendingToken;
        std::string m_pendingMetadata;
        std::string m_reportedMetadataErrorToken;
        uint64_t m_frame = 0;
    };
}
