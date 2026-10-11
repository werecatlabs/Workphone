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
            data = make_ptr<Properties>( *replacement );
        }
        SmartPtr<Properties> data;
        bool reject = false;
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
        check( AssetReferenceChange::referenceType( "Texture" ) == render::ITexture::typeInfo() &&
               AssetReferenceChange::referenceType( TypeManager::instance()->getName( render::ITexture::typeInfo() ) ) == render::ITexture::typeInfo() &&
               AssetReferenceChange::referenceType( "unknown-reference-family" ) == 0,
               "Reference family labels and registered names must resolve without inventing types" );
        AssetReferenceChange wrong;
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
