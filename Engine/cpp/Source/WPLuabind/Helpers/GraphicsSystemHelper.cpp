#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Helpers/GraphicsSystemHelper.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Graphics/IGraphicsPipeline.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
#if defined WP_PLATFORM_WIN32
#    include <windows.h>
#endif

namespace workphone
{
    namespace
    {
        struct IntegerSetting
        {
            const char *name;
            s32 fallback, minimum, maximum;
        };

        constexpr IntegerSetting integers[] = { { "graphics.resolution", 5, 0, 5 },
                                                { "graphics.customWidth", 1280, 320, 16384 },
                                                { "graphics.customHeight", 720, 200, 16384 },
                                                { "graphics.antiAliasing", 0, 0, 5 },
                                                { "graphics.tonemapping", 3, 0, 3 },
                                                { "quality.mesh", 2, 0, 3 },
                                                { "quality.texture", 2, 0, 3 },
                                                { "quality.maxTextureSize", 16384, 1, 16384 },
                                                { "graphics.textureFiltering", 2, 0, 2 },
                                                { "graphics.anisotropy", 1, 1, 16 } };

        struct BooleanSetting
        {
            const char *name;
            bool fallback;
        };

        constexpr BooleanSetting booleans[] = {
            { "graphics.fullscreen", false },    { "graphics.vsync", false },
            { "graphics.postProcessing", true }, { "graphics.bloom", false },
            { "graphics.motionBlur", false },    { "graphics.ambientOcclusion", false }
        };

        SmartPtr<Properties> copySettings( SmartPtr<Properties> source )
        {
            auto copy = make_ptr<Properties>();
            if( source )
            {
                *copy = *source;
                copy->setName( source->getName() );
            }
            return copy;
        }

        SmartPtr<render::IGraphicsWindow> outputWindow( render::IGraphicsSystem *system )
        {
            return system ? system->getDefaultWindow() : nullptr;
        }

        SmartPtr<Properties> runtimeSnapshot( render::IGraphicsSystem *system,
                                              SmartPtr<Properties> player )
        {
            auto result = make_ptr<Properties>();
            const auto window = outputWindow( system );
            const auto size = window ? window->getSize() : Vector2I( 1280, 720 );
            const auto app = core::IApplicationManager::instancePtr();
            const bool editor = app && app->isEditor();
            const auto native = system ? system->getProperties() : nullptr;
            const auto pipeline = system ? system->getGraphicsPipeline() : nullptr;
            const auto options = pipeline ? pipeline->getProperties() : nullptr;
            for( const auto &setting : integers )
            {
                auto value = setting.fallback;
                if( String( setting.name ) == "graphics.customWidth" )
                {
                    value = std::max( size.x, 320 );
                }
                if( String( setting.name ) == "graphics.customHeight" )
                {
                    value = std::max( size.y, 200 );
                }
                if( options )
                {
                    options->getPropertyValue( setting.name, value );
                }
                if( player )
                {
                    player->getPropertyValue( setting.name, value );
                }
                result->setProperty( setting.name, value );
            }
            const auto resolution = result->getPropertyAsInt( "graphics.resolution" );
            if( resolution >= 0 && resolution < 4 )
            {
                constexpr s32 sizes[][2] = {
                    { 1920, 1080 }, { 1280, 720 }, { 2560, 1440 }, { 3840, 2160 }
                };
                result->setProperty( "graphics.customWidth", sizes[resolution][0] );
                result->setProperty( "graphics.customHeight", sizes[resolution][1] );
            }
            // The game menu works with explicit dimensions regardless of whether
            // the project's startup profile originally selected a named preset.
            result->setProperty( "graphics.resolution", 5 );
            for( const auto &setting : booleans )
            {
                auto value = setting.fallback;
                if( String( setting.name ) == "graphics.fullscreen" )
                {
                    value = window && window->isFullScreen();
                }
                if( String( setting.name ) == "graphics.vsync" && native )
                {
                    native->getPropertyValue( "vsync", value );
                }
                if( options )
                {
                    options->getPropertyValue( setting.name, value );
                }
                if( player )
                {
                    player->getPropertyValue( setting.name, value );
                }
                result->setProperty( setting.name, value );
            }
            f32 scale = 1;
            if( options )
            {
                options->getPropertyValue( "graphics.renderScale", scale );
            }
            if( player )
            {
                player->getPropertyValue( "graphics.renderScale", scale );
            }
            result->setProperty( "graphics.renderScale", scale );
            result->setProperty( "editor", editor );
            result->setProperty( "displayEditable", !editor && window != nullptr );
            result->setProperty( "displayPending", editor || !window );
            result->setProperty( "actual.width", size.x );
            result->setProperty( "actual.height", size.y );
            result->setProperty( "actual.fullscreen", window && window->isFullScreen() );
            if( native && native->hasProperty( "msaa_supported_mask" ) )
            {
                result->setProperty( "msaa_supported_mask",
                                     native->getPropertyAsInt( "msaa_supported_mask" ) );
            }
            return result;
        }

