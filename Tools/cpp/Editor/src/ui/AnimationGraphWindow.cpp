#include <EditorPCH.hpp>
#include <ui/AnimationGraphWindow.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <cstdlib>
#include <cerrno>
#include <sstream>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, AnimationGraphWindow, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, AnimationGraphWindow::WindowListener, IEventListener );

    namespace
    {
        String trim( const String &value )
        {
            auto start = value.find_first_not_of( " \t\r\n" );
            if( start == String::npos )
            {
                return String();
            }
            auto end = value.find_last_not_of( " \t\r\n" );
            return value.substr( start, end - start + 1 );
        }

        Array<String> split( const String &value, char delimiter )
        {
            Array<String> tokens;
            String current;
            for( const auto character : value )
            {
                if( character == delimiter )
                {
                    tokens.push_back( trim( current ) );
                    current.clear();
                }
                else
                {
                    current += character;
                }
            }
            if( !current.empty() )
            {
                tokens.push_back( trim( current ) );
            }
            return tokens;
        }

        f32 toFloat( const String &value, f32 fallback )
        {
            const auto text = value.c_str();
            if( !text || *text == '\0' )
            {
                return fallback;
            }
            char *end = nullptr;
            errno = 0;
            const auto result = std::strtof( text, &end );
            if( end == text || errno == ERANGE )
            {
                return fallback;
            }
            return result;
        }

        u32 toU32( const String &value, u32 fallback )
        {
            const auto text = value.c_str();
            if( !text || *text == '\0' )
            {
                return fallback;
            }
            char *end = nullptr;
            errno = 0;
            const auto result = std::strtoul( text, &end, 10 );
            if( end == text || errno == ERANGE )
            {
                return fallback;
            }
            return static_cast<u32>( result );
        }
    }  // namespace

    AnimationGraphWindow::AnimationGraphWindow()
    {
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );
    }

    AnimationGraphWindow::~AnimationGraphWindow()
    {
        unload( nullptr );
    }

    Parameter AnimationGraphWindow::WindowListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventType != EventType::UI || !sender || !sender->isDerived<ui::IUIElement>() )
        {
            return {};
        }

        auto owner = getOwner();
        if( !owner )
        {
            return {};
        }

        const auto element = workphone::static_pointer_cast<ui::IUIElement>( sender );
        const auto id = static_cast<WidgetId>( element->getElementId() );

        if( eventValue == IEvent::handleSelection )
        {
            switch( id )
            {
            case WidgetId::Refresh:
                owner->refresh();
                break;
            case WidgetId::Apply:
                owner->applyEditedGraph();
                break;
            case WidgetId::ResetGraph:
                owner->loadFromAnimator();
                owner->rebuildTrees();
                owner->syncEditor();
                break;
            case WidgetId::Play:
                if( owner->m_animator )
                    owner->m_animator->play();
                break;
            case WidgetId::Pause:
                if( owner->m_animator )
                    owner->m_animator->pause();
                break;
            case WidgetId::Stop:
                if( owner->m_animator )
                    owner->m_animator->stop();
                break;
            case WidgetId::AddState:
                owner->addState();
                break;
            case WidgetId::RemoveState:
                owner->removeState();
                break;
            case WidgetId::SetInitial:
                owner->applyEditorFields();
                owner->setStatus( "Initial state updated" );
                break;
            case WidgetId::AddTransition:
                owner->addTransition();
                break;
            case WidgetId::RemoveTransition:
                owner->removeTransition();
                break;
            case WidgetId::AddCondition:
                owner->addCondition();
                break;
            case WidgetId::RemoveCondition:
                owner->removeCondition();
                break;
            case WidgetId::AddPoseNode:
                owner->addPoseNode();
                break;
            case WidgetId::RemovePoseNode:
                owner->removePoseNode();
                break;
            case WidgetId::AddParameter:
                owner->addParameter();
                break;
            case WidgetId::RemoveParameter:
                owner->removeParameter();
                break;
            default:
                break;
            }
        }
        else if( eventValue == IEvent::handleSelection || eventValue == IEvent::handlePropertyChanged ||
                 eventValue == IEvent::handleValueChanged )
        {
            switch( id )
            {
            case WidgetId::StateTree:
            case WidgetId::TransitionTree:
            case WidgetId::PoseNodeTree:
            case WidgetId::ParameterTree:
                owner->onSelectionChanged( id );
                break;
            case WidgetId::LiveParamValue:
            case WidgetId::LiveParamBool:
                owner->applyLiveParameter();
                break;
            default:
                owner->applyEditorFields();
                break;
            }
        }

        return {};
    }

    SmartPtr<AnimationGraphWindow> AnimationGraphWindow::WindowListener::getOwner() const
    {
        return m_owner.load().lock();
    }

    void AnimationGraphWindow::WindowListener::setOwner( SmartPtr<AnimationGraphWindow> owner )
    {
        m_owner = owner;
    }
    void AnimationGraphWindow::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            auto listener = workphone::make_ptr<WindowListener>();
            listener->setOwner( this );
            m_windowListener = listener;

            auto window = ui->addElementByType<ui::IUIWindow>();
            window->setLabel( "Animation Graph Editor" );
            window->setHasBorder( true );
            window->addObjectListener( listener );
            setParentWindow( window );
            m_window = window;

            auto addButton = [&]( const String &label, WidgetId id, bool sameLine ) {
                auto button = ui->addElementByType<ui::IUIButton>();
                button->setLabel( label );
                button->setElementId( static_cast<hash_type>( id ) );
                button->setSameLine( sameLine );
                button->addObjectListener( listener );
                window->addChild( button );
            };
            auto addSlider = [&]( const String &label, WidgetId id, f32 minimum, f32 maximum ) {
                auto slider = ui->addElementByType<ui::IUILabelSliderPair>();
                slider->setLabel( label );
                slider->setElementId( static_cast<hash_type>( id ) );
                slider->setMinValue( minimum );
                slider->setMaxValue( maximum );
                slider->addObjectListener( listener );
                window->addChild( slider );
                return slider;
            };
            auto addToggle = [&]( const String &label, WidgetId id ) {
                auto toggle = ui->addElementByType<ui::IUILabelTogglePair>();
                toggle->setLabel( label );
                toggle->setElementId( static_cast<hash_type>( id ) );
                toggle->addObjectListener( listener );
                window->addChild( toggle );
                return toggle;
            };
            auto addTextInput = [&]( const String &label, WidgetId id ) {
                auto input = ui->addElementByType<ui::IUILabelTextInputPair>();
                input->setLabel( label );
                input->setElementId( static_cast<hash_type>( id ) );
                input->addObjectListener( listener );
                window->addChild( input );
                return input;
            };
            auto addDropdown = [&]( const String &label, WidgetId id, const Array<String> &options ) {
                auto dropdown = ui->addElementByType<ui::IUILabelDropdownPair>();
                dropdown->setLabel( label );
                dropdown->setElementId( static_cast<hash_type>( id ) );
                dropdown->setOptions( options );
                dropdown->addObjectListener( listener );
                window->addChild( dropdown );
                return dropdown;
            };
            auto addTree = [&]( const String &label, WidgetId id ) {
                auto tree = ui->addElementByType<ui::IUITreeCtrl>();
                tree->setLabel( label );
                tree->setElementId( static_cast<hash_type>( id ) );
                tree->addObjectListener( listener );
                window->addChild( tree );
                return tree;
            };

            addButton( "Refresh", WidgetId::Refresh, false );
            addButton( "Apply", WidgetId::Apply, true );
            addButton( "Reset", WidgetId::ResetGraph, true );
            addButton( "Play", WidgetId::Play, true );
            addButton( "Pause", WidgetId::Pause, true );
            addButton( "Stop", WidgetId::Stop, true );

            m_statusText = ui->addElementByType<ui::IUIText>();
            window->addChild( m_statusText );

            m_parameterTree = addTree( "Parameters", WidgetId::ParameterTree );
            addButton( "Add Param", WidgetId::AddParameter, false );
            addButton( "Remove Param", WidgetId::RemoveParameter, true );
            m_liveParamValueSlider =
                addSlider( "Live Float Value", WidgetId::LiveParamValue, -10.0f, 10.0f );
            m_liveParamBoolToggle = addToggle( "Live Bool Value", WidgetId::LiveParamBool );

            m_stateTree = addTree( "States", WidgetId::StateTree );
            addButton( "Add State", WidgetId::AddState, false );
            addButton( "Remove State", WidgetId::RemoveState, true );
            addButton( "Set Initial", WidgetId::SetInitial, true );

            m_transitionTree = addTree( "Transitions", WidgetId::TransitionTree );
            addButton( "Add Transition", WidgetId::AddTransition, false );
            addButton( "Remove Transition", WidgetId::RemoveTransition, true );
            addButton( "Add Condition", WidgetId::AddCondition, true );
            addButton( "Remove Condition", WidgetId::RemoveCondition, true );

            m_poseNodeTree = addTree( "Pose Nodes", WidgetId::PoseNodeTree );
            addButton( "Add Pose Node", WidgetId::AddPoseNode, false );
            addButton( "Remove Pose Node", WidgetId::RemovePoseNode, true );

            m_idText = addTextInput( "Id", WidgetId::SelectedId );
            m_clipIdText = addTextInput( "Clip Id", WidgetId::SelectedClipId );
            m_poseNodeIdText = addTextInput( "Pose Node Id", WidgetId::SelectedPoseNodeId );
            m_speedSlider = addSlider( "Speed", WidgetId::SelectedSpeed, 0.0f, 4.0f );
            m_durationSlider = addSlider( "Duration", WidgetId::SelectedDuration, 0.0f, 5.0f );
            m_exitTimeSlider = addSlider( "Exit Time", WidgetId::SelectedExitTime, -1.0f, 1.0f );
            m_prioritySlider = addSlider( "Priority", WidgetId::SelectedPriority, -10.0f, 10.0f );
            m_syncToggle = addToggle( "Synchronize", WidgetId::SelectedSync );
            m_fromStateText = addTextInput( "From State", WidgetId::SelectedFromState );
            m_toStateText = addTextInput( "To State", WidgetId::SelectedToState );
            m_parameterText = addTextInput( "Parameter", WidgetId::SelectedParameter );
            m_isBoolToggle = addToggle( "Is Boolean", WidgetId::SelectedIsBool );
            m_boolValueToggle = addToggle( "Boolean Value", WidgetId::SelectedBoolValue );
            m_thresholdSlider = addSlider( "Threshold", WidgetId::SelectedThreshold, -10.0f, 10.0f );
            m_comparisonDropdown = addDropdown(
                "Comparison", WidgetId::SelectedComparison,
                { "Equal", "NotEqual", "Greater", "GreaterOrEqual", "Less", "LessOrEqual" } );
            m_nodeTypeDropdown = addDropdown( "Node Type", WidgetId::SelectedNodeType,
                                              { "Clip", "Blend1D", "Selector" } );
            m_childrenText =
                addTextInput( "Children (comma-separated ids)", WidgetId::SelectedChildren );
            m_blendRangesText =
                addTextInput( "Blend Ranges (low high idx0 idx1; ...)", WidgetId::SelectedBlendRanges );

            EditorWindow::load( data );
            loadFromAnimator();
            rebuildTrees();
            syncEditor();
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Unloaded );
        }
    }

    void AnimationGraphWindow::unload( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() != LoadingState::Loaded )
        {
            return;
        }

        setLoadingState( LoadingState::Unloading );
        m_animator = nullptr;
        m_selectedId.clear();
        m_selectionKind = SelectionKind::None;

        setParentWindow( nullptr );

        if( auto applicationManager = core::IApplicationManager::instancePtr() )
        {
            if( auto ui = applicationManager->getUI() )
            {
                if( m_window )
                {
                    ui->removeElement( m_window );
                }
            }
        }

        m_window = nullptr;
        m_windowListener = nullptr;
        m_statusText = nullptr;
        m_stateTree = nullptr;
        m_transitionTree = nullptr;
        m_poseNodeTree = nullptr;
        m_parameterTree = nullptr;
        m_idText = nullptr;
        m_clipIdText = nullptr;
        m_poseNodeIdText = nullptr;
        m_speedSlider = nullptr;
        m_durationSlider = nullptr;
        m_exitTimeSlider = nullptr;
        m_prioritySlider = nullptr;
        m_thresholdSlider = nullptr;
        m_parameterText = nullptr;
        m_fromStateText = nullptr;
        m_toStateText = nullptr;
        m_syncToggle = nullptr;
        m_isBoolToggle = nullptr;
        m_boolValueToggle = nullptr;
        m_comparisonDropdown = nullptr;
        m_nodeTypeDropdown = nullptr;
        m_childrenText = nullptr;
        m_blendRangesText = nullptr;
        m_liveParamValueSlider = nullptr;
        m_liveParamBoolToggle = nullptr;

        EditorWindow::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }
    void AnimationGraphWindow::update()
    {
        const auto selectedAnimator = findSelectedAnimator();
        if( selectedAnimator != m_animator )
        {
            loadFromAnimator();
            rebuildTrees();
            syncEditor();
        }
        else if( m_animator )
        {
            if( auto graph = m_animator->getAnimationGraph() )
            {
                const auto progress = graph->isTransitioning() ? graph->getTransitionProgress() : 0.0f;
                setStatus( String( "State: " ) + graph->getCurrentState() +
                           ( graph->isTransitioning()
                                 ? ( String( " -> " ) +
                                     StringUtil::toString( static_cast<u32>( progress * 100.0f ) ) +
                                     String( "%" ) )
                                 : String() ) +
                           String( "  Samples: " ) +
                           StringUtil::toString( static_cast<u32>( graph->getSamples().size() ) ) );
            }
        }
    }

    void AnimationGraphWindow::updateSelection()
    {
        loadFromAnimator();
        rebuildTrees();
        syncEditor();
    }

    void AnimationGraphWindow::refresh()
    {
        loadFromAnimator();
        rebuildTrees();
        syncEditor();
    }

    SmartPtr<scene::Animator> AnimationGraphWindow::findSelectedAnimator() const
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto selectionManager =
            applicationManager ? applicationManager->getSelectionManagerPtr() : nullptr;
        if( !selectionManager )
        {
            return nullptr;
        }

        for( const auto &selected : selectionManager->getSelection() )
        {
            if( !selected )
            {
                continue;
            }
            if( selected->isDerived<scene::Animator>() )
            {
                return workphone::static_pointer_cast<scene::Animator>( selected );
            }
            if( selected->isDerived<scene::IGameActor>() )
            {
                auto actor = workphone::static_pointer_cast<scene::IGameActor>( selected );
                if( auto animator = actor->getComponent<scene::Animator>() )
                {
                    return animator;
                }
            }
            if( selected->isDerived<scene::IComponent>() )
            {
                auto component = workphone::static_pointer_cast<scene::IComponent>( selected );
                if( auto actor = component->getActor() )
                {
                    if( auto animator = actor->getComponent<scene::Animator>() )
                    {
                        return animator;
                    }
                }
            }
        }

        return nullptr;
    }

    void AnimationGraphWindow::loadFromAnimator()
    {
        m_animator = findSelectedAnimator();
        m_selectedId.clear();
        m_selectionKind = SelectionKind::None;
        m_parameters.clear();

        if( !m_animator )
        {
            m_edited = animation::AnimationGraphDefinition();
            setStatus( "Select an actor with an Animator component." );
            return;
        }

        if( auto graph = m_animator->getAnimationGraph() )
        {
            m_edited = graph->getDefinition();
            setStatus( String( "Loaded graph: " ) +
                       StringUtil::toString( static_cast<u32>( m_edited.getStates().size() ) ) +
                       String( " states" ) );
        }
        else
        {
            m_edited = animation::AnimationGraphDefinition();
            setStatus( "Animator has no animation graph. Add states and Apply to author one." );
        }
    }

    void AnimationGraphWindow::setStatus( const String &status )
    {
        if( m_statusText )
        {
            m_statusText->setText( status );
        }
    }

    const String AnimationGraphWindow::comparisonLabel(
        animation::AnimationGraphComparison comparison ) const
    {
        switch( comparison )
        {
        case animation::AnimationGraphComparison::Equal:
            return "Equal";
        case animation::AnimationGraphComparison::NotEqual:
            return "NotEqual";
        case animation::AnimationGraphComparison::Greater:
            return "Greater";
        case animation::AnimationGraphComparison::GreaterOrEqual:
            return "GreaterOrEqual";
        case animation::AnimationGraphComparison::Less:
            return "Less";
        case animation::AnimationGraphComparison::LessOrEqual:
            return "LessOrEqual";
        default:
            return "Equal";
        }
    }

    const String AnimationGraphWindow::nodeTypeLabel( animation::AnimationGraphPoseNodeType type ) const
    {
        switch( type )
        {
        case animation::AnimationGraphPoseNodeType::Clip:
            return "Clip";
        case animation::AnimationGraphPoseNodeType::Blend1D:
            return "Blend1D";
        case animation::AnimationGraphPoseNodeType::Selector:
            return "Selector";
        default:
            return "Clip";
        }
    }
    void AnimationGraphWindow::rebuildTrees()
    {
        const auto buildTree = []( SmartPtr<ui::IUITreeCtrl> tree, const String &rootLabel,
                                   const Array<String> &entries, const String &selectedId ) {
            if( !tree )
            {
                return;
            }
            tree->clear();
            auto root = tree->addRoot();
            if( !root )
            {
                return;
            }
            root->setName( rootLabel );
            for( u32 i = 0; i < entries.size(); ++i )
            {
                auto node = tree->addNode();
                node->setName( entries[i] );
                node->setTreeNodeId( i );
                root->addChild( node );
                if( !selectedId.empty() && entries[i].find( selectedId ) == 0 )
                {
                    tree->setSelectedTreeNode( node );
                }
            }
            tree->expand( root );
        };

        Array<String> stateEntries;
        const auto &states = m_edited.getStates();
        stateEntries.reserve( states.size() );
        const auto &initialState = m_edited.getInitialState();
        for( const auto &state : states )
        {
            String entry = state.id;
            if( state.id == initialState )
            {
                entry += String( " *" );
            }
            entry += String( " [" ) + ( state.poseNodeId.empty() ? state.clipId : state.poseNodeId ) +
                     String( "]" );
            stateEntries.push_back( entry );
        }
        buildTree( m_stateTree, "States", stateEntries, m_selectedId );

        Array<String> transitionEntries;
        const auto &transitions = m_edited.getTransitions();
        transitionEntries.reserve( transitions.size() );
        u32 transitionIndex = 0;
        for( const auto &transition : transitions )
        {
            String entry = transition.fromState + String( " -> " ) + transition.toState +
                           String( " (" ) + StringUtil::toString( transitionIndex ) + String( ")" );
            transitionEntries.push_back( entry );
            ++transitionIndex;
        }
        buildTree( m_transitionTree, "Transitions", transitionEntries, m_selectedId );

        Array<String> poseNodeEntries;
        const auto &poseNodes = m_edited.getPoseNodes();
        poseNodeEntries.reserve( poseNodes.size() );
        for( const auto &node : poseNodes )
        {
            poseNodeEntries.push_back( node.id + String( " [" ) + nodeTypeLabel( node.type ) +
                                       String( "]" ) );
        }
        buildTree( m_poseNodeTree, "Pose Nodes", poseNodeEntries, m_selectedId );

        Array<String> parameterEntries;
        parameterEntries.reserve( m_parameters.size() );
        for( const auto &parameter : m_parameters )
        {
            parameterEntries.push_back( parameter.name + String( " (" ) +
                                        ( parameter.isBool ? "bool" : "float" ) + String( ")" ) );
        }
        buildTree( m_parameterTree, "Parameters", parameterEntries, m_selectedId );
    }

    animation::AnimationGraphState *AnimationGraphWindow::getSelectedState()
    {
        if( m_selectionKind != SelectionKind::State || m_selectedId.empty() )
        {
            return nullptr;
        }
        auto &states = const_cast<Array<animation::AnimationGraphState> &>( m_edited.getStates() );
        for( auto &state : states )
        {
            if( state.id == m_selectedId )
            {
                return &state;
            }
        }
        return nullptr;
    }

    animation::AnimationGraphTransition *AnimationGraphWindow::getSelectedTransition()
    {
        if( m_selectionKind != SelectionKind::Transition || m_selectedId.empty() )
        {
            return nullptr;
        }
        const auto index = m_selectedTransitionIndex;
        auto &transitions =
            const_cast<Array<animation::AnimationGraphTransition> &>( m_edited.getTransitions() );
        if( index >= transitions.size() )
        {
            return nullptr;
        }
        return &transitions[index];
    }

    animation::AnimationGraphCondition *AnimationGraphWindow::getSelectedCondition()
    {
        if( m_selectionKind != SelectionKind::Condition )
        {
            return nullptr;
        }
        const auto transitionIndex = m_selectedTransitionIndex;
        auto &transitions =
            const_cast<Array<animation::AnimationGraphTransition> &>( m_edited.getTransitions() );
        if( transitionIndex >= transitions.size() )
        {
            return nullptr;
        }
        auto &conditions = transitions[transitionIndex].conditions;
        if( m_selectedConditionIndex >= conditions.size() )
        {
            return nullptr;
        }
        return &conditions[m_selectedConditionIndex];
    }
    animation::AnimationGraphPoseNodeDefinition *AnimationGraphWindow::getSelectedPoseNode()
    {
        if( m_selectionKind != SelectionKind::PoseNode || m_selectedId.empty() )
        {
            return nullptr;
        }
        auto &nodes =
            const_cast<Array<animation::AnimationGraphPoseNodeDefinition> &>( m_edited.getPoseNodes() );
        for( auto &node : nodes )
        {
            if( node.id == m_selectedId )
            {
                return &node;
            }
        }
        return nullptr;
    }

    AnimationGraphWindow::AuthoredParameter *AnimationGraphWindow::getSelectedParameter()
    {
        if( m_selectionKind != SelectionKind::Parameter || m_selectedId.empty() )
        {
            return nullptr;
        }
        for( auto &parameter : m_parameters )
        {
            if( parameter.name == m_selectedId )
            {
                return &parameter;
            }
        }
        return nullptr;
    }
    void AnimationGraphWindow::onSelectionChanged( WidgetId source )
    {
        switch( source )
        {
        case WidgetId::StateTree:
        {
            m_selectionKind = SelectionKind::State;
            if( m_stateTree )
            {
                if( auto node = m_stateTree->getSelectedTreeNode() )
                {
                    const auto idx = node->getTreeNodeId();
                    const auto &states = m_edited.getStates();
                    if( idx < states.size() )
                    {
                        m_selectedId = states[idx].id;
                    }
                }
            }
            break;
        }
        case WidgetId::TransitionTree:
        {
            m_selectionKind = SelectionKind::Transition;
            m_selectedConditionIndex = 0;
            if( m_transitionTree )
            {
                if( auto node = m_transitionTree->getSelectedTreeNode() )
                {
                    m_selectedTransitionIndex = node->getTreeNodeId();
                    const auto &transitions = m_edited.getTransitions();
                    if( m_selectedTransitionIndex < transitions.size() )
                    {
                        m_selectedId = transitions[m_selectedTransitionIndex].fromState +
                                       String( " -> " ) + transitions[m_selectedTransitionIndex].toState;
                    }
                }
            }
            break;
        }
        case WidgetId::PoseNodeTree:
        {
            m_selectionKind = SelectionKind::PoseNode;
            if( m_poseNodeTree )
            {
                if( auto node = m_poseNodeTree->getSelectedTreeNode() )
                {
                    const auto idx = node->getTreeNodeId();
                    const auto &nodes = m_edited.getPoseNodes();
                    if( idx < nodes.size() )
                    {
                        m_selectedId = nodes[idx].id;
                    }
                }
            }
            break;
        }
        case WidgetId::ParameterTree:
        {
            m_selectionKind = SelectionKind::Parameter;
            if( m_parameterTree )
            {
                if( auto node = m_parameterTree->getSelectedTreeNode() )
                {
                    const auto idx = node->getTreeNodeId();
                    if( idx < m_parameters.size() )
                    {
                        m_selectedId = m_parameters[idx].name;
                    }
                }
            }
            break;
        }
        default:
            break;
        }

        syncEditor();
    }

    void AnimationGraphWindow::syncEditor()
    {
        const auto clearFields = [&]() {
            if( m_idText )
                m_idText->setValue( "" );
            if( m_clipIdText )
                m_clipIdText->setValue( "" );
            if( m_poseNodeIdText )
                m_poseNodeIdText->setValue( "" );
            if( m_speedSlider )
                m_speedSlider->setValue( 1.0f );
            if( m_durationSlider )
                m_durationSlider->setValue( 0.15f );
            if( m_exitTimeSlider )
                m_exitTimeSlider->setValue( -1.0f );
            if( m_prioritySlider )
                m_prioritySlider->setValue( 0.0f );
            if( m_syncToggle )
                m_syncToggle->setValue( true );
            if( m_fromStateText )
                m_fromStateText->setValue( "" );
            if( m_toStateText )
                m_toStateText->setValue( "" );
            if( m_parameterText )
                m_parameterText->setValue( "" );
            if( m_isBoolToggle )
                m_isBoolToggle->setValue( false );
            if( m_boolValueToggle )
                m_boolValueToggle->setValue( false );
            if( m_thresholdSlider )
                m_thresholdSlider->setValue( 0.0f );
            if( m_comparisonDropdown )
                m_comparisonDropdown->setSelectedOption( 0 );
            if( m_nodeTypeDropdown )
                m_nodeTypeDropdown->setSelectedOption( 0 );
            if( m_childrenText )
                m_childrenText->setValue( "" );
            if( m_blendRangesText )
                m_blendRangesText->setValue( "" );
            if( m_liveParamValueSlider )
            {
                m_liveParamValueSlider->setValue( 0.0f );
                m_liveParamValueSlider->setEnabled( false );
            }
            if( m_liveParamBoolToggle )
            {
                m_liveParamBoolToggle->setValue( false );
                m_liveParamBoolToggle->setEnabled( false );
            }
        };

        clearFields();

        switch( m_selectionKind )
        {
        case SelectionKind::State:
        {
            const auto *state = getSelectedState();
            if( !state )
            {
                break;
            }
            if( m_idText )
                m_idText->setValue( state->id );
            if( m_clipIdText )
                m_clipIdText->setValue( state->clipId );
            if( m_poseNodeIdText )
                m_poseNodeIdText->setValue( state->poseNodeId );
            if( m_speedSlider )
                m_speedSlider->setValue( state->speed );
            break;
        }
        case SelectionKind::Transition:
        {
            const auto *transition = getSelectedTransition();
            if( !transition )
            {
                break;
            }
            if( m_fromStateText )
                m_fromStateText->setValue( transition->fromState );
            if( m_toStateText )
                m_toStateText->setValue( transition->toState );
            if( m_durationSlider )
                m_durationSlider->setValue( transition->duration );
            if( m_exitTimeSlider )
                m_exitTimeSlider->setValue( transition->exitTime );
            if( m_prioritySlider )
                m_prioritySlider->setValue( static_cast<f32>( transition->priority ) );
            if( m_syncToggle )
                m_syncToggle->setValue( transition->synchronize );

            if( !transition->conditions.empty() &&
                m_selectedConditionIndex < transition->conditions.size() )
            {
                const auto &condition = transition->conditions[m_selectedConditionIndex];
                if( m_parameterText )
                    m_parameterText->setValue( condition.parameter );
                if( m_isBoolToggle )
                    m_isBoolToggle->setValue( condition.isBoolean );
                if( m_boolValueToggle )
                    m_boolValueToggle->setValue( condition.booleanValue );
                if( m_thresholdSlider )
                    m_thresholdSlider->setValue( condition.threshold );
                if( m_comparisonDropdown )
                    m_comparisonDropdown->setSelectedOption( static_cast<u32>( condition.comparison ) );
            }
            break;
        }
        case SelectionKind::PoseNode:
        {
            const auto *node = getSelectedPoseNode();
            if( !node )
            {
                break;
            }
            if( m_idText )
                m_idText->setValue( node->id );
            if( m_clipIdText )
                m_clipIdText->setValue( node->clipId );
            if( m_parameterText )
                m_parameterText->setValue( node->parameter );
            if( m_speedSlider )
                m_speedSlider->setValue( node->speed );
            if( m_nodeTypeDropdown )
                m_nodeTypeDropdown->setSelectedOption( static_cast<u32>( node->type ) );

            String children;
            for( u32 i = 0; i < node->childNodeIds.size(); ++i )
            {
                if( i > 0 )
                {
                    children += String( ", " );
                }
                children += node->childNodeIds[i];
            }
            if( m_childrenText )
                m_childrenText->setValue( children );

            String ranges;
            for( u32 i = 0; i < node->blendRanges.size(); ++i )
            {
                if( i > 0 )
                {
                    ranges += String( "; " );
                }
                const auto &range = node->blendRanges[i];
                ranges += StringUtil::toString( range.parameterLow ) + String( " " ) +
                          StringUtil::toString( range.parameterHigh ) + String( " " ) +
                          StringUtil::toString( range.inputIndex0 ) + String( " " ) +
                          StringUtil::toString( range.inputIndex1 );
            }
            if( m_blendRangesText )
                m_blendRangesText->setValue( ranges );
            break;
        }
        case SelectionKind::Parameter:
        {
            const auto *parameter = getSelectedParameter();
            if( !parameter )
            {
                break;
            }
            if( m_idText )
                m_idText->setValue( parameter->name );
            if( m_isBoolToggle )
                m_isBoolToggle->setValue( parameter->isBool );
            if( m_liveParamValueSlider )
            {
                m_liveParamValueSlider->setValue( parameter->floatValue );
                m_liveParamValueSlider->setEnabled( !parameter->isBool );
            }
            if( m_liveParamBoolToggle )
            {
                m_liveParamBoolToggle->setValue( parameter->boolValue );
                m_liveParamBoolToggle->setEnabled( parameter->isBool );
            }
            break;
        }
        default:
            break;
        }
    }
    void AnimationGraphWindow::applyEditorFields()
    {
        switch( m_selectionKind )
        {
        case SelectionKind::State:
        {
            auto *state = getSelectedState();
            if( !state )
            {
                break;
            }
            if( m_clipIdText )
            {
                state->clipId = trim( m_clipIdText->getValue() );
            }
            if( m_poseNodeIdText )
            {
                state->poseNodeId = trim( m_poseNodeIdText->getValue() );
            }
            if( m_speedSlider )
            {
                state->speed = m_speedSlider->getValue();
            }
            break;
        }
        case SelectionKind::Transition:
        {
            auto *transition = getSelectedTransition();
            if( !transition )
            {
                break;
            }
            if( m_fromStateText )
            {
                transition->fromState = trim( m_fromStateText->getValue() );
            }
            if( m_toStateText )
            {
                transition->toState = trim( m_toStateText->getValue() );
            }
            if( m_durationSlider )
            {
                transition->duration = m_durationSlider->getValue();
            }
            if( m_exitTimeSlider )
            {
                transition->exitTime = m_exitTimeSlider->getValue();
            }
            if( m_prioritySlider )
            {
                transition->priority = static_cast<s32>( m_prioritySlider->getValue() );
            }
            if( m_syncToggle )
            {
                transition->synchronize = m_syncToggle->getValue();
            }

            if( !transition->conditions.empty() &&
                m_selectedConditionIndex < transition->conditions.size() )
            {
                auto &condition = transition->conditions[m_selectedConditionIndex];
                if( m_parameterText )
                {
                    condition.parameter = trim( m_parameterText->getValue() );
                }
                if( m_isBoolToggle )
                {
                    condition.isBoolean = m_isBoolToggle->getValue();
                }
                if( m_boolValueToggle )
                {
                    condition.booleanValue = m_boolValueToggle->getValue();
                }
                if( m_thresholdSlider )
                {
                    condition.threshold = m_thresholdSlider->getValue();
                }
                if( m_comparisonDropdown )
                {
                    condition.comparison = static_cast<animation::AnimationGraphComparison>(
                        m_comparisonDropdown->getSelectedOption() );
                }
            }
            break;
        }
        case SelectionKind::PoseNode:
        {
            auto *node = getSelectedPoseNode();
            if( !node )
            {
                break;
            }
            if( m_nodeTypeDropdown )
            {
                node->type = static_cast<animation::AnimationGraphPoseNodeType>(
                    m_nodeTypeDropdown->getSelectedOption() );
            }
            if( m_clipIdText )
            {
                node->clipId = trim( m_clipIdText->getValue() );
            }
            if( m_parameterText )
            {
                node->parameter = trim( m_parameterText->getValue() );
            }
            if( m_speedSlider )
            {
                node->speed = m_speedSlider->getValue();
            }
            if( m_childrenText )
            {
                node->childNodeIds.clear();
                const auto children = split( m_childrenText->getValue(), ',' );
                for( const auto &child : children )
                {
                    if( !child.empty() )
                    {
                        node->childNodeIds.push_back( child );
                    }
                }
            }
            if( m_blendRangesText )
            {
                node->blendRanges.clear();
                const auto rangeTexts = split( m_blendRangesText->getValue(), ';' );
                for( const auto &rangeText : rangeTexts )
                {
                    Array<String> parts;
                    for( const auto &token : split( rangeText, ' ' ) )
                    {
                        if( !token.empty() )
                        {
                            parts.push_back( token );
                        }
                    }
                    if( parts.size() >= 4 )
                    {
                        animation::AnimationGraphBlendRange range;
                        range.parameterLow = toFloat( parts[0], 0.0f );
                        range.parameterHigh = toFloat( parts[1], 0.0f );
                        range.inputIndex0 = toU32( parts[2], 0 );
                        range.inputIndex1 = toU32( parts[3], 0 );
                        node->blendRanges.push_back( range );
                    }
                }
            }
            break;
        }
        case SelectionKind::Parameter:
        {
            auto *parameter = getSelectedParameter();
            if( !parameter )
            {
                break;
            }
            if( m_isBoolToggle )
            {
                parameter->isBool = m_isBoolToggle->getValue();
            }
            break;
        }
        default:
            break;
        }
    }

    void AnimationGraphWindow::applyLiveParameter()
    {
        auto *parameter = getSelectedParameter();
        if( !parameter || !m_animator )
        {
            return;
        }
        auto *graph = m_animator->getAnimationGraph();
        if( !graph )
        {
            return;
        }
        if( parameter->isBool )
        {
            if( m_liveParamBoolToggle )
            {
                parameter->boolValue = m_liveParamBoolToggle->getValue();
            }
            graph->setBoolParameter( parameter->name, parameter->boolValue );
        }
        else
        {
            if( m_liveParamValueSlider )
            {
                parameter->floatValue = m_liveParamValueSlider->getValue();
            }
            graph->setFloatParameter( parameter->name, parameter->floatValue );
        }
    }

    void AnimationGraphWindow::applyEditedGraph()
    {
        if( !m_animator )
        {
            setStatus( "No animator selected." );
            return;
        }
        if( m_animator->setAnimationGraph( m_edited ) )
        {
            setStatus( "Graph applied to animator." );
        }
        else
        {
            setStatus( "Graph is invalid; nothing applied. Check states, clips and pose nodes." );
        }
    }

    void AnimationGraphWindow::addState()
    {
        String clipId;
        String poseNodeId;
        if( !m_edited.getClips().empty() )
        {
            clipId = m_edited.getClips().front().id;
        }
        else if( !m_edited.getPoseNodes().empty() )
        {
            poseNodeId = m_edited.getPoseNodes().front().id;
        }
        if( clipId.empty() && poseNodeId.empty() )
        {
            setStatus( "Add a clip or pose node before adding a state." );
            return;
        }

        animation::AnimationGraphState state;
        state.id = String( "State" ) + StringUtil::toString( m_edited.getStates().size() );
        state.clipId = clipId;
        state.poseNodeId = poseNodeId;
        state.speed = 1.0f;
        while( m_edited.findState( state.id ) )
        {
            state.id += String( "x" );
        }
        if( m_edited.addState( state ) )
        {
            m_selectedId = state.id;
            m_selectionKind = SelectionKind::State;
            rebuildTrees();
            syncEditor();
            setStatus( String( "Added state " ) + state.id );
        }
    }

    void AnimationGraphWindow::removeState()
    {
        if( m_selectionKind != SelectionKind::State || m_selectedId.empty() )
        {
            return;
        }
        if( m_edited.removeState( m_selectedId ) )
        {
            m_selectedId.clear();
            m_selectionKind = SelectionKind::None;
            rebuildTrees();
            syncEditor();
            setStatus( "Removed state." );
        }
    }

    void AnimationGraphWindow::addTransition()
    {
        const auto &states = m_edited.getStates();
        if( states.size() < 2 )
        {
            setStatus( "Add at least two states before adding a transition." );
            return;
        }
        animation::AnimationGraphTransition transition;
        transition.fromState = states[0].id;
        transition.toState = states[1].id;
        transition.duration = 0.15f;
        if( m_edited.addTransition( transition ) )
        {
            m_selectedTransitionIndex = static_cast<u32>( m_edited.getTransitions().size() ) - 1;
            m_selectionKind = SelectionKind::Transition;
            m_selectedConditionIndex = 0;
            m_selectedId = transition.fromState + String( " -> " ) + transition.toState;
            rebuildTrees();
            syncEditor();
            setStatus( "Added transition." );
        }
    }

    void AnimationGraphWindow::removeTransition()
    {
        if( m_selectionKind != SelectionKind::Transition )
        {
            return;
        }
        auto &transitions =
            const_cast<Array<animation::AnimationGraphTransition> &>( m_edited.getTransitions() );
        if( m_selectedTransitionIndex >= transitions.size() )
        {
            return;
        }
        transitions.erase( transitions.begin() + m_selectedTransitionIndex );
        m_selectedId.clear();
        m_selectionKind = SelectionKind::None;
        m_selectedTransitionIndex = 0;
        m_selectedConditionIndex = 0;
        rebuildTrees();
        syncEditor();
        setStatus( "Removed transition." );
    }

    void AnimationGraphWindow::addCondition()
    {
        auto *transition = getSelectedTransition();
        if( !transition )
        {
            setStatus( "Select a transition before adding a condition." );
            return;
        }
        animation::AnimationGraphCondition condition;
        condition.parameter = String( "param" );
        condition.comparison = animation::AnimationGraphComparison::Equal;
        condition.threshold = 0.0f;
        transition->conditions.push_back( condition );
        m_selectedConditionIndex = static_cast<u32>( transition->conditions.size() ) - 1;
        syncEditor();
        setStatus( "Added condition." );
    }

    void AnimationGraphWindow::removeCondition()
    {
        auto *transition = getSelectedTransition();
        if( !transition || transition->conditions.empty() )
        {
            return;
        }
        const auto index =
            std::min( m_selectedConditionIndex, static_cast<u32>( transition->conditions.size() ) - 1 );
        transition->conditions.erase( transition->conditions.begin() + index );
        if( !transition->conditions.empty() &&
            m_selectedConditionIndex >= transition->conditions.size() )
        {
            m_selectedConditionIndex = static_cast<u32>( transition->conditions.size() ) - 1;
        }
        else if( transition->conditions.empty() )
        {
            m_selectedConditionIndex = 0;
        }
        syncEditor();
        setStatus( "Removed condition." );
    }

    void AnimationGraphWindow::addPoseNode()
    {
        if( m_edited.getClips().empty() )
        {
            setStatus( "The graph needs at least one clip before adding pose nodes." );
            return;
        }
        animation::AnimationGraphPoseNodeDefinition node;
        node.id = String( "PoseNode" ) + StringUtil::toString( m_edited.getPoseNodes().size() );
        node.type = animation::AnimationGraphPoseNodeType::Clip;
        node.clipId = m_edited.getClips().front().id;
        node.speed = 1.0f;
        node.loop = true;
        while( m_edited.findPoseNode( node.id ) )
        {
            node.id += String( "x" );
        }
        if( m_edited.addPoseNode( node ) )
        {
            m_selectedId = node.id;
            m_selectionKind = SelectionKind::PoseNode;
            rebuildTrees();
            syncEditor();
            setStatus( String( "Added pose node " ) + node.id );
        }
    }

    void AnimationGraphWindow::removePoseNode()
    {
        if( m_selectionKind != SelectionKind::PoseNode || m_selectedId.empty() )
        {
            return;
        }
        if( m_edited.removePoseNode( m_selectedId ) )
        {
            m_selectedId.clear();
            m_selectionKind = SelectionKind::None;
            rebuildTrees();
            syncEditor();
            setStatus( "Removed pose node." );
        }
    }

    void AnimationGraphWindow::addParameter()
    {
        AuthoredParameter parameter;
        parameter.name = String( "Param" ) + StringUtil::toString( m_parameters.size() );
        parameter.isBool = false;
        parameter.floatValue = 0.0f;
        for( auto &existing : m_parameters )
        {
            if( existing.name == parameter.name )
            {
                parameter.name += String( "x" );
            }
        }
        m_parameters.push_back( parameter );
        m_selectedId = parameter.name;
        m_selectionKind = SelectionKind::Parameter;
        rebuildTrees();
        syncEditor();
        setStatus( String( "Added parameter " ) + parameter.name );
    }

    void AnimationGraphWindow::removeParameter()
    {
        if( m_selectionKind != SelectionKind::Parameter || m_selectedId.empty() )
        {
            return;
        }
        for( u32 i = 0; i < m_parameters.size(); ++i )
        {
            if( m_parameters[i].name == m_selectedId )
            {
                m_parameters.erase( m_parameters.begin() + i );
                break;
            }
        }
        m_selectedId.clear();
        m_selectionKind = SelectionKind::None;
        rebuildTrees();
        syncEditor();
        setStatus( "Removed parameter." );
    }
}  // namespace workphone::editor
