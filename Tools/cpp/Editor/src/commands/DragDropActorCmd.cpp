#include <EditorPCH.hpp>
#include <commands/DragDropActorCmd.hpp>
#include <editor/EditorManager.hpp>
#include <ui/ProjectTreeData.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{

    WP_CLASS_REGISTER_DERIVED( workphone::editor, DragDropActorCmd, Command );

    DragDropActorCmd::DragDropActorCmd() = default;

    DragDropActorCmd::~DragDropActorCmd() = default;

    void DragDropActorCmd::undo()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto gameManager = applicationManager->getGameManagerPtr();
        auto gameScene = gameManager->getCurrentScene();

        if( m_actor )
        {
            if( auto parent = m_actor->getParent() )
            {
                parent->removeChild( m_actor );
            }

            gameScene->removeActor( m_actor );

            if( m_previousParent )
            {
                m_previousParent->addChild( m_actor );
            }
            else
            {
                gameScene->addActor( m_actor );
            }

            if( m_previousSiblingIndex >= 0 )
            {
                m_actor->setSiblingIndex( m_previousSiblingIndex );
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

        auto dragSrc = getSrc();
        auto dropDst = getDst();

        auto isSrcTreeNode = false;
        auto isDstTreeNode = false;

        if( dragSrc )
        {
            isSrcTreeNode = dragSrc->isDerived<ui::IUITreeNode>();
        }

        if( dropDst )
        {
            isDstTreeNode = dropDst->isDerived<ui::IUITreeNode>();
        }

        if( isSrcTreeNode && isDstTreeNode )
        {
            auto dragSrcNode = workphone::static_pointer_cast<ui::IUITreeNode>( dragSrc );
            auto dropDstNode = workphone::static_pointer_cast<ui::IUITreeNode>( dropDst );

            auto dragSrcData = dragSrcNode->getNodeUserData();
            auto dropDstData = dropDstNode->getNodeUserData();

            if( dragSrcData && dropDstData )
            {
                auto dragSrcProjectData = workphone::static_pointer_cast<ProjectTreeData>( dragSrcData );

                auto dropDstProjectData = workphone::static_pointer_cast<ProjectTreeData>( dropDstData );

                auto dragSrcObject = dragSrcProjectData->getObjectData();
                auto dropDstObject = dropDstProjectData->getObjectData();

                if( dragSrcObject && dropDstObject )
                {
                    if( dragSrcObject->isDerived<scene::IGameActor>() &&
                        dropDstObject->isDerived<scene::IGameActor>() )
                    {
                        auto dragSrcActor =
                            workphone::static_pointer_cast<scene::IGameActor>( dragSrcObject );
                        auto dropDstActor =
                            workphone::static_pointer_cast<scene::IGameActor>( dropDstObject );

                        auto parent = dragSrcActor->getParent();
                        if( parent != dropDstActor )
                        {
                            m_actor = dragSrcActor;
                            m_previousParent = parent;
                            m_previousSiblingIndex = dragSrcActor->getSiblingIndex();

                            if( parent )
                            {
                                parent->removeChild( dragSrcActor );
                            }

                            gameScene->removeActor( dragSrcActor );
                            if( dropDstActor->getParent() == dragSrcActor )
                            {
                                auto srcParent = dropDstActor->getParent();
                                if( srcParent )
                                {
                                    srcParent->removeChild( dropDstActor );
                                }

                                auto dstParent = dropDstActor->getParent();
                                if( dstParent )
                                {
                                    dstParent->removeChild( dropDstActor );
                                }

                                auto parent = dragSrcActor->getParent();
                                if( !parent )
                                {
                                    WP_ASSERT( dropDstActor->getParent() == nullptr );
                                    gameScene->addActor( dropDstActor );
                                }
                                else
                                {
                                    parent->addChild( dropDstActor );
                                }
                            }

                            dropDstActor->addChild( dragSrcActor );
                        }
                    }
                }
            }
        }
        else if( dragSrc->isDerived<ui::IUIWindow>() )
        {
            auto selectionManager = applicationManager->getSelectionManager();
            auto selection = selectionManager->getSelection();
            for( auto selected : selection )
            {
                if( selected->isDerived<scene::IGameActor>() )
                {
                    auto actor = workphone::static_pointer_cast<scene::IGameActor>( selected );
                    auto parent = actor->getParent();
                    if( parent )
                    {
                        m_actor = actor;
                        m_previousParent = parent;
                        m_previousSiblingIndex = actor->getSiblingIndex();

                        parent->removeChild( actor );

                        gameScene->addActor( actor );
                    }
                }
            }
        }

        if( isSrcTreeNode )
        {
            auto dragSrcNode = workphone::static_pointer_cast<ui::IUITreeNode>( dragSrc );

            auto dragSrcData = dragSrcNode->getNodeUserData();
            if( dragSrcData )
            {
                auto dragSrcProjectData = workphone::static_pointer_cast<ProjectTreeData>( dragSrcData );

                auto dragSrcObject = dragSrcProjectData->getObjectData();
                if( dragSrcObject )
                {
                    if( dragSrcObject->isDerived<scene::IGameActor>() )
                    {
                        auto dragSrcActor =
                            workphone::static_pointer_cast<scene::IGameActor>( dragSrcObject );

                        auto siblingIndex = getSiblingIndex();
                        if( siblingIndex >= 0 )
                        {
                            dragSrcActor->setSiblingIndex( siblingIndex );
                        }
                    }
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
