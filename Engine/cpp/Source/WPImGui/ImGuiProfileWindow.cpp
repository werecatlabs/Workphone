#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiProfileWindow.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <numeric>
#include <unordered_map>
#include <vector>

namespace workphone::ui::profiler_ui
{
    extern bool g_paused;
    extern int g_maxSamples;
    extern float g_warningMs;
    extern float g_criticalMs;
    extern bool g_showOnlyHot;
    extern bool g_showAdvancedProfilePanels;
    extern char g_filter[128];

    bool containsNoCase( const char *text, const char *filter );

    float safeFps( double seconds );
    float toMs( double seconds );
}  // namespace workphone::ui::profiler_ui

namespace workphone::ui
{
    namespace
    {
        struct ProfileStats
        {
            float minValue = 0.0f;
            float maxValue = 0.0f;
            float average = 0.0f;
            float p95 = 0.0f;
            int sampleCount = 0;
        };

        struct ProfileWindowState
        {
            std::vector<float> fpsHistory;
            std::vector<float> deltaMsHistory;
            std::vector<float> workRatioHistory;

            bool showFrameTime = true;
            bool showWorkTime = true;
            bool showFps = true;
            bool showHistogram = true;
            bool showStats = true;
            bool showBudget = true;
            bool showCaptureMarkers = false;
            bool showRawValues = false;
            bool pinOpen = false;
            bool localPaused = false;

            float graphHeight = 90.0f;
            float localWarningMs = 4.0f;
            float localCriticalMs = 12.0f;
            char note[128] = "";
        };

        std::unordered_map<const ImGuiProfileWindow *, ProfileWindowState> g_profileStates;

        template <class T>
        void trimSamples( T &container, int maxSamples )
        {
            if( maxSamples <= 0 )
            {
                return;
            }

            while( static_cast<int>( container.size() ) > maxSamples )
            {
                container.erase( container.begin() );
            }
        }

        ProfileStats calculateStats( const Array<float> &values )
        {
            ProfileStats stats;
            stats.sampleCount = static_cast<int>( values.size() );

            if( values.empty() )
            {
                return stats;
            }

            stats.minValue = std::numeric_limits<float>::max();
            stats.maxValue = std::numeric_limits<float>::lowest();
            stats.average = 0.0f;

            std::vector<float> sorted;
            sorted.reserve( values.size() );

            for( auto value : values )
            {
                stats.minValue = std::min( stats.minValue, value );
                stats.maxValue = std::max( stats.maxValue, value );
                stats.average += value;
                sorted.emplace_back( value );
            }

            stats.average /= static_cast<float>( values.size() );

            std::sort( sorted.begin(), sorted.end() );
            const auto p95Index = static_cast<std::size_t>(
                std::min<int>( static_cast<int>( sorted.size() ) - 1,
                               static_cast<int>( std::floor( sorted.size() * 0.95f ) ) ) );
            stats.p95 = sorted[p95Index];

            return stats;
        }

        void drawStatsTable( const ProfileStats &workStats,
                             const ProfileStats &deltaStats,
                             float currentWorkMs,
                             float currentDeltaMs,
                             float fps,
                             float fpsTaken )
        {
            if( ImGui::BeginTable( "ProfileStatsTable", 2,
                                   ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                       ImGuiTableFlags_Resizable ) )
            {
                auto row = []( const char *name, const char *value )
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex( 0 );
                    ImGui::TextUnformatted( name );
                    ImGui::TableSetColumnIndex( 1 );
                    ImGui::TextUnformatted( value );
                };

                char buffer[128];

                std::snprintf( buffer, sizeof( buffer ), "%.3f ms", currentDeltaMs );
                row( "Current delta", buffer );

                std::snprintf( buffer, sizeof( buffer ), "%.3f ms", currentWorkMs );
                row( "Current work", buffer );

                std::snprintf( buffer, sizeof( buffer ), "%.1f", fps );
                row( "FPS", buffer );

                std::snprintf( buffer, sizeof( buffer ), "%.1f", fpsTaken );
                row( "FPS taken", buffer );

                std::snprintf( buffer, sizeof( buffer ), "%d", workStats.sampleCount );
                row( "Samples", buffer );

                std::snprintf( buffer, sizeof( buffer ), "%.3f / %.3f / %.3f ms",
                               workStats.minValue,
                               workStats.average,
                               workStats.maxValue );
                row( "Work min / avg / max", buffer );

                std::snprintf( buffer, sizeof( buffer ), "%.3f ms", workStats.p95 );
                row( "Work p95", buffer );

                std::snprintf( buffer, sizeof( buffer ), "%.3f / %.3f / %.3f ms",
                               deltaStats.minValue,
                               deltaStats.average,
                               deltaStats.maxValue );
                row( "Delta min / avg / max", buffer );

                std::snprintf( buffer, sizeof( buffer ), "%.3f ms", deltaStats.p95 );
                row( "Delta p95", buffer );

                ImGui::EndTable();
            }
        }

