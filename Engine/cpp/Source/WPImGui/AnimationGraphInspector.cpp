#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiAnimationGraph.hpp>
#include <Workphone/Core/StringUtil.hpp>

#if WP_BUILD_IMGUI
#    include <imgui.h>
#    include <algorithm>
#    include <cmath>
#    include <set>
#    include <vector>

namespace workphone::ui
{
    namespace animation_graph_inspector
    {
        const char *const Comparisons[] = { "Equal", "Not equal", "Greater than", "At least",
                                            "Less than", "At most" };
        const char *const NodeTypes[] = { "Clip", "1D Blend", "Selector" };

        bool editName( const char *label, String &value )
        {
            std::vector<char> text( std::max<size_t>( value.size() + 1, 256 ), '\0' );
            std::copy( value.begin(), value.end(), text.begin() );
            if( !ImGui::InputText( label, text.data(), text.size(), ImGuiInputTextFlags_EnterReturnsTrue ) )
            {
                return false;
            }
            value = text.data();
            const auto first = value.find_first_not_of( " \t\r\n" );
            if( first == String::npos )
            {
                value.clear();
            }
            else
            {
                value = value.substr( first, value.find_last_not_of( " \t\r\n" ) - first + 1 );
            }
            return true;
        }

        bool inputNumber( const char *label, f32 &value )
        {
            f32 updated = value;
            if( ImGui::InputFloat( label, &updated, 0.0f, 0.0f, "%.5g" ) && std::isfinite( updated ) )
            {
                value = updated;
                return true;
            }
            return false;
        }

        template <class ItemRange, class GetName, class Include>
        bool chooseItem( const char *label, String &id, const ItemRange &items,
                         GetName nameOf, Include include )
        {
            bool changed = false;
            if( ImGui::BeginCombo( label, id.empty() ? "Choose..." : id.c_str() ) )
            {
                size_t index = 0;
                for( const auto &item : items )
                {
                    if( include( item ) )
                    {
                        const auto &name = nameOf( item );
                        ImGui::PushID( static_cast<int>( index ) );
                        if( ImGui::Selectable( name.c_str(), name == id ) )
                        {
                            id = name;
                            changed = true;
                        }
                        ImGui::PopID();
                    }
                    ++index;
                }
                ImGui::EndCombo();
            }
            return changed;
        }

        bool usesParameter( const animation::AnimationGraphDefinition &graph, const String &name )
        {
            for( const auto &transition : graph.getTransitions() )
            {
                for( const auto &condition : transition.conditions )
                {
                    if( condition.parameter == name )
                    {
                        return true;
                    }
                }
            }
            return std::any_of( graph.getPoseNodes().begin(), graph.getPoseNodes().end(),
                                [&]( const auto &node ) {
                                    return node.type != animation::AnimationGraphPoseNodeType::Clip &&
                                           node.parameter == name;
                                } );
        }

        bool usesPoseNode( const animation::AnimationGraphDefinition &graph, const String &name )
        {
            for( const auto &node : graph.getPoseNodes() )
            {
                if( node.id != name && node.type != animation::AnimationGraphPoseNodeType::Clip &&
                    std::find( node.childNodeIds.begin(), node.childNodeIds.end(), name ) !=
                        node.childNodeIds.end() )
                {
                    return true;
                }
            }
            return std::any_of( graph.getStates().begin(), graph.getStates().end(),
                                [&]( const auto &state ) { return state.poseNodeId == name; } );
        }

        bool canUseChild( const animation::AnimationGraphDefinition &graph,
                          const String &parent, const String &child )
        {
            std::set<String> seen;
            std::vector<String> pending{ child };
            while( !pending.empty() )
            {
                const auto current = std::move( pending.back() );
                pending.pop_back();
                if( current == parent )
                {
                    return false;
                }
                if( seen.insert( current ).second )
                {
                    if( const auto *node = graph.findPoseNode( current );
                        node && node->type != animation::AnimationGraphPoseNodeType::Clip )
                    {
                        pending.insert( pending.end(), node->childNodeIds.begin(), node->childNodeIds.end() );
                    }
                }
            }
            return true;
        }
    }  // namespace animation_graph_inspector

