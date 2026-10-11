#include <EditorPCH.hpp>
#include "ui/AssetDatabaseWindow.hpp"
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <Workphone/Database/AssetReferenceGraph.hpp>
#include <Workphone/Database/AssetDocumentSave.hpp>
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
        m_source = button( "Show bundled media##CatalogSource" );
        m_history = button("Operation history / Recovery##CatalogHistory");
        m_restore = button("Undo selected operation##CatalogRestore");
        m_restore->setEnabled(false);
        m_reindex = button( "Reindex selected document references##CatalogReindex" );
        m_reindex->setEnabled( false );
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
                m_tree, m_search, m_type, m_refresh, m_previous, m_next, m_source, m_history, m_restore, m_reindex };
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
        m_source = nullptr;
        m_history=nullptr;m_restore=nullptr;m_reindex=nullptr;
        m_operations={};m_operationCursors.clear();m_operationAfter.clear();
        m_mediaCatalog = nullptr;
        m_assetDatabaseManager = nullptr;
        m_browser.clear();
        EditorWindow::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    SmartPtr<AssetDatabaseManager> AssetDatabaseWindow::getAssetDatabaseManager() const
    {
        if( m_showMedia )
            return m_mediaCatalog;
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
    void AssetDatabaseWindow::setMediaCatalog( SmartPtr<AssetDatabaseManager> catalog )
    {
        if( catalog && !catalog->isReadOnly() )
        {
            WP_LOG_ERROR( "The bundled media browser requires a read-only catalog." );
            return;
        }
        m_mediaCatalog = catalog;
        refresh();
    }

    void AssetDatabaseWindow::refresh()
    {
        if( !m_search || !m_type )
            return;
        m_selectedOperation=static_cast<size_t>(-1);
        m_restore->setEnabled(false);
        m_reindex->setEnabled(false);
        m_history->setEnabled(!m_showMedia);
        if(m_showOperations && !m_showMedia)
        {
            m_operationAfter.clear();m_operationCursors.clear();
            m_operations=queryAssetOperations(getAssetDatabaseManager());
            populateOperations();return;
        }
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
        if(m_showOperations && !m_showMedia){populateOperations();return;}
        if( !m_tree || !m_status || !m_details )
            return;
        m_reindex->setEnabled( false );
        m_tree->clear();
        m_details->setText( "Select an asset to inspect its identity." );
        auto catalog = getAssetDatabaseManager();
        if( !catalog )
        {
            m_browser.clear();
            m_previous->setEnabled( false );
            m_next->setEnabled( false );
            m_status->setText( m_showMedia ? "Bundled media catalog is unavailable. See the Editor log." :
                                          "No active asset catalog. Open a project and refresh." );
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
        m_status->setText( String( m_showMedia ? "Bundled media (read-only) | " : "Project assets | " ) +
            catalog->getProjectRoot() + " | Page " +
            StringUtil::toString( m_browser.pageNumber() ) + " | " +
            StringUtil::toString( page.entries.size() ) + " assets" );
    }

    void AssetDatabaseWindow::populateOperations()
    {
        if(!m_tree || !m_status || !m_details) return;
        m_tree->clear();m_selectedOperation=static_cast<size_t>(-1);m_restore->setEnabled(false);
        m_details->setText("Select a journal record. Delete undo restores quarantined bytes; move/copy undo reverses that operation. Conflicting files are preserved.");
        m_previous->setEnabled(!m_operationCursors.empty());m_next->setEnabled(m_operations.succeeded && m_operations.hasMore);
        if(!m_operations.succeeded){m_status->setText(m_operations.error);return;}
        auto root=m_tree->addRoot();root->setId(0);Util::setText(root,"Operation history / Recovery");
        for(size_t i=0;i<m_operations.rows.size();++i)
        {
            const auto &row=m_operations.rows[i];auto node=m_tree->addNode();node->setId(i+1);
            Util::setText(node,row.source+" ["+row.kind+" / "+row.state+"]");root->addChild(node);
        }
        m_status->setText("Project journal | "+StringUtil::toString(m_operations.rows.size())+
            " records | Interrupted operations recover on catalog reopen. Conflicts require explicit repair.");
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
            else if( sender == m_source )
            {
                m_showOperations=false;m_history->setLabel("Operation history / Recovery##CatalogHistory");
                m_showMedia = !m_showMedia;
                m_source->setLabel( m_showMedia ? "Show project assets##CatalogSource" : "Show bundled media##CatalogSource" );
                refresh();
            }
            else if( sender == m_reindex )
            {
                auto node = m_tree->getSelectedTreeNode();
                const auto id = node ? node->getId() : 0;
                AssetDatabaseManager::EntrySnapshot entry;
                auto catalog = getAssetDatabaseManager();
                String error;
                if( m_showMedia || m_showOperations || !id ||
                    !m_browser.select( id - 1, catalog, entry ) ||
                    entry.kind != AssetDatabaseManager::EntryKind::File )
                    error = "Catalog changed. Refresh and select a project document again.";
                else if( reindexAssetDocument( catalog, entry.path, error, m_mediaCatalog ) )
                {
                    refresh();
                    m_details->setText( "Saved reference graph rebuilt from verified document bytes. The document was not modified." );
                    return {};
                }
                m_details->setText( error );
            }
            else if(sender==m_history)
            {
                m_showOperations=!m_showOperations;
                m_history->setLabel(m_showOperations?"Show assets##CatalogHistory":"Operation history / Recovery##CatalogHistory");
                refresh();
            }
            else if(sender==m_restore)
            {
                auto catalog=getAssetDatabaseManager();
                if(!m_operations.canUndo(catalog,m_selectedOperation))
                {m_details->setText("Operation or project session changed. Refresh and select it again.");return {};}
                const auto id=m_operations.rows[m_selectedOperation].id;
                const auto result=catalog->undoFileOperation(id);
                refresh();
                m_details->setText(result.succeeded?"Operation undone. Restored assets retain their original UUIDs; save or reload affected documents as needed.":
                    "Undo failed; existing content retained. "+result.error);
            }
            else if( sender == m_previous )
            {
                if(m_showOperations)
                {
                    if(!m_operations.session.isCurrent(getAssetDatabaseManager()))
                    {m_operations=queryAssetOperations(nullptr);populateOperations();return {};}
                    if(!m_operationCursors.empty())
                    {m_operationAfter=m_operationCursors.back();m_operationCursors.pop_back();m_operations=queryAssetOperations(getAssetDatabaseManager(),100,m_operationAfter);}
                    populateOperations();return {};
                }
                m_browser.previous( getAssetDatabaseManager() );
                populate();
            }
            else if( sender == m_next )
            {
                if(m_showOperations)
                {
                    if(m_operations.hasMore && !m_operations.rows.empty() && m_operations.session.isCurrent(getAssetDatabaseManager()))
                    {m_operationCursors.push_back(m_operationAfter);m_operationAfter=m_operations.rows.back().id;m_operations=queryAssetOperations(getAssetDatabaseManager(),100,m_operationAfter);}
                    else m_operations=queryAssetOperations(nullptr);
                    populateOperations();return {};
                }
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
                if(m_showOperations)
                {
                    const auto index=static_cast<size_t>(id-1);
                    if(index>=m_operations.rows.size() || !m_operations.session.isCurrent(getAssetDatabaseManager()))
                    {refresh();m_details->setText("Project journal changed. Select the record again.");return {};}
                    m_selectedOperation=index;const auto &row=m_operations.rows[index];
                    m_restore->setEnabled(m_operations.canUndo(getAssetDatabaseManager(),index));
                    m_restore->setLabel(row.kind=="delete"?"Restore quarantined asset##CatalogRestore":"Undo selected operation##CatalogRestore");
                    m_details->setText("Operation: "+row.id+"\nState: "+row.state+"\nSource: "+row.source+
                        "\nDestination: "+row.destination+"\nRoot: "+row.root);
                    return {};
                }
                AssetDatabaseManager::EntrySnapshot entry;
                if( !m_browser.select( id - 1, getAssetDatabaseManager(), entry ) )
                {
                    populate();
                    m_details->setText( "Catalog changed. Refresh and select the asset again." );
                }
                else
                {
                    m_reindex->setEnabled( !m_showMedia && entry.kind == AssetDatabaseManager::EntryKind::File );
                    String details = "UUID: " + entry.uuid + "\nPath: " + entry.path + "\nType: " + entry.type +
                        "\nKind: " + ( entry.kind == AssetDatabaseManager::EntryKind::File ? "File" : "Scene object" );
                    // References to bundled media are owned by project documents.
                    const auto showingMedia=m_showMedia;
                    m_showMedia=false;
                    auto project=getAssetDatabaseManager();
                    m_showMedia=showingMedia;
                    const auto references=queryAssetReferences(project,entry.uuid,true,showingMedia?"media":"project");
                    if(!references.succeeded) details+="\nReference graph: "+references.error;
                    else
                    {
                        details+="\nIndexed saved-document users: "+StringUtil::toString(references.edges.size())+
                            (references.hasMore?" (first page; additional users exist)":"");
                        for(const auto &edge:references.edges)
                            details+="\n"+edge.sourcePath+" | "+edge.slot+" | "+edge.expectedType+
                                (edge.required?" | required":" | optional")+" | "+edge.sourceState+
                                (edge.diagnostic.empty()?"":" | "+edge.diagnostic);
                        details+="\nCoverage: indexed saves only; unsaved edits and unindexed legacy documents are not represented.";
                    }
                    m_details->setText(details);
                }
            }
        }
        return {};
    }

    Parameter AssetDatabaseWindow::EventListener::handleEvent( EventType eventType, hash_type eventValue,
        const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
        SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        auto weak = m_owner;
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
