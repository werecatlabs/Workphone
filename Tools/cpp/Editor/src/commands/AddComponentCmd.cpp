#include <EditorPCH.hpp>
#include <commands/AddComponentCmd.hpp>
#include <commands/AddActorCmd.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <ui/ActorWindow.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, AddComponentCmd, Command );

    AddComponentCmd::AddComponentCmd() = default;

    AddComponentCmd::~AddComponentCmd() = default;

    void AddComponentCmd::undo()
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        if( auto actor = getActor() )
        {
            if( auto component = getComponent() )
            {
                actor->removeComponentInstance( component );
            }
        }

        auto editorManager = EditorManager::getSingletonPtr();
        auto ui = editorManager->getUI();
        if( auto actorWindow = ui->getActorWindow() )
        {
            actorWindow->buildTree();
        }
    }

    void AddComponentCmd::redo()
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        if( auto actor = getActor() )
        {
            if( auto component = getComponent() )
            {
                actor->addComponentInstance( component );
            }
        }

        auto editorManager = EditorManager::getSingletonPtr();
        auto ui = editorManager->getUI();
        if( auto actorWindow = ui->getActorWindow() )
        {
            actorWindow->buildTree();
        }
    }

    void AddComponentCmd::execute()
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();

        auto selectionManager = applicationManager->getSelectionManager();
        WP_ASSERT( selectionManager );

        auto selection = selectionManager->getSelection();
        for( auto selected : selection )
        {
            if( selected )
            {
                if( selected->isDerived<scene::IGameActor>() )
                {
                    auto actor = workphone::static_pointer_cast<scene::IGameActor>( selected );
                    if( actor )
                    {
                        setActor( actor );

                        auto factory = getFactory();

                        auto component = factory->make_ptr<scene::IComponent>();
                        if( component )
                        {
                            sceneManager->loadObject( component );

                            actor->addComponentInstance( component );
                            setComponent( component );

                            if( applicationManager->isEditor() )
                            {
                                component->setState( scene::IComponent::State::Edit );
                            }
                            else
                            {
                                component->setState( scene::IComponent::State::Play );
                            }
                        }
                    }
                }
            }
        }

        auto editorManager = EditorManager::getSingletonPtr();
        auto ui = editorManager->getUI();
        if( auto actorWindow = ui->getActorWindow() )
        {
            actorWindow->buildTree();
        }
    }

    SmartPtr<IFactory> AddComponentCmd::getFactory() const
    {
        return m_factory;
    }

    void AddComponentCmd::setFactory( SmartPtr<IFactory> factory )
    {
        m_factory = factory;
    }

    SmartPtr<scene::IComponent> AddComponentCmd::getComponent() const
    {
        return m_component;
    }

    void AddComponentCmd::setComponent( SmartPtr<scene::IComponent> component )
    {
        m_component = component;
    }

    SmartPtr<scene::IGameActor> AddComponentCmd::getActor() const
    {
        return m_actor;
    }

    void AddComponentCmd::setActor( SmartPtr<scene::IGameActor> actor )
    {
        m_actor = actor;
    }
}  // namespace workphone::editor
