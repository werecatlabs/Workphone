#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiProfilerWindow.hpp>
#include <WPImGui/ImGuiProfileWindow.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <limits>
#include <unordered_map>

namespace workphone::ui::profiler_ui
{
    bool g_paused = false;
    int g_maxSamples = 1000;
    float g_warningMs = 4.0f;
    float g_criticalMs = 12.0f;
    bool g_showOnlyHot = false;
    bool g_showAdvancedProfilePanels = true;
    char g_filter[128] = {};

    bool containsNoCase( const char *text, const char *filter )
    {
        if( filter == nullptr || filter[0] == '\0' )
        {
            return true;
        }

        if( text == nullptr )
        {
            return false;
        }

        const auto textLength = std::strlen( text );
        const auto filterLength = std::strlen( filter );

        if( filterLength == 0 )
        {
            return true;
        }

        if( filterLength > textLength )
        {
            return false;
        }

        for( std::size_t i = 0; i <= textLength - filterLength; ++i )
        {
            bool matched = true;
            for( std::size_t j = 0; j < filterLength; ++j )
            {
                auto a = static_cast<unsigned char>( text[i + j] );
                auto b = static_cast<unsigned char>( filter[j] );

                if( std::tolower( a ) != std::tolower( b ) )
                {
                    matched = false;
                    break;
                }
            }

            if( matched )
            {
                return true;
            }
        }

        return false;
    }

    float safeFps( double seconds )
    {
        return seconds > 0.0 ? static_cast<float>( 1.0 / seconds ) : 0.0f;
    }

    float toMs( double seconds )
    {
        return static_cast<float>( seconds * 1000.0 );
    }
}  // namespace workphone::ui::profiler_ui

namespace workphone::ui
{
    namespace
    {
        struct ProfilerWindowState
        {
            bool showOverview = true;
            bool showTimeline = true;
            bool showBudgets = true;
            bool showCaptureTools = true;
            bool showSettings = true;

            bool captureArmed = false;
            bool freezeOnSpike = false;
            bool showDeltaTime = true;
            bool showWorkTime = true;
            bool showFps = true;
            bool compactRows = false;
            bool autoRefreshProfileList = true;

            int selectedSortMode = 1;  // 0 = name, 1 = work ms, 2 = frame ms
            int captureFrameCount = 300;
            float captureSpikeThresholdMs = 16.67f;

            char captureName[128] = "ProfilerCapture";
        };

        std::unordered_map<const ImGuiProfilerWindow *, ProfilerWindowState> g_windowStates;

        const char *getBudgetLabel( float workMs )
        {
            if( workMs >= profiler_ui::g_criticalMs )
            {
                return "Critical";
            }

            if( workMs >= profiler_ui::g_warningMs )
            {
                return "Warning";
            }

            return "OK";
        }

        void drawMetric( const char *label, const char *value )
        {
            ImGui::BeginGroup();
            ImGui::TextDisabled( "%s", label );
            ImGui::Text( "%s", value );
            ImGui::EndGroup();
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiProfilerWindow, ImGuiElement<IUIProfilerWindow> );

    ImGuiProfilerWindow::ImGuiProfilerWindow() = default;

    ImGuiProfilerWindow::~ImGuiProfilerWindow() = default;

    void ImGuiProfilerWindow::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        m_profiles.reserve( 64 );

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        if( auto profiler = applicationManager->getProfiler() )
        {
            auto profiles = profiler->getProfiles();
            for( auto profile : profiles )
            {
                auto p = factoryManager->make_ptr<ImGuiProfileWindow>();
                p->setProfile( profile );
                m_profiles.emplace_back( p );
            }
        }

        setLoadingState( LoadingState::Loaded );
    }

