#include <EditorPCH.hpp>
#include <ui/ResourceDatabaseDialog.hpp>
#include <commands/ModifyPropertyCmd.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <Workphone/Database/ResourceDatabase.hpp>
#include <Workphone/System/FileSelection.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, ResourceDatabaseDialog, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, ResourceDatabaseDialog::UIElementListener, IEventListener );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, ResourceDatabaseDialog::DragSource, ui::IUIDragSource );

    ResourceDatabaseDialog::~ResourceDatabaseDialog() { unload( nullptr ); }
    void ResourceDatabaseDialog::load( SmartPtr<ISharedObject> data )
    {
        if( getParentWindow() )
            return;
        auto app = core::IApplicationManager::instancePtr();
        auto ui = app ? app->getUI() : nullptr;
        if( !ui )
            return;
        setLoadingState( LoadingState::Loading );
        auto window = ui->addElementByType<ui::IUIWindow>();
        window->setLabel( "Select asset reference" );
        setParentWindow( window );
        if( auto parent = getParent() )
            parent->addChild( window );
        m_uiListener = make_ptr<UIElementListener>();
        m_uiListener->setOwner( this );
        m_search = ui->addElementByType<ui::IUITextEntry>();
        m_search->setLabel( "Path or UUID##ResourcePickerSearch" );
        m_search->setPlaceholder( "Literal, case-sensitive search" );
        m_search->addObjectListener( m_uiListener );
        window->addChild( m_search );
        auto button = [&]( const String &label, WidgetId id ) {
            auto result = ui->addElementByType<ui::IUIButton>();
            result->setLabel( label );
            result->setElementId( id );
            result->addObjectListener( m_uiListener );
            window->addChild( result );
            return result;
        };
        m_refresh = button( "Search / Refresh##PickerRefresh", AddComponent );
        m_source = button( "Show bundled media##PickerSource", Source );
        m_select = button( "Select##PickerSelect", Select );
        m_select->setSameLine( true );
        m_previous = button( "Previous##PickerPrevious", Previous );
        m_previous->setSameLine( true );
        m_next = button( "Next##PickerNext", Next );
        m_next->setSameLine( true );
        m_status = ui->addElementByType<ui::IUIText>();
        window->addChild( m_status );
        m_tree = ui->addElementByType<ui::IUITreeCtrl>();
        m_tree->setMultiSelect( false );
        m_tree->setElementId( Tree );
        m_tree->addObjectListener( m_uiListener );
        window->addChild( m_tree );
        m_dragSource = make_ptr<DragSource>();
        m_dragSource->setOwner( this );
        m_tree->setDragSource( m_dragSource );
        setLoadingState( LoadingState::Loaded );
        populate();
    }
    void ResourceDatabaseDialog::unload( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() == LoadingState::Unloaded && !getParentWindow() )
            return;
        setLoadingState( LoadingState::Unloading );
        if( m_uiListener )
        {
            m_uiListener->unload( nullptr );
            Array<SmartPtr<ui::IUIElement>> controls = { m_tree, m_search, m_refresh, m_select, m_previous, m_next, m_source };
            for( auto &control : controls )
                if( control )
                    control->removeObjectListener( m_uiListener );
        }
        if( m_dragSource )
            m_dragSource->unload( nullptr );
        if( m_tree )
            m_tree->setDragSource( nullptr );
        auto app = core::IApplicationManager::instancePtr();
        if( auto window = getParentWindow() )
        {
            if( auto parent = getParent() )
                parent->removeChild( window );
            if( app )
                if( auto ui = app->getUI() )
                    ui->removeElement( window );
            setParentWindow( nullptr );
        }
        m_tree = nullptr;
        m_search = nullptr;
        m_status = nullptr;
        m_refresh = nullptr;
        m_select = nullptr;
        m_previous = nullptr;
        m_next = nullptr;
        m_source = nullptr;
        m_uiListener = nullptr;
        m_dragSource = nullptr;
        m_currentObject = nullptr;
        m_targets.clear();
        m_browser.clear();
        EditorWindow::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }
    SmartPtr<AssetDatabaseManager> ResourceDatabaseDialog::catalog() const
    {
        auto app = core::IApplicationManager::instancePtr();
        auto resources = app ? app->getResourceDatabase() : nullptr;
        auto result = resources ? dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() )
                                : nullptr;
        String root, error;
        if( !result || app->getProjectPath().empty() ||
            !canonicalAssetCatalogRoot( app->getProjectPath(), root, error ) || root != result->getProjectRoot() )
            return nullptr;
        if( m_showMedia )
        {
            auto concrete = dynamic_pointer_cast<ResourceDatabase>( resources );
            return concrete ? concrete->getMediaCatalog() : nullptr;
        }
        return result;
    }
    void ResourceDatabaseDialog::captureTargets()
    {
        m_targets.clear();
        if( auto target = getCurrentObject() )
            m_targets.push_back( target );
        else
        {
            auto app = core::IApplicationManager::instancePtr();
            auto selection = app ? app->getSelectionManager() : nullptr;
            if( selection )
                for( auto target : selection->getSelection() )
                {
                    if( auto file = dynamic_pointer_cast<FileSelection>( target ) )
                    {
                        auto resources = app->getResourceDatabase();
                        target = resources ? resources->loadResource( file->getFilePath() ) : nullptr;
                    }
                    m_targets.push_back( target );
                    if( m_targets.size() > 256 ) break;
                }
        }
    }
    void ResourceDatabaseDialog::setWindowVisible( bool visible )
    {
        if( visible )
        {
            captureTargets();
            populate();
        }
        else
        {
            m_targets.clear();
            m_currentObject = nullptr;
            m_browser.clear();
        }
        EditorWindow::setWindowVisible( visible );
    }
    void ResourceDatabaseDialog::updateSelection()
    {
        // UI callers execute on the owning thread. Never queue a raw window or
        // mutate controls from the worker job queue.
        populate();
    }
    void ResourceDatabaseDialog::populate()
    {
        if( !m_search )
            return;
        AssetDatabaseManager::EntryQuery query;
        query.search = m_search->getText();
        if( m_resourceType == render::IMaterial::typeInfo() ) query.type = "Material";
        else if( m_resourceType == render::ITexture::typeInfo() ) query.type = "Texture";
        else if( m_resourceType == scene::IGameActor::typeInfo() ) query.type = "Actor";
        else if( m_resourceType == ScriptAsset::typeInfo() ) query.type = "script";
        m_browser.refresh( catalog(), query );
        renderPage();
    }
    void ResourceDatabaseDialog::renderPage()
    {
        if( !m_tree || !m_status )
            return;
        m_tree->clear();
        const auto &page = m_browser.page();
        m_previous->setEnabled( m_browser.hasPrevious() );
        m_next->setEnabled( page.succeeded && page.hasMore );
        m_select->setEnabled( false );
        if( !page.succeeded )
        {
            m_status->setText( page.error );
            return;
        }
        auto root = m_tree->addRoot();
        root->setId( 0 );
        Util::setText( root, "Compatible assets" );
        size_t count = 0;
        for( size_t i = 0; i < page.entries.size(); ++i )
        {
            const auto &entry = page.entries[i];
            if( !AssetReferenceChange::typeMatches( entry.type, m_resourceType ) )
                continue;
            auto node = m_tree->addNode();
            node->setId( i + 1 );
            Util::setText( node, entry.path + " [" + entry.type + "] " + entry.uuid );
            root->addChild( node );
            ++count;
        }
        m_select->setEnabled( count != 0 && !m_targets.empty() );
        m_status->setText( String( m_showMedia ? "Bundled media | " : "Project assets | " ) + StringUtil::toString( count ) + " compatible assets on catalog page " +
                          StringUtil::toString( m_browser.pageNumber() ) );
    }
    bool ResourceDatabaseDialog::entryForNode( SmartPtr<ui::IUITreeNode> node,
        AssetDatabaseManager::EntrySnapshot &entry )
    {
        const auto id = node ? node->getId() : 0;
        return id > 0 && m_browser.select( id - 1, catalog(), entry ) &&
               AssetReferenceChange::typeMatches( entry.type, m_resourceType );
    }
    void ResourceDatabaseDialog::select()
    {
        if( !m_tree )
            return;
        AssetDatabaseManager::EntrySnapshot entry;
        if( !entryForNode( m_tree->getSelectedTreeNode(), entry ) )
        {
            renderPage();
            m_status->setText( "Selection expired or incompatible. Search / Refresh and select again." );
            return;
        }
        Array<SmartPtr<ISharedObject>> targets;
        for( auto &weak : m_targets )
        {
            auto target = weak.lock();
            if( !target )
            {
                m_status->setText( "An assignment target was destroyed. Reopen the picker." );
                return;
            }
            targets.push_back( target );
        }
        assign( targets, entry );
    }
    void ResourceDatabaseDialog::selectObject( SmartPtr<ISharedObject> target, const String &uuid )
    {
        for( size_t i = 0; i < m_browser.page().entries.size(); ++i )
            if( m_browser.page().entries[i].uuid == uuid )
            {
                AssetDatabaseManager::EntrySnapshot entry;
                if( m_browser.select( i, catalog(), entry ) )
                    assign( { target }, entry );
                return;
            }
    }
    void ResourceDatabaseDialog::assign( const Array<SmartPtr<ISharedObject>> &targets,
        const AssetDatabaseManager::EntrySnapshot &entry )
    {
        auto app = core::IApplicationManager::instancePtr();
        auto commands = app ? app->getCommandManager() : nullptr;
        auto resources = app ? app->getResourceDatabase() : nullptr;
        auto active = catalog();
        if( !commands || !resources || !active || targets.empty() ||
            !AssetReferenceChange::typeMatches( entry.type, m_resourceType ) || !active->isEntryCurrent( entry ) )
        {
            m_status->setText( "Assignment services, targets or catalog selection are no longer valid." );
            return;
        }
        try
        {
            // Resolve only the chosen resource. Properties' typed UUID readers
            // then reuse the production resource managers.
            auto object = resources->getObject( StringUtil::parseUUID( entry.uuid ) );
            auto types = TypeManager::instance();
            if( !object || !types || !types->isDerived( object->getTypeInfo(), m_resourceType ) ||
                !object->getHandle() || object->getHandle()->getUUIDAsString() != entry.uuid ||
                !active->isEntryCurrent( entry ) )
            {
                m_status->setText( "The selected asset could not resolve to the requested runtime type." );
                return;
            }
            auto command = make_ptr<ModifyPropertyCmd>();
            if( !command->configure( active, entry, targets, m_propertyName, m_resourceType ) )
            {
                m_status->setText( command->getError() );
                return;
            }
            command->execute();
            if( !command->wasApplied() )
            {
                m_status->setText( command->getError() );
                return;
            }
            commands->addCommand( command );
            m_selectedObject = entry.uuid;
            m_status->setText( "Reference assigned. Undo/redo is available; save the owning document to persist." );
        }
        catch( const std::exception &e )
        {
            m_status->setText( String( "Reference assignment failed: " ) + e.what() );
        }
    }
    String ResourceDatabaseDialog::getSelectedObject() const { return m_selectedObject; }
    void ResourceDatabaseDialog::setSelectedObject( const String &value ) { m_selectedObject = value; }
    SmartPtr<ui::IUITreeCtrl> ResourceDatabaseDialog::getTree() const { return m_tree; }
    void ResourceDatabaseDialog::setTree( SmartPtr<ui::IUITreeCtrl> tree ) { m_tree = tree; }
    SmartPtr<ISharedObject> ResourceDatabaseDialog::getCurrentObject() const { return m_currentObject.lock(); }
    void ResourceDatabaseDialog::setCurrentObject( SmartPtr<ISharedObject> object ) { m_currentObject = object; }
    String ResourceDatabaseDialog::getPropertyName() const { return m_propertyName; }
    void ResourceDatabaseDialog::setPropertyName( const String &name ) { m_propertyName = name; }

    Parameter ResourceDatabaseDialog::UIElementListener::handleEvent( EventType eventType, hash_type eventValue,
        const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
        SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        auto owner = getOwner();
        if( !owner || owner->getLoadingState() != LoadingState::Loaded )
            return {};
        if( eventValue == IEvent::handleValueChanged && sender == owner->m_search )
        {
            owner->m_browser.clear();
            owner->renderPage();
            owner->m_status->setText( "Press Search / Refresh to apply the filter." );
        }
        else if( eventValue == IEvent::handleSelection )
        {
            if( sender == owner->m_refresh ) owner->populate();
            else if( sender == owner->m_source )
            {
                owner->m_showMedia = !owner->m_showMedia;
                owner->m_source->setLabel( owner->m_showMedia ? "Show project assets##PickerSource" : "Show bundled media##PickerSource" );
                owner->populate();
            }
            else if( sender == owner->m_select ) owner->select();
            else if( sender == owner->m_previous )
            {
                owner->m_browser.previous( owner->catalog() );
                owner->renderPage();
            }
            else if( sender == owner->m_next )
            {
                owner->m_browser.next( owner->catalog() );
                owner->renderPage();
            }
        }
        return {};
    }
    SmartPtr<ResourceDatabaseDialog> ResourceDatabaseDialog::UIElementListener::getOwner() const
    {
        auto weak = m_owner.load();
        return weak.lock();
    }
    void ResourceDatabaseDialog::UIElementListener::setOwner( SmartPtr<ResourceDatabaseDialog> owner ) { m_owner = owner; }
    void ResourceDatabaseDialog::UIElementListener::unload( SmartPtr<ISharedObject> data ) { m_owner = nullptr; }
    Parameter ResourceDatabaseDialog::DragSource::handleEvent( EventType eventType, hash_type eventValue,
        const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
        SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::handleDrag )
            return Parameter( handleDrag( Vector2I::zero(), dynamic_pointer_cast<ui::IUIElement>( sender ) ) );
        return {};
    }
    String ResourceDatabaseDialog::DragSource::handleDrag( const Vector2I &position, SmartPtr<ui::IUIElement> element )
    {
        auto owner = getOwner();
        auto node = dynamic_pointer_cast<ui::IUITreeNode>( element );
        AssetDatabaseManager::EntrySnapshot entry;
        if( !owner || owner->getLoadingState() != LoadingState::Loaded || !owner->entryForNode( node, entry ) )
            return {};
        auto data = make_ptr<Properties>();
        data->setProperty( "sourceId", node->getTreeNodeId() );
        data->setProperty( "resourceUUID", entry.uuid );
        data->setProperty( "resourceType", entry.type );
        return DataUtil::toString( data.get(), true );
    }
    SmartPtr<ResourceDatabaseDialog> ResourceDatabaseDialog::DragSource::getOwner() const
    {
        auto weak = m_owner.load();
        return weak.lock();
    }
    void ResourceDatabaseDialog::DragSource::setOwner( SmartPtr<ResourceDatabaseDialog> owner ) { m_owner = owner; }
    void ResourceDatabaseDialog::DragSource::unload( SmartPtr<ISharedObject> data ) { m_owner = nullptr; }
}