        void copyRuntimeValues( Properties &target, const Properties &source )
        {
            for( const auto &setting : integers )
            {
                target.addProperty( source.getPropertyObject( setting.name ) );
            }
            for( const auto &setting : booleans )
            {
                target.addProperty( source.getPropertyObject( setting.name ) );
            }
            target.addProperty( source.getPropertyObject( "graphics.renderScale" ) );
        }

        void applyPipeline( render::IGraphicsSystem *system, SmartPtr<Properties> values )
        {
            auto properties = copySettings( values );
            properties->setProperty( "vsync", values->getPropertyAsBool( "graphics.vsync" ) );
            // Software effect toggles and DX11 policy keys describe the same choice.
            const bool post = values->getPropertyAsBool( "graphics.postProcessing" );
            properties->setProperty( "Bloom", post && values->getPropertyAsBool( "graphics.bloom" ) );
            properties->setProperty( "Motion blur",
                                     post && values->getPropertyAsBool( "graphics.motionBlur" ) );
            properties->setProperty( "Ambient occlusion",
                                     post && values->getPropertyAsBool( "graphics.ambientOcclusion" ) );
            properties->setProperty( "Temporal anti-aliasing",
                                     values->getPropertyAsInt( "graphics.antiAliasing" ) == 2 );
            system->setProperties( properties );
            if( auto pipeline = system->getGraphicsPipeline() )
            {
                pipeline->setProperties( properties );
            }
        }

        void applyDisplay( SmartPtr<render::IGraphicsWindow> window, const Properties &values )
        {
            if( !window )
            {
                return;
            }
            const bool fullscreen = values.getPropertyAsBool( "graphics.fullscreen" );
            window->setFullscreen( fullscreen );
            // Borderless fullscreen uses monitor dimensions. Keep the requested
            // windowed size in player settings for the next windowed transition.
            if( fullscreen )
            {
                return;
            }
            constexpr s32 sizes[][2] = { { 1920, 1080 }, { 1280, 720 }, { 2560, 1440 }, { 3840, 2160 } };
            const auto resolution = values.getPropertyAsInt( "graphics.resolution" );
            if( resolution < 4 )
            {
                window->resize( sizes[resolution][0], sizes[resolution][1] );
            }
            else if( resolution == 4 )
            {
                window->maximize();
            }
            else
            {
                window->resize( values.getPropertyAsInt( "graphics.customWidth" ),
                                values.getPropertyAsInt( "graphics.customHeight" ) );
            }
        }

        class DisplayJob : public Job
        {
        public:
            explicit DisplayJob( std::function<void()> callback ) : m_callback( std::move( callback ) )
            {
            }

            void execute() override
            {
                if( !isInterrupted() )
                {
                    m_callback();
                }
            }

        private:
            std::function<void()> m_callback;
        };

        bool requiresPrimary( SmartPtr<render::IGraphicsWindow> window )
        {
#if defined WP_PLATFORM_WIN32
            void *native = nullptr;
            if( window )
            {
                window->getWindowHandle( &native );
            }
            return native && GetWindowThreadProcessId( static_cast<HWND>( native ), nullptr ) !=
                                 GetCurrentThreadId();
#else
            return false;
#endif
        }

        bool sameDisplayPolicy( const Properties &left, const Properties &right )
        {
            return left.getPropertyAsInt( "graphics.resolution" ) ==
                       right.getPropertyAsInt( "graphics.resolution" ) &&
                   left.getPropertyAsInt( "graphics.customWidth" ) ==
                       right.getPropertyAsInt( "graphics.customWidth" ) &&
                   left.getPropertyAsInt( "graphics.customHeight" ) ==
                       right.getPropertyAsInt( "graphics.customHeight" ) &&
                   left.getPropertyAsBool( "graphics.fullscreen" ) ==
                       right.getPropertyAsBool( "graphics.fullscreen" );
        }
    }  // namespace

    SmartPtr<Properties> GraphicsSystemHelper::getRuntimeSettings( render::IGraphicsSystem *system )
    {
        const auto app = core::IApplicationManager::instancePtr();
        ScopedLock lock( system );
        return runtimeSnapshot( system, app ? app->getPlayerSettings() : nullptr );
    }

