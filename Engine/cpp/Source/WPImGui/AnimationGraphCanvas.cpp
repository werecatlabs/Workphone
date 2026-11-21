#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiAnimationGraph.hpp>
#include <Workphone/Core/StringUtil.hpp>

#if WP_BUILD_IMGUI
#    include <imgui.h>
#    include <algorithm>
#    include <cmath>
#    include <map>
#    include <vector>

namespace workphone::ui
{
    namespace animation_graph_canvas
    {
        constexpr f32 NodeWidth = 186.0f;
        constexpr f32 NodeHeight = 78.0f;
        constexpr f32 NodeSpacingX = 254.0f;
        constexpr f32 NodeSpacingY = 134.0f;
        constexpr ImU32 Accent = IM_COL32( 100, 182, 249, 255 );
        constexpr ImU32 Entry = IM_COL32( 237, 177, 90, 255 );
        constexpr ImU32 Active = IM_COL32( 109, 215, 163, 255 );

        Vector2F positionOf( const animation::AnimationGraphState &state, size_t index )
        {
            if( state.hasEditorPosition )
            {
                return { state.editorX, state.editorY };
            }
            return { NodeSpacingX * static_cast<f32>( index % 3 ),
                     NodeSpacingY * static_cast<f32>( index / 3 ) };
        }

        ImVec2 native( const Vector2F &value )
        {
            return { value.X(), value.Y() };
        }

        Vector2F local( const ImVec2 &value )
        {
            return { value.x, value.y };
        }

        f32 squareLength( Vector2F value )
        {
            return value.X() * value.X() + value.Y() * value.Y();
        }

        struct Node
        {
            const animation::AnimationGraphState *state;
            Vector2F min;
            Vector2F max;

            Vector2F input() const { return { min.X(), ( min.Y() + max.Y() ) * 0.5f }; }
            Vector2F output() const { return { max.X(), ( min.Y() + max.Y() ) * 0.5f }; }

            bool contains( const Vector2F &point ) const
            {
                return point.X() >= min.X() && point.X() <= max.X() && point.Y() >= min.Y() &&
                       point.Y() <= max.Y();
            }
        };

        struct Edge
        {
            Vector2F from;
            Vector2F out;
            Vector2F in;
            Vector2F to;

            Vector2F pointAt( f32 t ) const
            {
                const auto u = 1.0f - t;
                return from * ( u * u * u ) + out * ( 3.0f * u * u * t ) +
                       in * ( 3.0f * u * t * t ) + to * ( t * t * t );
            }

            f32 distanceTo( Vector2F mouse ) const
            {
                auto previous = from;
                auto best = 100.0f;
                for( int step = 1; step <= 24; ++step )
                {
                    const auto point = pointAt( step / 24.0f );
                    const auto line = point - previous;
                    const auto delta = mouse - previous;
                    const auto length = squareLength( line );
                    const auto along = length > 0.0f
                                           ? std::clamp( ( delta.X() * line.X() + delta.Y() * line.Y() ) /
                                                             length,
                                                         0.0f, 1.0f )
                                           : 0.0f;
                    best = std::min( best, squareLength( mouse - ( previous + line * along ) ) );
                    previous = point;
                }
                return best;
            }

            void draw( ImDrawList &list, ImU32 color, f32 thickness, bool arrow = true ) const
            {
                list.AddBezierCubic( native( from ), native( out ), native( in ), native( to ), color,
                                     thickness );
                if( !arrow )
                {
                    return;
                }
                const auto tip = pointAt( 0.56f );
                auto direction = tip - pointAt( 0.51f );
                const auto length = std::sqrt( squareLength( direction ) );
                if( length > 0.0f )
                {
                    direction *= 1.0f / length;
                    const Vector2F perpendicular( -direction.Y(), direction.X() );
                    const auto base = tip - direction * 10.0f;
                    list.AddTriangleFilled( native( tip + direction * 3.0f ),
                                            native( base + perpendicular * 5.0f ),
                                            native( base - perpendicular * 5.0f ), color );
                }
            }
        };

        Edge makeEdge( Vector2F from, Vector2F to, f32 zoom, int lane )
        {
            const auto bend = std::max( 44.0f * zoom, std::abs( to.X() - from.X() ) * 0.38f );
            const auto offset = to.X() > from.X() + 20.0f * zoom
                                    ? lane * 36.0f * zoom
                                    : -( 84.0f + lane * 38.0f ) * zoom;
            return { from, { from.X() + bend, from.Y() + offset },
                     { to.X() - bend, to.Y() + offset }, to };
        }

