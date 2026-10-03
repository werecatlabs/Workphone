#include <EditorPCH.hpp>
#include <ui/LayerDialog.hpp>
#include <ui/ActorWindow.hpp>
#include <ui/LayerManager.hpp>
#include <ui/UIManager.hpp>
#include <editor/EditorManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, LayerDialog, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, LayerDialog::UIElementListener, IEventListener );

    LayerDialog::LayerDialog() = default;

    LayerDialog::~LayerDialog()
    {
        unload( nullptr );
    }

    void LayerDialog::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUIPtr();
            WP_ASSERT( ui );

            auto parentWindow = ui->addElementByType<ui::IUIWindow>();
            WP_ASSERT( parentWindow );

            setParentWindow( parentWindow );
            parentWindow->setLabel( "Layers" );
            parentWindow->setSize( Vector2F( 360.0f, 320.0f ) );

            auto uiListener = workphone::make_ptr<UIElementListener>();
            uiListener->setOwner( this );
            m_uiListener = uiListener;

            m_layerName = ui->addElementByType<ui::IUILabelTextInputPair>();
            m_layerName->setLabel( "Layer" );
            m_layerName->setValue( "" );
            m_layerName->setElementId( LayerName );
            parentWindow->addChild( m_layerName );

            m_addLayerButton = ui->addElementByType<ui::IUIButton>();
            m_addLayerButton->setLabel( "Add Layer" );
            m_addLayerButton->setElementId( AddLayer );
            m_addLayerButton->addObjectListener( uiListener );
            parentWindow->addChild( m_addLayerButton );

            m_removeLayerButton = ui->addElementByType<ui::IUIButton>();
            m_removeLayerButton->setLabel( "Remove Layer" );
            m_removeLayerButton->setSameLine( true );
            m_removeLayerButton->setElementId( RemoveLayer );
            m_removeLayerButton->addObjectListener( uiListener );
            parentWindow->addChild( m_removeLayerButton );

            m_tree = ui->addElementByType<ui::IUITreeCtrl>();
            m_tree->setElementId( LayerTree );
            m_tree->setMultiSelect( false );
            m_tree->addObjectListener( uiListener );
            parentWindow->addChild( m_tree );

            populate();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void LayerDialog::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( !isLoaded() )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUIPtr();
            WP_ASSERT( ui );

            if( m_tree )
            {
                m_tree->removeObjectListener( m_uiListener );
                ui->removeElement( m_tree );
                m_tree = nullptr;
            }

            if( m_addLayerButton )
            {
                ui->removeElement( m_addLayerButton );
                m_addLayerButton = nullptr;
            }

            if( m_removeLayerButton )
            {
                ui->removeElement( m_removeLayerButton );
                m_removeLayerButton = nullptr;
            }

            if( m_layerName )
            {
                ui->removeElement( m_layerName );
                m_layerName = nullptr;
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

            EditorWindow::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void LayerDialog::populate()
    {
        if( !m_tree )
        {
            return;
        }

        m_tree->clear();

        auto editorManager = EditorManager::getSingletonPtr();
        auto uiManager = editorManager ? editorManager->getUI() : nullptr;
        auto layerManager = uiManager ? uiManager->getLayerManager() : nullptr;
        if( !layerManager )
        {
            return;
        }

        layerManager->refreshFromScene();

        auto root = m_tree->addRoot();
        WP_ASSERT( root );
        Util::setText( root, "Layers" );
        root->setExpanded( true );

        auto layers = layerManager->getLayers();
        for( const auto &layer : layers )
        {
            auto node = m_tree->addNode();
            WP_ASSERT( node );
            Util::setText( node, layer );
            root->addChild( node );
        }
    }

    void LayerDialog::setWindowVisible( bool visible )
    {
        if( visible )
        {
            populate();
        }

        EditorWindow::setWindowVisible( visible );
    }

    LayerDialog::UIElementListener::UIElementListener() = default;

    LayerDialog::UIElementListener::~UIElementListener() = default;

    Parameter LayerDialog::UIElementListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        auto owner = getOwner();
        if( !owner )
        {
            return {};
        }

        auto editorManager = EditorManager::getSingletonPtr();
        auto uiManager = editorManager ? editorManager->getUI() : nullptr;
        auto layerManager = uiManager ? uiManager->getLayerManager() : nullptr;
        if( !layerManager )
        {
            return {};
        }

        auto element = workphone::dynamic_pointer_cast<ui::IUIElement>( sender );
        auto elementId = element ? static_cast<WidgetId>( element->getElementId() ) : WidgetId::Count;

        if( eventValue == IEvent::handleSelection )
        {
            switch( elementId )
            {
            case WidgetId::LayerTree:
            {
                if( auto selectedNode = owner->m_tree->getSelectedTreeNode() )
                {
                    owner->m_layerName->setValue( selectedNode->getLabel() );
                }
            }
            break;
            case WidgetId::AddLayer:
            {
                layerManager->addLayer( owner->m_layerName->getValue() );
                owner->populate();
            }
            break;
            case WidgetId::RemoveLayer:
            {
                auto layerName = owner->m_layerName->getValue();
                if( auto selectedNode = owner->m_tree->getSelectedTreeNode() )
                {
                    layerName = selectedNode->getLabel();
                }

                layerManager->removeLayer( layerName );
                owner->m_layerName->setValue( "" );
                owner->populate();
            }
            break;
            default:
            {
            }
            break;
            }

            if( uiManager )
            {
                if( auto actorWindow = uiManager->getActorWindow() )
                {
                    actorWindow->refreshLayerDropdown();
                }
                uiManager->rebuildSceneTree();
            }
        }

        return {};
    }

    SmartPtr<LayerDialog> LayerDialog::UIElementListener::getOwner() const
    {
        return m_owner.lock();
    }

    void LayerDialog::UIElementListener::setOwner( SmartPtr<LayerDialog> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone::editor
