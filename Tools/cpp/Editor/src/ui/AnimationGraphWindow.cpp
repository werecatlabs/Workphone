#include <EditorPCH.hpp>
#include <ui/AnimationGraphWindow.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/Components/Animator.hpp>
#include <algorithm>
#include <map>
#include <utility>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, AnimationGraphWindow, EditorWindow );

    AnimationGraphWindow::~AnimationGraphWindow()
    {
        unload( nullptr );
    }

    void AnimationGraphWindow::load( SmartPtr<ISharedObject> data )
    {
        auto app = core::IApplicationManager::instance();
        auto uiManager = app ? app->getUI() : nullptr;
        if( !uiManager )
        {
            return;
        }

        EditorWindow::load( data );
        auto window = uiManager->addElementByType<ui::IUIWindow>();
        if( !window )
        {
            EditorWindow::unload( data );
            return;
        }

        window->setLabel( "Animation Graph Editor" );
        window->setSize( { 1080.0f, 690.0f } );
        window->setVisible( false, false );

        auto graphUI = uiManager->addElementByType<ui::IUIAnimationGraph>();
        if( graphUI )
        {
            if( auto properties = window->getProperties() )
            {
                properties->setProperty( "initialPlacement", String( "floating" ) );
                window->setProperties( properties );
            }
            window->addChild( graphUI );
        }
        else if( auto message = uiManager->addElementByType<ui::IUIText>() )
        {
            message->setText( "The animation graph canvas requires the ImGui Editor." );
            window->addChild( message );
        }

        const std::lock_guard guard( m_windowMutex );
        setParentWindow( window );
        m_graphUI = graphUI;
    }

    void AnimationGraphWindow::unload( SmartPtr<ISharedObject> data )
    {
        SmartPtr<ui::IUIWindow> window;
        {
            const std::lock_guard guard( m_windowMutex );
            window = getParentWindow();
            setParentWindow( nullptr );
            if( m_graphUI )
            {
                auto &model = m_graphUI->getModel();
                const std::lock_guard modelLock( model.mutex );
                loadFromAnimator( nullptr, model );
            }
            m_graphUI = nullptr;
        }
        if( window )
        {
            auto app = core::IApplicationManager::instancePtr();
            if( auto uiManager = app ? app->getUI() : nullptr )
            {
                uiManager->removeElement( window );
            }
        }
        EditorWindow::unload( data );
    }

    SmartPtr<scene::Animator> AnimationGraphWindow::findSelectedAnimator() const
    {
        auto app = core::IApplicationManager::instancePtr();
        auto selection = app ? app->getSelectionManagerPtr() : nullptr;
        if( !selection )
        {
            return nullptr;
        }

        for( const auto &selected : selection->getSelection() )
        {
            if( !selected )
            {
                continue;
            }
            if( auto animator = workphone::dynamic_pointer_cast<scene::Animator>( selected ) )
            {
                return animator;
            }

            auto actor = workphone::dynamic_pointer_cast<scene::IGameActor>( selected );
            if( !actor )
            {
                if( auto component = workphone::dynamic_pointer_cast<scene::IComponent>( selected ) )
                {
                    actor = component->getActor();
                }
            }
            if( actor )
            {
                if( auto animator = actor->getComponent<scene::Animator>() )
                {
                    return animator;
                }
            }
        }
        return nullptr;
    }

    void AnimationGraphWindow::loadFromAnimator( SmartPtr<scene::Animator> animator, Model &model )
    {
        m_animator = std::move( animator );
        model.edited.clear();
        model.availableClips.clear();
        model.liveChanges.clear();
        model.playback = {};
        model.action = ui::IUIAnimationGraph::Action::None;
        model.status.clear();
        model.targetName.clear();
        model.ambiguousClipCount = 0;
        model.selectionChanged = false;
        model.dirty = false;
        model.resetView = true;

        if( !m_animator )
        {
            return;
        }

        if( auto actor = m_animator->getActor() )
        {
            model.targetName = actor->getName();
        }
        if( model.targetName.empty() )
        {
            model.targetName = "Animator";
        }

        bool graphLoadFailed = false;
        try
        {
            graphLoadFailed = !m_animator->prepareAnimationGraphClips();
        }
        catch( const std::exception &error )
        {
            WP_LOG_EXCEPTION( error );
            graphLoadFailed = true;
        }
        if( const auto *graph = m_animator->getAnimationGraph() )
        {
            model.edited = graph->getDefinition();
            model.availableClips = model.edited.getClips();

            // Older graphs can refer to external parameters without declaring defaults.
            for( const auto &transition : model.edited.getTransitions() )
            {
                for( const auto &condition : transition.conditions )
                {
                    if( !model.edited.findParameter( condition.parameter ) )
                    {
                        model.edited.addParameter( { condition.parameter, condition.isBoolean } );
                    }
                }
            }
            for( const auto &node : model.edited.getPoseNodes() )
            {
                if( node.type != animation::AnimationGraphPoseNodeType::Clip &&
                    !model.edited.findParameter( node.parameter ) )
                {
                    const auto usesBool = node.type == animation::AnimationGraphPoseNodeType::Selector &&
                                          node.childNodeIds.size() == 2 &&
                                          graph->getBoolParameter( node.parameter, false ) ==
                                              graph->getBoolParameter( node.parameter, true );
                    model.edited.addParameter( { node.parameter, usesBool } );
                }
            }
        }

        const auto animations = m_animator->getAnimations();
        const auto descriptors = m_animator->getAnimationClips();
        std::map<String, size_t> counts;
        for( const auto &source : animations )
        {
            if( source )
            {
                ++counts[source->getName()];
            }
        }
        for( const auto &source : animations )
        {
            if( !source )
            {
                continue;
            }
            const auto name = source->getName();
            if( name.empty() || counts[name] > 1 )
            {
                ++model.ambiguousClipCount;
                continue;
            }
            if( model.edited.findClip( name ) )
            {
                continue;
            }

            animation::AnimationGraphClip clip;
            clip.id = name;
            clip.animation = source;
            const auto descriptor =
                std::find_if( descriptors.begin(), descriptors.end(),
                              [&]( const auto &item ) { return item && item->getName() == name; } );
            if( descriptor != descriptors.end() )
            {
                clip.loop = ( *descriptor )->isLooping();
                clip.speed = std::max( 0.0f, ( *descriptor )->getSpeed() );
            }
            if( clip.isValid() )
            {
                model.availableClips.push_back( std::move( clip ) );
            }
        }

        if( graphLoadFailed )
        {
            model.status = m_animator->getPlaybackStatus();
            if( model.status.empty() )
            {
                model.status = "The animator's saved animation graph or clips could not be loaded.";
            }
        }
    }

    void AnimationGraphWindow::applyGraph( Model &model )
    {
        try
        {
            if( model.edited.getStates().empty() )
            {
                m_animator->clearAnimationGraph();
                model.status = "Graph removed from Animator. Save the scene to keep this change.";
            }
            else if( m_animator->setAuthoredAnimationGraph( model.edited ) )
            {
                model.status = "Graph applied to Animator. Save the scene to keep this change.";
            }
            else
            {
                model.status = "Cannot apply this graph. Check clip assignments and pose node inputs.";
                return;
            }
            if( auto actor = m_animator->getActor() )
            {
                actor->setDirty( true );
            }
            model.dirty = false;
        }
        catch( const std::exception &error )
        {
            WP_LOG_EXCEPTION( error );
            model.status = "Applying the animation graph failed.";
        }
    }

    void AnimationGraphWindow::updatePlayback( Model &model )
    {
        const auto *graph = m_animator->getAnimationGraph();
        model.playback.available = graph != nullptr;
        model.playback.playing = m_animator->isPlaying();
        model.playback.paused = m_animator->isPaused();
        if( !graph )
        {
            model.playback.state.clear();
            model.playback.destination.clear();
            model.playback.parameterValues.clear();
            model.playback.samples.clear();
            return;
        }

        model.playback.state = graph->getCurrentState();
        model.playback.destination = graph->getDestinationState();
        model.playback.progress = graph->getTransitionProgress();
        model.playback.samples = graph->getSamples();
        model.playback.parameterValues.clear();
        for( auto parameter : model.edited.getParameters() )
        {
            if( parameter.isBoolean )
            {
                parameter.booleanValue =
                    graph->getBoolParameter( parameter.name, parameter.booleanValue );
            }
            else
            {
                parameter.floatValue = graph->getFloatParameter( parameter.name, parameter.floatValue );
            }
            model.playback.parameterValues.push_back( std::move( parameter ) );
        }
    }

    void AnimationGraphWindow::update()
    {
        const std::lock_guard guard( m_windowMutex );
        auto parent = getParentWindow();
        if( !parent || !parent->isVisible() || !m_graphUI )
        {
            return;
        }

        auto &model = m_graphUI->getModel();
        std::unique_lock modelLock( model.mutex );
        auto selected = findSelectedAnimator();
        model.selectionChanged = selected != m_animator;
        if( model.selectionChanged &&
            ( !model.dirty || !m_animator || !m_animator->getActor() || !m_animator->isLoaded() ||
              model.action == ui::IUIAnimationGraph::Action::Revert ) )
        {
            loadFromAnimator( std::move( selected ), model );
        }

        if( !m_animator )
        {
            return;
        }

        const auto action = std::exchange( model.action, ui::IUIAnimationGraph::Action::None );
        switch( action )
        {
        case ui::IUIAnimationGraph::Action::Apply:
            applyGraph( model );
            break;
        case ui::IUIAnimationGraph::Action::Revert:
            loadFromAnimator( m_animator, model );
            break;
        case ui::IUIAnimationGraph::Action::Play:
            m_animator->play();
            break;
        case ui::IUIAnimationGraph::Action::Pause:
            m_animator->pause();
            break;
        case ui::IUIAnimationGraph::Action::Stop:
            m_animator->stop();
            break;
        default:
            break;
        }

        if( auto *graph = m_animator->getAnimationGraph() )
        {
            for( const auto &parameter : model.liveChanges )
            {
                if( parameter.isBoolean )
                {
                    graph->setBoolParameter( parameter.name, parameter.booleanValue );
                }
                else
                {
                    graph->setFloatParameter( parameter.name, parameter.floatValue );
                }
            }
        }
        model.liveChanges.clear();
        updatePlayback( model );

        const bool refreshPreview = ( model.playback.available && model.playback.playing ) ||
                                    action == ui::IUIAnimationGraph::Action::Stop;
        modelLock.unlock();
        if( refreshPreview )
        {
            auto app = core::IApplicationManager::instancePtr();
            auto window =
                app ? workphone::dynamic_pointer_cast<ui::IUIRenderWindow>( app->getSceneRenderWindow() )
                    : nullptr;
            auto texture = window ? window->getRenderTexture() : nullptr;
            if( auto target = texture ? texture->getRenderTarget() : nullptr )
            {
                target->update();
            }
        }
    }
}  // namespace workphone::editor
