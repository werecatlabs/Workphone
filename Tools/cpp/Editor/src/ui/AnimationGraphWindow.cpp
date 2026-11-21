#include <EditorPCH.hpp>
#include <ui/AnimationGraphWindow.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/Components/Animator.hpp>
#include <algorithm>
#include <cmath>
#include <map>
#include <utility>

#if WP_BUILD_IMGUI
#    include <WPImGui/ImGuiWindow.hpp>
#    include <imgui.h>
#endif

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, AnimationGraphWindow, EditorWindow );

#if WP_BUILD_IMGUI
    namespace animation_graph_ui
    {
        // The renderer can live in another module in a shared-engine build.
        struct ScopedImGuiContext
        {
            explicit ScopedImGuiContext( const ui::ImGuiWindow::ContentContext &frame ) :
                previous( ImGui::GetCurrentContext() )
            {
                ImGui::GetAllocatorFunctions( &allocate, &free, &allocatorData );
                ImGui::SetAllocatorFunctions( frame.allocate, frame.free, frame.allocatorData );
                ImGui::SetCurrentContext( frame.context );
            }

            ~ScopedImGuiContext()
            {
                ImGui::SetCurrentContext( previous );
                ImGui::SetAllocatorFunctions( allocate, free, allocatorData );
            }

            ImGuiContext *previous;
            ImGuiMemAllocFunc allocate;
            ImGuiMemFreeFunc free;
            void *allocatorData;
        };
    }
#endif

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

#if WP_BUILD_IMGUI
        if( auto native = workphone::dynamic_pointer_cast<ui::ImGuiWindow>( window ) )
        {
            auto properties = window->getProperties();
            properties->setProperty( "initialPlacement", String( "floating" ) );
            window->setProperties( properties );
            AtomicWeakPtr<AnimationGraphWindow> weak;
            weak = this;
            native->setContentRenderer(
                [weak]( const ui::ImGuiWindow::ContentContext &frame ) {
                    if( auto owner = weak.load().lock() )
                    {
                        const animation_graph_ui::ScopedImGuiContext scope( frame );
                        owner->draw();
                    }
                } );
        }
        else