    SmartPtr<Properties> GraphicsSystemHelper::applyRuntimeSettings( render::IGraphicsSystem *system,
                                                                     SmartPtr<Properties> requested )
    {
        auto result = make_ptr<Properties>();
        result->setProperty( "success", false );
        const auto app = core::IApplicationManager::instancePtr();
        if( !system || !app || !requested )
        {
            result->setProperty( "status", String( "Graphics settings are unavailable." ) );
            return result;
        }
        ScopedLock lock( system );
        auto previousPlayer = app->getPlayerSettings();
        auto previous = runtimeSnapshot( system, previousPlayer );
        auto next = copySettings( previous );
        try
        {
            for( const auto &setting : integers )
            {
                if( requested->hasProperty( setting.name ) )
                {
                    const auto value = requested->getPropertyAsFloat( setting.name );
                    if( !std::isfinite( value ) || std::floor( value ) != value ||
                        value < setting.minimum || value > setting.maximum )
                    {
                        throw std::invalid_argument( String( "Invalid graphics setting: " ) +
                                                     setting.name );
                    }
                    next->setProperty( setting.name, static_cast<s32>( value ) );
                }
            }
            for( const auto &setting : booleans )
            {
                if( requested->hasProperty( setting.name ) )
                {
                    const auto value = requested->getPropertyObject( setting.name ).getValue();
                    if( value != "true" && value != "false" && value != "1" && value != "0" )
                    {
                        throw std::invalid_argument( String( "Invalid graphics setting: " ) +
                                                     setting.name );
                    }
                    next->setProperty( setting.name, requested->getPropertyAsBool( setting.name ) );
                }
            }
            if( requested->hasProperty( "graphics.renderScale" ) )
            {
                const auto scale = requested->getPropertyAsFloat( "graphics.renderScale" );
                if( !std::isfinite( scale ) || scale < 0.5f || scale > 1.5f )
                {
                    throw std::invalid_argument( "Render scale must be between 50% and 150%." );
                }
                next->setProperty( "graphics.renderScale", scale );
            }
            const auto aa = next->getPropertyAsInt( "graphics.antiAliasing" );
            if( aa > 2 && previous->hasProperty( "msaa_supported_mask" ) )
            {
                const auto samples = aa == 3 ? 2 : aa == 4 ? 4 : 8;
                if( !( previous->getPropertyAsInt( "msaa_supported_mask" ) & samples ) )
                {
                    throw std::invalid_argument(
                        "The graphics device does not support that MSAA sample count." );
                }
            }
            if( next->getPropertyAsInt( "quality.texture" ) !=
                previous->getPropertyAsInt( "quality.texture" ) )
            {
                const auto quality = next->getPropertyAsInt( "quality.texture" );
                constexpr s32 maximum[] = { 1024, 2048, 4096, 16384 };
                constexpr s32 anisotropy[] = { 1, 2, 8, 16 };
                if( !requested->hasProperty( "quality.maxTextureSize" ) )
                {
                    next->setProperty( "quality.maxTextureSize", maximum[quality] );
                }
                if( !requested->hasProperty( "graphics.textureFiltering" ) )
                {
                    next->setProperty( "graphics.textureFiltering", quality == 0 ? 1 : 2 );
                }
                if( !requested->hasProperty( "graphics.anisotropy" ) )
                {
                    next->setProperty( "graphics.anisotropy", anisotropy[quality] );
                }
            }
        }
        catch( const std::exception &error )
        {
            result->setProperty( "status", String( error.what() ) );
            return result;
        }

        const auto window = outputWindow( system );
        bool displayChanged = previous->getPropertyAsBool( "graphics.fullscreen" ) !=
                                  next->getPropertyAsBool( "graphics.fullscreen" ) ||
                              previous->getPropertyAsInt( "graphics.resolution" ) !=
                                  next->getPropertyAsInt( "graphics.resolution" ) ||
                              previous->getPropertyAsInt( "graphics.customWidth" ) !=
                                  next->getPropertyAsInt( "graphics.customWidth" ) ||
                              previous->getPropertyAsInt( "graphics.customHeight" ) !=
                                  next->getPropertyAsInt( "graphics.customHeight" );
        if( window && !app->isEditor() )
        {
            const bool fullscreen = next->getPropertyAsBool( "graphics.fullscreen" );
            displayChanged |= window->isFullScreen() != fullscreen;
            if( !fullscreen )
            {
                const auto resolution = next->getPropertyAsInt( "graphics.resolution" );
                constexpr s32 sizes[][2] = {
                    { 1920, 1080 }, { 1280, 720 }, { 2560, 1440 }, { 3840, 2160 }
                };
                const auto requestedSize =
                    resolution < 4 ? Vector2I( sizes[resolution][0], sizes[resolution][1] )
                                   : Vector2I( next->getPropertyAsInt( "graphics.customWidth" ),
                                               next->getPropertyAsInt( "graphics.customHeight" ) );
                if( resolution != 4 )
                {
                    displayChanged |= window->getSize() != requestedSize;
                }
            }
        }
        const bool displayPending = displayChanged && ( app->isEditor() || !window );
        const bool displayQueued = displayChanged && !displayPending && requiresPrimary( window );
        bool nativeAttempted = false;
        try
        {
            auto nextPlayer = copySettings( previousPlayer );
            copyRuntimeValues( *nextPlayer, *next );
            nativeAttempted = true;
            applyPipeline( system, next );
            if( displayChanged && !displayPending && !displayQueued )
            {
                applyDisplay( window, *next );
            }
            if( displayQueued )
            {
                auto queue = app->getJobQueue();
                if( !queue || !queue->isRunning() )
                {
                    throw std::runtime_error( "Display changes require a running primary job queue." );
                }
                SmartPtr<render::IGraphicsSystem> graphics( system );
                auto job = make_ptr<DisplayJob>( [graphics, window, next, previous]() {
                    const auto active = core::IApplicationManager::instancePtr();
                    if( !active || active->isEditor() || active->getGraphicsSystem() != graphics )
                    {
                        return;
                    }
                    ScopedLock displayLock( graphics );
                    const auto latest = active->getPlayerSettings();
                    // A later Apply or rollback supersedes this queued request.
                    if( !latest || !sameDisplayPolicy( *latest, *next ) )
                    {
                        return;
                    }
                    if( requiresPrimary( window ) )
                    {
                        WP_LOG_ERROR(
                            "Display change skipped: primary task does not own the native window." );
                        return;
                    }
                    try
                    {
                        applyDisplay( window, *next );
                    }
                    catch( const std::exception &error )
                    {
                        WP_LOG_EXCEPTION( error );
                        try
                        {
                            applyDisplay( window, *previous );
                        }
                        catch( ... )
                        {
                        }
                    }
                } );
                job->setPrimary( true );
                queue->addJob( job, TaskId::Primary );
            }
            app->setPlayerSettings( nextPlayer );
            copyRuntimeValues( *result, *next );
            result->setProperty( "success", true );
            result->setProperty( "displayPending", displayPending );
            result->setProperty( "displayQueued", displayQueued );
            result->setProperty(
                "status",
                String(
                    displayPending
                        ? "Graphics applied. Display mode changes take effect in the standalone game."
                    : displayQueued
                        ? "Graphics applied. Display mode change queued for the window thread."
                        : "Graphics settings applied." ) );
        }
        catch( const std::exception &error )
        {
            bool recovered = true;
            try
            {
                if( nativeAttempted )
                {
                    applyPipeline( system, previous );
                    if( displayChanged && !displayPending && !displayQueued )
                    {
                        applyDisplay( window, *previous );
                    }
                }
                app->setPlayerSettings( previousPlayer );
            }
            catch( ... )
            {
                recovered = false;
            }
            result->setProperty(
                "status", String( error.what() ) + ( recovered ? "; previous graphics settings retained."
                                                               : "; graphics recovery failed." ) );
        }
        return result;
    }

