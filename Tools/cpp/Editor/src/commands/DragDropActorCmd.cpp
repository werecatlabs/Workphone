#include <EditorPCH.hpp>
#include <commands/DragDropActorCmd.hpp>
#include <editor/EditorManager.hpp>
#include <ui/ProjectTreeData.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    namespace
    {
        void moveRootActorToIndex( SmartPtr<scene::IGameScene> scene,
                                   SmartPtr<scene::IGameActor> actor, s32 index )
        {
            auto actors = scene->getActors();
            auto it = std::find( actors.begin(), actors.end(), actor );
            if( it == actors.end() )
            {
                return;
            }

            actors.erase( it );
            auto insertionIndex = std::min( static_cast<size_t>( index ), actors.size() );
            actors.insert( actors.begin() + insertionIndex, actor );
            scene->setActors( actors );
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone::editor, DragDropActorCmd, Command );

    DragDropActorCmd::DragDropActorCmd() = default;

    DragDropActorCmd::~DragDropActorCmd() = default;

    void DragDropActorCmd::undo()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto gameManager = applicationManager->getGameManagerPtr();
        auto gameScene = gameManager->getCurrentScene();

        if( !gameScene || ( m_actor && m_actor->getScene() != gameScene ) )
        {
            return;
        }

        if( m_actor )
        {
            if( m_actor->getParent() != m_previousParent )
            {
                if( auto parent = m_actor->getParent() )
                {
                    parent->removeChild( m_actor );
                }
                else
                {
                    gameScene->removeActor( m_actor );
                }

                if( m_previousParent )
                {
                    m_previousParent->addChild( m_actor );
                }
                else
                {
                    gameScene->addActor( m_actor );
                }
            }

            if( m_previousSiblingIndex >= 0 )
            {
                if( m_previousParent )
                {
                    m_actor->setSiblingIndex( m_previousSiblingIndex );
                }
                else
                {
                    moveRootActorToIndex( gameScene, m_actor, m_previousSiblingIndex );
                }
            }
        }

        gameManager->makeActorTransformsDirty();

        auto sceneActors = gameScene->getActors();
        for( auto actor : sceneActors )
        {
            actor->updateOrder();
        }

        auto editorManager = EditorManager::getSingletonPtr();
        auto ui = editorManager->getUI();
        ui->rebuildSceneTree();
    }

    void DragDropActorCmd::redo()
    {
        execute();
    }

    void DragDropActorCmd::execute()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto gameManager = applicationManager->getGameManagerPtr();
        auto gameScene = gameManager->getCurrentScene();

        if( m_dragActor )
        {
            auto destinationActor = m_destinationActor;
            if( !gameScene || m_dragActor->getScene() != gameScene ||
                ( destinationActor && destinationActor->getScene() != gameScene ) )
            {
                return;
            }

            auto roots = gameScene->getActors();
            const auto isInScene = [&roots]( SmartPtr<scene::IGameActor> actor ) {
                while( actor && actor->getParent() )
                {
                    actor = actor->getParent();
                }
                return std::find( roots.begin(), roots.end(), actor ) != roots.end();
            };
            if( !isInScene( m_dragActor ) ||
                ( destinationActor && !isInScene( destinationActor ) ) )
            {
                return;
            }

            // A parent cannot be moved into itself or any of its descendants.
            for( auto ancestor = destinationActor; ancestor; ancestor = ancestor->getParent() )
            {
                if( ancestor == m_dragActor )
                {
                    return;
                }
            }

            auto previousParent = m_dragActor->getParent();
            auto siblingIndex = getSiblingIndex();
            auto previousSiblingIndex = m_dragActor->getSiblingIndex();
            if( !previousParent )
            {
                auto it = std::find( roots.begin(), roots.end(), m_dragActor );
                previousSiblingIndex = static_cast<s32>( it - roots.begin() );
            }
            if( previousParent == destinationActor && siblingIndex >= 0 &&
                previousSiblingIndex < siblingIndex )
            {
                --siblingIndex;
            }
            if( previousParent == destinationActor &&
                ( siblingIndex < 0 || siblingIndex == previousSiblingIndex ) )
            {
                return;
            }

            m_previousParent = previousParent;
            m_previousSiblingIndex = previousSiblingIndex;
            m_actor = m_dragActor;

            if( previousParent != destinationActor )
            {
                if( previousParent )
                {
                    previousParent->removeChild( m_dragActor );
                }
                else
                {
                    gameScene->removeActor( m_dragActor );
                }

                if( destinationActor )
                {
                    destinationActor->addChild( m_dragActor );
                }
                else
                {
                    gameScene->addActor( m_dragActor );
                }
            }

            if( siblingIndex >= 0 )
            {
                if( destinationActor )
                {
                    auto children = destinationActor->getChildren();
                    if( !children.empty() )
                    {
                        auto targetIndex = std::min( static_cast<size_t>( siblingIndex ),
                                                     children.size() - 1 );
                        m_dragActor->setSiblingIndex( static_cast<s32>( targetIndex ) );
                    }
                }
                else
                {
                    moveRootActorToIndex( gameScene, m_dragActor, siblingIndex );
                }
            }

            gameManager->makeActorTransformsDirty();
            for( auto actor : gameScene->getActors() )
            {
                actor->updateOrder();
            }
            if( auto editorManager = EditorManager::getSingletonPtr() )
            {
                if( auto ui = editorManager->getUI() )
                {
                    ui->rebuildSceneTree();
                }
            }
            return;
        }

    }

    Vector2I DragDropActorCmd::getPosition() const
    {
        return m_position;
    }

    void DragDropActorCmd::setPosition( const Vector2I &position )
    {
        m_position = position;
    }

    SmartPtr<ui::IUIElement> DragDropActorCmd::getSrc() const
    {
        return m_src;
    }

    void DragDropActorCmd::setSrc( SmartPtr<ui::IUIElement> src )
    {
        m_src = src;
    }

    void DragDropActorCmd::setActor( SmartPtr<scene::IGameActor> actor )
    {
        m_dragActor = actor;
    }

    void DragDropActorCmd::setDestinationActor( SmartPtr<scene::IGameActor> actor )
    {
        m_destinationActor = actor;
    }

    SmartPtr<ui::IUIElement> DragDropActorCmd::getDst() const
    {
        return m_dst;
    }

    void DragDropActorCmd::setDst( SmartPtr<ui::IUIElement> dst )
    {
        m_dst = dst;
    }

    String DragDropActorCmd::getData() const
    {
        return m_data;
    }

    void DragDropActorCmd::setData( const String &data )
    {
        m_data = data;
    }

    s32 DragDropActorCmd::getSiblingIndex() const
    {
        return m_siblingIndex;
    }

    void DragDropActorCmd::setSiblingIndex( s32 siblingIndex )
    {
        m_siblingIndex = siblingIndex;
    }
}  // namespace workphone::editor