#endif
        {
            auto message = uiManager->addElementByType<ui::IUIText>();
            message->setText( "The animation graph canvas requires the ImGui Editor." );
            window->addChild( message );
        }

        const std::lock_guard guard( m_mutex );
        setParentWindow( window );
        m_window = window;
    }

    void AnimationGraphWindow::unload( SmartPtr<ISharedObject> data )
    {
        SmartPtr<ui::IUIWindow> window;
        {
            const std::lock_guard guard( m_mutex );
            window = m_window;
            m_window = nullptr;
            m_animator = nullptr;
            m_edited.clear();
            m_availableClips.clear();
            m_liveChanges.clear();
            m_playback = {};
            m_action = Action::None;
        }
        if( window )
        {
            setParentWindow( nullptr );
            auto app = core::IApplicationManager::instancePtr();
            if( auto uiManager = app ? app->getUI() : nullptr )
            {
                uiManager->removeElement( window );
            }
            EditorWindow::unload( data );
        }
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

    void AnimationGraphWindow::loadFromAnimator( SmartPtr<scene::Animator> animator )
    {
        m_animator = std::move( animator );
        m_edited.clear();
        m_availableClips.clear();
        m_liveChanges.clear();
        m_playback = {};
        m_action = Action::None;
        m_status.clear();
        m_targetName.clear();
        m_ambiguousClipCount = 0;
        m_selectionChanged = false;
        m_dirty = false;
        m_graphLoadFailed = false;
        m_fitOnDraw = true;
        m_panning = false;
        m_selectedState.clear();
        m_selectedPoseNode.clear();
        m_poseDraft.reset();
        m_selectedTransition.reset();
        m_showInspector = false;
        m_linkSource.clear();
        m_draggedState.clear();

        if( !m_animator )
        {
            return;
        }

        if( auto actor = m_animator->getActor() )
        {
            m_targetName = actor->getName();
        }
        if( m_targetName.empty() )
        {
            m_targetName = "Animator";
        }

        try
        {
            m_graphLoadFailed = !m_animator->prepareAnimationGraphClips();
        }
        catch( const std::exception &error )
        {
            WP_LOG_EXCEPTION( error );
            m_graphLoadFailed = true;
        }
        if( const auto *graph = m_animator->getAnimationGraph() )
        {
            m_edited = graph->getDefinition();
            m_availableClips = m_edited.getClips();
            m_selectedState = m_edited.getInitialState();

            // Older graphs can refer to external parameters without declaring defaults.
            for( const auto &transition : m_edited.getTransitions() )
            {
                for( const auto &condition : transition.conditions )
                {
                    if( !m_edited.findParameter( condition.parameter ) )
                    {
                        m_edited.addParameter( { condition.parameter, condition.isBoolean } );
                    }
                }
            }
            for( const auto &node : m_edited.getPoseNodes() )
            {
                if( node.type != animation::AnimationGraphPoseNodeType::Clip &&
                    !m_edited.findParameter( node.parameter ) )
                {
                    const auto usesBool = node.type == animation::AnimationGraphPoseNodeType::Selector &&
                                          node.childNodeIds.size() == 2 &&
                                          graph->getBoolParameter( node.parameter, false ) ==
                                              graph->getBoolParameter( node.parameter, true );
                    m_edited.addParameter( { node.parameter, usesBool } );
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
                ++m_ambiguousClipCount;
                continue;
            }
            if( m_edited.findClip( name ) )
            {
                continue;
            }

            animation::AnimationGraphClip clip;
            clip.id = name;
            clip.animation = source;
            const auto descriptor = std::find_if( descriptors.begin(), descriptors.end(),
                                                   [&]( const auto &item ) {
                                                       return item && item->getName() == name;
                                                   } );
            if( descriptor != descriptors.end() )
            {
                clip.loop = ( *descriptor )->isLooping();
                clip.speed = std::max( 0.0f, ( *descriptor )->getSpeed() );
            }
            if( clip.isValid() )
            {
                m_availableClips.push_back( std::move( clip ) );
            }
        }

        if( m_graphLoadFailed )
        {
            m_status = m_animator->getPlaybackStatus();
            if( m_status.empty() )
            {
                m_status = "The animator's saved animation graph or clips could not be loaded.";
            }
        }
    }

    void AnimationGraphWindow::applyGraph()
    {
        try
        {
            if( m_edited.getStates().empty() )
            {
                m_animator->clearAnimationGraph();
                m_status = "Graph removed from Animator. Save the scene to keep this change.";
            }
            else if( m_animator->setAuthoredAnimationGraph( m_edited ) )
            {
                m_status = "Graph applied to Animator. Save the scene to keep this change.";
            }
            else
            {
                m_status = "Cannot apply this graph. Check clip assignments and pose node inputs.";
                return;
            }
            if( auto actor = m_animator->getActor() )
            {
                actor->setDirty( true );
            }
            m_dirty = false;
        }
        catch( const std::exception &error )
        {
            WP_LOG_EXCEPTION( error );
            m_status = "Applying the animation graph failed.";
        }
    }

    void AnimationGraphWindow::updatePlayback()
    {
        const auto *graph = m_animator->getAnimationGraph();
        m_playback.available = graph != nullptr;
        m_playback.playing = m_animator->isPlaying();
        m_playback.paused = m_animator->isPaused();
        if( !graph )
        {
            m_playback.state.clear();
            m_playback.destination.clear();
            m_playback.parameterValues.clear();
            m_playback.samples.clear();
            return;
        }

        m_playback.state = graph->getCurrentState();
        m_playback.destination = graph->getDestinationState();
        m_playback.progress = graph->getTransitionProgress();
        m_playback.samples = graph->getSamples();
        m_playback.parameterValues.clear();
        for( auto parameter : m_edited.getParameters() )
        {
            if( parameter.isBoolean )
            {
                parameter.booleanValue = graph->getBoolParameter( parameter.name, parameter.booleanValue );
            }
            else
            {
                parameter.floatValue = graph->getFloatParameter( parameter.name, parameter.floatValue );
            }
            m_playback.parameterValues.push_back( std::move( parameter ) );
        }
    }

    void AnimationGraphWindow::update()
    {
        const std::lock_guard guard( m_mutex );
        if( !m_window || !m_window->isVisible() )
        {
            return;
        }

        auto selected = findSelectedAnimator();
        m_selectionChanged = selected != m_animator;
        if( m_selectionChanged &&
            ( !m_dirty || !m_animator || !m_animator->getActor() ||
              !m_animator->isLoaded() || m_action == Action::Revert ) )
        {
            loadFromAnimator( std::move( selected ) );
        }

        if( !m_animator )
        {
            return;
        }

        const auto action = std::exchange( m_action, Action::None );
        switch( action )
        {
        case Action::Apply:
            applyGraph();
            break;
        case Action::Revert:
            loadFromAnimator( m_animator );
            break;
        case Action::Play:
            m_animator->play();
            break;
        case Action::Pause:
            m_animator->pause();
            break;
        case Action::Stop:
            m_animator->stop();
            break;
        default:
            break;
        }

        if( auto *graph = m_animator->getAnimationGraph() )
        {
            for( const auto &parameter : m_liveChanges )
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
        m_liveChanges.clear();
        updatePlayback();

        if( ( m_playback.available && m_playback.playing ) || action == Action::Stop )
        {
            auto app = core::IApplicationManager::instancePtr();
            auto window = app ? workphone::dynamic_pointer_cast<ui::IUIRenderWindow>(
                                    app->getSceneRenderWindow() ) : nullptr;
            auto texture = window ? window->getRenderTexture() : nullptr;
            if( auto target = texture ? texture->getRenderTarget() : nullptr )
            {
                target->update();
            }
        }
    }
}  // namespace workphone::editor
