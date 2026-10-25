#pragma once
#include <Workphone/Database/AssetReferenceChange.hpp>
#ifdef WP_EDITOR_ASSET_COMMAND_CONTRACTS
#include <commands/ModifyPropertyCmd.hpp>
#endif

namespace workphone
{
    class ReferenceContractTarget : public ISharedObject
    {
    public:
        ReferenceContractTarget()
        {
            data = make_ptr<Properties>();
            data->setProperty( "texture", "old-value" );
            data->getPropertyObject( "texture" ).setAttribute( "resourceType", "Texture" );
            data->getPropertyObject( "texture" ).setAttribute( "label", "Surface texture" );
            data->setProperty( "unrelated", "keep" );
        }
        SmartPtr<Properties> getProperties() const override { return make_ptr<Properties>( *data ); }
        void setProperties( SmartPtr<Properties> replacement ) override
        {
            if( reject && replacement->getProperty( "texture" ) != "old-value" ) return;
            auto nested = resolvePropertyGroup( replacement, nestedPath );
            const bool nestedChanged = nested && nested->getProperty( "texture" ) != "nested-old";
            if( rejectNested && nestedChanged ) return;
            data = make_ptr<Properties>( *replacement );
            if( throwNested && nestedChanged ) throw std::runtime_error( "Nested setter rejected after writing" );
            if( regenerateType )
                data->getPropertyObject( "texture" ).setAttribute( "resourceType",
                    data->getProperty( "texture" ) == "old-value" ? String( "Texture" ) :
                    String( TypeManager::instance()->getName( render::ITexture::typeInfo() ) ) );
        }
        SmartPtr<Properties> data;
        bool reject = false;
        bool regenerateType = false;
        PropertyGroupPath nestedPath;
        bool rejectNested = false;
        bool throwNested = false;
        WP_CLASS_REGISTER_DECL;
    };
    WP_CLASS_REGISTER_DERIVED( workphone, ReferenceContractTarget, ISharedObject );
}
namespace
{
    using namespace workphone;
    void assetReferenceChangeContracts( const std::filesystem::path &folder )
    {
        auto check = []( bool ok, const char *message ) {
            if( !ok ) throw std::runtime_error( message );
        };
        auto app = core::IApplicationManager::instancePtr();
        const auto oldRoot = app->getProjectPath();
        auto oldResources = app->getResourceDatabase();
        struct Restore
        {
            SmartPtr<core::IApplicationManager> app;
            String root;
            SmartPtr<IResourceDatabase> resources;
            ~Restore() { app->setProjectPath( root ); app->setResourceDatabase( resources ); }
        } restore{ app, oldRoot, oldResources };
        app->setProjectPath( folder.u8string().c_str() );
        auto catalog = make_ptr<AssetDatabaseManager>();
        const auto path = folder / "references.db";
        catalog->setProjectRoot( folder.u8string().c_str() );
        catalog->loadFromFile( path.u8string().c_str() );
        auto resources = make_ptr<ResourceDatabase>();
        resources->setDatabaseManager( catalog );
        app->setResourceDatabase( resources );
        auto bound = dynamic_cast<IParameterizedDatabase *>( catalog->getDatabase().get() );
        const auto id = StringUtil::getUUID();
        check( bound->queryBound( "INSERT INTO resources(uuid,path,path_key,type,kind) VALUES(?,'surface.png','surface.png','Texture','file')", { id } ) != nullptr,
               "Reference fixture must seed" );
        AssetDatabaseManager::EntrySnapshot entry;
        check( catalog->tryGetEntry( id, entry ), "Reference fixture must capture a detached identity" );
        auto target = make_ptr<ReferenceContractTarget>();
        target->regenerateType = true;
        check( AssetReferenceChange::referenceType( "Texture" ) == render::ITexture::typeInfo() &&
               AssetReferenceChange::referenceType( TypeManager::instance()->getName( render::ITexture::typeInfo() ) ) == render::ITexture::typeInfo() &&
               AssetReferenceChange::referenceType( "unknown-reference-family" ) == 0,
               "Reference family labels and registered names must resolve without inventing types" );
        AssetReferenceChange wrong;
        check( !AssetReferenceChange::typeMatches( "Texture", 0 ) &&
               !AssetReferenceChange::typeMatches( "Texture", TypeManager::instance()->getTotalNumTypes() + 100 ),
               "Invalid requested type IDs must reject without entering the type registry" );
        check( !wrong.prepare( catalog, entry, target, "texture", render::IMaterial::typeInfo() ),
               "Reference preparation must reject incompatible asset types" );
        AssetReferenceChange missing;
        check( !missing.prepare( catalog, entry, target, "missing", render::ITexture::typeInfo() ),
               "Reference edits cannot create missing properties" );
        AssetReferenceChange change;
        check( change.prepare( catalog, entry, target, "texture", render::ITexture::typeInfo() ) && change.apply( catalog ),
               change.error().empty() ? "Compatible reference assignment must apply" : change.error().c_str() );
        check( target->data->getProperty( "texture" ) == id && target->data->getProperty( "unrelated" ) == "keep" &&
               target->data->getPropertyObject( "texture" ).getAttribute( "label" ) == "Surface texture",
               "Reference edits must preserve property metadata and unrelated values" );
        check( change.undo( catalog ) && target->data->getProperty( "texture" ) == "old-value",
               "Reference undo must restore the previous exact value" );

        auto nestedTarget = make_ptr<ReferenceContractTarget>();
        auto firstLayer = make_ptr<Properties>(); firstLayer->setName( "layer" );
        firstLayer->setProperty( "texture", "first-layer" );
        auto secondLayer = make_ptr<Properties>(); secondLayer->setName( "layer" );
        secondLayer->setProperty( "unrelated", "second-layer" );
        auto nestedDetails = make_ptr<Properties>(); nestedDetails->setName( "details" );
        nestedDetails->setProperty( "texture", "nested-old" );
        nestedDetails->getPropertyObject( "texture" ).setAttribute( "resourceType", "Texture" );
        nestedDetails->getPropertyObject( "texture" ).setAttribute( "label", "Nested texture" );
        secondLayer->addChild( nestedDetails );
        nestedTarget->data->addChild( firstLayer ); nestedTarget->data->addChild( secondLayer );
        const PropertyGroupPath nestedPath = { { "layer", 1 }, { "details", 0 } };
        nestedTarget->nestedPath = nestedPath;
        AssetReferenceChange nestedChange;
        check( nestedChange.prepare( catalog, entry, nestedTarget, "texture", render::ITexture::typeInfo(), nestedPath ) &&
               nestedChange.apply( catalog ), "Nested reference assignment must resolve the chosen repeated group" );
        auto installedDetails = resolvePropertyGroup( nestedTarget->data, nestedPath );
        check( installedDetails && installedDetails->getProperty( "texture" ) == id &&
               nestedTarget->data->getProperty( "texture" ) == "old-value" &&
               nestedTarget->data->getChild( "layer" )->getProperty( "texture" ) == "first-layer" &&
               installedDetails->getName() == "details" &&
               installedDetails->getPropertyObject( "texture" ).getAttribute( "label" ) == "Nested texture" &&
               nestedDetails->getProperty( "texture" ) == "nested-old",
               "Nested references must preserve root and sibling leaves, group names, metadata and earlier snapshots" );
        installedDetails->setProperty( "unrelated", "latest" );
        check( nestedChange.undo( catalog ) &&
               resolvePropertyGroup( nestedTarget->data, nestedPath )->getProperty( "texture" ) == "nested-old" &&
               resolvePropertyGroup( nestedTarget->data, nestedPath )->getProperty( "unrelated" ) == "latest",
               "Nested undo must change only its leaf and preserve independently edited siblings" );
        check( nestedChange.apply( catalog ), "Nested reference redo must apply" );
        resolvePropertyGroup( nestedTarget->data, nestedPath )->setProperty( "texture", "external" );
        check( !nestedChange.undo( catalog ) &&
               resolvePropertyGroup( nestedTarget->data, nestedPath )->getProperty( "texture" ) == "external",
               "Nested undo must reject an independently changed leaf" );
        resolvePropertyGroup( nestedTarget->data, nestedPath )->setProperty( "texture", id );
        check( nestedChange.undo( catalog ), "Nested undo must resume when the expected leaf is restored" );
        auto lockedDetails = resolvePropertyGroup( nestedTarget->data, nestedPath );
        lockedDetails->getPropertyObject( "texture" ).setReadOnly( true );
        AssetReferenceChange nestedLocked;
        check( !nestedLocked.prepare( catalog, entry, nestedTarget, "texture", render::ITexture::typeInfo(), nestedPath ),
               "Nested read-only references must reject without falling back to a root property" );
        lockedDetails->getPropertyObject( "texture" ).setReadOnly( false );
        AssetReferenceChange nestedMissing;
        check( !nestedMissing.prepare( catalog, entry, nestedTarget, "texture", render::ITexture::typeInfo(),
                                     { { "missing-layer", 0 } } ),
               "A missing nested reference group must reject without editing the root" );
        AssetReferenceChange nestedRejected;
        nestedTarget->rejectNested = true;
        check( nestedRejected.prepare( catalog, entry, nestedTarget, "texture", render::ITexture::typeInfo(), nestedPath ) &&
               !nestedRejected.apply( catalog ) &&
               resolvePropertyGroup( nestedTarget->data, nestedPath )->getProperty( "texture" ) == "nested-old",
               "A rejected nested setter must leave the original group unchanged" );
        nestedTarget->rejectNested = false; nestedTarget->throwNested = true;
        AssetReferenceChange nestedThrown;
        check( nestedThrown.prepare( catalog, entry, nestedTarget, "texture", render::ITexture::typeInfo(), nestedPath ) &&
               !nestedThrown.apply( catalog ) &&
               resolvePropertyGroup( nestedTarget->data, nestedPath )->getProperty( "texture" ) == "nested-old" &&
               nestedTarget->data->getProperty( "texture" ) == "old-value",
               "A nested setter throwing after mutation must roll back the independent original hierarchy" );
        nestedTarget->throwNested = false;
#ifdef WP_EDITOR_ASSET_COMMAND_CONTRACTS
        auto nestedSecond = make_ptr<ReferenceContractTarget>();
        nestedSecond->data = clonePropertyTree( nestedTarget->data ); nestedSecond->nestedPath = nestedPath;
        auto nestedCommand = make_ptr<editor::ModifyPropertyCmd>();
        check( nestedCommand->configure( catalog, entry, { nestedTarget, nestedSecond }, "texture",
                                         render::ITexture::typeInfo(), nestedPath ),
               "Editor reference commands must capture the nested group path" );
        nestedSecond->rejectNested = true; nestedCommand->execute();
        check( !nestedCommand->wasApplied() &&
               resolvePropertyGroup( nestedTarget->data, nestedPath )->getProperty( "texture" ) == "nested-old" &&
               resolvePropertyGroup( nestedSecond->data, nestedPath )->getProperty( "texture" ) == "nested-old",
               "A rejected nested target must roll back every earlier target in the Editor command" );
        nestedSecond->rejectNested = false; nestedCommand->execute();
        check( nestedCommand->wasApplied() &&
               resolvePropertyGroup( nestedTarget->data, nestedPath )->getProperty( "texture" ) == id &&
               resolvePropertyGroup( nestedSecond->data, nestedPath )->getProperty( "texture" ) == id,
               "Editor reference commands must assign the nested leaf in every selected target" );
        nestedCommand->undo();
        check( !nestedCommand->wasApplied() &&
               resolvePropertyGroup( nestedTarget->data, nestedPath )->getProperty( "texture" ) == "nested-old" &&
               resolvePropertyGroup( nestedSecond->data, nestedPath )->getProperty( "texture" ) == "nested-old",
               "Editor nested reference undo must restore every target" );
        nestedCommand->redo(); check( nestedCommand->wasApplied(), "Editor nested reference redo must apply" );
#endif
        check( bound->queryBound( "INSERT INTO resources(uuid,path,path_key,type,kind) VALUES('unrelated','other.png','other.png','Texture','file')", {} ) != nullptr,
               "Unrelated revision fixture must seed" );
        check( change.apply( catalog ), "Redo must tolerate unrelated catalog revisions in the same session" );
        target->data->setProperty( "texture", "external" );
        check( !change.undo( catalog ) && target->data->getProperty( "texture" ) == "external",
               "Undo must not overwrite independent edits" );
        target->data->setProperty( "texture", id );
        check( change.undo( catalog ), "Undo may resume once its expected property value is restored" );
        AssetReferenceChange stale;
        check( !stale.prepare( catalog, entry, target, "texture", render::ITexture::typeInfo() ),
               "Initial assignment must reject a stale catalog snapshot" );
        check( catalog->tryGetEntry( id, entry ), "Refresh reference identity" );
        AssetReferenceChange dead;
        auto ephemeral = make_ptr<ReferenceContractTarget>();
        check( dead.prepare( catalog, entry, ephemeral, "texture", render::ITexture::typeInfo() ), "Prepare weak target" );
        ephemeral = nullptr;
        check( !dead.apply( catalog ), "Expired targets must reject assignment" );
        // Factories can reuse the same address. A previous observer must not
        // attach to the new object's lifetime even when its counts are positive.
        alignas( ReferenceContractTarget ) unsigned char storage[sizeof( ReferenceContractTarget )];
        auto pooled = new( storage ) ReferenceContractTarget;
        pooled->setPoolElement( true );
        SmartPtr<ReferenceContractTarget> pooledOwner( pooled );
        pooled->removeReference(); // transfer the constructor's initial owner
        ObjectObserver<ReferenceContractTarget> oldObserver( pooledOwner );
        pooledOwner = nullptr;
        check( oldObserver.expired() && !oldObserver.lock(), "Last owner release must expire pooled observers" );
        pooled->~ReferenceContractTarget();
        pooled = new( storage ) ReferenceContractTarget;
        pooled->setPoolElement( true );
        pooledOwner = pooled;
        pooled->removeReference();
        ObjectObserver<ReferenceContractTarget> newObserver( pooledOwner );
        check( !oldObserver.lock() && newObserver.lock() == pooledOwner,
               "Recycled storage must have a fresh lifetime token" );
        pooledOwner = nullptr;
        pooled->~ReferenceContractTarget();
#ifdef WP_EDITOR_ASSET_COMMAND_CONTRACTS
        auto second = make_ptr<ReferenceContractTarget>();
        auto command = make_ptr<editor::ModifyPropertyCmd>();
        check( command->configure( catalog, entry, { target, second, target }, "texture", render::ITexture::typeInfo() ),
               "Real Editor command must prepare a deduplicated target group" );
        second->reject = true;
        command->execute();
        check( !command->wasApplied() && !command->getError().empty() &&
               target->data->getProperty( "texture" ) == "old-value" && second->data->getProperty( "texture" ) == "old-value",
               "A rejected setter must roll back prior edits in the Editor command group" );
        second->reject = false;
        command->execute();
        check( command->wasApplied() && target->data->getProperty( "texture" ) == id && second->data->getProperty( "texture" ) == id,
               "Successful Editor assignment must apply the whole target group" );
        second->data->setProperty( "texture", "external" );
        command->undo();
        check( command->wasApplied() && target->data->getProperty( "texture" ) == id,
               "Undo conflict in one target must prevent every target's mutation" );
        second->data->setProperty( "texture", id );
        command->undo();
        check( !command->wasApplied() && target->data->getProperty( "texture" ) == "old-value", "Editor group undo must restore values" );
        command->redo();
        check( command->wasApplied(), "Editor group redo must apply" );
        app->setProjectPath( ( folder / "other-project" ).u8string().c_str() );
        command->undo();
        check( command->wasApplied() && target->data->getProperty( "texture" ) == id,
               "Project switching before service rebinding must invalidate reference commands" );
        app->setProjectPath( folder.u8string().c_str() );
        catalog->close();
        catalog->loadFromFile( path.u8string().c_str() );
        command->undo();
        check( command->wasApplied(), "Closing and reopening the same catalog must expire old commands" );
        const auto mediaRoot = folder / "reference-media";
        std::filesystem::create_directories( mediaRoot );
        { std::ofstream file( mediaRoot / "surface.png", std::ios::binary ); file << "texture fixture"; }
        String mediaError;
        auto media = openMediaAssetCatalog( mediaRoot.u8string().c_str(), mediaError );
        check( media && resources->setMediaCatalog( media ), "Mount media for real Editor reference commands" );
        auto mediaPage = media->queryEntries( {} );
        check( mediaPage.succeeded && mediaPage.entries.size() == 1, "Capture media reference identity" );
        auto mediaEntry = mediaPage.entries.front();
        auto mediaTarget = make_ptr<ReferenceContractTarget>();
        auto mediaCommand = make_ptr<editor::ModifyPropertyCmd>();
        check( mediaCommand->configure( media, mediaEntry, {mediaTarget}, "texture", render::ITexture::typeInfo() ),
               "Editor reference commands must accept the read-only mounted media catalog" );
        mediaCommand->execute();
        check( mediaCommand->wasApplied() && mediaTarget->data->getProperty( "texture" ) == mediaEntry.uuid,
               "A media assignment must write the stable media UUID" );
        mediaCommand->undo();
        check( !mediaCommand->wasApplied() && mediaTarget->data->getProperty( "texture" ) == "old-value",
               "Media references must support undo" );
        mediaCommand->redo();
        check( mediaCommand->wasApplied(), "Media references must support redo" );
        resources->setMediaCatalog( nullptr );
        mediaCommand->undo();
        check( mediaCommand->wasApplied() && mediaTarget->data->getProperty( "texture" ) == mediaEntry.uuid,
               "Changing the media mount must invalidate its existing reference commands" );
        media->unload( nullptr );
#endif
        const auto databasePath = catalog->getDatabasePath();
        const auto generation = catalog->getCatalogGeneration();
        check( !resources->getObject( StringUtil::parseUUID( StringUtil::getUUID() ) ) &&
               catalog->getDatabasePath() == databasePath && catalog->getCatalogGeneration() == generation,
               "Missing UUID lookup must not reopen or rebind the project catalog" );
        catalog->unload( nullptr );
        std::cout << "PASS: typed reference edits, metadata preservation, stale sessions, weak targets, grouped Editor rollback and undo/redo\n";
    }
}