        void cardText( ImDrawList &list, const String &text, const Node &node, f32 zoom,
                       f32 vertical, ImU32 color )
        {
            const ImVec4 clip( node.min.X() + 11.0f * zoom, node.min.Y(),
                               node.max.X() - 11.0f * zoom, node.max.Y() );
            const auto fontSize = ImGui::GetFontSize() * zoom;
            list.AddText( ImGui::GetFont(), fontSize,
                          { clip.x, node.min.Y() + vertical * zoom }, color, text.c_str(), nullptr,
                          0.0f, &clip );
        }
    }  // namespace animation_graph_canvas

    void ImGuiAnimationGraph::draw()
    {
        const std::lock_guard guard( m_mutex );
        if( !m_window )
        {
            return;
        }

        drawToolbar();
        ImGui::Separator();

        if( !m_animator )
        {
            ImGui::TextDisabled( "Select an actor with an Animator component." );
            return;
        }

        const auto available = ImGui::GetContentRegionAvail();
        const bool sideBySide = available.x >= 680.0f;
        const auto inspectorWidth = sideBySide ? std::clamp( available.x * 0.33f, 300.0f, 380.0f )
                                               : available.x;
        const auto canvasWidth =
            sideBySide ? available.x - inspectorWidth - ImGui::GetStyle().ItemSpacing.x : available.x;
        const auto canvasHeight = sideBySide ? std::max( available.y, 180.0f )
                                            : std::clamp( available.y * 0.55f, 180.0f, 330.0f );
        ImGui::BeginDisabled( m_action == Action::Apply || m_action == Action::Revert );
        drawCanvas( canvasWidth, canvasHeight );
        if( sideBySide )
        {
            ImGui::SameLine();
        }
        drawInspector( inspectorWidth, sideBySide ? canvasHeight : 320.0f );
        ImGui::EndDisabled();
    }