        void drawHistoryPlot( const char *label,
                              const float *data,
                              int count,
                              float graphHeight,
                              const char *overlay = nullptr )
        {
            if( data == nullptr || count <= 0 )
            {
                ImGui::TextDisabled( "%s: no samples", label );
                return;
            }

            float minValue = std::numeric_limits<float>::max();
            float maxValue = std::numeric_limits<float>::lowest();

            for( int i = 0; i < count; ++i )
            {
                minValue = std::min( minValue, data[i] );
                maxValue = std::max( maxValue, data[i] );
            }

            if( minValue == maxValue )
            {
                maxValue += 1.0f;
            }

            const auto padding = std::max( ( maxValue - minValue ) * 0.1f, 0.001f );
            minValue -= padding;
            maxValue += padding;

            ImGui::PlotLines( label, data, count, 0, overlay, minValue, maxValue,
                              ImVec2( 0.0f, graphHeight ) );
        }

        const char *statusLabel( float workMs )
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
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiProfileWindow, ImGuiElement<IUIProfileWindow> );

    ImGuiProfileWindow::ImGuiProfileWindow() = default;

    ImGuiProfileWindow::~ImGuiProfileWindow() = default;

    void ImGuiProfileWindow::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        m_data.reserve( 1000 );
        setLoadingState( LoadingState::Loaded );
    }

    void ImGuiProfileWindow::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        m_data.clear();
        g_profileStates.erase( this );
        setLoadingState( LoadingState::Unloaded );
    }

    void ImGuiProfileWindow::update()
    {
        auto applicationManager = core::IApplicationManager::instance();
        if( applicationManager == nullptr )
        {
            return;
        }

        auto &state = g_profileStates[this];

        auto p = getProfile();
        if( p == nullptr )
        {
            ImGui::TextDisabled( "No profile bound." );
            return;
        }

        const auto label = p->getLabel();
        if( !profiler_ui::containsNoCase( label.c_str(), profiler_ui::g_filter ) )
        {
            return;
        }

        const auto averageTimeTaken = p->getAverageTimeTaken();
        const auto averageDeltaTime = p->getAverageDeltaTime();

        const auto workMs = profiler_ui::toMs( averageTimeTaken );
        const auto deltaMs = profiler_ui::toMs( averageDeltaTime );
        const auto fps = profiler_ui::safeFps( averageDeltaTime );
        const auto fpsTaken = profiler_ui::safeFps( averageTimeTaken );

        if( profiler_ui::g_showOnlyHot && workMs < profiler_ui::g_warningMs )
        {
            return;
        }

        const auto paused = profiler_ui::g_paused || state.localPaused;
        const auto maxSamples = std::max( profiler_ui::g_maxSamples, 16 );

        if( !paused )
        {
            m_data.push_back( workMs );
            state.deltaMsHistory.emplace_back( deltaMs );
            state.fpsHistory.emplace_back( fps );
            state.workRatioHistory.emplace_back(
                profiler_ui::g_criticalMs > 0.0f ? std::min( workMs / profiler_ui::g_criticalMs, 1.0f ) : 0.0f );

            trimSamples( m_data, maxSamples );
            trimSamples( state.deltaMsHistory, maxSamples );
            trimSamples( state.fpsHistory, maxSamples );
            trimSamples( state.workRatioHistory, maxSamples );
        }

        ImGui::PushID( this );

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
        if( state.pinOpen )
        {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        if( ImGui::CollapsingHeader( label.c_str(), flags ) )
        {
            ImGui::Text( "Status: %s", statusLabel( workMs ) );
            ImGui::SameLine();
            ImGui::TextDisabled( paused ? "(paused)" : "(live)" );

            ImGui::Columns( 4, nullptr, false );
            ImGui::TextDisabled( "Delta" );
            ImGui::Text( "%.3f ms", deltaMs );
            ImGui::NextColumn();

            ImGui::TextDisabled( "Work" );
            ImGui::Text( "%.3f ms", workMs );
            ImGui::NextColumn();

            ImGui::TextDisabled( "FPS" );
            ImGui::Text( "%.1f", fps );
            ImGui::NextColumn();

            ImGui::TextDisabled( "FPS Taken" );
            ImGui::Text( "%.1f", fpsTaken );
            ImGui::NextColumn();
            ImGui::Columns( 1 );

            if( state.showBudget )
            {
                const auto ratio = profiler_ui::g_criticalMs > 0.0f
                                       ? std::min( workMs / profiler_ui::g_criticalMs, 1.0f )
                                       : 0.0f;

                char budgetText[96];
                std::snprintf( budgetText, sizeof( budgetText ), "%.2f / %.2f ms", workMs,
                               profiler_ui::g_criticalMs );
                ImGui::ProgressBar( ratio, ImVec2( -1.0f, 0.0f ), budgetText );
            }

            if( profiler_ui::g_showAdvancedProfilePanels )
            {
                if( ImGui::TreeNode( "Display" ) )
                {
                    ImGui::Checkbox( "Frame time", &state.showFrameTime );
                    ImGui::SameLine();
                    ImGui::Checkbox( "Work time", &state.showWorkTime );
                    ImGui::SameLine();
                    ImGui::Checkbox( "FPS", &state.showFps );
                    ImGui::SameLine();
                    ImGui::Checkbox( "Histogram", &state.showHistogram );

                    ImGui::Checkbox( "Stats", &state.showStats );
                    ImGui::SameLine();
                    ImGui::Checkbox( "Budget", &state.showBudget );
                    ImGui::SameLine();
                    ImGui::Checkbox( "Raw values", &state.showRawValues );
                    ImGui::SameLine();
                    ImGui::Checkbox( "Pause this profile", &state.localPaused );

                    ImGui::SliderFloat( "Graph height", &state.graphHeight, 40.0f, 240.0f, "%.0f" );
                    ImGui::InputText( "Note", state.note, sizeof( state.note ) );

                    ImGui::TreePop();
                }
            }

            auto data = Array<f32>( m_data.begin(), m_data.end() );
            const auto workStats = calculateStats( data );

            Array<float> deltaArray;
            deltaArray.reserve( state.deltaMsHistory.size() );
            for( auto value : state.deltaMsHistory )
            {
                deltaArray.push_back( value );
            }

            const auto deltaStats = calculateStats( deltaArray );

            if( state.showFrameTime )
            {
                drawHistoryPlot( "Frame Delta ms",
                                 state.deltaMsHistory.empty() ? nullptr : state.deltaMsHistory.data(),
                                 static_cast<int>( state.deltaMsHistory.size() ),
                                 state.graphHeight );
            }

            if( state.showWorkTime )
            {
                drawHistoryPlot( "Work ms",
                                 m_data.empty() ? nullptr : m_data.data(),
                                 static_cast<int>( m_data.size() ),
                                 state.graphHeight );
            }

            if( state.showFps )
            {
                drawHistoryPlot( "FPS",
                                 state.fpsHistory.empty() ? nullptr : state.fpsHistory.data(),
                                 static_cast<int>( state.fpsHistory.size() ),
                                 state.graphHeight );
            }

            if( state.showHistogram )
            {
                if( !m_data.empty() )
                {
                    ImGui::PlotHistogram( "Work Histogram",
                                          m_data.data(),
                                          static_cast<int>( m_data.size() ),
                                          0,
                                          nullptr,
                                          workStats.minValue,
                                          std::max( workStats.maxValue, workStats.minValue + 0.001f ),
                                          ImVec2( 0.0f, state.graphHeight ) );
                }
                else
                {
                    ImGui::TextDisabled( "Work Histogram: no samples" );
                }
            }

            if( state.showStats )
            {
                drawStatsTable( workStats, deltaStats, workMs, deltaMs, fps, fpsTaken );
            }

            if( state.showRawValues )
            {
                ImGui::Text( "Average delta seconds: %.8f", averageDeltaTime );
                ImGui::Text( "Average work seconds: %.8f", averageTimeTaken );
                ImGui::Text( "Samples stored: %d", static_cast<int>( m_data.size() ) );
            }

            if( ImGui::Button( "Clear Samples" ) )
            {
                m_data.clear();
                state.deltaMsHistory.clear();
                state.fpsHistory.clear();
                state.workRatioHistory.clear();
            }

            ImGui::SameLine();
            if( ImGui::Button( "Pin Open" ) )
            {
                state.pinOpen = !state.pinOpen;
            }

            ImGui::Separator();
        }

        ImGui::PopID();
    }

    SmartPtr<IProfile> ImGuiProfileWindow::getProfile() const
    {
        return m_profile;
    }

    void ImGuiProfileWindow::setProfile( SmartPtr<IProfile> profile )
    {
        m_profile = profile;
    }
}  // namespace workphone::ui
