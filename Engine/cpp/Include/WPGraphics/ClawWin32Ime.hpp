#pragma once

#if defined WP_PLATFORM_WIN32
#    include <Windows.h>
#    include <imm.h>
#    include <cmath>
#    include <limits>

#    if defined _MSC_VER
#        pragma comment( lib, "imm32" )
#    endif

namespace workphone::render::detail
{
    inline UINT platformImeMessage( bool visible )
    {
        // The message ID carries visibility, leaving both parameters available
        // for complete signed 32-bit coordinates without pointer payloads.
        static const UINT shown = RegisterWindowMessageW( L"Workphone.Claw.ImGui.ImeShown.v1" );
        static const UINT hidden = RegisterWindowMessageW( L"Workphone.Claw.ImGui.ImeHidden.v1" );
        return visible ? shown : hidden;
    }

    inline LONG platformImeCoordinate( float value )
    {
        if( !std::isfinite( value ) ) return 0;
        const auto coordinate = static_cast<double>( value );
        if( coordinate <= static_cast<double>( ( std::numeric_limits<LONG>::min )() ) )
            return ( std::numeric_limits<LONG>::min )();
        if( coordinate >= static_cast<double>( ( std::numeric_limits<LONG>::max )() ) )
            return ( std::numeric_limits<LONG>::max )();
        return static_cast<LONG>( coordinate );
    }

    inline bool postPlatformImeUpdate( HWND hwnd, bool visible, LONG x, LONG y )
    {
        DWORD processId = 0;
        if( !hwnd || !GetWindowThreadProcessId( hwnd, &processId ) ||
            processId != GetCurrentProcessId() ) return false;
        const auto message = platformImeMessage( visible );
        return message && PostMessageW( hwnd, message, static_cast<WPARAM>( x ),
                                       static_cast<LPARAM>( y ) ) != FALSE;
    }

    inline bool handlePlatformImeMessage( HWND hwnd, UINT message, WPARAM x, LPARAM y )
    {
        const auto shown = platformImeMessage( true );
        const auto hidden = platformImeMessage( false );
        if( !message || ( message != shown && message != hidden ) ) return false;
        if( GetWindowThreadProcessId( hwnd, nullptr ) != GetCurrentThreadId() ) return false;

        // These APIs may synchronously send window messages. Execute them only
        // on the HWND owner, outside the render thread's GraphicsSystem lock.
        ImmAssociateContextEx( hwnd, nullptr, message == shown ? IACE_DEFAULT : 0 );
        if( auto context = ImmGetContext( hwnd ) )
        {
            COMPOSITIONFORM composition{};
            composition.ptCurrentPos.x = static_cast<LONG>( x );
            composition.ptCurrentPos.y = static_cast<LONG>( y );
            composition.dwStyle = CFS_FORCE_POSITION;
            ImmSetCompositionWindow( context, &composition );
            CANDIDATEFORM candidate{};
            candidate.dwStyle = CFS_CANDIDATEPOS;
            candidate.ptCurrentPos = composition.ptCurrentPos;
            ImmSetCandidateWindow( context, &candidate );
            ImmReleaseContext( hwnd, context );
        }
        return true;
    }
}  // namespace workphone::render::detail
#endif
