#include <EditorPCH.hpp>
#include <ui/AnimationWindow.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, AnimationWindow, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, AnimationWindow::WindowListener, IEventListener );

    namespace
    {
        String getSolveStatusText( animation::TwoBoneIKStatus status )
        {
            switch( status )
            {
            case animation::TwoBoneIKStatus::Solved:
                return "Solved";
            case animation::TwoBoneIKStatus::InvalidEffector:
                return "Effector bone was not found";
            case animation::TwoBoneIKStatus::InvalidChain:
                return "Effector needs two parent bones";
            case animation::TwoBoneIKStatus::DegenerateChain:
                return "Chain contains a zero-length bone";
            default:
                return "Not solved";
            }
        }
    }  // namespace

    Parameter AnimationWindow::WindowListener::handleEvent( EventType eventType, hash_type eventValue,
                                                            const Array<Parameter> &arguments,
                                                            SmartPtr<ISharedObject> sender,
                                                            SmartPtr<ISharedObject> object,
                                                            SmartPtr<IEvent> event )
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
            case WidgetId::Rewind:
                if( owner->m_animator )
                    owner->m_animator->setAnimationTime( 0.0f );
                owner->syncPlaybackControls();
                break;
            case WidgetId::AddConstraint:
                owner->addConstraint();
                break;
            case WidgetId::RemoveConstraint:
                owner->removeConstraint();
                break;
            case WidgetId::ApplyConstraint:
                owner->applyConstraint();
                break;
            default:
                break;
            }
        }
        else if( eventValue == IEvent::handlePropertyChanged ||
                 eventValue == IEvent::handleValueChanged )
        {
            switch( id )
            {
            case WidgetId::ClipTree:
                owner->selectClip();
                break;
            case WidgetId::ConstraintTree:
                owner->selectConstraint();
                break;
            case WidgetId::Time:
            case WidgetId::Speed:
            case WidgetId::Weight:
            case WidgetId::Looping:
                owner->applyPlaybackControls();
                break;
            default:
                break;
            }
        }

        return {};
    }

    SmartPtr<AnimationWindow> AnimationWindow::WindowListener::getOwner() const
    {
        return m_owner.load().lock();
    }

    void AnimationWindow::WindowListener::setOwner( SmartPtr<AnimationWindow> owner )
    {
        m_owner = owner;
    }

    AnimationWindow::AnimationWindow()
    {
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );
    }

    AnimationWindow::~AnimationWindow()
    {
    }

    void AnimationWindow::load( SmartPtr<ISharedObject> data )
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
            window->setLabel( "Animation & IK Editor" );
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
            auto addVector = [&]( const String &label, WidgetId id ) {
                auto input = ui->addElementByType<ui::IUIVector3>();
                input->setLabel( label );
                input->setElementId( static_cast<hash_type>( id ) );
                input->addObjectListener( listener );
                window->addChild( input );
                return input;
            };

            addButton( "Refresh Selection", WidgetId::Refresh, false );
            addButton( "Play", WidgetId::Play, true );
            addButton( "Pause", WidgetId::Pause, true );
            addButton( "Stop", WidgetId::Stop, true );
            addButton( "Rewind", WidgetId::Rewind, true );

            m_statusText = ui->addElementByType<ui::IUIText>();
            window->addChild( m_statusText );

            m_clipTree = ui->addElementByType<ui::IUITreeCtrl>();
            m_clipTree->setLabel( "Clips" );
            m_clipTree->setElementId( static_cast<hash_type>( WidgetId::ClipTree ) );
            m_clipTree->addObjectListener( listener );
            window->addChild( m_clipTree );

            m_timeSlider = addSlider( "Timeline", WidgetId::Time, 0.0f, 1.0f );
            m_speedSlider = addSlider( "Playback Speed", WidgetId::Speed, 0.0f, 4.0f );
            m_weightSlider = addSlider( "Animation Weight", WidgetId::Weight, 0.0f, 1.0f );
            m_loopingToggle = addToggle( "Loop", WidgetId::Looping );

            m_constraintTree = ui->addElementByType<ui::IUITreeCtrl>();
            m_constraintTree->setLabel( "Two Bone IK Constraints" );
            m_constraintTree->setElementId( static_cast<hash_type>( WidgetId::ConstraintTree ) );
            m_constraintTree->addObjectListener( listener );
            window->addChild( m_constraintTree );

            addButton( "Add IK", WidgetId::AddConstraint, false );
            addButton( "Remove IK", WidgetId::RemoveConstraint, true );
            addButton( "Apply IK", WidgetId::ApplyConstraint, true );

            m_constraintIdText = addTextInput( "Constraint Name", WidgetId::ConstraintId );
            m_effectorBoneText = addTextInput( "Effector Bone", WidgetId::EffectorBone );
            m_targetPosition = addVector( "Target Position", WidgetId::TargetPosition );
            m_targetRotation = addVector( "Target Rotation", WidgetId::TargetRotation );
            m_poleTarget = addVector( "Pole Target", WidgetId::PoleTarget );
            m_constraintWeightSlider = addSlider( "IK Weight", WidgetId::ConstraintWeight, 0.0f, 1.0f );
            m_chainRotationWeightSlider =
                addSlider( "Chain Rotation", WidgetId::ChainRotationWeight, 0.0f, 1.0f );
            m_constraintEnabledToggle = addToggle( "Constraint Enabled", WidgetId::ConstraintEnabled );
            m_matchTargetOrientationToggle =
                addToggle( "Match Target Orientation", WidgetId::MatchTargetOrientation );
            m_poseBlendToggle = addToggle( "Blend Solved Pose", WidgetId::PoseBlend );

            EditorWindow::load( data );
            refresh();
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Unloaded );
        }
    }

    void AnimationWindow::unload( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() != LoadingState::Loaded )
        {
            return;
        }

        setLoadingState( LoadingState::Unloading );

        m_animator = nullptr;
        m_selectedConstraintId.clear();

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
        m_clipTree = nullptr;
        m_constraintTree = nullptr;
        m_timeSlider = nullptr;
        m_speedSlider = nullptr;
        m_weightSlider = nullptr;
        m_loopingToggle = nullptr;
        m_constraintIdText = nullptr;
        m_effectorBoneText = nullptr;
        m_targetPosition = nullptr;
        m_targetRotation = nullptr;
        m_poleTarget = nullptr;
        m_constraintWeightSlider = nullptr;
        m_chainRotationWeightSlider = nullptr;
        m_constraintEnabledToggle = nullptr;
        m_matchTargetOrientationToggle = nullptr;
        m_poseBlendToggle = nullptr;

        EditorWindow::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void AnimationWindow::update()
    {
        const auto selectedAnimator = findSelectedAnimator();
        if( selectedAnimator != m_animator )
        {
            refresh();
        }
        else if( m_animator && m_animator->isPlaying() )
        {
            syncPlaybackControls();
        }
    }

    void AnimationWindow::updateSelection()
    {
        refresh();
    }

    SmartPtr<scene::Animator> AnimationWindow::findSelectedAnimator() const
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

    void AnimationWindow::refresh()
    {
        m_animator = findSelectedAnimator();
        m_selectedConstraintId.clear();
        rebuildClipTree();
        rebuildConstraintTree();
        syncPlaybackControls();
        syncConstraintControls();

        if( m_animator )
        {
            if( auto graph = m_animator->getAnimationGraph() )
            {
                setStatus( String( "Animation graph state: " ) + graph->getCurrentState() );
            }
            else
            {
                setStatus( String( "Animator: " ) + m_animator->getSelectedAnimationName() );
            }
        }
        else
        {
            setStatus( "Select an actor with an Animator component." );
        }
    }

    void AnimationWindow::rebuildClipTree()
    {
        if( !m_clipTree )
        {
            return;
        }
        m_clipTree->clear();
        auto root = m_clipTree->addRoot();
        if( !root )
        {
            return;
        }
        root->setName( "Animation Clips" );

        if( !m_animator )
        {
            return;
        }

        if( auto graph = m_animator->getAnimationGraph() )
        {
            root->setName( "Animation Graph States" );
            const auto &states = graph->getDefinition().getStates();
            for( u32 i = 0; i < states.size(); ++i )
            {
                auto node = m_clipTree->addNode();
                node->setName( states[i].id + String( " [" ) + states[i].clipId + String( "]" ) );
                node->setTreeNodeId( i );
                root->addChild( node );
                if( states[i].id == graph->getCurrentState() )
                {
                    m_clipTree->setSelectedTreeNode( node );
                }
            }
            m_clipTree->expand( root );
            return;
        }

        const auto runtimeClips = m_animator->getAnimations();
        const auto editableClips = m_animator->getAnimationClips();
        const auto clipCount = std::max( runtimeClips.size(), editableClips.size() );
        for( u32 i = 0; i < clipCount; ++i )
        {
            String name;
            if( i < runtimeClips.size() && runtimeClips[i] )
            {
                name = runtimeClips[i]->getName();
            }
            else if( i < editableClips.size() && editableClips[i] )
            {
                name = editableClips[i]->getName();
            }
            if( name.empty() )
            {
                name = String( "Clip " ) + StringUtil::toString( i );
            }

            auto node = m_clipTree->addNode();
            node->setName( name );
            node->setTreeNodeId( i );
            root->addChild( node );
            if( i == m_animator->getSelectedAnimationIndex() )
            {
                m_clipTree->setSelectedTreeNode( node );
            }
        }
        m_clipTree->expand( root );
    }

    void AnimationWindow::rebuildConstraintTree()
    {
        if( !m_constraintTree )
        {
            return;
        }
        m_constraintTree->clear();
        auto root = m_constraintTree->addRoot();
        if( !root )
        {
            return;
        }
        root->setName( "Post-animation IK" );

        if( !m_animator )
        {
            return;
        }

        const auto &constraints = m_animator->getIKSystem().getConstraints();
        for( u32 i = 0; i < constraints.size(); ++i )
        {
            const auto &constraint = constraints[i];
            auto node = m_constraintTree->addNode();
            node->setName( constraint.id + String( " -> " ) + constraint.effectorBone );
            node->setTreeNodeId( i );
            root->addChild( node );
            if( constraint.id == m_selectedConstraintId )
            {
                m_constraintTree->setSelectedTreeNode( node );
            }
        }
        m_constraintTree->expand( root );
    }

    animation::TwoBoneIKConstraint *AnimationWindow::getSelectedConstraint()
    {
        if( !m_animator || m_selectedConstraintId.empty() )
        {
            return nullptr;
        }
        return m_animator->getIKSystem().findConstraint( m_selectedConstraintId );
    }

    const animation::TwoBoneIKConstraint *AnimationWindow::getSelectedConstraint() const
    {
        if( !m_animator || m_selectedConstraintId.empty() )
        {
            return nullptr;
        }
        return m_animator->getIKSystem().findConstraint( m_selectedConstraintId );
    }

    void AnimationWindow::syncPlaybackControls()
    {
        if( !m_animator )
        {
            if( m_timeSlider )
                m_timeSlider->setValue( 0.0f );
            return;
        }
        if( m_timeSlider )
        {
            auto timelineLength = m_animator->getSelectedAnimationLength();
            auto timelineTime = m_animator->getAnimationTime();
            if( auto graph = m_animator->getAnimationGraph() )
            {
                for( const auto &sample : graph->getSamples() )
                {
                    if( sample.stateId == graph->getCurrentState() )
                    {
                        timelineLength = sample.animation ? sample.animation->getLength() : 0.0f;
                        timelineTime = sample.time;
                        break;
                    }
                }
            }
            m_timeSlider->setMaxValue( std::max( 0.001f, timelineLength ) );
            m_timeSlider->setValue( timelineTime );
        }
        if( m_speedSlider )
            m_speedSlider->setValue( m_animator->getAnimationSpeed() );
        if( m_weightSlider )
            m_weightSlider->setValue( m_animator->getAnimationWeight() );
        if( m_loopingToggle )
            m_loopingToggle->setValue( m_animator->isLooping() );
    }

    void AnimationWindow::syncConstraintControls()
    {
        const auto constraint = getSelectedConstraint();
        if( !constraint )
        {
            if( m_constraintIdText )
                m_constraintIdText->setValue( "" );
            if( m_effectorBoneText )
                m_effectorBoneText->setValue( "" );
            return;
        }

        m_constraintIdText->setValue( constraint->id );
        m_effectorBoneText->setValue( constraint->effectorBone );
        m_targetPosition->setValue( constraint->targetTransform.getPosition() );
        m_targetRotation->setValue( constraint->targetTransform.getRotation() );
        m_poleTarget->setValue( constraint->settings.poleTarget );
        m_constraintWeightSlider->setValue( constraint->settings.blendWeight );
        m_chainRotationWeightSlider->setValue( constraint->settings.chainRotationWeight );
        m_constraintEnabledToggle->setValue( constraint->enabled );
        m_matchTargetOrientationToggle->setValue( constraint->settings.matchTargetOrientation );
        m_poseBlendToggle->setValue( constraint->settings.blendMode == animation::IKBlendMode::Pose );
    }

    void AnimationWindow::selectClip()
    {
        if( !m_animator || !m_clipTree )
        {
            return;
        }
        if( auto node = m_clipTree->getSelectedTreeNode() )
        {
            if( auto graph = m_animator->getAnimationGraph() )
            {
                const auto &states = graph->getDefinition().getStates();
                const auto index = node->getTreeNodeId();
                if( index < states.size() && graph->forceState( states[index].id ) )
                {
                    syncPlaybackControls();
                    setStatus( String( "Graph state: " ) + states[index].id );
                }
                return;
            }
            m_animator->setSelectedAnimationIndex( node->getTreeNodeId() );
            syncPlaybackControls();
            setStatus( String( "Clip: " ) + m_animator->getSelectedAnimationName() );
        }
    }

    void AnimationWindow::selectConstraint()
    {
        m_selectedConstraintId.clear();
        if( m_animator && m_constraintTree )
        {
            if( auto node = m_constraintTree->getSelectedTreeNode() )
            {
                const auto &constraints = m_animator->getIKSystem().getConstraints();
                const auto index = node->getTreeNodeId();
                if( index < constraints.size() )
                {
                    m_selectedConstraintId = constraints[index].id;
                }
            }
        }
        syncConstraintControls();
    }

    void AnimationWindow::applyPlaybackControls()
    {
        if( !m_animator )
        {
            return;
        }
        if( !m_animator->hasAnimationGraph() )
        {
            m_animator->setAnimationTime( m_timeSlider->getValue() );
        }
        m_animator->setAnimationSpeed( m_speedSlider->getValue() );
        m_animator->setAnimationWeight( m_weightSlider->getValue() );
        m_animator->setLooping( m_loopingToggle->getValue() );
    }

    void AnimationWindow::addConstraint()
    {
        if( !m_animator )
        {
            setStatus( "Select an Animator before adding IK." );
            return;
        }

        auto &system = m_animator->getIKSystem();
        animation::TwoBoneIKConstraint constraint;
        u32 suffix = static_cast<u32>( system.getConstraints().size() + 1 );
        do
        {
            constraint.id = String( "IKConstraint" ) + StringUtil::toString( suffix++ );
        } while( system.findConstraint( constraint.id ) );
        constraint.effectorBone = "effector";
        constraint.settings.poleTarget = Vector3<real_Num>::unitZ();

        if( system.addConstraint( constraint ) )
        {
            m_selectedConstraintId = constraint.id;
            rebuildConstraintTree();
            syncConstraintControls();
            setStatus( "IK constraint added. Choose its effector bone and target." );
        }
    }

    void AnimationWindow::removeConstraint()
    {
        if( m_animator && !m_selectedConstraintId.empty() &&
            m_animator->getIKSystem().removeConstraint( m_selectedConstraintId ) )
        {
            m_selectedConstraintId.clear();
            rebuildConstraintTree();
            syncConstraintControls();
            setStatus( "IK constraint removed." );
        }
    }

    void AnimationWindow::applyConstraint()
    {
        auto existing = getSelectedConstraint();
        if( !m_animator || !existing )
        {
            setStatus( "Select an IK constraint to apply changes." );
            return;
        }

        const auto original = *existing;
        auto edited = original;
        edited.id = m_constraintIdText->getValue();
        edited.effectorBone = m_effectorBoneText->getValue();
        edited.targetTransform.setPosition( m_targetPosition->getValue() );
        edited.targetTransform.setRotation( m_targetRotation->getValue() );
        edited.settings.poleTarget = m_poleTarget->getValue();
        edited.settings.blendWeight = m_constraintWeightSlider->getValue();
        edited.settings.chainRotationWeight = m_chainRotationWeightSlider->getValue();
        edited.settings.matchTargetOrientation = m_matchTargetOrientationToggle->getValue();
        edited.settings.blendMode = m_poseBlendToggle->getValue() ? animation::IKBlendMode::Pose
                                                                  : animation::IKBlendMode::Effector;
        edited.enabled = m_constraintEnabledToggle->getValue();

        auto &system = m_animator->getIKSystem();
        bool applied = false;
        if( edited.id == original.id )
        {
            applied = system.updateConstraint( edited );
        }
        else if( edited.isValid() && !system.findConstraint( edited.id ) )
        {
            system.removeConstraint( original.id );
            applied = system.addConstraint( edited );
            if( !applied )
            {
                system.addConstraint( original );
            }
        }

        if( applied )
        {
            m_selectedConstraintId = edited.id;
            rebuildConstraintTree();
            syncConstraintControls();
            m_animator->onUpdate();
            auto status = String( "IK constraint applied." );
            for( const auto &solveResult : m_animator->getLastIKSolveResults() )
            {
                if( solveResult.constraintId == edited.id )
                {
                    status = getSolveStatusText( solveResult.result.status );
                    if( solveResult.result.success )
                    {
                        status += solveResult.result.targetClamped ? String( " (target clamped, error " )
                                                                   : String( " (error " );
                        status +=
                            StringUtil::toString( solveResult.result.effectorError ) + String( ")" );
                    }
                    break;
                }
            }
            setStatus( status );
        }
        else
        {
            setStatus( "IK constraint is invalid or its name is already in use." );
        }
    }

    void AnimationWindow::setStatus( const String &status )
    {
        if( m_statusText )
        {
            m_statusText->setText( status );
        }
    }
}  // namespace workphone::editor
