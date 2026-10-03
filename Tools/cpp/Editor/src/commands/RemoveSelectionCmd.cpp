#include <EditorPCH.hpp>
#include "RemoveSelectionCmd.hpp"
#include <editor/EditorManager.hpp>
#include <ui/ActorWindow.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    namespace
    {
        void refreshEditorWindows()
        {
            auto editorManager = EditorManager::getSingletonPtr();
            auto uiManager = editorManager ? editorManager->getUI() : nullptr;
            if( !uiManager )
            {
                return;
            }

            uiManager->rebuildSceneTree();
            if( auto actorWindow = uiManager->getActorWindow() )
            {
                actorWindow->updateSelection();
            }
        }

        bool containsActor( const Array<SmartPtr<scene::IGameActor>> &actors,
                            SmartPtr<scene::IGameActor> actor )
        {
            for( const auto &candidate : actors )
            {
                if( candidate && actor && candidate.get() == actor.get() )
                {
                    return true;
                }
            }

            return false;
        }

        bool hasSelectedAncestor( const Array<SmartPtr<scene::IGameActor>> &actors,
                                  SmartPtr<scene::IGameActor> actor )
        {
            auto parent = actor ? actor->getParent() : nullptr;
            while( parent )
            {
                if( containsActor( actors, parent ) )
                {
                    return true;
                }

                parent = parent->getParent();
            }

            return false;
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone::editor, RemoveSelectionCmd, Command );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, RemoveSelectionCmd::ActorData, ISharedObject );

    RemoveSelectionCmd::RemoveSelectionCmd()
    {
        // Scene and component destruction update editor-owned UI and must run on the primary task.
        setPrimary( true );
    }

    RemoveSelectionCmd::~RemoveSelectionCmd() = default;

    void RemoveSelectionCmd::undo()
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto sceneManager = applicationManager ? applicationManager->getGameManagerPtr() : nullptr;
        auto selectionManager = applicationManager ? applicationManager->getSelectionManagerPtr() : nullptr;
        auto scene = sceneManager ? sceneManager->getCurrentScenePtr() : nullptr;
        if( !sceneManager || !selectionManager || !scene )
        {
            return;
        }

        selectionManager->clearSelection();

        for( auto selectionData : m_actorData )
        {
            if( !selectionData || !selectionData->getActorData() )
            {
                continue;
            }

            auto actor = sceneManager->createActor();
            if( !actor )
            {
                continue;
            }

            auto data = selectionData->getActorData();
            actor->fromData( data );

            auto parent = selectionData->getParent();
            if( parent )
            {
                parent->addChild( actor );
            }
            else
            {
                scene->addActor( actor );
                scene->registerAllUpdates( actor );
            }

            selectionData->setActor( actor );
            selectionManager->addSelectedObject( actor );
        }

        for( auto &componentData : m_componentData )
        {
            if( componentData.actor && componentData.component )
            {
                componentData.actor->addComponentInstance( componentData.component );
                selectionManager->addSelectedObject( componentData.component );
            }
        }

        refreshEditorWindows();
    }

    void RemoveSelectionCmd::redo()
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto sceneManager = applicationManager ? applicationManager->getGameManagerPtr() : nullptr;
        auto selectionManager = applicationManager ? applicationManager->getSelectionManagerPtr() : nullptr;
        auto scene = sceneManager ? sceneManager->getCurrentScenePtr() : nullptr;
        if( !sceneManager || !selectionManager || !scene )
        {
            return;
        }

        selectionManager->clearSelection();

        auto componentOwners = Array<SmartPtr<scene::IGameActor>>();
        for( auto &componentData : m_componentData )
        {
            if( componentData.actor && componentData.component )
            {
                componentData.actor->removeComponentInstance( componentData.component );
                if( !containsActor( componentOwners, componentData.actor ) )
                {
                    componentOwners.emplace_back( componentData.actor );
                    selectionManager->addSelectedObject( componentData.actor );
                }
            }
        }

        for( auto selectionData : m_actorData )
        {
            if( !selectionData )
            {
                continue;
            }

            auto actor = selectionData->getActor();
            if( actor )
            {
                auto parent = actor->getParent();
                if( parent )
                {
                    parent->removeChild( actor );
                    selectionData->setParent( parent );
                }

                auto data = actor->toData();
                if( data )
                {
                    selectionData->setActorData( data );
                }

                if( !parent )
                {
                    scene->removeActor( actor );
                }

                sceneManager->destroyActor( actor );
                selectionData->setActor( nullptr );
            }
        }

        refreshEditorWindows();
    }

    void RemoveSelectionCmd::execute()
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        m_actorData.clear();
        m_componentData.clear();

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto factoryManager = applicationManager ? applicationManager->getFactoryManagerPtr() : nullptr;
        auto selectionManager = applicationManager ? applicationManager->getSelectionManagerPtr() : nullptr;

        auto sceneManager = applicationManager ? applicationManager->getGameManagerPtr() : nullptr;
        auto scene = sceneManager ? sceneManager->getCurrentScenePtr() : nullptr;
        if( !factoryManager || !selectionManager || !sceneManager || !scene )
        {
            return;
        }

        auto selection = selectionManager->getSelection();
        auto selectedActors = Array<SmartPtr<scene::IGameActor>>();
        for( auto selected : selection )
        {
            if( auto actor = workphone::dynamic_pointer_cast<scene::IGameActor>( selected ) )
            {
                selectedActors.emplace_back( actor );
            }
        }

        auto componentOwners = Array<SmartPtr<scene::IGameActor>>();
        for( auto selected : selection )
        {
            auto actor = workphone::dynamic_pointer_cast<scene::IGameActor>( selected );
            if( actor )
            {
                if( hasSelectedAncestor( selectedActors, actor ) )
                {
                    continue;
                }

                auto selectionData = factoryManager->make_ptr<ActorData>();
                selectionData->setActor( actor );

                auto parent = actor->getParent();
                if( parent )
                {
                    parent->removeChild( actor );
                    selectionData->setParent( parent );
                }

                auto data = actor->toData();
                if( data )
                {
                    selectionData->setActorData( data );
                }

                if( !parent )
                {
                    scene->removeActor( actor );
                }

                sceneManager->destroyActor( actor );
                selectionData->setActor( nullptr );

                m_actorData.push_back( selectionData );
            }
            else if( auto component =
                         workphone::dynamic_pointer_cast<scene::IComponent>( selected ) )
            {
                auto componentActor = component->getActor();
                if( componentActor && !containsActor( selectedActors, componentActor ) &&
                    !hasSelectedAncestor( selectedActors, componentActor ) )
                {
                    m_componentData.push_back( { componentActor, component } );
                    componentActor->removeComponentInstance( component );
                    if( !containsActor( componentOwners, componentActor ) )
                    {
                        componentOwners.emplace_back( componentActor );
                    }
                }
            }
        }

        selectionManager->clearSelection();
        for( const auto &actor : componentOwners )
        {
            selectionManager->addSelectedObject( actor );
        }

        refreshEditorWindows();
    }

    RemoveSelectionCmd::ActorData::ActorData() = default;

    RemoveSelectionCmd::ActorData::~ActorData() = default;

    SmartPtr<scene::IGameActor> RemoveSelectionCmd::ActorData::getParent() const
    {
        return m_parent;
    }

    void RemoveSelectionCmd::ActorData::setParent( SmartPtr<scene::IGameActor> parent )
    {
        m_parent = parent;
    }

    SmartPtr<scene::IGameActor> RemoveSelectionCmd::ActorData::getActor() const
    {
        return m_actor;
    }

    void RemoveSelectionCmd::ActorData::setActor( SmartPtr<scene::IGameActor> actor )
    {
        m_actor = actor;
    }

    SmartPtr<ISharedObject> RemoveSelectionCmd::ActorData::getActorData() const
    {
        return m_actorData;
    }

    void RemoveSelectionCmd::ActorData::setActorData( SmartPtr<ISharedObject> actorData )
    {
        m_actorData = actorData;
    }
}  // namespace workphone::editor
