#include <EditorPCH.hpp>
#include <ui/ActorWindow.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <editor/EditorManager.hpp>
#include <ui/EventsWindow.hpp>
#include <ui/ProjectTreeData.hpp>
#include <ui/PropertiesWindow.hpp>
#include <ui/SceneWindow.hpp>
#include <ui/ScriptWindow.hpp>
#include <ui/TransformWindow.hpp>
#include <ui/UIManager.hpp>
#include <ui/CollisionMaskDialog.hpp>
#include <ui/CollisionMaskManager.hpp>
#include <ui/ObjectBrowserDialog.hpp>
#include <ui/MaterialWindow.hpp>
#include <ui/TerrainWindow.hpp>
#include <ui/LayerDialog.hpp>
#include <ui/LayerManager.hpp>
#include <ui/TagDialog.hpp>
#include <ui/TagManager.hpp>
#include <commands/RemoveSelectionCmd.hpp>
#include <Workphone/Workphone.hpp>
#include <string>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, ActorWindow, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, ActorWindow::UIElementListener, IEventListener );

    namespace
    {
        String joinTags( const Array<String> &tags )
        {
            auto result = String();

            for( const auto &tag : tags )
            {
                if( StringUtil::isNullOrEmpty( tag ) )
                {
                    continue;
                }

                if( !result.empty() )
                {
                    result += ", ";
                }

                result += tag;
            }

            return result;
        }

        SmartPtr<scene::IGameActor> getActorFromSelection( SmartPtr<ISharedObject> selected )
        {
            if( !selected )
            {
                return nullptr;
            }

            if( selected->isDerived<scene::IGameActor>() )
            {
                return workphone::static_pointer_cast<scene::IGameActor>( selected );
            }

            if( selected->isDerived<scene::IComponent>() )
            {
                auto component = workphone::static_pointer_cast<scene::IComponent>( selected );
                return component ? component->getActor() : nullptr;
            }

            return nullptr;
        }
    }  // namespace

    ActorWindow::ActorWindow( SmartPtr<ui::IUIWindow> parent )
    {
        setParent( parent );
    }

    void ActorWindow::setActorName( const String &textStr )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            return;
        }

        auto selectionManager = applicationManager->getSelectionManagerPtr();
        if( !selectionManager )
        {
            return;
        }

        auto editorManager = EditorManager::getSingletonPtr();
        if( !editorManager )
        {
            return;
        }

        auto ui = editorManager->getUI();
        if( !ui )
        {
            return;
        }

        auto selection = selectionManager->getSelection();
        for( auto selected : selection )
        {
            if( auto actor = getActorFromSelection( selected ) )
            {
                auto name = actor->getName();
                if( name != textStr )
                {
                    actor->setName( textStr );

                    if( auto sceneWindow = ui->getSceneWindow() )
                    {
                        sceneWindow->buildTree();
                    }
                }
            }
        }
    }

    void ActorWindow::updateObjectSelection( SmartPtr<ISharedObject> object )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto selectionManager = applicationManager ? applicationManager->getSelectionManagerPtr() : nullptr;
        if( !selectionManager || !object )
        {
            return;
        }

        selectionManager->clearSelection();
        selectionManager->addSelectedObject( object );
        updateDetailsForObject( object );
    }

    void ActorWindow::updateDetailsForObject( SmartPtr<ISharedObject> object )
    {
        auto component = workphone::dynamic_pointer_cast<scene::IComponent>( object );
        if( component )
        {
            if( component->isDerived<scene::Button>() )
            {
                m_objectType = ObjectType::Button;
            }
            else if( component->isDerived<scene::Material>() )
            {
                m_objectType = ObjectType::Material;
            }
            else if( component->isDerived<scene::TerrainSystem>() )
            {
                m_objectType = ObjectType::Terrain;
            }
            else if( component->isDerived<scene::ParticleSystem>() )
            {
                m_objectType = ObjectType::Particle;
            }
            else
            {
                m_objectType = ObjectType::None;
            }

            switch( m_objectType )
            {
            case ObjectType::Button:
            {
                m_particleSystemWindow->setWindowVisible( false );
                m_materialWindow->setWindowVisible( false );
                m_propertiesWindow->setWindowVisible( true );
                m_eventsWindow->setWindowVisible( true );
                m_terrainWindow->setWindowVisible( false );

                m_propertiesWindow->updateSelection();
                m_eventsWindow->updateSelection();
            }
            break;
            case ObjectType::Material:
            {
                m_particleSystemWindow->setWindowVisible( false );
                m_materialWindow->setWindowVisible( true );
                m_propertiesWindow->setWindowVisible( false );
                m_eventsWindow->setWindowVisible( false );
                m_terrainWindow->setWindowVisible( false );

                m_materialWindow->updateSelection();
            }
            break;
            case ObjectType::Particle:
            {
                m_particleSystemWindow->setWindowVisible( true );
                m_materialWindow->setWindowVisible( false );
                m_propertiesWindow->setWindowVisible( false );
                m_eventsWindow->setWindowVisible( false );
                m_terrainWindow->setWindowVisible( false );

                m_propertiesWindow->updateSelection();
                m_particleSystemWindow->updateSelection();
            }
            break;
            case ObjectType::Terrain:
            {
                m_particleSystemWindow->setWindowVisible( false );
                m_materialWindow->setWindowVisible( false );
                m_propertiesWindow->setWindowVisible( true );
                m_eventsWindow->setWindowVisible( false );
                m_terrainWindow->setWindowVisible( true );

                m_propertiesWindow->updateSelection();
                m_terrainWindow->updateSelection();
            }
            break;
            default:
            {
                m_particleSystemWindow->setWindowVisible( false );
                m_materialWindow->setWindowVisible( false );
                m_propertiesWindow->setWindowVisible( true );
                m_eventsWindow->setWindowVisible( false );
                m_terrainWindow->setWindowVisible( false );

                m_propertiesWindow->updateSelection();
                m_eventsWindow->updateSelection();
            }
            }
        }
        else
        {
            m_particleSystemWindow->setWindowVisible( false );
            m_materialWindow->setWindowVisible( false );
            m_propertiesWindow->setWindowVisible( true );
            m_eventsWindow->setWindowVisible( false );
            m_terrainWindow->setWindowVisible( false );

            m_propertiesWindow->updateSelection();
        }
    }

    ActorWindow::~ActorWindow()
    {
        unload( nullptr );
    }

    void ActorWindow::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( getLoadingState() == LoadingState::Loading ||
                getLoadingState() == LoadingState::Loaded )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            auto parent = getParent();

            auto parentWindow = ui->addElementByType<ui::IUIWindow>();
            WP_ASSERT( parentWindow );

            setParentWindow( parentWindow );
            parentWindow->setLabel( "Inspector" );

            if( parent )
            {
                parent->addChild( parentWindow );
            }

            auto actorWindow = ui->addElementByType<ui::IUIWindow>();
            m_actorWindow = actorWindow;
            actorWindow->setLabel( "Actor" );
            actorWindow->setSize( Vector2F( 0.0f, 250.0f ) );
            actorWindow->setHasBorder( true );

            auto componentWindow = ui->addElementByType<ui::IUIWindow>();
            m_componentWindow = componentWindow;
            componentWindow->setLabel( "Components" );
            componentWindow->setSize( Vector2F( 0.0f, 180.0f ) );
            componentWindow->setHasBorder( true );

            auto uiListener = factoryManager->make_ptr<UIElementListener>();
            uiListener->setOwner( this );
            m_uiListener = uiListener;

            m_selectionSummary = ui->addElementByType<ui::IUIText>();
            m_selectionSummary->setText( "No actor selected" );
            actorWindow->addChild( m_selectionSummary );

            m_actorNamePair = ui->addElementByType<ui::IUILabelTextInputPair>();
            m_actorNamePair->setLabel( "Name" );
            m_actorNamePair->setValue( "" );
            m_actorNamePair->setElementId( static_cast<s32>( WidgetId::Label ) );
            m_actorNamePair->addObjectListener( uiListener );
            actorWindow->addChild( m_actorNamePair );

            m_actorEnabled = ui->addElementByType<ui::IUILabelTogglePair>();
            m_actorVisible = ui->addElementByType<ui::IUILabelTogglePair>();
            m_actorStatic = ui->addElementByType<ui::IUILabelTogglePair>();
            m_actorSmoothMotion = ui->addElementByType<ui::IUILabelTogglePair>();
            m_actorCollisionMask = ui->addElementByType<ui::IUILabelDropdownPair>();
            m_actorLayerPair = ui->addElementByType<ui::IUILabelDropdownPair>();
            m_actorTagsPair = ui->addElementByType<ui::IUILabelTextInputPair>();

            m_actorEnabled->setElementId( static_cast<s32>( WidgetId::Enabled ) );
            m_actorEnabled->addObjectListener( uiListener );
            m_actorEnabled->setLabel( "Enabled" );

            m_actorVisible->setSameLine( true );
            m_actorVisible->setElementId( static_cast<s32>( WidgetId::Visible ) );
            m_actorVisible->addObjectListener( uiListener );
            m_actorVisible->setLabel( "Show" );

            m_actorStatic->setSameLine( false );
            m_actorStatic->setElementId( static_cast<s32>( WidgetId::Static ) );
            m_actorStatic->addObjectListener( uiListener );
            m_actorStatic->setLabel( "Static" );

            m_actorSmoothMotion->setSameLine( true );
            m_actorSmoothMotion->setElementId( static_cast<s32>( WidgetId::SmoothMotion ) );
            m_actorSmoothMotion->addObjectListener( uiListener );
            m_actorSmoothMotion->setLabel( "Smooth Motion" );

            m_actorCollisionMask->setLabel( "Collision Mask" );
            m_actorCollisionMask->setElementId( static_cast<s32>( WidgetId::CollisionMask ) );
            m_actorCollisionMask->addObjectListener( uiListener );
            refreshCollisionMaskDropdown();

            m_actorLayerPair->setLabel( "Layer" );
            m_actorLayerPair->setElementId( static_cast<s32>( WidgetId::Layer ) );
            m_actorLayerPair->addObjectListener( uiListener );
            refreshLayerDropdown();

            m_actorTagsPair->setLabel( "Tags" );
            m_actorTagsPair->setValue( "" );
            m_actorTagsPair->setElementId( static_cast<s32>( WidgetId::Tags ) );
            m_actorTagsPair->setEnabled( false, false );

            actorWindow->addChild( m_actorEnabled );
            actorWindow->addChild( m_actorVisible );
            actorWindow->addChild( m_actorStatic );
            actorWindow->addChild( m_actorSmoothMotion );
            actorWindow->addChild( m_actorCollisionMask );

            auto manageCollisionMasksButton = ui->addElementByType<ui::IUIButton>();
            WP_ASSERT( manageCollisionMasksButton );
            m_manageCollisionMasksButton = manageCollisionMasksButton;
            manageCollisionMasksButton->setLabel( "Manage Masks" );
            manageCollisionMasksButton->setSameLine( true );
            manageCollisionMasksButton->setElementId(
                static_cast<s32>( WidgetId::ManageCollisionMasks ) );
            manageCollisionMasksButton->addObjectListener( uiListener );
            actorWindow->addChild( manageCollisionMasksButton );

            actorWindow->addChild( m_actorLayerPair );

            auto manageLayersButton = ui->addElementByType<ui::IUIButton>();
            WP_ASSERT( manageLayersButton );
            m_manageLayersButton = manageLayersButton;
            manageLayersButton->setLabel( "Manage Layers" );
            manageLayersButton->setSameLine( true );
            manageLayersButton->setElementId( static_cast<s32>( WidgetId::ManageLayers ) );
            manageLayersButton->addObjectListener( uiListener );
            actorWindow->addChild( manageLayersButton );

            actorWindow->addChild( m_actorTagsPair );

            auto manageTagsButton = ui->addElementByType<ui::IUIButton>();
            WP_ASSERT( manageTagsButton );
            m_manageTagsButton = manageTagsButton;
            manageTagsButton->setLabel( "Manage Tags" );
            manageTagsButton->setSameLine( true );
            manageTagsButton->setElementId( static_cast<s32>( WidgetId::ManageTags ) );
            manageTagsButton->addObjectListener( uiListener );
            actorWindow->addChild( manageTagsButton );

            m_networkSummary = ui->addElementByType<ui::IUIText>();
            m_networkSummary->setText( "Networking: Local only" );
            actorWindow->addChild( m_networkSummary );

            m_configureNetworkButton = ui->addElementByType<ui::IUIButton>();
            m_configureNetworkButton->setLabel( "Add Network View" );
            m_configureNetworkButton->setElementId(
                static_cast<s32>( WidgetId::ConfigureNetwork ) );
            m_configureNetworkButton->addObjectListener( uiListener );
            actorWindow->addChild( m_configureNetworkButton );

            auto componentSearchEntry = ui->addElementByType<ui::IUITextEntry>();
            componentSearchEntry->setLabel( "Search##ActorComponentSearch" );
            componentSearchEntry->setPlaceholder( "Filter components by type..." );
            componentSearchEntry->setText( "" );
            componentSearchEntry->setElementId( static_cast<s32>( WidgetId::ComponentSearch ) );
            componentSearchEntry->addObjectListener( uiListener );
            componentWindow->addChild( componentSearchEntry );
            m_componentSearchEntry = componentSearchEntry;

            auto addComponentButton = ui->addElementByType<ui::IUIButton>();
            WP_ASSERT( addComponentButton );
            m_addComponentButton = addComponentButton;

            addComponentButton->setElementId( static_cast<s32>( WidgetId::AddComponent ) );
            addComponentButton->setLabel( "Add Component" );
            componentWindow->addChild( addComponentButton );
            m_addComponentButton->addObjectListener( uiListener );

            auto removeComponentButton = ui->addElementByType<ui::IUIButton>();
            WP_ASSERT( removeComponentButton );
            m_removeComponentButton = removeComponentButton;

            removeComponentButton->setSameLine( true );
            removeComponentButton->setElementId( static_cast<s32>( WidgetId::RemoveComponent ) );
            removeComponentButton->setLabel( "Remove Component" );
            componentWindow->addChild( removeComponentButton );
            m_removeComponentButton->addObjectListener( uiListener );

            auto tree = ui->addElementByType<ui::IUITreeCtrl>();
            m_tree = tree;
            componentWindow->addChild( tree );
            tree->addObjectListener( uiListener );

            setActorControlsEnabled( false );

            if( actorWindow )
            {
                parentWindow->addChild( actorWindow );
            }

            m_transformWindow = factoryManager->make_ptr<TransformWindow>();
            m_transformWindow->setParent( parentWindow );
            m_transformWindow->load( data );

            if( componentWindow )
            {
                parentWindow->addChild( componentWindow );
            }

            m_propertiesWindow = factoryManager->make_ptr<PropertiesWindow>();
            m_propertiesWindow->setParent( parentWindow );
            m_propertiesWindow->load( data );

            m_eventsWindow = factoryManager->make_ptr<EventsWindow>();
            m_eventsWindow->setParent( parentWindow );
            m_eventsWindow->load( data );

            //auto materialWindow = factoryManager->make_ptr<MaterialWindow>( parentWindow );
            //materialWindow->load( data );
            //materialWindow->setWindowVisible( false );
            //m_materialWindow = materialWindow;

            auto materialWindow = factoryManager->make_ptr<ScriptWindow>();
            m_materialWindow = materialWindow;
            materialWindow->setClassName( "MaterialEditor" );
            materialWindow->setParent( parentWindow );
            materialWindow->load( nullptr );
            materialWindow->setWindowVisible( false );

            //auto terrainWindow = factoryManager->make_ptr<TerrainWindow>();
            //terrainWindow->setParent( parentWindow );
            //terrainWindow->load( data );
            //m_terrainWindow = terrainWindow;

            auto terrainWindow = factoryManager->make_ptr<ScriptWindow>();
            m_terrainWindow = terrainWindow;
            terrainWindow->setClassName( "TerrainEditor" );
            terrainWindow->setParent( parentWindow );
            terrainWindow->load( nullptr );
            terrainWindow->setWindowVisible( false );

            auto particleSystemWindow = factoryManager->make_ptr<ScriptWindow>();
            setParticleSystemWindow( particleSystemWindow );
            particleSystemWindow->setClassName( "ParticleEditor" );
            particleSystemWindow->setParent( parentWindow );
            particleSystemWindow->load( nullptr );
            particleSystemWindow->setWindowVisible( false );

            setLoadingState( LoadingState::Loaded );
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ActorWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto hasResources =
                m_actorWindow || m_componentWindow || m_tree || m_uiListener || m_actorEnabled ||
                m_actorVisible || m_actorStatic || m_actorSmoothMotion || m_actorCollisionMask ||
                m_manageCollisionMasksButton || m_actorLayerPair || m_manageLayersButton ||
                m_actorTagsPair || m_manageTagsButton || m_addComponentButton ||
                m_networkSummary || m_configureNetworkButton || m_removeComponentButton ||
                m_actorNamePair || m_transformWindow ||
                m_propertiesWindow || m_eventsWindow || m_materialWindow || m_terrainWindow ||
                m_particleSystemWindow || m_selectedObject || m_selectedEntity ||
                m_selectionSummary || m_componentSearchEntry || getParentWindow() ||
                !m_dataArray.empty();

            if( getLoadingState() == LoadingState::Unloaded && !hasResources )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            if( m_uiListener )
            {
                if( m_actorNamePair )
                {
                    m_actorNamePair->removeObjectListener( m_uiListener );
                }

                if( m_actorEnabled )
                {
                    m_actorEnabled->removeObjectListener( m_uiListener );
                }

                if( m_actorVisible )
                {
                    m_actorVisible->removeObjectListener( m_uiListener );
                }

                if( m_actorStatic )
                {
                    m_actorStatic->removeObjectListener( m_uiListener );
                }

                if( m_actorSmoothMotion )
                {
                    m_actorSmoothMotion->removeObjectListener( m_uiListener );
                }

                if( m_actorCollisionMask )
                {
                    m_actorCollisionMask->removeObjectListener( m_uiListener );
                }

                if( m_manageCollisionMasksButton )
                {
                    m_manageCollisionMasksButton->removeObjectListener( m_uiListener );
                }

                if( m_actorLayerPair )
                {
                    m_actorLayerPair->removeObjectListener( m_uiListener );
                }

                if( m_manageLayersButton )
                {
                    m_manageLayersButton->removeObjectListener( m_uiListener );
                }

                if( m_manageTagsButton )
                {
                    m_manageTagsButton->removeObjectListener( m_uiListener );
                }

                if( m_configureNetworkButton )
                {
                    m_configureNetworkButton->removeObjectListener( m_uiListener );
                }

                if( m_addComponentButton )
                {
                    m_addComponentButton->removeObjectListener( m_uiListener );
                }

                if( m_removeComponentButton )
                {
                    m_removeComponentButton->removeObjectListener( m_uiListener );
                }

                if( m_tree )
                {
                    m_tree->removeObjectListener( m_uiListener );
                }

                if( m_componentSearchEntry )
                {
                    m_componentSearchEntry->removeObjectListener( m_uiListener );
                }
            }

            if( m_selectionSummary )
            {
                m_selectionSummary->unload( data );
                ui->removeElement( m_selectionSummary );
                m_selectionSummary = nullptr;
            }

            if( m_componentSearchEntry )
            {
                m_componentSearchEntry->unload( data );
                ui->removeElement( m_componentSearchEntry );
                m_componentSearchEntry = nullptr;
            }

            m_componentSearchFilter.clear();

            if( m_actorNamePair )
            {
                m_actorNamePair->unload( data );
                ui->removeElement( m_actorNamePair );
                m_actorNamePair = nullptr;
            }

            if( m_actorEnabled )
            {
                m_actorEnabled->unload( data );
                ui->removeElement( m_actorEnabled );
                m_actorEnabled = nullptr;
            }

            if( m_actorVisible )
            {
                m_actorVisible->unload( data );
                ui->removeElement( m_actorVisible );
                m_actorVisible = nullptr;
            }

            if( m_actorStatic )
            {
                m_actorStatic->unload( data );
                ui->removeElement( m_actorStatic );
                m_actorStatic = nullptr;
            }

            if( m_actorSmoothMotion )
            {
                m_actorSmoothMotion->unload( data );
                ui->removeElement( m_actorSmoothMotion );
                m_actorSmoothMotion = nullptr;
            }

            if( m_actorCollisionMask )
            {
                m_actorCollisionMask->unload( data );
                ui->removeElement( m_actorCollisionMask );
                m_actorCollisionMask = nullptr;
            }

            if( m_manageCollisionMasksButton )
            {
                m_manageCollisionMasksButton->unload( data );
                ui->removeElement( m_manageCollisionMasksButton );
                m_manageCollisionMasksButton = nullptr;
            }

            if( m_actorLayerPair )
            {
                m_actorLayerPair->unload( data );
                ui->removeElement( m_actorLayerPair );
                m_actorLayerPair = nullptr;
            }

            if( m_manageLayersButton )
            {
                m_manageLayersButton->unload( data );
                ui->removeElement( m_manageLayersButton );
                m_manageLayersButton = nullptr;
            }

            if( m_actorTagsPair )
            {
                m_actorTagsPair->unload( data );
                ui->removeElement( m_actorTagsPair );
                m_actorTagsPair = nullptr;
            }

            if( m_manageTagsButton )
            {
                m_manageTagsButton->unload( data );
                ui->removeElement( m_manageTagsButton );
                m_manageTagsButton = nullptr;
            }

            if( m_networkSummary )
            {
                m_networkSummary->unload( data );
                ui->removeElement( m_networkSummary );
                m_networkSummary = nullptr;
            }

            if( m_configureNetworkButton )
            {
                m_configureNetworkButton->unload( data );
                ui->removeElement( m_configureNetworkButton );
                m_configureNetworkButton = nullptr;
            }

            if( m_addComponentButton )
            {
                m_addComponentButton->unload( data );
                ui->removeElement( m_addComponentButton );
                m_addComponentButton = nullptr;
            }

            if( m_removeComponentButton )
            {
                m_removeComponentButton->unload( data );
                ui->removeElement( m_removeComponentButton );
                m_removeComponentButton = nullptr;
            }

            if( m_tree )
            {
                m_tree->clear();
                ui->removeElement( m_tree );
                m_tree = nullptr;
            }

            m_uiListener = nullptr;
            m_selectedObject = nullptr;
            m_selectedEntity = nullptr;
            //m_parentFilter = nullptr;

            if( m_actorWindow )
            {
                ui->removeElement( m_actorWindow );
                m_actorWindow = nullptr;
            }

            if( m_componentWindow )
            {
                ui->removeElement( m_componentWindow );
                m_componentWindow = nullptr;
            }

            if( auto propertiesWindow = getPropertiesWindow() )
            {
                propertiesWindow->unload( nullptr );
                setPropertiesWindow( nullptr );
            }

            if( auto transformWindow = getTransformWindow() )
            {
                transformWindow->unload( nullptr );
                setTransformWindow( nullptr );
            }

            if( m_eventsWindow )
            {
                m_eventsWindow->unload( nullptr );
                m_eventsWindow = nullptr;
            }

            if( m_materialWindow )
            {
                m_materialWindow->unload( nullptr );
                m_materialWindow = nullptr;
            }

            if( m_terrainWindow )
            {
                m_terrainWindow->unload( nullptr );
                m_terrainWindow = nullptr;
            }

            if( m_particleSystemWindow )
            {
                m_particleSystemWindow->unload( nullptr );
                m_particleSystemWindow = nullptr;
            }

            if( auto parentWindow = getParentWindow() )
            {
                ui->removeElement( parentWindow );
                setParentWindow( nullptr );
            }

            for( auto data : m_dataArray )
            {
                data->unload( nullptr );
            }

            m_dataArray.clear();

            EditorWindow::unload( nullptr );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ActorWindow::buildTree()
    {
        try
        {
            if( getLoadingState() != LoadingState::Loaded || !m_tree )
            {
                return;
            }

            m_tree->clear();

            for( auto data : m_dataArray )
            {
                if( data )
                {
                    data->unload( nullptr );
                }
            }

            m_dataArray.clear();
            m_selectedObject = nullptr;
            m_selectedEntity = nullptr;

            auto applicationManager = core::IApplicationManager::instancePtr();
            auto selectionManager = applicationManager ? applicationManager->getSelectionManagerPtr() : nullptr;
            if( !selectionManager )
            {
                clearInspector( "No selection service available" );
                return;
            }

            auto selection = selectionManager->getSelection();
            if( selection.empty() )
            {
                clearInspector( "No actor selected" );
                return;
            }

            if( selection.size() != 1 )
            {
                clearInspector( StringUtil::toString( static_cast<u32>( selection.size() ) ) +
                                " objects selected" );
                return;
            }

            auto selectedObject = selection.front();
            auto actor = getActorFromSelection( selectedObject );
            if( !actor )
            {
                clearInspector( "Selection is not an actor" );
                return;
            }

            m_selectedObject = selectedObject;
            m_selectedEntity = actor;
            setActorControlsEnabled( true );

            auto actorName = actor->getName();
            if( StringUtil::isNullOrEmpty( actorName ) )
            {
                actorName = "Untitled";
            }

            if( m_selectionSummary )
            {
                auto summary = actorName;
                if( selectedObject && selectedObject->isDerived<scene::IComponent>() )
                {
                    auto typeManager = TypeManager::instance();
                    auto componentLabel = typeManager ?
                                              String( typeManager->getLabel( selectedObject->getTypeInfo() ) ) :
                                              String( "Component" );
                    summary += " / " + componentLabel;
                }

                m_selectionSummary->setText( summary );
            }

            m_actorNamePair->setValue( actorName );
            m_actorEnabled->setValue( actor->isEnabled() );
            m_actorStatic->setValue( actor->isStatic() );
            m_actorVisible->setValue( actor->isVisible() );
            m_actorSmoothMotion->setValue( actor->isSmoothMotion() );
            selectCollisionMask( actor->getCollisionMask() );
            selectLayer( actor->getLayer() );
            m_actorTagsPair->setValue( joinTags( actor->getTags() ) );

            auto networkView = actor->getComponent<scene::NetworkView>();
            if( m_networkSummary )
            {
                if( networkView )
                {
                    auto authorityIndex = static_cast<u32>( networkView->getAuthorityMode() );
                    auto authorityName = authorityIndex < scene::NetworkView::authorityModeNames.size() ?
                                             scene::NetworkView::authorityModeNames[authorityIndex] :
                                             String( "Unknown" );
                    m_networkSummary->setText(
                        "Networking: View " + StringUtil::toString( networkView->getViewId() ) +
                        " | Owner " + StringUtil::toString( networkView->getOwnerId() ) + " | " +
                        authorityName + " @ " +
                        StringUtil::toString( networkView->getSendRate() ) + " Hz" );
                }
                else
                {
                    m_networkSummary->setText( "Networking: Local only" );
                }
            }

            if( m_configureNetworkButton )
            {
                m_configureNetworkButton->setLabel( networkView ? "Edit Network View" :
                                                                  "Add Network View" );
            }

            //if( m_addComponentButton )
            //{
            //    m_addComponentButton->setEnabled( true, false );
            //}

            //if( m_removeComponentButton )
            //{
            //    auto componentSelected =
            //        selectedObject && selectedObject->isDerived<scene::IComponent>();
            //    m_removeComponentButton->setEnabled( componentSelected, false );
            //}

            auto rootNode = m_tree->addRoot();
            if( !rootNode )
            {
                return;
            }

            Util::setText( rootNode, "Components" );
            rootNode->setExpanded( true );
            addActorToTree( actor, rootNode );
            applyComponentFilter( m_componentSearchFilter );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ActorWindow::setActorControlsEnabled( bool enabled )
    {
        if( m_actorNamePair )
        {
            m_actorNamePair->setEnabled( enabled, false );
        }

        if( m_actorEnabled )
        {
            m_actorEnabled->setEnabled( enabled, false );
        }

        if( m_actorVisible )
        {
            m_actorVisible->setEnabled( enabled, false );
        }

        if( m_actorStatic )
        {
            m_actorStatic->setEnabled( enabled, false );
        }

        if( m_actorSmoothMotion )
        {
            m_actorSmoothMotion->setEnabled( enabled, false );
        }

        if( m_actorCollisionMask )
        {
            m_actorCollisionMask->setEnabled( enabled, false );
        }

        if( m_actorLayerPair )
        {
            m_actorLayerPair->setEnabled( enabled, false );
        }

        // Tags are managed through the dedicated dialog, so the field remains read-only.
        if( m_actorTagsPair )
        {
            m_actorTagsPair->setEnabled( false, false );
        }

        if( m_addComponentButton )
        {
            m_addComponentButton->setEnabled( enabled, false );
        }

        if( m_removeComponentButton )
        {
            m_removeComponentButton->setEnabled( enabled, false );
        }

        if( m_configureNetworkButton )
        {
            m_configureNetworkButton->setEnabled( enabled, false );
        }
    }

    void ActorWindow::clearInspector( const String &message )
    {
        setActorControlsEnabled( false );

        if( m_selectionSummary )
        {
            m_selectionSummary->setText( message );
        }

        if( m_actorNamePair )
        {
            m_actorNamePair->setValue( "" );
        }

        if( m_actorTagsPair )
        {
            m_actorTagsPair->setValue( "" );
        }

        if( m_networkSummary )
        {
            m_networkSummary->setText( "Networking: No actor selected" );
        }

        if( m_configureNetworkButton )
        {
            m_configureNetworkButton->setLabel( "Add Network View" );
        }

        applyComponentFilter( m_componentSearchFilter );
    }

    void ActorWindow::applyComponentFilter( const String &filter )
    {
        m_componentSearchFilter = StringUtil::make_lower( filter );

        auto root = m_tree ? m_tree->getRoot() : nullptr;
        if( !root )
        {
            return;
        }

        root->setVisible( true, false );
        for( auto child : root->getChildren() )
        {
            if( auto childNode = workphone::dynamic_pointer_cast<ui::IUITreeNode>( child ) )
            {
                updateComponentNodeFilter( childNode, m_componentSearchFilter );
            }
        }

        root->setExpanded( true );
    }

    bool ActorWindow::updateComponentNodeFilter( SmartPtr<ui::IUITreeNode> node,
                                                  const String &filter )
    {
        if( !node )
        {
            return false;
        }

        auto searchable = StringUtil::make_lower( Util::getText( node ) );
        if( auto data =
                workphone::dynamic_pointer_cast<ProjectTreeData>( node->getNodeUserData() ) )
        {
            searchable += " " + StringUtil::make_lower( data->getObjectType() );
        }

        auto matches = filter.empty() || searchable.find( filter ) != String::npos;
        auto childMatches = false;
        for( auto child : node->getChildren() )
        {
            if( auto childNode = workphone::dynamic_pointer_cast<ui::IUITreeNode>( child ) )
            {
                childMatches = updateComponentNodeFilter( childNode, filter ) || childMatches;
            }
        }

        auto visible = matches || childMatches;
        node->setVisible( visible, false );
        if( !filter.empty() && childMatches )
        {
            node->setExpanded( true );
        }

        return visible;
    }

    void ActorWindow::addComponentToTree( SmartPtr<scene::IComponent> component,
                                          SmartPtr<ui::IUITreeNode> node )
    {
        try
        {
            if( component && m_tree )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                auto factoryManager = applicationManager ? applicationManager->getFactoryManagerPtr() : nullptr;
                if( !factoryManager )
                {
                    return;
                }

                auto editorManager = EditorManager::getSingletonPtr();

                auto data = factoryManager->make_ptr<ProjectTreeData>( "component", "component",
                                                                       component, component );

                auto typeinfo = component->getTypeInfo();
                WP_ASSERT( typeinfo );

                auto typeManager = TypeManager::instance();
                WP_ASSERT( typeManager );

                auto className = typeManager->getLabel( typeinfo );
                if( StringUtil::isNullOrEmpty( className ) )
                {
                    className = "Untitled";
                }

                auto treeNode = m_tree->addNode();
                if( !treeNode )
                {
                    return;
                }
                Util::setText( treeNode, className );

                treeNode->setNodeUserData( data );

                if( node )
                {
                    node->addChild( treeNode );
                }

                m_dataArray.emplace_back( data );

                auto subComponents = component->getSubComponents();
                if( !subComponents.empty() )
                {
                    auto subComponentsTreeNode = m_tree->addNode();
                    if( !subComponentsTreeNode )
                    {
                        return;
                    }

                    Util::setText( subComponentsTreeNode, "Sub Components" );

                    if( treeNode )
                    {
                        treeNode->addChild( subComponentsTreeNode );
                    }

                    for( auto child : subComponents )
                    {
                        if( child )
                        {
                            addObjectToTree( child, subComponentsTreeNode );
                        }
                    }
                }

                auto bShowDebug = editorManager && editorManager->getShowDebug();
                if( bShowDebug )
                {
                    auto childrenTreeNode = m_tree->addNode();

                    WP_ASSERT( childrenTreeNode );
                    Util::setText( childrenTreeNode, "Children" );

                    if( treeNode )
                    {
                        treeNode->addChild( childrenTreeNode );
                    }

                    auto children = component->getChildObjects();
                    for( auto child : children )
                    {
                        if( child )
                        {
                            addObjectToTree( child, childrenTreeNode );
                        }
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ActorWindow::addObjectToTree( SmartPtr<ISharedObject> object, SmartPtr<ui::IUITreeNode> node )
    {
        try
        {
            if( object && m_tree )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                auto factoryManager = applicationManager ? applicationManager->getFactoryManagerPtr() : nullptr;
                if( !factoryManager )
                {
                    return;
                }

                auto data =
                    factoryManager->make_ptr<ProjectTreeData>( "object", "object", object, object );

                auto typeinfo = object->getTypeInfo();
                WP_ASSERT( typeinfo != 0 );

                auto typeManager = TypeManager::instance();
                WP_ASSERT( typeManager );

                auto className = String( typeManager->getName( typeinfo ) );
                if( StringUtil::isNullOrEmpty( className ) )
                {
                    className = "Untitled";
                }

                if( object->isDerived<physics::IPhysicsShape>() )
                {
                    className = String( "PhysicsShape" );
                }
                else if( object->isDerived<physics::IPhysicsBody3>() )
                {
                    className = String( "PhysicsBody3" );
                }
                else if( object->isDerived<IStateContext>() )
                {
                    className = String( "StateObject" );
                }
                else if( object->isDerived<IStateListener>() )
                {
                    className = String( "StateListener" );
                }
                else if( object->isDerived<render::IGraphicsScene>() )
                {
                    className = String( "SceneManager" );
                }
                else if( object->isDerived<render::IGraphicsSceneNode>() )
                {
                    className = String( "SceneNode" );
                }
                else if( object->isDerived<render::IGraphicsMesh>() )
                {
                    className = String( "Mesh" );
                }
                else if( object->isDerived<render::IMaterial>() )
                {
                    className = String( "Material" );
                }
                else if( object->isDerived<render::IMaterialTechnique>() )
                {
                    className = String( "Technique" );
                }
                else if( object->isDerived<render::IMaterialPass>() )
                {
                    className = String( "Pass" );
                }
                else if( object->isDerived<render::IMaterialTexture>() )
                {
                    className = object->getName();
                }
                else if( object->isDerived<ui::IUIImage>() )
                {
                    className = String( "Image" );
                }

                auto treeNode = m_tree->addNode();
                if( !treeNode )
                {
                    return;
                }
                Util::setText( treeNode, className );

                treeNode->setNodeUserData( data );
                m_dataArray.emplace_back( data );

                if( node )
                {
                    node->addChild( treeNode );
                }

                auto children = object->getChildObjects();
                for( auto child : children )
                {
                    if( child )
                    {
                        addObjectToTree( child, treeNode );
                    }
                }

                // if (!children.empty())
                //{
                //	m_tree->Expand(treeId);
                // }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ActorWindow::addActorToTree( SmartPtr<scene::IGameActor> actor,
                                      SmartPtr<ui::IUITreeNode> parentNode )
    {
        try
        {
            auto components = actor->getComponentsByType<scene::IComponent>();
            for( auto component : components )
            {
                addComponentToTree( component, parentNode );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ActorWindow::updateSelection()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto selectionManager = applicationManager ? applicationManager->getSelectionManagerPtr() : nullptr;
            if( !selectionManager )
            {
                return;
            }

            auto selection = selectionManager->getSelection();
            buildTree();

            if( selection.size() != 1 )
            {
                if( m_eventsWindow )
                {
                    m_eventsWindow->setWindowVisible( false );
                }

                return;
            }

            auto object = selection.front();
            auto actor = getActorFromSelection( object );
            if( actor && m_transformWindow )
            {
                m_transformWindow->setTransform( actor->getTransform() );
                m_transformWindow->updateSelection();
            }

            if( object && object->isDerived<scene::IComponent>() )
            {
                // Component-specific panes are selected by the same routing used by tree activation.
                updateDetailsForObject( object );
            }
            else
            {
                if( m_particleSystemWindow )
                {
                    m_particleSystemWindow->setWindowVisible( false );
                }

                if( m_materialWindow )
                {
                    m_materialWindow->setWindowVisible( false );
                }

                if( m_terrainWindow )
                {
                    m_terrainWindow->setWindowVisible( false );
                }

                if( m_propertiesWindow )
                {
                    m_propertiesWindow->setWindowVisible( true );
                    m_propertiesWindow->updateSelection();
                }

                if( m_eventsWindow )
                {
                    m_eventsWindow->setWindowVisible( true );
                    m_eventsWindow->updateSelection();
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<TransformWindow> ActorWindow::getTransformWindow() const
    {
        return m_transformWindow;
    }

    void ActorWindow::setTransformWindow( SmartPtr<TransformWindow> transformWindow )
    {
        m_transformWindow = transformWindow;
    }

    SmartPtr<PropertiesWindow> ActorWindow::getPropertiesWindow() const
    {
        return m_propertiesWindow;
    }

    void ActorWindow::setPropertiesWindow( SmartPtr<PropertiesWindow> propertiesWindow )
    {
        m_propertiesWindow = propertiesWindow;
    }

    void ActorWindow::setParticleSystemWindow( SmartPtr<EditorWindow> particleWindow )
    {
        m_particleSystemWindow = particleWindow;
    }

    SmartPtr<EditorWindow> ActorWindow::getParticleSystemWindow() const
    {
        return m_particleSystemWindow;
    }

    void ActorWindow::refreshLayerDropdown()
    {
        auto editorManager = EditorManager::getSingletonPtr();
        auto uiManager = editorManager ? editorManager->getUI() : nullptr;
        auto layerManager = uiManager ? uiManager->getLayerManager() : nullptr;
        if( !m_actorLayerPair || !layerManager )
        {
            return;
        }

        layerManager->refreshFromScene();

        auto previousLayer = layerManager->getLayerByIndex( m_actorLayerPair->getSelectedOption() );
        m_actorLayerPair->setOptions( layerManager->getLayers() );
        selectLayer( previousLayer );
    }

    void ActorWindow::selectLayer( const String &layer )
    {
        auto editorManager = EditorManager::getSingletonPtr();
        auto uiManager = editorManager ? editorManager->getUI() : nullptr;
        auto layerManager = uiManager ? uiManager->getLayerManager() : nullptr;
        if( !m_actorLayerPair || !layerManager )
        {
            return;
        }

        layerManager->addLayer( layer );
        m_actorLayerPair->setOptions( layerManager->getLayers() );
        m_actorLayerPair->setSelectedOption( layerManager->getLayerIndex( layer ) );
    }

    void ActorWindow::refreshCollisionMaskDropdown()
    {
        auto editorManager = EditorManager::getSingletonPtr();
        auto uiManager = editorManager ? editorManager->getUI() : nullptr;
        auto maskManager = uiManager ? uiManager->getCollisionMaskManager() : nullptr;
        if( !m_actorCollisionMask || !maskManager )
        {
            return;
        }

        auto previousMask = maskManager->getMaskByIndex( m_actorCollisionMask->getSelectedOption() );
        m_actorCollisionMask->setOptions( maskManager->getOptionLabels() );
        selectCollisionMask( previousMask );
    }

    void ActorWindow::selectCollisionMask( u32 collisionMask )
    {
        auto editorManager = EditorManager::getSingletonPtr();
        auto uiManager = editorManager ? editorManager->getUI() : nullptr;
        auto maskManager = uiManager ? uiManager->getCollisionMaskManager() : nullptr;
        if( !m_actorCollisionMask || !maskManager )
        {
            return;
        }

        maskManager->addOption( "", collisionMask );
        m_actorCollisionMask->setOptions( maskManager->getOptionLabels() );
        m_actorCollisionMask->setSelectedOption( maskManager->getOptionIndexByMask( collisionMask ) );
    }

    void ActorWindow::refreshTagDisplay()
    {
        if( !m_actorTagsPair )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto selectionManager = applicationManager ? applicationManager->getSelectionManagerPtr() : nullptr;
        if( !selectionManager )
        {
            m_actorTagsPair->setValue( "" );
            return;
        }

        auto tags = Array<String>();
        auto hasDisplayTags = false;
        auto selection = selectionManager->getSelection();
        for( auto selected : selection )
        {
            auto actor = getActorFromSelection( selected );
            if( actor )
            {
                tags = actor->getTags();
                hasDisplayTags = true;
                break;
            }
        }

        m_actorTagsPair->setValue( hasDisplayTags ? joinTags( tags ) : String() );
    }

    ActorWindow::UIElementListener::UIElementListener()
    {
        setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );
    }

    ActorWindow::UIElementListener::~UIElementListener() = default;

    Parameter ActorWindow::UIElementListener::handleEvent( EventType eventType, hash_type eventValue,
                                                           const Array<Parameter> &arguments,
                                                           SmartPtr<ISharedObject> sender,
                                                           SmartPtr<ISharedObject> object,
                                                           SmartPtr<IEvent> event )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto editorManager = EditorManager::getSingletonPtr();
        auto owner = getOwner();
        auto selectionManager = applicationManager ? applicationManager->getSelectionManagerPtr() : nullptr;
        auto ui = editorManager ? editorManager->getUI() : nullptr;
        if( !owner || !selectionManager || !ui )
        {
            return {};
        }

        auto eventObject = sender ? sender : object;
        auto element = workphone::dynamic_pointer_cast<ui::IUIElement>( eventObject );
        if( !element )
        {
            return {};
        }

        auto elementId = static_cast<WidgetId>( element->getElementId() );

        if( eventValue == IEvent::handleValueChanged &&
            ( element == owner->m_componentSearchEntry || elementId == WidgetId::ComponentSearch ) )
        {
            auto filter = arguments.empty() ? owner->m_componentSearchEntry->getText() :
                                              arguments[0].getStr();
            owner->applyComponentFilter( filter );
            return {};
        }

        auto forEachSelectedActor = [&]( auto callback ) {
            for( auto selected : selectionManager->getSelection() )
            {
                if( auto actor = getActorFromSelection( selected ) )
                {
                    callback( actor );
                }
            }
        };

        if( eventValue == IEvent::handleSelection )
        {
            switch( elementId )
            {
            case WidgetId::AddComponent:
            {
                if( auto objectBrowserDialog = ui->getObjectBrowserDialog() )
                {
                    objectBrowserDialog->setWindowVisible( true );
                }
            }
            break;
            case WidgetId::RemoveComponent:
            {
                auto commandManager = applicationManager->getCommandManagerPtr();
                auto factoryManager = applicationManager->getFactoryManagerPtr();
                if( commandManager && factoryManager )
                {
                    auto command = factoryManager->make_ptr<RemoveSelectionCmd>();
                    commandManager->addCommand( command );
                }
            }
            break;
            case WidgetId::ManageCollisionMasks:
            {
                if( auto collisionMaskDialog = ui->getCollisionMaskDialog() )
                {
                    collisionMaskDialog->setWindowVisible( true );
                }
            }
            break;
            case WidgetId::ManageLayers:
            {
                if( auto layerDialog = ui->getLayerDialog() )
                {
                    layerDialog->setWindowVisible( true );
                }
            }
            break;
            case WidgetId::ManageTags:
            {
                if( auto tagDialog = ui->getTagDialog() )
                {
                    tagDialog->setWindowVisible( true );
                }
            }
            break;
            case WidgetId::ConfigureNetwork:
            {
                auto selection = selectionManager->getSelection();
                auto actor = selection.size() == 1 ? getActorFromSelection( selection.front() ) :
                                                     nullptr;
                if( actor )
                {
                    auto networkView = actor->getComponent<scene::NetworkView>();
                    if( !networkView )
                    {
                        networkView = actor->addComponent<scene::NetworkView>();
                        if( networkView )
                        {
                            networkView->setState( scene::IComponent::State::Edit );
                        }
                    }

                    if( networkView )
                    {
                        selectionManager->clearSelection();
                        selectionManager->addSelectedObject( networkView );
                        owner->buildTree();
                        owner->updateDetailsForObject( networkView );
                    }
                }
            }
            break;
            default:
            {
            }
            break;
            }
        }
        else if( eventValue == IEvent::handleValueChanged )
        {
            switch( elementId )
            {
            case WidgetId::Label:
            {
                auto namePair = owner->m_actorNamePair;
                auto actorName = namePair->getValue();

                forEachSelectedActor( [&]( SmartPtr<scene::IGameActor> actor ) {
                    actor->setName( actorName );
                } );

                ui->rebuildSceneTree();
            }
            break;
            case WidgetId::Enabled:
            {
                auto toggle = owner->m_actorEnabled;
                auto value = toggle->getValue();

                forEachSelectedActor( [&]( SmartPtr<scene::IGameActor> actor ) {
                    actor->setEnabled( value );
                } );
            }
            break;
            case WidgetId::Visible:
            {
                auto toggle = owner->m_actorVisible;
                auto value = toggle->getValue();

                forEachSelectedActor( [&]( SmartPtr<scene::IGameActor> actor ) {
                    actor->setVisible( value );
                } );
            }
            break;
            case WidgetId::Static:
            {
                auto toggle = owner->m_actorStatic;
                auto value = toggle->getValue();

                forEachSelectedActor( [&]( SmartPtr<scene::IGameActor> actor ) {
                    actor->setStatic( value );
                } );
            }
            break;
            case WidgetId::SmoothMotion:
            {
                auto toggle = owner->m_actorSmoothMotion;
                auto value = toggle->getValue();

                forEachSelectedActor( [&]( SmartPtr<scene::IGameActor> actor ) {
                    actor->setSmoothMotion( value, true );
                } );
            }
            break;
            case WidgetId::CollisionMask:
            {
                auto collisionMaskInput = owner->m_actorCollisionMask;
                auto collisionMaskManager = ui ? ui->getCollisionMaskManager() : nullptr;
                auto collisionMask = collisionMaskManager ?
                                         collisionMaskManager->getMaskByIndex(
                                             collisionMaskInput->getSelectedOption() ) :
                                         u32( 0 );

                forEachSelectedActor( [&]( SmartPtr<scene::IGameActor> actor ) {
                    actor->setCollisionMask( collisionMask );
                } );

                owner->selectCollisionMask( collisionMask );
            }
            break;
            case WidgetId::Layer:
            {
                auto layerInput = owner->m_actorLayerPair;
                auto layerManager = ui ? ui->getLayerManager() : nullptr;
                auto layer = layerManager ?
                                 layerManager->getLayerByIndex( layerInput->getSelectedOption() ) :
                                 String();

                forEachSelectedActor( [&]( SmartPtr<scene::IGameActor> actor ) {
                    actor->setLayer( layer );
                } );

                owner->selectLayer( layer );
                ui->rebuildSceneTree();
            }
            break;
            default:
            {
            }
            break;
            }
        }
        else if( eventValue == IEvent::handleTreeSelectionActivated )
        {
            if( arguments.empty() )
            {
                return {};
            }

            auto node = workphone::static_pointer_cast<ui::IUITreeNode>( arguments[0].object );
            auto data = node ? workphone::dynamic_pointer_cast<ProjectTreeData>( node->getNodeUserData() ) :
                               nullptr;

            if( data )
            {
                auto object = data->getObjectData();
                if( object )
                {
                    owner->updateObjectSelection( object );
                }
            }
        }

        return {};
    }

    SmartPtr<ActorWindow> ActorWindow::UIElementListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void ActorWindow::UIElementListener::setOwner( SmartPtr<ActorWindow> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone::editor
