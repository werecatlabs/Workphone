#include <EditorPCH.hpp>
#include "TerrainWindow.hpp"
#include <editor/EditorManager.hpp>
#include <ui/UIManager.hpp>
#include <ui/ResourceDatabaseDialog.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainRecipes.hpp>
#include <cmath>
#include <limits>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, TerrainWindow, ScriptWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, TerrainWindow::UIElementListener, IEventListener );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, TerrainWindow::ViewportListener, IEventListener );

    TerrainWindow::TerrainWindow()
    {
        static const auto className = String( "TerrainEditor" );
        setClassName( className );
    }

    TerrainWindow::~TerrainWindow() = default;

    u64 TerrainWindow::getViewportScriptGeneration() const
    {
        return m_viewportScriptGeneration.load( std::memory_order_acquire );
    }

    void TerrainWindow::invalidateViewportScriptState()
    {
        std::lock_guard<std::mutex> lock( m_viewportScriptStateMutex );
        m_viewportScriptGeneration.fetch_add( 1, std::memory_order_acq_rel );
        m_recipePreview.clear();
        m_viewportBrushEnabled = false;
        m_viewportBrushPickHeightfield = false;
        m_viewportBrushRadius = 0.0;
    }

    void TerrainWindow::refreshViewportScriptState()
    {
        if( Thread::getCurrentTask() != TaskId::Application ) return;
        auto app = core::IApplicationManager::instancePtr();
        if( !app || app->getQuit() ) return;
        auto scripts = app ? app->getScriptManager() : nullptr;
        if( scripts && scripts->isDebuggerPaused() ) return;
        const auto generation = getViewportScriptGeneration();
        Parameters recipe, state;
        if( isLoaded() && isWindowVisible() )
            if( auto invoker = getInvoker() )
            {
                // These getters used to run synchronously inside ImGui. Refresh
                // the explicit preview snapshot outside the native render frame.
                invoker->callObjectMember( "getViewportScriptState", {}, state );
            }
        double brushRadius = 0.0;
        if( state.size() == 3 || state.size() == 5 )
        {
            const auto &r = state[1];
            brushRadius = r.type == ParameterType::PARAM_TYPE_F32 ? r.getF32() : r.getF64();
        }
        const bool brushEnabled = ( state.size() == 3 || state.size() == 5 ) && state[0].getBool();
        const bool brushPickHeightfield = brushEnabled && state[2].getBool();
        if( state.size() == 5 ) recipe = Parameters{ state[3], state[4] };
        bool cancelStroke = false;
        {
            std::lock_guard<std::mutex> lock( m_viewportScriptStateMutex );
            if( generation != getViewportScriptGeneration() ) return;
            cancelStroke = m_viewportBrushEnabled && !brushEnabled;
            m_recipePreview = std::move( recipe );
            m_viewportBrushEnabled = brushEnabled;
            m_viewportBrushPickHeightfield = brushPickHeightfield;
            m_viewportBrushRadius = brushRadius;
        }
        if( cancelStroke ) cancelViewportStroke();
    }

    void TerrainWindow::load( SmartPtr<ISharedObject> data )
    {
        ScriptWindow::load( data );
        attachViewport();
    }

    void TerrainWindow::attachViewport()
    {
        auto application = core::IApplicationManager::instancePtr();
        auto viewport = application ? application->getSceneRenderWindow() : nullptr;
        auto previous = m_sceneViewport.load().lock();
        if( previous == viewport )
        {
            return;
        }
        invalidateViewportScriptState();
        cancelViewportStroke();
        if( previous && m_viewportListener )
        {
            previous->removeObjectListener( m_viewportListener );
        }
        m_sceneViewport = viewport;
        if( !m_viewportListener )
        {
            m_viewportListener = make_ptr<ViewportListener>();
            m_viewportListener->setOwner( this );
        }
        if( viewport )
        {
            viewport->addObjectListener( m_viewportListener );
        }
    }

    void TerrainWindow::cancelViewportStroke()
    {
        if( auto invoker = getInvoker() )
        {
            callScriptMemberWithDiscardedResults(
                "handleViewportBrushEvent",
                { Parameter( "cancel" ), Parameter( 0.0 ), Parameter( 0.0 ), Parameter( 0.0 ),
                  Parameter( 0.0 ), Parameter( -1.0 ), Parameter( 0.0 ) } );
        }
    }

    void TerrainWindow::setWindowVisible( bool visible )
    {
        if( !visible )
        {
            invalidateViewportScriptState();
            cancelViewportStroke();
        }
        else
        {
            attachViewport();
        }
        ScriptWindow::setWindowVisible( visible );
    }

    void TerrainWindow::unload( SmartPtr<ISharedObject> data )
    {
        invalidateViewportScriptState();
        cancelViewportStroke();
        if( auto viewport = m_sceneViewport.load().lock() )
        {
            if( m_viewportListener )
            {
                viewport->removeObjectListener( m_viewportListener );
            }
        }
        m_sceneViewport = nullptr;
        if( m_viewportListener )
        {
            m_viewportListener->setOwner( nullptr );
        }
        m_viewportListener = nullptr;
        m_terrainEditor = nullptr;
        ScriptWindow::unload( data );
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
            invalidateViewportScriptState();
            cancelViewportStroke();
            attachViewport();
            ScriptWindow::updateSelection();

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

    Parameter TerrainWindow::ViewportListener::handleEvent( EventType type, hash_type value,
                                                            const Array<Parameter> &arguments,
                                                            SmartPtr<ISharedObject> sender,
                                                            SmartPtr<ISharedObject> object,
                                                            SmartPtr<IEvent> event )
    {
        auto owner = m_owner.load().lock();
        if( !owner || !owner->isLoaded() || !owner->isWindowVisible() )
        {
            return {};
        }
        auto invoker = owner->getInvoker();
        if( !invoker )
        {
            return {};
        }
        if( value == StringUtil::getHash( "terrainRecipePreview" ) )
        {
            std::lock_guard<std::mutex> lock( owner->m_viewportScriptStateMutex );
            return Parameter( owner->m_recipePreview );
        }
        if( value != StringUtil::getHash( "terrainViewportBrush" ) || arguments.size() != 7 )
        {
            return {};
        }
        bool enabled = false;
        bool pickHeightfield = false;
        f64 cachedRadius = 0.0;
        u64 generation = 0;
        {
            std::lock_guard<std::mutex> lock( owner->m_viewportScriptStateMutex );
            enabled = owner->m_viewportBrushEnabled;
            pickHeightfield = owner->m_viewportBrushPickHeightfield;
            cachedRadius = owner->m_viewportBrushRadius;
            generation = owner->getViewportScriptGeneration();
        }
        if( !enabled )
        {
            return {};
        }
        const auto &phaseArgument = arguments.front();
        const auto phase = phaseArgument.type == ParameterType::PARAM_TYPE_CHAR_PTR
            ? String( phaseArgument.getCharPtr() ? phaseArgument.getCharPtr() : "" )
            : phaseArgument.getStr();
        if( phase != "hover" )
        {
            if( auto editor = EditorManager::getSingletonPtr() )
                if( auto ui = editor->getUI() )
                    ui->deferScriptCall( owner.get(), "handleViewportBrushEvent", arguments, true, true,
                                         generation );
        }
        // Return world-space ring samples to the viewport's existing draw list.
        // Authoring state and brush execution stay in the Lua terrain editor.
        Parameters preview{ Parameter( true ) };
        auto app = core::IApplicationManager::instancePtr();
        auto selection = app ? app->getSelectionManager() : nullptr;
        if( !selection )
        {
            return Parameter( preview );
        }
        SmartPtr<scene::TerrainSystem> terrain;
        for( auto selected : selection->getSelection() )
        {
            terrain = dynamic_pointer_cast<scene::TerrainSystem>( selected );
            if( !terrain )
            {
                if( auto actor = dynamic_pointer_cast<scene::IGameActor>( selected ) )
                {
                    terrain = actor->getComponent<scene::TerrainSystem>();
                }
            }
            if( terrain )
            {
                break;
            }
        }
        if( !terrain )
        {
            return Parameter( preview );
        }
        const f64 radius = cachedRadius;
        if( !std::isfinite( radius ) || radius <= 0 )
        {
            return Parameter( preview );
        }
        const Ray3F ray(
            { f32( arguments[1].getF64() ), f32( arguments[2].getF64() ), f32( arguments[3].getF64() ) },
            { f32( arguments[4].getF64() ), f32( arguments[5].getF64() ),
              f32( arguments[6].getF64() ) } );
        const auto snapshot = terrain->getTerrainSnapshot();
        auto actor = terrain->getActor();
        const auto placement = actor ? actor->getWorldTransform() : Transform3<real_Num>();
        Vector3<real_Num> hit;
        const bool picked = snapshot && ( pickHeightfield
            ? render::intersectTerrainHeightfieldForAuthoring( *snapshot, placement, ray, hit )
            : render::intersectTerrain( *snapshot, placement, ray, hit ) );
        if( !picked )
        {
            return Parameter( preview );
        }
        const auto matrix = placement.getTransformationMatrix();
        const auto local = matrix.inverse().transformAffine( hit );
        for( u32 i = 0; i <= 64; ++i )
        {
            const auto angle = i * 6.283185307179586 / 64;
            auto world = matrix.transformAffine( Vector3<real_Num>(
                local.x + radius * std::cos( angle ), 0, local.z + radius * std::sin( angle ) ) );
            f32 height = 0;
            const bool sampled = pickHeightfield
                ? render::sampleTerrainHeightForAuthoring( *snapshot, placement, world, height )
                : render::sampleTerrainHeight( *snapshot, placement, world, height );
            if( !sampled )
            {
                world.y = std::numeric_limits<real_Num>::quiet_NaN();
            }
            else
            {
                world.y = height + 0.01f;
            }
            Parameter point;
            point.setVector3( world );
            preview.push_back( point );
        }
        return Parameter( preview );
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

    Parameter TerrainWindow::UIElementListener::handleEvent( EventType eventType, hash_type eventValue,
                                                             const Array<Parameter> &arguments,
                                                             SmartPtr<ISharedObject> sender,
                                                             SmartPtr<ISharedObject> object,
                                                             SmartPtr<IEvent> event )
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
