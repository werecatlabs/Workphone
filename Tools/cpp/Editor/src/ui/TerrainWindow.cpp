#include <EditorPCH.hpp>
#include "TerrainWindow.hpp"
#include <editor/EditorManager.hpp>
#include <ui/UIManager.hpp>
#include <ui/ResourceDatabaseDialog.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, TerrainWindow, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, TerrainWindow::UIElementListener, IEventListener );

    TerrainWindow::TerrainWindow()
    {
        static const auto className = String( "TerrainEditor" );
        setClassName( className );
    }

    TerrainWindow::~TerrainWindow() = default;

    void TerrainWindow::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            EditorWindow::load( data );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            auto parent = getParent();

            auto parentWindow = ui->addElementByType<ui::IUIWindow>();
            WP_ASSERT( parentWindow );

            setParentWindow( parentWindow );
            parentWindow->setLabel( "TerrainWindowChild" );

            if( parent )
            {
                parent->addChild( parentWindow );
            }

            auto debugWindow = ui->addElementByType<ui::IUIWindow>();
            setDebugWindow( debugWindow );

            if( parent )
            {
                parent->addChild( debugWindow );
            }

            auto terrainEditor = ui->addElementByType<ui::IUITerrainEditor>();
            debugWindow->addChild( terrainEditor );
            m_terrainEditor = terrainEditor;

            auto elementListener = workphone::make_ptr<UIElementListener>();
            elementListener->setOwner( this );
            m_terrainEditorListener = elementListener;

            terrainEditor->addObjectListener( elementListener );

            if( auto invoker = getInvoker() )
            {
                invoker->callObjectMember( loadStr );
            }

            debugWindow->setVisible( false, false );

            setLoadingState( LoadingState::Loaded );
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void TerrainWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                destroyScriptObject();

                if( m_terrainEditorListener )
                {
                    m_terrainEditorListener->unload( nullptr );
                    m_terrainEditorListener = nullptr;
                }

                m_terrainEditor = nullptr;

                EditorWindow::unload( data );
                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<ui::IUITerrainEditor> TerrainWindow::getTerrainEditor() const
    {
        return m_terrainEditor;
    }

    void TerrainWindow::setTerrainEditor( SmartPtr<ui::IUITerrainEditor> terrainEditor )
    {
        m_terrainEditor = terrainEditor;
    }

    void TerrainWindow::updateSelection()
    {
        try
        {
            EditorWindow::updateSelection();

            auto applicationManager = core::IApplicationManager::instance();
            auto selectionManager = applicationManager->getSelectionManager();

            auto selection = selectionManager->getSelection();
            for( auto object : selection )
            {
                if( object )
                {
                    if( object->isDerived<scene::TerrainSystem>() )
                    {
                        auto terrain = workphone::dynamic_pointer_cast<scene::TerrainSystem>( object );

                        if( auto terrainEditor = getTerrainEditor() )
                        {
                            terrainEditor->setTerrain( terrain );
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

    void TerrainWindow::loadImages()
    {
    }

    void TerrainWindow::addComboBoxBitmap()
    {
    }

    void TerrainWindow::OnRaiseBtnToggled()
    {
    }

    void TerrainWindow::OnLowerBtnToggled()
    {
    }

    void TerrainWindow::OnMinimumBtnToggled()
    {
    }

    void TerrainWindow::OnMaximumBtnToggled()
    {
    }

    void TerrainWindow::OnsetHeightBtnToggled()
    {
    }

    void TerrainWindow::OnPaintBtnToggled()
    {
    }

    void TerrainWindow::OnEraseBtnToggled()
    {
    }

    void TerrainWindow::OnBlendBtnToggled()
    {
    }

    void TerrainWindow::OnAddTextureBtn()
    {
    }

    void TerrainWindow::OnComboBox()
    {
    }

    void TerrainWindow::OnPropertyGridChangeTool()
    {
    }

    void TerrainWindow::populateToolProperties()
    {
    }

    Parameter TerrainWindow::UIElementListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto editorManager = EditorManager::getSingletonPtr();
        WP_ASSERT( editorManager );

        auto ui = editorManager->getUI();
        WP_ASSERT( ui );

        if( eventValue == ui::IUITerrainEditor::selectTerrainTextureHash )
        {
            auto resourceType = render::ITexture::typeInfo();

            if( auto resourceDatabaseDialog = ui->getResourceDatabaseDialog() )
            {
                resourceDatabaseDialog->setResourceType( resourceType );

                //if( auto selected = getSelected() )
                //{
                if( arguments.size() >= 1 )
                {
                    auto selected = arguments[0].object;
                    resourceDatabaseDialog->setCurrentObject( selected );
                }
                //}

                resourceDatabaseDialog->setPropertyName( "baseTexture" );
                resourceDatabaseDialog->setWindowVisible( true );
                resourceDatabaseDialog->populate();
            }
        }

        return {};
    }

    TerrainWindow *TerrainWindow::UIElementListener::getOwner() const
    {
        return m_owner;
    }

    void TerrainWindow::UIElementListener::setOwner( TerrainWindow *owner )
    {
        m_owner = owner;
    }

    TerrainWindow::UIElementListener::UIElementListener() = default;

    TerrainWindow::UIElementListener::~UIElementListener() = default;
}  // namespace workphone::editor