    SmartPtr<Properties> GraphicsSystemHelper::getDisplayModes( render::IGraphicsSystem *system )
    {
        auto result = make_ptr<Properties>();
        std::set<std::pair<s32, s32>> modes;
#if defined WP_PLATFORM_WIN32
        DEVMODE mode{};
        mode.dmSize = sizeof( mode );
        for( DWORD index = 0; EnumDisplaySettings( nullptr, index, &mode ); ++index )
        {
            if( mode.dmPelsWidth >= 320 && mode.dmPelsWidth <= 16384 && mode.dmPelsHeight >= 200 &&
                mode.dmPelsHeight <= 16384 )
            {
                modes.emplace( static_cast<s32>( mode.dmPelsWidth ),
                               static_cast<s32>( mode.dmPelsHeight ) );
            }
        }
#endif
        if( modes.empty() )
        {
            modes = { { 1280, 720 }, { 1600, 900 }, { 1920, 1080 }, { 2560, 1440 }, { 3840, 2160 } };
        }
        if( const auto window = outputWindow( system ) )
        {
            const auto size = window->getSize();
            if( size.x >= 320 && size.y >= 200 )
            {
                modes.emplace( size.x, size.y );
            }
        }
        s32 index = 0;
        for( const auto &mode : modes )
        {
            const auto prefix = String( "mode." ) + StringUtil::toString( index++ );
            result->setProperty( prefix + ".width", mode.first );
            result->setProperty( prefix + ".height", mode.second );
        }
        result->setProperty( "count", index );
        return result;
    }
}  // namespace workphone