    void ImGuiAnimationGraph::drawToolbar()
    {
        if( !m_targetName.empty() )
        {
            ImGui::TextUnformatted( m_targetName.c_str() );
            if( m_dirty )
            {
                ImGui::SameLine();
                ImGui::TextColored( { 0.93f, 0.69f, 0.35f, 1.0f }, "Unapplied edits" );
            }
        }

        ImGui::BeginDisabled( !m_animator || m_action != Action::None );
        ImGui::BeginDisabled( !m_dirty || ( m_edited.getStates().empty() && !m_playback.available ) );
        if( ImGui::Button( "Apply" ) )
        {
            m_action = Action::Apply;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if( ImGui::Button( m_dirty ? "Revert" : "Reload" ) )
        {
            if( m_dirty )
            {
                ImGui::OpenPopup( "Revert graph changes?" );
            }
            else
            {
                m_action = Action::Revert;
            }
        }
        if( ImGui::BeginPopupModal( "Revert graph changes?", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
        {
            ImGui::TextUnformatted( "Discard all unapplied changes to this animation graph?" );
            if( ImGui::Button( "Discard changes" ) )
            {
                m_action = Action::Revert;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if( ImGui::Button( "Keep editing" ) )
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        ImGui::BeginDisabled( !m_playback.available || m_playback.playing );
        if( ImGui::Button( "Play" ) )
        {
            m_action = Action::Play;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled( !m_playback.playing );
        if( ImGui::Button( "Pause" ) )
        {
            m_action = Action::Pause;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled( !( m_playback.playing || m_playback.paused ) );
        if( ImGui::Button( "Stop" ) )
        {
            m_action = Action::Stop;
        }
        ImGui::EndDisabled();
        ImGui::EndDisabled();

        if( !m_playback.state.empty() && ( m_playback.playing || m_playback.paused ) )
        {
            if( ImGui::GetContentRegionAvail().x > 720.0f )
            {
                ImGui::SameLine( 0.0f, 18.0f );
            }
            const auto mode = m_playback.paused ? "Paused" : "Preview";
            if( m_playback.destination.empty() )
            {
                ImGui::Text( "%s  %s", mode, m_playback.state.c_str() );
            }
            else
            {
                ImGui::Text( "%s  %s > %s   %.0f%%", mode, m_playback.state.c_str(),
                             m_playback.destination.c_str(), m_playback.progress * 100.0f );
            }
        }

        if( m_selectionChanged && m_dirty )
        {
            ImGui::TextWrapped( "Selection changed. Apply or revert this graph to switch to the new selection." );
        }
        if( !m_status.empty() )
        {
            ImGui::TextWrapped( "%s", m_status.c_str() );
        }
    }

    void ImGuiAnimationGraph::fitCanvas( Vector2F size )
    {
        using namespace animation_graph_canvas;
        m_fitOnDraw = false;
        const auto &states = m_edited.getStates();
        if( states.empty() )
        {
            m_camera = { 32.0f, 36.0f };
            m_zoom = 1.0f;
            return;
        }

        auto low = positionOf( states.front(), 0 );
        auto high = low;
        for( size_t index = 1; index < states.size(); ++index )
        {
            const auto point = positionOf( states[index], index );
            low = { std::min( low.X(), point.X() ), std::min( low.Y(), point.Y() ) };
            high = { std::max( high.X(), point.X() ), std::max( high.Y(), point.Y() ) };
        }
        const auto content = high - low + Vector2F( NodeWidth, NodeHeight );
        m_zoom = std::clamp( std::min( ( size.X() - 60.0f ) / content.X(),
                                      ( size.Y() - 90.0f ) / content.Y() ),
                             0.28f, 1.0f );
        m_camera = ( size - content * m_zoom ) * 0.5f - low * m_zoom;
    }

    void ImGuiAnimationGraph::drawCanvas( f32 width, f32 height )
    {
        using namespace animation_graph_canvas;
        const auto visible = ImGui::BeginChild( "##AnimationStateCanvas", { width, height }, true,
                                               ImGuiWindowFlags_NoScrollbar |
                                                   ImGuiWindowFlags_NoScrollWithMouse );
        if( !visible )
        {
            ImGui::EndChild();
            return;
        }
        const auto canvasWidth = ImGui::GetContentRegionAvail().x;
        ImGui::BeginDisabled( m_availableClips.empty() && m_edited.getPoseNodes().empty() );
        if( ImGui::Button( "Add State" ) )
        {
            // Use the first vacant cell within the graph's ordinary layout.
            for( size_t i = 0; i <= m_edited.getStates().size(); ++i )
            {
                const Vector2F slot( NodeSpacingX * static_cast<f32>( i % 3 ),
                                     NodeSpacingY * static_cast<f32>( i / 3 ) );
                bool occupied = false;
                const auto &states = m_edited.getStates();
                for( size_t index = 0; index < states.size(); ++index )
                {
                    const auto delta = positionOf( states[index], index ) - slot;
                    occupied = occupied ||
                               ( std::abs( delta.X() ) < NodeWidth && std::abs( delta.Y() ) < NodeHeight );
                }
                m_addPosition = slot;
                if( !occupied )
                {
                    break;
                }
            }
            ImGui::OpenPopup( "Add state from motion" );
        }
        ImGui::EndDisabled();
        if( ImGui::BeginPopup( "Add state from motion" ) )
        {
            drawAddStateMenu( m_addPosition );
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        if( ImGui::Button( "Frame All" ) )
        {
            m_fitOnDraw = true;
        }
        if( canvasWidth >= 480.0f )
        {
            ImGui::SameLine( 0.0f, 16.0f );
            ImGui::TextDisabled( "Middle-drag to pan | Scroll to zoom" );
        }
        else if( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "Middle-drag to pan. Scroll to zoom." );
        }

        const auto origin = local( ImGui::GetCursorScreenPos() );
        const auto space = ImGui::GetContentRegionAvail();
        const auto size = Vector2F( std::max( space.x, 1.0f ), std::max( space.y, 1.0f ) );
        ImGui::InvisibleButton( "##StateGraphSurface", native( size ),
                                ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle |
                                    ImGuiButtonFlags_MouseButtonRight );
        const auto hovered = ImGui::IsItemHovered();
        const auto &io = ImGui::GetIO();
        const auto mouse = local( io.MousePos );
        if( m_fitOnDraw )
        {
            fitCanvas( size );
        }
        if( hovered && io.MouseWheel != 0.0f )
        {
            const auto before = m_zoom;
            m_zoom = std::clamp( before * std::pow( 1.13f, io.MouseWheel ), 0.28f, 2.4f );
            m_camera = mouse - origin - ( mouse - origin - m_camera ) * ( m_zoom / before );
        }
        if( hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Middle ) )
        {
            m_panning = true;
        }
        if( m_panning && ImGui::IsMouseDown( ImGuiMouseButton_Middle ) )
        {
            m_camera += local( io.MouseDelta );
        }
        else
        {
            m_panning = false;
        }

        auto &list = *ImGui::GetWindowDrawList();
        const auto end = origin + size;
        list.PushClipRect( native( origin ), native( end ), true );
        list.AddRectFilled( native( origin ), native( end ), IM_COL32( 20, 27, 37, 255 ), 5.0f );
        const auto grid = 28.0f * m_zoom;
        auto gridOrigin = [&]( f32 pan ) { return std::fmod( std::fmod( pan, grid ) + grid, grid ); };
        for( f32 y = origin.Y() + gridOrigin( m_camera.Y() ); y < end.Y(); y += grid )
        {
            for( f32 x = origin.X() + gridOrigin( m_camera.X() ); x < end.X(); x += grid )
            {
                list.AddRectFilled( { x, y }, { x + 1.0f, y + 1.0f },
                                    IM_COL32( 70, 88, 113, 147 ) );
            }
        }

        const auto start = origin + m_camera;
        const auto &states = m_edited.getStates();
        std::vector<Node> nodes;
        nodes.reserve( states.size() );
        for( size_t index = 0; index < states.size(); ++index )
        {
            const auto &state = states[index];
            const auto top = start + positionOf( state, index ) * m_zoom;
            nodes.push_back( { &state, top, top + Vector2F( NodeWidth, NodeHeight ) * m_zoom } );
        }
        const auto body = std::find_if( nodes.rbegin(), nodes.rend(),
                                        [&]( const Node &node ) { return node.contains( mouse ); } );
        const auto portRadius = std::max( 8.0f, 10.0f * m_zoom );
        const auto port = std::find_if( nodes.rbegin(), nodes.rend(), [&]( const Node &node ) {
            return squareLength( node.output() - mouse ) <= portRadius * portRadius;
        } );
        const auto findNode = [&]( const String &id ) -> const Node * {
            const auto it = std::find_if( nodes.begin(), nodes.end(),
                                          [&]( const Node &node ) { return node.state->id == id; } );
            return it != nodes.end() ? &*it : nullptr;
        };

        const auto &transitions = m_edited.getTransitions();
        std::map<std::pair<const Node *, const Node *>, int> lanes;
        std::optional<size_t> hoveredEdge;
        auto closest = 81.0f;
        std::optional<Edge> highlightedEdge;
        const bool previewing = m_playback.playing || m_playback.paused;
        for( size_t i = 0; i < transitions.size(); ++i )
        {
            const auto &transition = transitions[i];
            const auto *from = findNode( transition.fromState );
            const auto *to = findNode( transition.toState );
            if( !from || !to )
            {
                continue;
            }
            const auto lane = lanes[{ from, to }]++;
            const auto edge = makeEdge( from->output(), to->input(), m_zoom, lane );
            const auto active = previewing && m_playback.state == transition.fromState &&
                                m_playback.destination == transition.toState;
            if( m_selectedTransition == i )
            {
                highlightedEdge = edge;
            }
            else
            {
                edge.draw( list, active ? Active : IM_COL32( 124, 148, 175, 220 ),
                           active ? 3.0f : 2.0f );
            }
            if( hovered && body == nodes.rend() )
            {
                const auto distance = edge.distanceTo( mouse );
                if( distance < closest )
                {
                    closest = distance;
                    hoveredEdge = i;
                }
            }
        }
        if( highlightedEdge )
        {
            highlightedEdge->draw( list, Accent, 3.0f );
        }
        if( const auto *source = findNode( m_linkSource ); source && ImGui::IsMousePosValid() )
        {
            const auto target = body != nodes.rend() && body->state->id != m_linkSource
                                    ? body->input()
                                    : mouse;
            makeEdge( source->output(), target, m_zoom, 0 ).draw( list, Accent, 2.0f, false );
        }

        for( const auto &node : nodes )
        {
            const auto &state = *node.state;
            const bool entry = state.id == m_edited.getInitialState();
            const bool active = previewing &&
                                ( state.id == m_playback.state || state.id == m_playback.destination );
            const bool selected = state.id == m_selectedState || state.id == m_linkSource;
            const auto radius = std::max( 3.0f, 7.0f * m_zoom );
            const auto color = active ? Active : entry ? Entry : Accent;
            const auto headerEnd = Vector2F( node.max.X(), node.min.Y() + 29.0f * m_zoom );
            list.AddRectFilled( native( node.min + Vector2F( 2.0f, 3.0f ) ),
                                native( node.max + Vector2F( 2.0f, 3.0f ) ),
                                IM_COL32( 0, 0, 0, 75 ), radius );
            list.AddRectFilled( native( node.min ), native( node.max ), IM_COL32( 36, 49, 66, 255 ),
                                radius );
            list.AddRectFilled( native( node.min ), native( headerEnd ),
                                IM_COL32( 49, 65, 86, 255 ), radius, ImDrawFlags_RoundCornersTop );
            list.AddRect( native( node.min ), native( node.max ),
                          selected ? Accent : active ? Active : IM_COL32( 93, 112, 138, 255 ), radius,
                          0, selected ? 2.0f : 1.0f );
            list.AddCircleFilled( native( node.input() ), std::max( 3.0f, 4.0f * m_zoom ),
                                  IM_COL32( 143, 161, 184, 255 ) );
            list.AddCircleFilled( native( node.output() ), std::max( 4.0f, 5.5f * m_zoom ),
                                  hovered && port != nodes.rend() && port->state == &state
                                      ? IM_COL32( 201, 228, 250, 255 )
                                      : color );
            if( m_zoom >= 0.48f )
            {
                cardText( list, state.id, node, m_zoom, 6.0f, IM_COL32( 240, 245, 250, 255 ) );
                cardText( list, state.poseNodeId.empty() ? state.clipId : state.poseNodeId,
                          node, m_zoom, 35.0f, IM_COL32( 178, 198, 224, 255 ) );
                if( entry )
                {
                    cardText( list, "Entry state", node, m_zoom, 55.0f, Entry );
                }
            }
            if( active )
            {
                const auto sample = std::find_if( m_playback.samples.begin(), m_playback.samples.end(),
                                                   [&]( const auto &value ) {
                                                       return value.stateId == state.id;
                                                   } );
                if( sample != m_playback.samples.end() )
                {
                    const auto progress = std::clamp( sample->normalizedTime, 0.0f, 1.0f );
                    const auto left = node.min.X() + 7.0f * m_zoom;
                    const auto right = node.max.X() - 7.0f * m_zoom;
                    const auto y = node.max.Y() - 4.0f * m_zoom;
                    list.AddLine( { left, y }, { left + ( right - left ) * progress, y }, Active, 2.0f );
                }
            }
        }
        if( states.empty() )
        {
            const char *hint = m_availableClips.empty() ? "Import animation clips onto this Animator."
                                                       : "Add a state from a clip or pose node.";
            list.AddText( native( origin + Vector2F( 17.0f, 15.0f ) ),
                          ImGui::GetColorU32( ImGuiCol_TextDisabled ), hint );
            if( m_ambiguousClipCount )
            {
                list.AddText( native( origin + Vector2F( 17.0f, 36.0f ) ),
                              ImGui::GetColorU32( ImGuiCol_TextDisabled ), "Clips need unique names to appear here." );
            }
        }
        if( !m_linkSource.empty() )
        {
            list.AddText( native( origin + Vector2F( 12.0f, size.Y() - 24.0f ) ), Accent,
                          "Choose a destination state. Esc cancels." );
        }
        list.PopClipRect();

        if( hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
        {
            if( !m_linkSource.empty() && body != nodes.rend() && body->state->id != m_linkSource )
            {
                addTransition( m_linkSource, body->state->id );
                m_linkSource.clear();
            }
            else if( port != nodes.rend() )
            {
                selectState( port->state->id );
                m_linkSource = port->state->id;
            }
            else if( body != nodes.rend() )
            {
                selectState( body->state->id );
                m_draggedState = body->state->id;
                m_dragOrigin = ( body->min - start ) * ( 1.0f / m_zoom );
                m_dragStart = mouse;
            }
            else if( hoveredEdge )
            {
                selectTransition( *hoveredEdge );
                m_linkSource.clear();
            }
            else
            {
                m_selectedState.clear();
                m_selectedTransition.reset();
                m_linkSource.clear();
            }
        }
        if( !m_draggedState.empty() && ImGui::IsMouseDragging( ImGuiMouseButton_Left ) )
        {
            const auto position = m_dragOrigin + ( mouse - m_dragStart ) * ( 1.0f / m_zoom );
            if( m_edited.setStatePosition( m_draggedState, position.X(), position.Y() ) )
            {
                markEdited();
            }
        }
        if( ImGui::IsMouseReleased( ImGuiMouseButton_Left ) )
        {
            m_draggedState.clear();
            if( hovered && !m_linkSource.empty() && body != nodes.rend() &&
                body->state->id != m_linkSource )
            {
                addTransition( m_linkSource, body->state->id );
                m_linkSource.clear();
            }
        }
        if( ImGui::IsWindowFocused() && ImGui::IsKeyPressed( ImGuiKey_Escape, false ) )
        {
            m_linkSource.clear();
        }
        if( hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
        {
            if( body != nodes.rend() )
            {
                selectState( body->state->id );
                ImGui::OpenPopup( "State actions" );
            }
            else
            {
                m_addPosition = ( mouse - start ) * ( 1.0f / m_zoom );
                ImGui::OpenPopup( "Canvas actions" );
            }
        }
        if( ImGui::BeginPopup( "State actions" ) )
        {
            if( ImGui::MenuItem( "Make Transition", nullptr, false, states.size() > 1 ) )
            {
                m_linkSource = m_selectedState;
            }
            if( ImGui::MenuItem( "Set as Entry", nullptr, false,
                                 m_selectedState != m_edited.getInitialState() ) )
            {
                m_edited.setInitialState( m_selectedState );
                markEdited();
            }
            ImGui::EndPopup();
        }
        if( ImGui::BeginPopup( "Canvas actions" ) )
        {
            if( ImGui::MenuItem( "Frame All" ) )
            {
                m_fitOnDraw = true;
            }
            ImGui::Separator();
            drawAddStateMenu( m_addPosition );
            ImGui::EndPopup();
        }
        ImGui::EndChild();
    }

    void ImGuiAnimationGraph::selectState( const String &id )
    {
        m_selectedState = id;
        m_selectedTransition.reset();
        m_showInspector = true;
    }

    void ImGuiAnimationGraph::selectTransition( size_t index )
    {
        m_selectedTransition = index;
        m_selectedState.clear();
        m_showInspector = true;
    }

    void ImGuiAnimationGraph::addState( const String &clipId, const String &poseNodeId,
                                         Vector2F position )
    {
        animation::AnimationGraphState state;
        const auto &base = poseNodeId.empty() ? clipId : poseNodeId;
        state.id = base;
        for( u32 index = 2; m_edited.findState( state.id ); ++index )
        {
            state.id = base + " " + StringUtil::toString( index );
        }
        state.clipId = clipId;
        state.poseNodeId = poseNodeId;
        state.editorX = position.X();
        state.editorY = position.Y();
        state.hasEditorPosition = true;
        if( !clipId.empty() && !m_edited.findClip( clipId ) )
        {
            const auto clip = std::find_if( m_availableClips.begin(), m_availableClips.end(),
                                            [&]( const auto &item ) { return item.id == clipId; } );
            if( clip == m_availableClips.end() || !m_edited.addClip( *clip ) )
            {
                m_status = "The selected animation clip is unavailable.";
                return;
            }
        }
        if( m_edited.addState( state ) )
        {
            selectState( state.id );
            markEdited();
            m_fitOnDraw = true;
        }
    }

    void ImGuiAnimationGraph::addTransition( const String &from, const String &to )
    {
        animation::AnimationGraphTransition transition;
        transition.fromState = from;
        transition.toState = to;
        transition.exitTime = 1.0f;
        transition.synchronize = false;
        size_t index = 0;
        if( m_edited.addTransition( transition, &index ) )
        {
            selectTransition( index );
            markEdited();
        }
    }

    void ImGuiAnimationGraph::drawAddStateMenu( Vector2F position )
    {
        if( m_availableClips.empty() && m_edited.getPoseNodes().empty() )
        {
            ImGui::TextDisabled( "No animation clips or pose nodes" );
            return;
        }
        if( !m_availableClips.empty() && ImGui::BeginMenu( "From Clip" ) )
        {
            for( const auto &clip : m_availableClips )
            {
                if( ImGui::MenuItem( clip.id.c_str() ) )
                {
                    addState( clip.id, {}, position );
                }
            }
            ImGui::EndMenu();
        }
        if( !m_edited.getPoseNodes().empty() && ImGui::BeginMenu( "From Pose Node" ) )
        {
            for( const auto &node : m_edited.getPoseNodes() )
            {
                if( ImGui::MenuItem( node.id.c_str() ) )
                {
                    addState( {}, node.id, position );
                }
            }
            ImGui::EndMenu();
        }
    }

    void ImGuiAnimationGraph::markEdited()
    {
        m_dirty = true;
        m_status.clear();
    }
}  // namespace workphone::editor
#endif