    void ImGuiProfilerWindow::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        m_profiles.clear();
        g_windowStates.erase( this );
        setLoadingState( LoadingState::Unloaded );
    }

    void ImGuiProfilerWindow::update()
    {
        auto applicationManager = core::IApplicationManager::instance();
        if( applicationManager == nullptr )
        {
            return;
        }

        auto &state = g_windowStates[this];

        profiler_ui::g_paused = profiler_ui::g_paused;
        profiler_ui::g_maxSamples = std::max( profiler_ui::g_maxSamples, 16 );
        profiler_ui::g_warningMs = std::max( profiler_ui::g_warningMs, 0.01f );
        profiler_ui::g_criticalMs = std::max( profiler_ui::g_criticalMs, profiler_ui::g_warningMs );

        auto rebuildProfiles = [&]()
        {
            m_profiles.clear();

            auto factoryManager = applicationManager->getFactoryManager();
            if( factoryManager == nullptr )
            {
                return;
            }

            if( auto profiler = applicationManager->getProfiler() )
            {
                auto profiles = profiler->getProfiles();
                for( auto profile : profiles )
                {
                    auto p = factoryManager->make_ptr<ImGuiProfileWindow>();
                    p->setProfile( profile );
                    m_profiles.emplace_back( p );
                }
            }
        };

        // Toolbar
        if( ImGui::Button( profiler_ui::g_paused ? "Resume" : "Pause" ) )
        {
            profiler_ui::g_paused = !profiler_ui::g_paused;
        }

        ImGui::SameLine();
        if( ImGui::Button( "Refresh Profiles" ) )
        {
            rebuildProfiles();
        }

        ImGui::SameLine();
        if( ImGui::Button( state.captureArmed ? "Stop Capture" : "Start Capture" ) )
        {
            state.captureArmed = !state.captureArmed;
        }

        ImGui::SameLine();
        if( ImGui::Button( "Mark Frame" ) )
        {
            // Hook point: push a named marker into your profiler/capture stream.
        }

        ImGui::SameLine();
        if( ImGui::Button( "Export CSV" ) )
        {
            // Hook point: export captured profile samples to disk.
        }

        ImGui::Separator();

        ImGui::InputText( "Filter", profiler_ui::g_filter, sizeof( profiler_ui::g_filter ) );
        ImGui::SameLine();
        ImGui::Checkbox( "Hot only", &profiler_ui::g_showOnlyHot );
        ImGui::SameLine();
        ImGui::Checkbox( "Paused", &profiler_ui::g_paused );
        ImGui::SameLine();
        ImGui::Checkbox( "Advanced panels", &profiler_ui::g_showAdvancedProfilePanels );

        ImGui::SliderInt( "Max Samples", &profiler_ui::g_maxSamples, 64, 20000 );
        ImGui::SliderFloat( "Warning ms", &profiler_ui::g_warningMs, 0.1f, 33.3f, "%.2f" );
        ImGui::SliderFloat( "Critical ms", &profiler_ui::g_criticalMs, 0.1f, 66.6f, "%.2f" );

        profiler_ui::g_maxSamples = std::max( profiler_ui::g_maxSamples, 16 );
        profiler_ui::g_warningMs = std::max( profiler_ui::g_warningMs, 0.01f );
        profiler_ui::g_criticalMs = std::max( profiler_ui::g_criticalMs, profiler_ui::g_warningMs );

        auto profiler = applicationManager->getProfiler();
        auto liveProfiles = profiler ? profiler->getProfiles() : decltype( profiler->getProfiles() )();

        float totalWorkMs = 0.0f;
        float worstWorkMs = 0.0f;
        float worstDeltaMs = 0.0f;
        int visibleProfiles = 0;
        int hotProfiles = 0;
        String worstLabel;

        for( auto profile : liveProfiles )
        {
            if( profile == nullptr )
            {
                continue;
            }

            const auto label = profile->getLabel();
            const auto workMs = profiler_ui::toMs( profile->getAverageTimeTaken() );
            const auto deltaMs = profiler_ui::toMs( profile->getAverageDeltaTime() );

            if( !profiler_ui::containsNoCase( label.c_str(), profiler_ui::g_filter ) )
            {
                continue;
            }

            if( profiler_ui::g_showOnlyHot && workMs < profiler_ui::g_warningMs )
            {
                continue;
            }

            ++visibleProfiles;
            totalWorkMs += workMs;

            if( workMs >= profiler_ui::g_warningMs )
            {
                ++hotProfiles;
            }

            if( workMs > worstWorkMs )
            {
                worstWorkMs = workMs;
                worstDeltaMs = deltaMs;
                worstLabel = label;
            }
        }

        char totalText[64];
        char worstText[64];
        char visibleText[64];
        char hotText[64];

        std::snprintf( totalText, sizeof( totalText ), "%.2f ms", totalWorkMs );
        std::snprintf( worstText, sizeof( worstText ), "%.2f ms", worstWorkMs );
        std::snprintf( visibleText, sizeof( visibleText ), "%d", visibleProfiles );
        std::snprintf( hotText, sizeof( hotText ), "%d", hotProfiles );

        drawMetric( "Visible profiles", visibleText );
        ImGui::SameLine( 160.0f );
        drawMetric( "Total work", totalText );
        ImGui::SameLine( 320.0f );
        drawMetric( "Worst profile", worstLabel.empty() ? "None" : worstLabel.c_str() );
        ImGui::SameLine( 520.0f );
        drawMetric( "Worst time", worstText );
        ImGui::SameLine( 680.0f );
        drawMetric( "Hot profiles", hotText );

        ImGui::Separator();

        if( ImGui::BeginTabBar( "ProfilerTabs" ) )
        {
            if( ImGui::BeginTabItem( "Overview" ) )
            {
                state.showOverview = true;

                ImGui::Checkbox( "Show delta time", &state.showDeltaTime );
                ImGui::SameLine();
                ImGui::Checkbox( "Show work time", &state.showWorkTime );
                ImGui::SameLine();
                ImGui::Checkbox( "Show FPS", &state.showFps );
                ImGui::SameLine();
                ImGui::Checkbox( "Compact rows", &state.compactRows );

                const char *sortModes[] = { "Name", "Work Time", "Delta Time" };
                ImGui::Combo( "Sort", &state.selectedSortMode, sortModes, 3 );

                if( ImGui::BeginTable( "ProfilerOverviewTable", 7,
                                       ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                           ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY,
                                       ImVec2( 0.0f, 360.0f ) ) )
                {
                    ImGui::TableSetupColumn( "Profile" );
                    ImGui::TableSetupColumn( "Delta ms" );
                    ImGui::TableSetupColumn( "Work ms" );
                    ImGui::TableSetupColumn( "FPS" );
                    ImGui::TableSetupColumn( "FPS Taken" );
                    ImGui::TableSetupColumn( "Budget" );
                    ImGui::TableSetupColumn( "Status" );
                    ImGui::TableHeadersRow();

                    for( auto profile : liveProfiles )
                    {
                        if( profile == nullptr )
                        {
                            continue;
                        }

                        const auto label = profile->getLabel();
                        const auto deltaTime = profile->getAverageDeltaTime();
                        const auto workTime = profile->getAverageTimeTaken();
                        const auto deltaMs = profiler_ui::toMs( deltaTime );
                        const auto workMs = profiler_ui::toMs( workTime );

                        if( !profiler_ui::containsNoCase( label.c_str(), profiler_ui::g_filter ) )
                        {
                            continue;
                        }

                        if( profiler_ui::g_showOnlyHot && workMs < profiler_ui::g_warningMs )
                        {
                            continue;
                        }

                        ImGui::TableNextRow( 0, state.compactRows ? 18.0f : 0.0f );
                        ImGui::TableSetColumnIndex( 0 );
                        ImGui::TextUnformatted( label.c_str() );

                        ImGui::TableSetColumnIndex( 1 );
                        ImGui::Text( "%.3f", deltaMs );

                        ImGui::TableSetColumnIndex( 2 );
                        ImGui::Text( "%.3f", workMs );

                        ImGui::TableSetColumnIndex( 3 );
                        ImGui::Text( "%.1f", profiler_ui::safeFps( deltaTime ) );

                        ImGui::TableSetColumnIndex( 4 );
                        ImGui::Text( "%.1f", profiler_ui::safeFps( workTime ) );

                        ImGui::TableSetColumnIndex( 5 );
                        const auto budgetRatio = profiler_ui::g_criticalMs > 0.0f
                                                     ? std::min( workMs / profiler_ui::g_criticalMs, 1.0f )
                                                     : 0.0f;
                        ImGui::ProgressBar( budgetRatio, ImVec2( -1.0f, 0.0f ) );

                        ImGui::TableSetColumnIndex( 6 );
                        ImGui::TextUnformatted( getBudgetLabel( workMs ) );
                    }

                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            if( ImGui::BeginTabItem( "Timeline" ) )
            {
                state.showTimeline = true;
                ImGui::TextDisabled( "Per-profile sample history. Use the filter and budget controls above to focus the view." );
                ImGui::BeginChild( "ProfilerTimelineChild", ImVec2( 0.0f, 0.0f ), true );

                for( auto profile : m_profiles )
                {
                    if( profile )
                    {
                        profile->update();
                    }
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if( ImGui::BeginTabItem( "Budgets" ) )
            {
                state.showBudgets = true;
                ImGui::Text( "Frame Budgets" );
                ImGui::SliderFloat( "CPU warning threshold", &profiler_ui::g_warningMs, 0.1f, 33.3f, "%.2f ms" );
                ImGui::SliderFloat( "CPU critical threshold", &profiler_ui::g_criticalMs, 0.1f, 66.6f, "%.2f ms" );
                ImGui::SliderFloat( "Capture spike threshold", &state.captureSpikeThresholdMs, 0.1f, 100.0f,
                                    "%.2f ms" );
                ImGui::Checkbox( "Freeze capture on spike", &state.freezeOnSpike );

                ImGui::Separator();
                ImGui::Text( "Suggested default budgets" );
                ImGui::BulletText( "60 FPS frame: 16.67 ms" );
                ImGui::BulletText( "120 FPS frame: 8.33 ms" );
                ImGui::BulletText( "CPU gameplay/render budget often needs to be below the full frame budget." );

                ImGui::EndTabItem();
            }

            if( ImGui::BeginTabItem( "Capture" ) )
            {
                state.showCaptureTools = true;
                ImGui::InputText( "Capture name", state.captureName, sizeof( state.captureName ) );
                ImGui::SliderInt( "Capture frames", &state.captureFrameCount, 1, 10000 );
                ImGui::Checkbox( "Capture armed", &state.captureArmed );
                ImGui::Checkbox( "Auto refresh profile list", &state.autoRefreshProfileList );

                if( ImGui::Button( "Capture Current Frame" ) )
                {
                    // Hook point: snapshot the current profile tree.
                }

                ImGui::SameLine();
                if( ImGui::Button( "Capture Range" ) )
                {
                    state.captureArmed = true;
                }

                ImGui::SameLine();
                if( ImGui::Button( "Clear Capture" ) )
                {
                    state.captureArmed = false;
                }

                ImGui::Separator();
                ImGui::TextDisabled( "Hook these buttons into your profiler recorder/exporter when available." );

                ImGui::EndTabItem();
            }

            if( ImGui::BeginTabItem( "Settings" ) )
            {
                state.showSettings = true;
                ImGui::Checkbox( "Pause sampling", &profiler_ui::g_paused );
                ImGui::Checkbox( "Show only profiles over warning threshold", &profiler_ui::g_showOnlyHot );
                ImGui::Checkbox( "Show advanced profile panels", &profiler_ui::g_showAdvancedProfilePanels );
                ImGui::SliderInt( "History samples", &profiler_ui::g_maxSamples, 64, 20000 );

                ImGui::Separator();
                ImGui::Text( "Profiler UI" );
                ImGui::Checkbox( "Overview tab", &state.showOverview );
                ImGui::Checkbox( "Timeline tab", &state.showTimeline );
                ImGui::Checkbox( "Budgets tab", &state.showBudgets );
                ImGui::Checkbox( "Capture tab", &state.showCaptureTools );

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        if( state.captureArmed )
        {
            ImGui::Separator();
            ImGui::Text( "Capture armed: %s, %d frames, spike threshold %.2f ms",
                         state.captureName,
                         state.captureFrameCount,
                         state.captureSpikeThresholdMs );
        }
    }

    SmartPtr<IUIProfileWindow> ImGuiProfilerWindow::addProfile()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto profile = factoryManager->make_ptr<ImGuiProfileWindow>();
        m_profiles.emplace_back( profile );
        return profile;
    }

    void ImGuiProfilerWindow::removeProfile( SmartPtr<IUIProfileWindow> profile )
    {
        auto it = std::find( m_profiles.begin(), m_profiles.end(), profile );
        if( it != m_profiles.end() )
        {
            m_profiles.erase( it );
        }
    }

    Array<SmartPtr<IUIProfileWindow>> ImGuiProfilerWindow::getProfiles() const
    {
        return m_profiles;
    }

    void ImGuiProfilerWindow::setProfiles( const Array<SmartPtr<IUIProfileWindow>> &profiles )
    {
        m_profiles = profiles;
    }
}  // namespace workphone::ui
