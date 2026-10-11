#include <EditorPCH.hpp>
#include "ui/AssetDatabaseWindow.hpp"
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, AssetDatabaseWindow, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, AssetDatabaseWindow::EventListener, IEventListener );

    AssetDatabaseWindow::~AssetDatabaseWindow()
    {
        unload( nullptr );
    }

    void AssetDatabaseWindow::load( SmartPtr<ISharedObject> data )
    {
        if( getParentWindow() )
            return;
        auto application = core::IApplicationManager::instancePtr();
        auto ui = application ? application->getUI() : nullptr;
        if( !ui )
            return;
        setLoadingState( LoadingState::Loading );
        auto window = ui->addElementByType<ui::IUIWindow>();
        setParentWindow( window );
        window->setLabel( "Asset Database" );
        if( auto parent = getParent() )
            parent->addChild( window );
        auto listener = make_ptr<EventListener>();
        listener->setOwner( this );
        setEventListener( listener );

        m_search = ui->addElementByType<ui::IUITextEntry>();
        m_search->setLabel( "Search path or UUID##CatalogSearch" );
        m_search->setPlaceholder( "Literal substring (case sensitive)" );
        m_search->addObjectListener( listener );
        window->addChild( m_search );
        m_type = ui->addElementByType<ui::IUITextEntry>();
        m_type->setLabel( "Asset type##CatalogType" );
        m_type->setPlaceholder( "Exact type, or empty for all" );
        m_type->addObjectListener( listener );
        window->addChild( m_type );
        auto button = [&]( const String &label ) {
            auto result = ui->addElementByType<ui::IUIButton>();
            result->setLabel( label );
            result->addObjectListener( listener );
            window->addChild( result );
            return result;
        };
        m_refresh = button( "Search / Refresh##CatalogRefresh" );
        m_previous = button( "Previous##CatalogPrevious" );
        m_previous->setSameLine( true );
        m_next = button( "Next##CatalogNext" );
        m_next->setSameLine( true );
        m_status = ui->addElementByType<ui::IUIText>();
        window->addChild( m_status );
        m_tree = ui->addElementByType<ui::IUITreeCtrl>();
        m_tree->addObjectListener( listener );
        window->addChild( m_tree );
        m_details = ui->addElementByType<ui::IUIText>();
        window->addChild( m_details );
        setLoadingState( LoadingState::Loaded );
        refresh();
    }

    void AssetDatabaseWindow::unload( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() == LoadingState::Unloaded )
            return;
        setLoadingState( LoadingState::Unloading );
        auto listener = getEventListener();
        if( listener )
        {
            listener->unload( nullptr );
            Array<SmartPtr<ui::IUIElement>> controls = {
                m_tree, m_search, m_type, m_refresh, m_previous, m_next };
            for( auto &control : controls )
                if( control )
                    control->removeObjectListener( listener );
            setEventListener( nullptr );
        }
        auto application = core::IApplicationManager::instancePtr();
        if( auto window = getParentWindow() )
        {
            if( auto parent = getParent() )
                parent->removeChild( window );
            if( application )
                if( auto ui = application->getUI() )
                    ui->removeElement( window );
            setParentWindow( nullptr );
        }
        m_tree = nullptr;
        m_search = nullptr;
        m_type = nullptr;
        m_status = nullptr;
        m_details = nullptr;
        m_refresh = nullptr;
        m_previous = nullptr;
        m_next = nullptr;
        m_assetDatabaseManager = nullptr;
        m_browser.clear();
        EditorWindow::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    SmartPtr<AssetDatabaseManager> AssetDatabaseWindow::getAssetDatabaseManager() const
    {
        if( m_assetDatabaseManager )
            return m_assetDatabaseManager;
        auto application = core::IApplicationManager::instancePtr();
        auto resources = application ? application->getResourceDatabase() : nullptr;
        auto catalog = resources ? dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() )
                                 : nullptr;
        String projectRoot, error;
        // Project activation may precede resource-service rebinding. Do not
        // expose the previous project's catalog during that transition.
        if( !catalog || application->getProjectPath().empty() ||
            !canonicalAssetCatalogRoot( application->getProjectPath(), projectRoot, error ) ||
            projectRoot != catalog->getProjectRoot() )
            return nullptr;
        return catalog;
    }

    void AssetDatabaseWindow::setAssetDatabaseManager( SmartPtr<AssetDatabaseManager> manager )
    {
        m_assetDatabaseManager = manager;
        refresh();
    }

    void AssetDatabaseWindow::refresh()
    {
        if( !m_search || !m_type )
            return;
        AssetDatabaseManager::EntryQuery query;
        query.search = m_search->getText();
        query.type = m_type->getText();
        m_browser.refresh( getAssetDatabaseManager(), query );
        populate();
    }

    void AssetDatabaseWindow::setWindowVisible( bool visible )
    {
        if( visible )
            refresh();
        EditorWindow::setWindowVisible( visible );
    }

    void AssetDatabaseWindow::populate()
    {
        if( !m_tree || !m_status || !m_details )
            return;
        m_tree->clear();
        m_details->setText( "Select an asset to inspect its identity." );
        auto catalog = getAssetDatabaseManager();
        if( !catalog )
        {
            m_browser.clear();
            m_previous->setEnabled( false );
            m_next->setEnabled( false );
            m_status->setText( "No active asset catalog. Open a project and refresh." );
            return;
        }
        const auto &page = m_browser.page();
        m_previous->setEnabled( m_browser.hasPrevious() );
        m_next->setEnabled( page.succeeded && page.hasMore );
        if( !page.succeeded )
        {
            m_status->setText( page.error );
            return;
        }
        auto root = m_tree->addRoot();
        root->setId( 0 );
        Util::setText( root, "Assets" );
        for( size_t i = 0; i < page.entries.size(); ++i )
        {
            const auto &entry = page.entries[i];
            auto node = m_tree->addNode();
            node->setId( i + 1 );
            Util::setText( node, entry.path + " [" + entry.type + "] " + entry.uuid );
            root->addChild( node );
        }
        m_status->setText( catalog->getProjectRoot() + " | Page " +
            StringUtil::toString( m_browser.pageNumber() ) + " | " +
            StringUtil::toString( page.entries.size() ) + " assets" );
    }

    Parameter AssetDatabaseWindow::handleEvent( EventType eventType, hash_type eventValue,
        const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
        SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( getLoadingState() != LoadingState::Loaded )
            return {};
        if( eventValue == IEvent::handleValueChanged && ( sender == m_search || sender == m_type ) )
        {
            // Literal searches may scan the catalog. Apply explicitly instead of
            // issuing a synchronous database scan for every typed character.
            m_browser.clear();
            populate();
            m_status->setText( "Press Search / Refresh to apply the filters." );
        }
        else if( eventValue == IEvent::handleSelection )
        {
            if( sender == m_refresh )
                refresh();
            else if( sender == m_previous )
            {
                m_browser.previous( getAssetDatabaseManager() );
                populate();
            }
            else if( sender == m_next )
            {
                m_browser.next( getAssetDatabaseManager() );
                populate();
            }
        }
        else if( eventValue == IEvent::handleTreeSelectionRelease && sender == m_tree )
        {
            auto node = m_tree->getSelectedTreeNode();
            const auto id = node ? node->getId() : 0;
            if( id > 0 )
            {
                AssetDatabaseManager::EntrySnapshot entry;
                if( !m_browser.select( id - 1, getAssetDatabaseManager(), entry ) )
                {
                    populate();
                    m_details->setText( "Catalog changed. Refresh and select the asset again." );
                }
                else
                    m_details->setText( "UUID: " + entry.uuid + "\nPath: " + entry.path +
                        "\nType: " + entry.type + "\nKind: " +
                        ( entry.kind == AssetDatabaseManager::EntryKind::File ? "File" : "Scene object" ) );
            }
        }
        return {};
    }

    Parameter AssetDatabaseWindow::EventListener::handleEvent( EventType eventType, hash_type eventValue,
        const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
        SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        auto weak = m_owner.load();
        if( auto owner = weak.lock() )
            return owner->handleEvent( eventType, eventValue, arguments, sender, object, event );
        return {};
    }
    void AssetDatabaseWindow::EventListener::setOwner( SmartPtr<AssetDatabaseWindow> owner )
    {
        m_owner = owner;
    }
    void AssetDatabaseWindow::EventListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }
}