    void ImGuiAnimationGraph::drawInspector( f32 width, f32 height )
    {
        if( ImGui::BeginChild( "##AnimationGraphProperties", { width, height }, true ) )
        {
            if( ImGui::BeginTabBar( "##AnimationGraphTabs" ) )
            {
                const auto flags = m_showInspector ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
                m_showInspector = false;
                if( ImGui::BeginTabItem( "Inspector", nullptr, flags ) )
                {
                    if( m_selectedTransition )
                    {
                        drawTransitionInspector();
                    }
                    else if( !m_selectedState.empty() )
                    {
                        drawStateInspector();
                    }
                    else
                    {
                        ImGui::TextWrapped( "Select a state or transition on the graph." );
                    }

                    ImGui::Spacing();
                    if( ImGui::CollapsingHeader( "States and transitions" ) )
                    {
                        for( const auto &state : m_edited.getStates() )
                        {
                            ImGui::PushID( &state );
                            if( ImGui::Selectable( state.id.c_str(), m_selectedState == state.id ) )
                            {
                                selectState( state.id );
                            }
                            ImGui::PopID();
                        }
                        ImGui::Separator();
                        const auto &transitions = m_edited.getTransitions();
                        for( size_t i = 0; i < transitions.size(); ++i )
                        {
                            ImGui::PushID( static_cast<int>( i ) );
                            const auto label = transitions[i].fromState + " > " + transitions[i].toState;
                            if( ImGui::Selectable( label.c_str(), m_selectedTransition == i ) )
                            {
                                selectTransition( i );
                            }
                            ImGui::PopID();
                        }
                    }
                    ImGui::EndTabItem();
                }
                if( ImGui::BeginTabItem( "Parameters" ) )
                {
                    drawParameters();
                    ImGui::EndTabItem();
                }
                if( ImGui::BeginTabItem( "Pose Nodes" ) )
                {
                    drawPoseNodes();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::EndChild();
    }

    bool ImGuiAnimationGraph::chooseClip( const char *label, String &id )
    {
        using namespace animation_graph_inspector;
        if( !chooseItem( label, id, m_availableClips,
                         []( const auto &clip ) -> const String & { return clip.id; },
                         []( const auto & ) { return true; } ) )
        {
            return false;
        }
        if( !m_edited.findClip( id ) )
        {
            const auto clip = std::find_if( m_availableClips.begin(), m_availableClips.end(),
                                            [&]( const auto &item ) { return item.id == id; } );
            if( clip == m_availableClips.end() || !m_edited.addClip( *clip ) )
            {
                m_status = "The selected animation clip is unavailable.";
                return false;
            }
        }
        return true;
    }

    bool ImGuiAnimationGraph::chooseParameter( const char *label, String &name,
                                                 std::optional<bool> isBoolean )
    {
        return animation_graph_inspector::chooseItem(
            label, name, m_edited.getParameters(),
            []( const auto &parameter ) -> const String & { return parameter.name; },
            [&]( const auto &parameter ) { return !isBoolean || parameter.isBoolean == *isBoolean; } );
    }

    bool ImGuiAnimationGraph::chooseState( const char *label, String &id, const String &excluded )
    {
        return animation_graph_inspector::chooseItem(
            label, id, m_edited.getStates(),
            []( const auto &state ) -> const String & { return state.id; },
            [&]( const auto &state ) { return state.id != excluded; } );
    }

    void ImGuiAnimationGraph::drawStateInspector()
    {
        using namespace animation_graph_inspector;
        const auto *state = m_edited.findState( m_selectedState );
        if( !state )
        {
            m_selectedState.clear();
            return;
        }

        ImGui::TextDisabled( "State" );
        auto name = state->id;
        ImGui::SetNextItemWidth( -1.0f );
        if( editName( "##StateName", name ) )
        {
            if( m_edited.renameState( m_selectedState, name ) )
            {
                if( m_linkSource == m_selectedState )
                {
                    m_linkSource = name;
                }
                m_selectedState = name;
                markEdited();
            }
            else
            {
                m_status = "Enter a unique state name.";
            }
            return;
        }

        auto candidate = *state;
        auto changed = inputNumber( "Speed", candidate.speed );
        candidate.speed = std::max( 0.0f, candidate.speed );
        if( chooseClip( "Clip", candidate.clipId ) )
        {
            candidate.poseNodeId.clear();
            changed = true;
        }
        if( !m_edited.getPoseNodes().empty() )
        {
            changed |= chooseItem( "Pose Node", candidate.poseNodeId, m_edited.getPoseNodes(),
                                   []( const auto &node ) -> const String & { return node.id; },
                                   []( const auto & ) { return true; } );
        }
        if( changed && m_edited.editState( candidate ) )
        {
            markEdited();
        }

        ImGui::Spacing();
        ImGui::BeginDisabled( m_selectedState == m_edited.getInitialState() );
        if( ImGui::Button( "Set as Entry" ) )
        {
            m_edited.setInitialState( m_selectedState );
            markEdited();
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled( m_edited.getStates().size() < 2 );
        if( ImGui::Button( "Make Transition" ) )
        {
            m_linkSource = m_selectedState;
        }
        ImGui::EndDisabled();
        if( ImGui::Button( "Delete State" ) && m_edited.removeState( m_selectedState ) )
        {
            if( m_linkSource == m_selectedState )
            {
                m_linkSource.clear();
            }
            m_selectedState.clear();
            markEdited();
        }
    }

    void ImGuiAnimationGraph::drawTransitionInspector()
    {
        using namespace animation_graph_inspector;
        const auto index = *m_selectedTransition;
        if( index >= m_edited.getTransitions().size() )
        {
            m_selectedTransition.reset();
            return;
        }

        auto draft = m_edited.getTransitions()[index];
        ImGui::TextDisabled( "Transition" );
        auto changed = chooseState( "From", draft.fromState, draft.toState );
        changed |= chooseState( "To", draft.toState, draft.fromState );
        changed |= inputNumber( "Blend (seconds)", draft.duration );
        draft.duration = std::max( 0.0f, draft.duration );
        auto useExitTime = draft.exitTime >= 0.0f;
        if( ImGui::Checkbox( "Use exit time", &useExitTime ) )
        {
            draft.exitTime = useExitTime ? 1.0f : -1.0f;
            changed = true;
        }
        if( useExitTime )
        {
            changed |= inputNumber( "Normalized exit", draft.exitTime );
            draft.exitTime = std::clamp( draft.exitTime, 0.0f, 1.0f );
        }
        int priority = draft.priority;
        if( ImGui::InputInt( "Priority", &priority ) )
        {
            draft.priority = priority;
            changed = true;
        }
        if( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "Higher priority transitions are checked first." );
        }
        changed |= ImGui::Checkbox( "Sync destination time", &draft.synchronize );

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextUnformatted( "Conditions" );
        if( !draft.conditions.empty() )
        {
            ImGui::TextDisabled( "All conditions must pass." );
        }
        for( size_t i = 0; i < draft.conditions.size(); ++i )
        {
            ImGui::PushID( static_cast<int>( i ) );
            auto &condition = draft.conditions[i];
            if( chooseParameter( "Parameter", condition.parameter, std::nullopt ) )
            {
                const auto *parameter = m_edited.findParameter( condition.parameter );
                condition.isBoolean = parameter->isBoolean;
                condition.comparison = animation::AnimationGraphComparison::Equal;
                changed = true;
            }
            int comparison = static_cast<int>( condition.comparison );
            if( ImGui::Combo( "Test", &comparison, Comparisons, condition.isBoolean ? 2 : 6 ) )
            {
                condition.comparison = static_cast<animation::AnimationGraphComparison>( comparison );
                changed = true;
            }
            if( condition.isBoolean )
            {
                int required = condition.booleanValue ? 1 : 0;
                if( ImGui::Combo( "Value", &required, "False\0True\0" ) )
                {
                    condition.booleanValue = required != 0;
                    changed = true;
                }
            }
            else
            {
                changed |= inputNumber( "Value", condition.threshold );
            }
            if( ImGui::SmallButton( "Remove Condition" ) )
            {
                draft.conditions.erase( draft.conditions.begin() + i );
                --i;
                changed = true;
            }
            ImGui::PopID();
            ImGui::Separator();
        }
        const auto &parameters = m_edited.getParameters();
        ImGui::BeginDisabled( parameters.empty() );
        if( ImGui::Button( "Add Condition" ) )
        {
            animation::AnimationGraphCondition condition;
            condition.parameter = parameters.front().name;
            condition.isBoolean = parameters.front().isBoolean;
            condition.booleanValue = true;
            draft.conditions.push_back( std::move( condition ) );
            changed = true;
        }
        ImGui::EndDisabled();
        if( parameters.empty() )
        {
            ImGui::TextDisabled( "Add a parameter to create a condition." );
        }
        if( ImGui::Button( "Delete Transition" ) && m_edited.removeTransition( index ) )
        {
            m_selectedTransition.reset();
            markEdited();
            return;
        }
        if( changed )
        {
            size_t updated = index;
            if( m_edited.updateTransition( index, draft, &updated ) )
            {
                m_selectedTransition = updated;
                markEdited();
            }
        }
    }

    void ImGuiAnimationGraph::drawParameters()
    {
        using namespace animation_graph_inspector;
        if( ImGui::Button( "Add Parameter" ) )
        {
            m_newParameterName.fill( '\0' );
            m_newParameterIsBoolean = false;
            ImGui::OpenPopup( "New Parameter" );
        }
        if( ImGui::BeginPopup( "New Parameter" ) )
        {
            if( ImGui::IsWindowAppearing() )
            {
                ImGui::SetKeyboardFocusHere();
            }
            const auto submit = ImGui::InputText( "Name", m_newParameterName.data(),
                                                  m_newParameterName.size(),
                                                  ImGuiInputTextFlags_EnterReturnsTrue );
            int type = m_newParameterIsBoolean ? 1 : 0;
            if( ImGui::Combo( "Type", &type, "Float\0Boolean\0" ) )
            {
                m_newParameterIsBoolean = type == 1;
            }
            String name( m_newParameterName.data() );
            const auto first = name.find_first_not_of( " \t\r\n" );
            if( first != String::npos )
            {
                name = name.substr( first, name.find_last_not_of( " \t\r\n" ) - first + 1 );
            }
            const auto valid = first != String::npos && !m_edited.findParameter( name );
            ImGui::BeginDisabled( !valid );
            if( ( ImGui::Button( "Create" ) || submit ) && valid &&
                m_edited.addParameter( { name, m_newParameterIsBoolean } ) )
            {
                markEdited();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndDisabled();
            if( !valid && first != String::npos )
            {
                ImGui::TextDisabled( "Use a unique name." );
            }
            ImGui::EndPopup();
        }

        auto parameters = m_edited.getParameters();
        if( parameters.empty() )
        {
            ImGui::TextWrapped( "Parameters drive transition conditions and pose nodes." );
        }
        bool changed = false;
        for( auto parameter = parameters.begin(); parameter != parameters.end(); )
        {
            ImGui::PushID( parameter->name.c_str() );
            ImGui::Separator();
            ImGui::TextUnformatted( parameter->name.c_str() );
            ImGui::SameLine();
            ImGui::TextDisabled( "%s", parameter->isBoolean ? "Boolean" : "Float" );

            if( parameter->isBoolean )
            {
                changed |= ImGui::Checkbox( "Default", &parameter->booleanValue );
            }
            else
            {
                changed |= inputNumber( "Default", parameter->floatValue );
            }
            const auto live = std::find_if( m_playback.parameterValues.begin(),
                                            m_playback.parameterValues.end(), [&]( const auto &item ) {
                                                return item.name == parameter->name;
                                            } );
            if( live != m_playback.parameterValues.end() )
            {
                auto preview = *live;
                const auto update = preview.isBoolean ? ImGui::Checkbox( "Live", &preview.booleanValue )
                                                      : inputNumber( "Live", preview.floatValue );
                if( update )
                {
                    m_liveChanges.push_back( std::move( preview ) );
                }
                if( ImGui::IsItemHovered() )
                {
                    ImGui::SetTooltip( "Preview value for the current Animator." );
                }
            }

            ImGui::BeginDisabled( usesParameter( m_edited, parameter->name ) );
            if( ImGui::SmallButton( "Delete Parameter" ) )
            {
                parameter = parameters.erase( parameter );
                changed = true;
            }
            else
            {
                ++parameter;
            }
            ImGui::EndDisabled();
            ImGui::PopID();
        }
        if( changed && m_edited.setParameters( parameters ) )
        {
            markEdited();
        }
    }

    void ImGuiAnimationGraph::drawPoseNodes()
    {
        if( ImGui::Button( "Add Clip Node" ) )
        {
            ImGui::OpenPopup( "Choose pose clip" );
        }
        if( ImGui::BeginPopup( "Choose pose clip" ) )
        {
            for( const auto &clip : m_availableClips )
            {
                if( ImGui::Selectable( clip.id.c_str() ) )
                {
                    if( !m_edited.findClip( clip.id ) && !m_edited.addClip( clip ) )
                    {
                        m_status = "The selected animation clip is unavailable.";
                        break;
                    }
                    animation::AnimationGraphPoseNodeDefinition node;
                    node.clipId = clip.id;
                    node.id = clip.id + " Pose";
                    for( u32 i = 2; m_edited.findPoseNode( node.id ); ++i )
                    {
                        node.id = clip.id + " Pose " + StringUtil::toString( i );
                    }
                    if( m_edited.addPoseNode( node ) )
                    {
                        m_selectedPoseNode = node.id;
                        m_poseDraft = node;
                        markEdited();
                    }
                }
            }
            ImGui::EndPopup();
        }
        if( m_availableClips.empty() )
        {
            ImGui::TextDisabled( "The Animator needs clips before adding pose nodes." );
        }

        const auto &nodes = m_edited.getPoseNodes();
        for( const auto &node : nodes )
        {
            ImGui::PushID( &node );
            if( ImGui::Selectable( node.id.c_str(), node.id == m_selectedPoseNode ) )
            {
                m_selectedPoseNode = node.id;
                m_poseDraft = node;
            }
            ImGui::PopID();
        }
        if( m_poseDraft )
        {
            ImGui::Separator();
            drawPoseNodeInspector();
        }
    }

    void ImGuiAnimationGraph::drawPoseNodeInspector()
    {
        using namespace animation_graph_inspector;
        using Type = animation::AnimationGraphPoseNodeType;
        auto &node = *m_poseDraft;
        auto type = static_cast<int>( node.type );
        if( ImGui::Combo( "Type", &type, NodeTypes, 3 ) )
        {
            node.type = static_cast<Type>( type );
        }

        if( node.type == Type::Clip )
        {
            chooseClip( "Clip", node.clipId );
            if( inputNumber( "Speed", node.speed ) )
            {
                node.speed = std::max( 0.0f, node.speed );
            }
            ImGui::Checkbox( "Loop", &node.loop );
        }
        else
        {
            const auto isBoolean = node.type == Type::Selector && node.childNodeIds.size() == 2
                                       ? std::optional<bool>{}
                                       : std::optional<bool>{ false };
            chooseParameter( "Parameter", node.parameter, isBoolean );
            if( const auto *parameter = m_edited.findParameter( node.parameter );
                parameter && isBoolean && parameter->isBoolean )
            {
                node.parameter.clear();
            }

            ImGui::TextUnformatted( "Inputs" );
            for( size_t i = 0; i < node.childNodeIds.size(); ++i )
            {
                ImGui::PushID( static_cast<int>( i ) );
                ImGui::Text( "%zu   %s", i, node.childNodeIds[i].c_str() );
                ImGui::SameLine();
                if( ImGui::SmallButton( "Remove" ) )
                {
                    node.childNodeIds.erase( node.childNodeIds.begin() + i );
                    auto &ranges = node.blendRanges;
                    ranges.erase( std::remove_if( ranges.begin(), ranges.end(), [&]( const auto &range ) {
                        return range.inputIndex0 == i || range.inputIndex1 == i;
                    } ), ranges.end() );
                    for( auto &range : ranges )
                    {
                        if( range.inputIndex0 > i )
                        {
                            --range.inputIndex0;
                        }
                        if( range.inputIndex1 > i )
                        {
                            --range.inputIndex1;
                        }
                    }
                    --i;
                }
                ImGui::PopID();
            }
            String child;
            if( chooseItem( "Add Input", child, m_edited.getPoseNodes(),
                            []( const auto &item ) -> const String & { return item.id; },
                            [&]( const auto &item ) { return canUseChild( m_edited, node.id, item.id ); } ) )
            {
                node.childNodeIds.push_back( child );
            }
            if( node.type == Type::Blend1D )
            {
                ImGui::Spacing();
                ImGui::TextUnformatted( "Blend ranges" );
                for( size_t i = 0; i < node.blendRanges.size(); ++i )
                {
                    ImGui::PushID( static_cast<int>( i ) );
                    auto &range = node.blendRanges[i];
                    inputNumber( "Minimum", range.parameterLow );
                    inputNumber( "Maximum", range.parameterHigh );
                    int source = static_cast<int>( range.inputIndex0 );
                    int target = static_cast<int>( range.inputIndex1 );
                    if( ImGui::InputInt( "From input", &source ) )
                    {
                        range.inputIndex0 = static_cast<u32>( std::max( 0, source ) );
                    }
                    if( ImGui::InputInt( "To input", &target ) )
                    {
                        range.inputIndex1 = static_cast<u32>( std::max( 0, target ) );
                    }
                    if( ImGui::SmallButton( "Remove Range" ) )
                    {
                        node.blendRanges.erase( node.blendRanges.begin() + i );
                        --i;
                    }
                    ImGui::PopID();
                    ImGui::Separator();
                }
                ImGui::BeginDisabled( node.childNodeIds.size() < 2 );
                if( ImGui::Button( "Add Range" ) )
                {
                    const auto low = node.blendRanges.empty() ? 0.0f : node.blendRanges.back().parameterHigh;
                    node.blendRanges.push_back( { 0, 1, low, low + 1.0f } );
                }
                ImGui::EndDisabled();
            }
        }

        ImGui::Separator();
        if( ImGui::Button( "Update Pose Node" ) )
        {
            bool valid = node.isValid();
            if( node.type == Type::Blend1D )
            {
                for( size_t i = 0; i < node.blendRanges.size(); ++i )
                {
                    const auto &range = node.blendRanges[i];
                    valid = valid && range.inputIndex0 < node.childNodeIds.size() &&
                            range.inputIndex1 < node.childNodeIds.size() &&
                            ( i == 0 || node.blendRanges[i - 1].parameterHigh <= range.parameterLow );
                }
            }
            if( valid && m_edited.editPoseNode( node ) )
            {
                markEdited();
            }
            else
            {
                m_status = "Check the pose node's clip, parameter, inputs or range limits.";
            }
        }
        ImGui::SameLine();
        const bool referenced = usesPoseNode( m_edited, node.id );
        ImGui::BeginDisabled( referenced );
        if( ImGui::Button( "Delete" ) && m_edited.removePoseNode( node.id ) )
        {
            m_selectedPoseNode.clear();
            m_poseDraft.reset();
            markEdited();
        }
        ImGui::EndDisabled();
    }
}  // namespace workphone::editor
#endif
