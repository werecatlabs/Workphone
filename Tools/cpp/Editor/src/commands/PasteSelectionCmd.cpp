#include <EditorPCH.hpp>
#include <commands/PasteSelectionCmd.hpp>
#include <commands/EditCommandsUtil.hpp>
#include <editor/EditorManager.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, PasteSelectionCmd, Command );

    PasteSelectionCmd::PasteSelectionCmd() = default;
    PasteSelectionCmd::~PasteSelectionCmd() = default;

    void PasteSelectionCmd::execute()
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        m_actorData.clear();
        m_createdActors.clear();

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        for( auto data : UIManager::getClipboardActorData() )
        {
            if( !data )
            {
                continue;
            }

            auto copiedData = factoryManager->make_ptr<Properties>( *data );
            m_actorData.push_back( copiedData );
        }

        for( auto data : m_actorData )
        {
            if( data )
            {
                prepareActorDataForDuplicate( data );
            }
        }

        createActorsFromData();
    }

    void PasteSelectionCmd::undo()
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        for( auto actor : m_createdActors )
        {
            if( !actor )
            {
                continue;
            }

            scene->removeActor( actor );

            if( auto parent = actor->getParent() )
            {
                parent->removeChild( actor );
            }

            sceneManager->destroyActor( actor );
        }

        m_createdActors.clear();

        auto editorManager = EditorManager::getSingletonPtr();
        auto uiManager = editorManager->getUI();
        if( uiManager )
        {
            uiManager->rebuildSceneTree();
        }
    }

    void PasteSelectionCmd::redo()
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        createActorsFromData();
    }

    void PasteSelectionCmd::createActorsFromData()
    {
        m_createdActors.clear();

        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        for( auto data : m_actorData )
        {
            if( !data )
            {
                continue;
            }

            auto actor = sceneManager->createActor();
            if( !actor )
            {
                continue;
            }

            actor->fromData( data );

            scene->addActor( actor );
            scene->registerAllUpdates( actor );
            if( applicationManager->isPlaying() )
            {
                actor->setState( scene::IGameActor::State::Play );
            }
            else
            {
                actor->setState( scene::IGameActor::State::Edit );
            }

            m_createdActors.push_back( actor );
        }

        auto editorManager = EditorManager::getSingletonPtr();
        auto uiManager = editorManager->getUI();
        if( uiManager )
        {
            uiManager->rebuildSceneTree();
        }
    }
}  // namespace workphone::editor
