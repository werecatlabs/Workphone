#include <EditorPCH.hpp>
#include <ui/CutsceneWindow.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, CutsceneWindow, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, CutsceneWindow::WindowListener, IEventListener );

    namespace
    {
        enum class WidgetId
        {
            None,
            NewButton,
            OpenButton,
            SaveButton,
            AddTrackButton,
            RemoveTrackButton,
            AddKeyframeButton,
            RemoveKeyframeButton,
            ApplyKeyframeButton,
            PlayPreviewButton,
            StopPreviewButton,
            TrackTree,
            KeyframeTree
        };

        String getDefaultCutscenePath()
        {
            auto editorManager = EditorManager::getSingletonPtr();
            if( !editorManager )
            {
                return String();
            }

            auto project = editorManager->getProject();
            if( !project )
            {
                return String();
            }

            auto mediaPaths = project->getMediaPaths();
            if( !mediaPaths.empty() )
            {
                return mediaPaths.front();
            }

            return project->getPath();
        }

        String makeCutsceneFileName( const String &basePath, const String &name )
        {
            if( basePath.empty() )
            {
                return name + String( ".cutscene" );
            }

            return basePath + String( "/" ) + name + String( ".cutscene" );
        }
    }  // namespace

    CutsceneWindow::WindowListener::WindowListener() = default;
    CutsceneWindow::WindowListener::~WindowListener() = default;

    Parameter CutsceneWindow::WindowListener::handleEvent( EventType eventType, hash_type eventValue,
                                                           const Array<Parameter> &arguments,
                                                           SmartPtr<ISharedObject> sender,
                                                           SmartPtr<ISharedObject> object,
                                                           SmartPtr<IEvent> event )
    {
        if( eventType != EventType::UI )
        {
            return {};
        }

        if( eventValue != IEvent::handleSelection && eventValue != IEvent::handlePropertyChanged )
        {
            return {};
        }

        auto owner = getOwner();
        if( !owner )
        {
            return {};
        }

        if( !sender || !sender->isDerived<ui::IUIElement>() )
        {
            return {};
        }

        auto element = workphone::static_pointer_cast<ui::IUIElement>( sender );
        auto id = static_cast<WidgetId>( element->getElementId() );

        if( eventValue == IEvent::handleSelection )
        {
            switch( id )
            {
            case WidgetId::NewButton:
                owner->newCutscene();
                break;
            case WidgetId::OpenButton:
                owner->openCutscene();
                break;
            case WidgetId::SaveButton:
                owner->saveCutscene();
                break;
            case WidgetId::AddTrackButton:
                owner->addTrack();
                break;
            case WidgetId::RemoveTrackButton:
                owner->removeSelectedTrack();
                break;
            case WidgetId::AddKeyframeButton:
                owner->addKeyframe();
                break;
            case WidgetId::RemoveKeyframeButton:
                owner->removeSelectedKeyframe();
                break;
            case WidgetId::ApplyKeyframeButton:
                owner->applyKeyframeEdit();
                break;
            case WidgetId::PlayPreviewButton:
                owner->playPreview();
                break;
            case WidgetId::StopPreviewButton:
                owner->stopPreview();
                break;
            default:
                break;
            }
        }
        else if( eventValue == IEvent::handlePropertyChanged )
        {
            if( id == WidgetId::TrackTree )
            {
                owner->rebuildKeyframeTree();
                owner->syncEditFields();
            }
            else if( id == WidgetId::KeyframeTree )
            {
                owner->syncEditFields();
            }
        }

        return {};
    }

    SmartPtr<CutsceneWindow> CutsceneWindow::WindowListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void CutsceneWindow::WindowListener::setOwner( SmartPtr<CutsceneWindow> owner )
    {
        m_owner = owner;
    }

    CutsceneWindow::CutsceneWindow()
    {
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );
    }

    CutsceneWindow::~CutsceneWindow() = default;

    void CutsceneWindow::load( SmartPtr<ISharedObject> data )
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
            if( window )
            {
                window->setLabel( "Cutscene Editor" );
                window->setHasBorder( true );
                setParentWindow( window );
                m_window = window;
                window->addObjectListener( m_windowListener );

                auto addButton = [&]( const String &label, WidgetId widgetId, bool sameLine ) {
                    auto button = ui->addElementByType<ui::IUIButton>();
                    button->setLabel( label );
                    button->setElementId( static_cast<hash_type>( widgetId ) );
                    button->setSameLine( sameLine );
                    window->addChild( button );
                    button->addObjectListener( m_windowListener );
                    return button;
                };

                addButton( "New", WidgetId::NewButton, false );
                addButton( "Open", WidgetId::OpenButton, true );
                addButton( "Save", WidgetId::SaveButton, true );
                addButton( "Add Track", WidgetId::AddTrackButton, true );
                addButton( "Remove Track", WidgetId::RemoveTrackButton, true );
                addButton( "Add Keyframe", WidgetId::AddKeyframeButton, true );
                addButton( "Remove Keyframe", WidgetId::RemoveKeyframeButton, true );
                addButton( "Apply", WidgetId::ApplyKeyframeButton, true );
                addButton( "Play", WidgetId::PlayPreviewButton, true );
                addButton( "Stop", WidgetId::StopPreviewButton, true );

                auto filePathText = ui->addElementByType<ui::IUITextEntry>();
                filePathText->setLabel( "File" );
                filePathText->setText(
                    makeCutsceneFileName( getDefaultCutscenePath(), String( "NewCutscene" ) ) );
                filePathText->setElementId( static_cast<hash_type>( WidgetId::None ) );
                window->addChild( filePathText );
                m_filePathText = filePathText;

                auto lengthText = ui->addElementByType<ui::IUITextEntry>();
                lengthText->setLabel( "Length (s)" );
                lengthText->setText( "10.0" );
                lengthText->setElementId( static_cast<hash_type>( WidgetId::None ) );
                lengthText->addObjectListener( m_windowListener );
                window->addChild( lengthText );
                m_lengthText = lengthText;

                auto trackTree = ui->addElementByType<ui::IUITreeCtrl>();
                trackTree->setLabel( "Tracks" );
                trackTree->setElementId( static_cast<hash_type>( WidgetId::TrackTree ) );
                window->addChild( trackTree );
                trackTree->addObjectListener( m_windowListener );
                m_trackTree = trackTree;

                auto keyframeTree = ui->addElementByType<ui::IUITreeCtrl>();
                keyframeTree->setLabel( "Keyframes" );
                keyframeTree->setElementId( static_cast<hash_type>( WidgetId::KeyframeTree ) );
                window->addChild( keyframeTree );
                keyframeTree->addObjectListener( m_windowListener );
                m_keyframeTree = keyframeTree;

                auto targetActorText = ui->addElementByType<ui::IUITextEntry>();
                targetActorText->setLabel( "Target Actor" );
                targetActorText->setText( "" );
                targetActorText->setElementId( static_cast<hash_type>( WidgetId::None ) );
                window->addChild( targetActorText );
                m_targetActorText = targetActorText;

                auto trackTypeText = ui->addElementByType<ui::IUITextEntry>();
                trackTypeText->setLabel( "Track Type (Position/Rotation/Scale/CameraFOV)" );
                trackTypeText->setText( "Position" );
                trackTypeText->setElementId( static_cast<hash_type>( WidgetId::None ) );
                window->addChild( trackTypeText );
                m_trackTypeText = trackTypeText;

                auto timeText = ui->addElementByType<ui::IUITextEntry>();
                timeText->setLabel( "Time (s)" );
                timeText->setText( "0.0" );
                timeText->setElementId( static_cast<hash_type>( WidgetId::None ) );
                window->addChild( timeText );
                m_timeText = timeText;

                auto valueVector = ui->addElementByType<ui::IUIVector3>();
                valueVector->setLabel( "Vector Value" );
                valueVector->setValue( Vector3<real_Num>::zero() );
                window->addChild( valueVector );
                m_valueVector = valueVector;

                auto valueScalarText = ui->addElementByType<ui::IUITextEntry>();
                valueScalarText->setLabel( "Scalar Value (FOV)" );
                valueScalarText->setText( "45.0" );
                valueScalarText->setElementId( static_cast<hash_type>( WidgetId::None ) );
                window->addChild( valueScalarText );
                m_valueScalarText = valueScalarText;
            }

            newCutscene();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CutsceneWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( getLoadingState() == LoadingState::Loaded )
            {
                setLoadingState( LoadingState::Unloading );

                stopPreview();
                m_previewPlayer = nullptr;
                m_cutscene = nullptr;

                setParentWindow( nullptr );

                auto applicationManager = core::IApplicationManager::instance();
                if( applicationManager )
                {
                    auto ui = applicationManager->getUI();
                    if( ui )
                    {
                        if( m_window )
                        {
                            ui->removeElement( m_window );
                            m_window = nullptr;
                        }
                    }
                }

                EditorWindow::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CutsceneWindow::update()
    {
        tickPreview();
    }

    SmartPtr<scene::Cutscene> CutsceneWindow::getCutscene() const
    {
        return m_cutscene;
    }

    void CutsceneWindow::setCutscene( SmartPtr<scene::Cutscene> cutscene )
    {
        m_cutscene = cutscene;
        rebuildTrackTree();
        rebuildKeyframeTree();
    }

    void CutsceneWindow::newCutscene()
    {
        stopPreview();
        auto cutscene = workphone::make_ptr<scene::Cutscene>();
        cutscene->setName( "NewCutscene" );
        cutscene->setLength( 10.0f );
        m_cutscene = cutscene;

        if( m_filePathText )
        {
            m_filePathText->setText(
                makeCutsceneFileName( getDefaultCutscenePath(), String( "NewCutscene" ) ) );
        }

        if( m_lengthText )
        {
            m_lengthText->setText( "10.0" );
        }

        rebuildTrackTree();
        rebuildKeyframeTree();
    }

    void CutsceneWindow::openCutscene()
    {
        if( !m_filePathText )
        {
            return;
        }

        auto filePath = m_filePathText->getText();
        if( filePath.empty() )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
        {
            return;
        }

        auto fileSystem = applicationManager->getFileSystemPtr();
        if( !fileSystem )
        {
            return;
        }

        auto cutscene = workphone::make_ptr<scene::Cutscene>();
        cutscene->loadFromFile( filePath );

        {
            stopPreview();
            m_cutscene = cutscene;

            if( m_lengthText )
            {
                m_lengthText->setText( StringUtil::toString( m_cutscene->getLength() ) );
            }

            rebuildTrackTree();
            rebuildKeyframeTree();
        }
    }

    void CutsceneWindow::saveCutscene()
    {
        if( !m_cutscene )
        {
            return;
        }

        applyKeyframeEdit();

        if( m_lengthText )
        {
            f32 length = 10.0f;
            StringUtil::parseFloat( m_lengthText->getText(), length );
            m_cutscene->setLength( length );
        }

        if( !m_filePathText )
        {
            return;
        }

        auto filePath = m_filePathText->getText();
        if( filePath.empty() )
        {
            return;
        }

        m_cutscene->saveToFile( filePath );
    }

    void CutsceneWindow::addTrack()
    {
        if( !m_cutscene )
        {
            return;
        }

        scene::Cutscene::Track track;
        if( m_targetActorText )
        {
            track.targetActorName = m_targetActorText->getText();
        }
        if( m_trackTypeText )
        {
            track.type = scene::Cutscene::stringToTrackType( m_trackTypeText->getText() );
        }

        m_cutscene->addTrack( track );
        rebuildTrackTree();
    }

    void CutsceneWindow::removeSelectedTrack()
    {
        if( !m_cutscene )
        {
            return;
        }

        auto index = getSelectedTrackIndex();
        if( index >= 0 )
        {
            m_cutscene->removeTrack( index );
            rebuildTrackTree();
            rebuildKeyframeTree();
        }
    }

    void CutsceneWindow::addKeyframe()
    {
        if( !m_cutscene )
        {
            return;
        }

        auto tracks = m_cutscene->getTracks();
        auto trackIndex = getSelectedTrackIndex();
        if( trackIndex < 0 || trackIndex >= static_cast<s32>( tracks.size() ) )
        {
            return;
        }

        scene::Cutscene::Keyframe keyframe;
        if( m_timeText )
        {
            StringUtil::parseFloat( m_timeText->getText(), keyframe.time );
        }
        if( m_valueVector )
        {
            keyframe.vectorValue = m_valueVector->getValue();
        }
        if( m_valueScalarText )
        {
            StringUtil::parseFloat( m_valueScalarText->getText(), keyframe.scalarValue );
        }

        tracks[trackIndex].keyframes.push_back( keyframe );
        m_cutscene->setTracks( tracks );
        rebuildKeyframeTree();
    }

    void CutsceneWindow::removeSelectedKeyframe()
    {
        if( !m_cutscene )
        {
            return;
        }

        auto tracks = m_cutscene->getTracks();
        auto trackIndex = getSelectedTrackIndex();
        auto keyframeIndex = getSelectedKeyframeIndex();

        if( trackIndex >= 0 && trackIndex < static_cast<s32>( tracks.size() ) && keyframeIndex >= 0 &&
            keyframeIndex < static_cast<s32>( tracks[trackIndex].keyframes.size() ) )
        {
            tracks[trackIndex].keyframes.erase( tracks[trackIndex].keyframes.begin() + keyframeIndex );
            m_cutscene->setTracks( tracks );
            rebuildKeyframeTree();
        }
    }

    void CutsceneWindow::applyKeyframeEdit()
    {
        if( !m_cutscene )
        {
            return;
        }

        auto tracks = m_cutscene->getTracks();
        auto trackIndex = getSelectedTrackIndex();
        auto keyframeIndex = getSelectedKeyframeIndex();

        if( trackIndex >= 0 && trackIndex < static_cast<s32>( tracks.size() ) && keyframeIndex >= 0 &&
            keyframeIndex < static_cast<s32>( tracks[trackIndex].keyframes.size() ) )
        {
            auto &keyframe = tracks[trackIndex].keyframes[keyframeIndex];
            if( m_timeText )
            {
                StringUtil::parseFloat( m_timeText->getText(), keyframe.time );
            }
            if( m_valueVector )
            {
                keyframe.vectorValue = m_valueVector->getValue();
            }
            if( m_valueScalarText )
            {
                StringUtil::parseFloat( m_valueScalarText->getText(), keyframe.scalarValue );
            }

            m_cutscene->setTracks( tracks );
            rebuildKeyframeTree();
        }
    }

    void CutsceneWindow::rebuildTrackTree()
    {
        if( !m_trackTree )
        {
            return;
        }

        m_trackTree->clear();

        if( !m_cutscene )
        {
            return;
        }

        auto root = m_trackTree->addRoot();
        if( !root )
        {
            return;
        }

        auto tracks = m_cutscene->getTracks();
        for( s32 i = 0; i < static_cast<s32>( tracks.size() ); ++i )
        {
            auto node = m_trackTree->addNode();
            if( node )
            {
                auto label = tracks[i].targetActorName + String( " - " ) +
                             scene::Cutscene::trackTypeToString( tracks[i].type );
                node->setName( label );
                node->setTreeNodeId( static_cast<u32>( i ) );
                root->addChild( node );
            }
        }
    }

    void CutsceneWindow::rebuildKeyframeTree()
    {
        if( !m_keyframeTree )
        {
            return;
        }

        m_keyframeTree->clear();

        auto trackIndex = getSelectedTrackIndex();
        if( trackIndex < 0 || !m_cutscene )
        {
            return;
        }

        auto tracks = m_cutscene->getTracks();
        if( trackIndex >= static_cast<s32>( tracks.size() ) )
        {
            return;
        }

        auto root = m_keyframeTree->addRoot();
        if( !root )
        {
            return;
        }

        const auto &keyframes = tracks[trackIndex].keyframes;
        for( s32 i = 0; i < static_cast<s32>( keyframes.size() ); ++i )
        {
            auto node = m_keyframeTree->addNode();
            if( node )
            {
                node->setName( StringUtil::toString( keyframes[i].time ) + String( "s" ) );
                node->setTreeNodeId( static_cast<u32>( i ) );
                root->addChild( node );
            }
        }
    }

    s32 CutsceneWindow::getSelectedTrackIndex() const
    {
        if( !m_trackTree )
        {
            return -1;
        }

        auto selected = m_trackTree->getSelectedTreeNodes();
        if( selected.empty() || !selected.front() )
        {
            return -1;
        }

        return static_cast<s32>( selected.front()->getTreeNodeId() );
    }

    s32 CutsceneWindow::getSelectedKeyframeIndex() const
    {
        if( !m_keyframeTree )
        {
            return -1;
        }

        auto selected = m_keyframeTree->getSelectedTreeNodes();
        if( selected.empty() || !selected.front() )
        {
            return -1;
        }

        return static_cast<s32>( selected.front()->getTreeNodeId() );
    }

    void CutsceneWindow::syncEditFields()
    {
        if( !m_cutscene )
        {
            return;
        }

        auto tracks = m_cutscene->getTracks();
        auto trackIndex = getSelectedTrackIndex();
        auto keyframeIndex = getSelectedKeyframeIndex();

        if( trackIndex >= 0 && trackIndex < static_cast<s32>( tracks.size() ) )
        {
            const auto &track = tracks[trackIndex];
            if( m_targetActorText )
            {
                m_targetActorText->setText( track.targetActorName );
            }
            if( m_trackTypeText )
            {
                m_trackTypeText->setText( scene::Cutscene::trackTypeToString( track.type ) );
            }

            if( keyframeIndex >= 0 && keyframeIndex < static_cast<s32>( track.keyframes.size() ) )
            {
                const auto &keyframe = track.keyframes[keyframeIndex];
                if( m_timeText )
                {
                    m_timeText->setText( StringUtil::toString( keyframe.time ) );
                }
                if( m_valueVector )
                {
                    m_valueVector->setValue( keyframe.vectorValue );
                }
                if( m_valueScalarText )
                {
                    m_valueScalarText->setText( StringUtil::toString( keyframe.scalarValue ) );
                }
            }
        }
    }

    void CutsceneWindow::playPreview()
    {
        if( !m_cutscene )
        {
            return;
        }

        stopPreview();
        m_previewTime = 0.0f;
        m_previewPlaying = true;

        m_previewPlayer = workphone::make_ptr<scene::CutscenePlayer>();
        m_previewPlayer->setCutscene( m_cutscene );
    }

    void CutsceneWindow::stopPreview()
    {
        m_previewPlaying = false;
        m_previewTime = 0.0f;
        if( m_previewPlayer )
        {
            m_previewPlayer->stop();
            m_previewPlayer = nullptr;
        }
    }

    void CutsceneWindow::tickPreview()
    {
        if( !m_previewPlaying || !m_previewPlayer || !m_cutscene )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            return;
        }

        auto timer = applicationManager->getTimerPtr();
        if( !timer )
        {
            return;
        }

        m_previewTime += static_cast<f32>( timer->getDeltaTime() );
        if( m_previewTime > m_cutscene->getLength() )
        {
            if( m_cutscene->isLooping() )
            {
                m_previewTime = std::fmod( m_previewTime, m_cutscene->getLength() );
            }
            else
            {
                m_previewTime = m_cutscene->getLength();
                m_previewPlaying = false;
            }
        }

        m_previewPlayer->setCurrentTime( m_previewTime );
        m_previewPlayer->play();

        Map<Pair<String, scene::Cutscene::TrackType>, scene::Cutscene::Keyframe> values;
        m_cutscene->evaluate( m_previewTime, values );

        auto sceneManager = applicationManager->getGameManagerPtr();
        if( !sceneManager )
        {
            return;
        }

        for( const auto &entry : values )
        {
            auto actor = sceneManager->getActorByName( entry.first.first );
            if( actor )
            {
                m_previewPlayer->applyValue( actor, entry.first.second, entry.second );
            }
        }
    }
}  // namespace workphone::editor
