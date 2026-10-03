#include <EditorPCH.hpp>
#include <ui/ObjectBrowserDialog.hpp>
#include <ui/ProjectTreeData.hpp>
#include <editor/EditorManager.hpp>
#include "ui/ActorWindow.hpp"
#include "ui/UIManager.hpp"
#include "commands/AddComponentCmd.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone, ObjectBrowserDialog, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone, ObjectBrowserDialog::UIElementListener, IEventListener );

    ObjectBrowserDialog::ObjectBrowserDialog() = default;

    ObjectBrowserDialog::~ObjectBrowserDialog()
    {
        unload( nullptr );
    }

    void ObjectBrowserDialog::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            auto parent = getParent();

            auto parentWindow = ui->addElementByType<ui::IUIWindow>();
            if( parentWindow )
            {
                setParentWindow( parentWindow );
                parentWindow->setLabel( "ObjectBrowserDialogChild" );

                if( parent )
                {
                    parent->addChild( parentWindow );
                }

                auto uiListener = workphone::make_ptr<UIElementListener>();
                uiListener->setOwner( this );
                m_uiListener = uiListener;

                auto addComponentButton = ui->addElementByType<ui::IUIButton>();
                WP_ASSERT( addComponentButton );

                addComponentButton->setLabel( "Add Component" );
                parentWindow->addChild( addComponentButton );
                addComponentButton->addObjectListener( uiListener );
                addComponentButton->setElementId( AddComponent );
                m_addComponentButton = addComponentButton;

                auto tree = ui->addElementByType<ui::IUITreeCtrl>();
                parentWindow->addChild( tree );
                tree->setMultiSelect( false );
                tree->addObjectListener( uiListener );
                tree->setElementId( Tree );
                m_tree = tree;

                populate();
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ObjectBrowserDialog::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            if( m_tree )
            {
                m_tree->removeObjectListener( m_uiListener );

                ui->removeElement( m_tree );
                m_tree = nullptr;
            }

            if( m_addComponentButton )
            {
                ui->removeElement( m_addComponentButton );
                m_addComponentButton = nullptr;
            }

            if( auto parentWindow = getParentWindow() )
            {
                ui->removeElement( parentWindow );
                setParentWindow( nullptr );
            }

            if( m_uiListener )
            {
                m_uiListener->unload( nullptr );
                m_uiListener = nullptr;
            }

            for( auto data : m_dataArray )
            {
                data->unload( nullptr );
            }

            m_dataArray.clear();

            EditorWindow::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ObjectBrowserDialog::populate()
    {
        if( !m_tree )
        {
            return;
        }

        if( m_tree )
        {
            m_tree->clear();
        }

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        auto factories = factoryManager->getFactories();

        std::sort( factories.begin(), factories.end(), []( SmartPtr<IFactory> a, SmartPtr<IFactory> b ) {
            return a->getObjectTypeName() < b->getObjectTypeName();
        } );

        auto rootNode = m_tree->addRoot();
        WP_ASSERT( rootNode );
        rootNode->setExpanded( true );

        for( auto factory : factories )
        {
            if( factory->isObjectDerivedFrom<scene::IComponent>() )
            {
                auto name = factory->getObjectTypeName();

                auto treeNode = m_tree->addNode();

                WP_ASSERT( treeNode );
                Util::setText( treeNode, name );

                auto data =
                    factoryManager->make_ptr<ProjectTreeData>( "factory", "factory", factory, factory );
                treeNode->setNodeUserData( data );

                rootNode->addChild( treeNode );
            }
        }
    }

    String ObjectBrowserDialog::getSelectedObject() const
    {
        return m_selectedObject;
    }

    void ObjectBrowserDialog::setSelectedObject( const String &selectedObject )
    {
        m_selectedObject = selectedObject;
    }

    SmartPtr<ui::IUITreeCtrl> ObjectBrowserDialog::getTree() const
    {
        return m_tree;
    }

    void ObjectBrowserDialog::setTree( SmartPtr<ui::IUITreeCtrl> tree )
    {
        m_tree = tree;
    }

    void ObjectBrowserDialog::setWindowVisible( bool visible )
    {
        if( visible )
        {
            populate();
        }

        EditorWindow::setWindowVisible( visible );
    }

    Parameter ObjectBrowserDialog::UIElementListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::handleSelection )
        {
            auto owner = getOwner();
            auto tree = owner->getTree();
            auto selectedNode = tree->getSelectedTreeNode();

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            auto commandManager = applicationManager->getCommandManager();

            auto userData = selectedNode->getNodeUserData();
            auto projectData = workphone::static_pointer_cast<ProjectTreeData>( userData );

            auto objectData = projectData->getObjectData();
            auto factory = workphone::static_pointer_cast<IFactory>( objectData );

            auto cmd = factoryManager->make_ptr<AddComponentCmd>();
            cmd->setFactory( factory );
            commandManager->addCommand( cmd );
        }

        return {};
    }

    SmartPtr<ObjectBrowserDialog> ObjectBrowserDialog::UIElementListener::getOwner() const
    {
        auto p = m_owner.lock();
        return p;
    }

    void ObjectBrowserDialog::UIElementListener::setOwner( SmartPtr<ObjectBrowserDialog> owner )
    {
        m_owner = owner;
    }

    ObjectBrowserDialog::UIElementListener::UIElementListener() = default;

    ObjectBrowserDialog::UIElementListener::~UIElementListener() = default;
}  // namespace workphone::editor
