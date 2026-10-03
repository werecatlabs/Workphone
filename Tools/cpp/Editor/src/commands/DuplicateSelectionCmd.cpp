#include <EditorPCH.hpp>
#include <commands/DuplicateSelectionCmd.hpp>
#include <commands/EditCommandsUtil.hpp>
#include <editor/EditorManager.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, DuplicateSelectionCmd, Command );

    DuplicateSelectionCmd::DuplicateSelectionCmd() = default;
    DuplicateSelectionCmd::~DuplicateSelectionCmd() = default;

    void DuplicateSelectionCmd::execute()
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        m_actorData.clear();
        m_createdActors.clear();

        auto applicationManager = core::IApplicationManager::instance();
        auto selectionManager = applicationManager->getSelectionManager();
        WP_ASSERT( selectionManager );

        const auto selection = selectionManager->getSelection();
        for( auto selected : selection )
        {
            auto actor = workphone::dynamic_pointer_cast<scene::IGameActor>( selected );
            if( !actor )
            {
                continue;
            }

            auto data = workphone::static_pointer_cast<Properties>( actor->toData() );
            if( !data )
            {
                continue;
            }

            prepareActorDataForDuplicate( data );
            m_actorData.push_back( data );
        }

        createActorsFromData();
    }

    void DuplicateSelectionCmd::undo()
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

    void DuplicateSelectionCmd::redo()
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        createActorsFromData();
    }

    void DuplicateSelectionCmd::createActorsFromData()
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
