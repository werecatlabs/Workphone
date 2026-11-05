#pragma once
#include <Workphone/Interface/Scene/IGamePrefab.hpp>
#include <Workphone/Scene/GamePrefabManager.hpp>
#include <fstream>
#include <iterator>

namespace workphone
{
    class DomainContractResourceDatabase : public ResourceDatabase
    {
    public:
        using ResourceDatabase::loadAssetNode;
        using ResourceDatabase::loadAssetProperties;
        WP_CLASS_REGISTER_DECL;
    };
    WP_CLASS_REGISTER_DERIVED( workphone, DomainContractResourceDatabase, ResourceDatabase );
}
namespace
{
    using namespace workphone;
    void domainAssetReadContracts( const std::filesystem::path &folder )
    {
        auto check = []( bool ok, const char *message ) {
            if( !ok ) throw std::runtime_error( message );
        };
        auto app = core::IApplicationManager::instancePtr();
        const auto oldMedia = app->getMediaPath();
        const auto oldWorkingDirectory = std::filesystem::current_path();
        struct Restore
        {
            SmartPtr<core::IApplicationManager> app;
            String media;
            std::filesystem::path workingDirectory;
            ~Restore() { std::filesystem::current_path( workingDirectory ); app->setMediaPath( media ); }
        } restore{app,oldMedia,oldWorkingDirectory};
        const auto root = folder / "domain-media";
        std::filesystem::create_directories( root );
        std::filesystem::current_path( folder );
        app->setMediaPath( "domain-media" );
        auto resources = make_ptr<DomainContractResourceDatabase>();
        const auto uuid = StringUtil::getUUID();
        check( !resources->loadAssetProperties( StringUtil::parseUUID( uuid ) ) &&
               !std::filesystem::exists( root / "AssetDatabase.db" ),
               "Reading a missing domain asset must never create a domain database" );
        auto domain = make_ptr<DatabaseManager>();
        domain->loadFromFile( ( root / "AssetDatabase.db" ).u8string().c_str() );
        auto bound = dynamic_cast<IParameterizedDatabase *>( domain->getDatabase().get() );
        auto sql = [&]( const String &statement, const Array<String> &values = Array<String>() ) {
            return bound->queryBound( statement, values );
        };
        check( sql( "CREATE TABLE configured_actors(id INTEGER PRIMARY KEY,uuid TEXT,name TEXT,parent_id INTEGER);"
                    "CREATE TABLE attribs(actor_id INTEGER,configured_actor_id INTEGER,title TEXT,value TEXT)" ) != nullptr,
               "Create domain actor/attribute fixture" );
        check( sql( "INSERT INTO configured_actors(id,uuid,name,parent_id) VALUES(10,?,'Root',0)",
                    { StringUtil::make_upper( uuid ) } ) != nullptr &&
               sql( "INSERT INTO configured_actors(id,name,parent_id) VALUES(20,'Child',10);"
                    "INSERT INTO configured_actors(id,name,parent_id) VALUES(21,'Grandchild',20);"
                    "INSERT INTO attribs VALUES(10,10,'first','one');"
                    "INSERT INTO attribs VALUES(10,10,'second','two');"
                    "INSERT INTO attribs VALUES(10,20,'child-only','three')" ) != nullptr,
               "Seed numeric descendants with an explicit UUID root identity" );
        auto properties = resources->loadAssetProperties( StringUtil::parseUUID( uuid ) );
        check( properties && properties->getProperty( "uuid" ) == uuid &&
               properties->getProperty( "first" ) == "one" && properties->getProperty( "second" ) == "two" &&
               !properties->hasProperty( "child-only" ),
               "Domain UUID mapping must load every root attribute without mixing descendant attributes" );
        auto children = properties->getChildren();
        check( children.size() == 1 && children[0]->getProperty( "id" ) == "10" &&
               children[0]->getChildren().size() == 1 &&
               children[0]->getChildren()[0]->getProperty( "id" ) == "20" &&
               children[0]->getChildren()[0]->getProperty( "child-only" ) == "three" &&
               children[0]->getChildren()[0]->getChildren().size() == 1 &&
               children[0]->getChildren()[0]->getChildren()[0]->getProperty( "id" ) == "21",
               "Domain tree traversal must retain numeric IDs instead of parsing them as UUIDs" );
        auto tables = sql( "SELECT count(*) AS n FROM sqlite_master WHERE name IN ('resources','wp_asset_catalog_schema','wp_media_catalog')" );
        check( tables && tables->getFieldValueAsInt( "n" ) == 0,
               "Domain reads must never initialize or migrate a project/media catalog in the authored domain database" );
        check( !resources->loadAssetProperties( StringUtil::parseUUID( StringUtil::getUUID() ) ),
               "Missing domain UUIDs must return failure instead of an empty model" );
        check( sql( "INSERT INTO attribs VALUES(10,10,'first','duplicate')" ) != nullptr, "Seed ambiguous attributes" );
        check( !resources->loadAssetProperties( StringUtil::parseUUID( uuid ) ),
               "Duplicate attribute names must fail without publishing a partial model" );
        check( sql( "DELETE FROM attribs WHERE value='duplicate'; UPDATE configured_actors SET parent_id=21 WHERE id=10" ) != nullptr,
               "Seed reachable domain cycle" );
        auto parent = make_ptr<Properties>();
        check( !resources->loadAssetNode( domain, parent, StringUtil::parseUUID( uuid ) ) && parent->getChildren().empty(),
               "A cyclic domain tree must fail without appending partial children" );
        check( sql( "UPDATE configured_actors SET parent_id=0 WHERE id=10" ) != nullptr, "Repair fixture cycle" );
        auto recovered = resources->loadAssetProperties( StringUtil::parseUUID( uuid ) );
        check( recovered != nullptr, "Failed domain reads must release their transaction before retry" );
        check( sql( "INSERT INTO configured_actors(id,uuid,name,parent_id) VALUES(30,?,'Other',0)", {uuid} ) != nullptr,
               "Seed ambiguous domain UUID" );
        check( !resources->loadAssetProperties( StringUtil::parseUUID( uuid ) ), "Duplicate domain UUIDs must be diagnosed" );
        domain->unload( nullptr );

        const auto numericRoot = folder / "numeric-domain";
        std::filesystem::create_directory( numericRoot );
        app->setMediaPath( "numeric-domain" );
        const auto numericPath = numericRoot / "AssetDatabase.db";
        domain = make_ptr<DatabaseManager>();
        domain->loadFromFile( numericPath.u8string().c_str() );
        bound = dynamic_cast<IParameterizedDatabase *>( domain->getDatabase().get() );
        check( bound != nullptr &&
               sql( "CREATE TABLE configured_actors(id INTEGER PRIMARY KEY,name TEXT,parent_id INTEGER);"
                    "CREATE TABLE attribs(actor_id INTEGER,configured_actor_id TEXT,title TEXT,value TEXT);"
                    "INSERT INTO configured_actors VALUES(4294967301,'Authored Model',1);"
                    "INSERT INTO configured_actors VALUES(4294967302,'Mesh Child',4294967301);"
                    "INSERT INTO attribs VALUES(4294967301,'4294967302','mesh','mesh-reference')" ) != nullptr,
               "Create standalone numeric domain layout without a UUID column" );
        const auto authoredUuid = StringUtil::getUUID();
        const auto authoredId = StringUtil::parseUUID( authoredUuid );
        auto readAuthored = [&] { return resources->loadAssetProperties( authoredId ); };
        check( !readAuthored(), "A numeric row without an authored UUID must remain unresolved" );
        check( sql( "INSERT INTO attribs VALUES(4294967302,'4294967301','uuid',?)",
                    { StringUtil::make_upper( authoredUuid ) } ) != nullptr,
               "Author a UUID on a configured actor without changing its numeric row ID" );
        auto authored = readAuthored();
        check( authored && authored->getProperty( "uuid" ) == authoredUuid &&
               authored->getChildren().size() == 1 &&
               authored->getChildren()[0]->getProperty( "id" ) == "4294967301" &&
               authored->getChildren()[0]->getProperty( "uuid" ) == authoredUuid &&
               authored->getChildren()[0]->getChildren().size() == 1 &&
               authored->getChildren()[0]->getChildren()[0]->getProperty( "mesh" ) == "mesh-reference",
               "The authored UUID must resolve the exact 64-bit configured owner and its complete subtree" );
        {
            auto prefabs = make_ptr<scene::GamePrefabManager>();
            prefabs->load( nullptr );
            struct PrefabScope
            {
                SmartPtr<core::IApplicationManager> app;
                SmartPtr<scene::IGamePrefabManager> previous, installed;
                ~PrefabScope() { app->setPrefabManager( previous ); installed->unload( nullptr ); }
            } prefabScope{ app, app->getPrefabManager(), prefabs };
            app->setPrefabManager( prefabs );
            check( app->getGraphicsSystemPtr() == nullptr, "Domain fixture must run without graphics" );
            auto model = dynamic_pointer_cast<scene::IGamePrefab>( resources->loadResource( authoredId ) );
            auto modelData = model ? dynamic_pointer_cast<Properties>( model->getData() ) : nullptr;
            check( model && model->getHandle()->getUUIDAsString() == authoredUuid && modelData &&
                   modelData->getChildren().size() == 1 &&
                   modelData->getChildren()[0]->getProperty( "id" ) == "4294967301",
                   "Headless resource load must publish the requested model UUID and authored data" );
        }
        check( sql( "INSERT INTO attribs VALUES(4294967301,'4294967302','uuid',?)", { authoredUuid } ) != nullptr,
               "Seed a second owner for the same UUID" );
        check( !resources->loadAssetNode( domain, parent, authoredId ) && parent->getChildren().empty(),
               "Case-insensitive duplicate UUID bindings must not append a partial tree" );
        check( sql( "DELETE FROM attribs WHERE configured_actor_id='4294967302' AND title='uuid';"
                    "ALTER TABLE configured_actors ADD COLUMN uuid TEXT" ) != nullptr &&
               sql( "UPDATE configured_actors SET uuid=? WHERE id=4294967302", { authoredUuid } ) != nullptr &&
               !readAuthored(),
               "Column and attribute bindings for one UUID must agree on the actor row" );
        const auto conflictingUuid = StringUtil::getUUID();
        check( sql( "UPDATE configured_actors SET uuid=NULL" ) != nullptr &&
               sql( "UPDATE configured_actors SET uuid=? WHERE id=4294967301", { conflictingUuid } ) != nullptr &&
               !readAuthored() && !resources->loadAssetProperties( StringUtil::parseUUID( conflictingUuid ) ),
               "A configured actor cannot expose different UUIDs through its column and attribute" );
        check( sql( "UPDATE configured_actors SET uuid=NULL;"
                    "UPDATE attribs SET configured_actor_id='4294967399' WHERE title='uuid'" ) != nullptr &&
               !readAuthored(), "A dangling authored UUID binding must fail" );
        // UTF-8 bytes for the numeric owner followed by NUL and 'x'.
        check( sql( "UPDATE attribs SET configured_actor_id=CAST(X'343239343936373330310078' AS TEXT) "
                    "WHERE title='uuid'" ) != nullptr &&
               !readAuthored(), "An embedded NUL cannot turn an invalid owner into a valid numeric ID" );
        check( sql( "UPDATE attribs SET configured_actor_id='4294967301' WHERE title='uuid'" ) != nullptr,
               "Restore the authored numeric binding after rejected reads" );
        domain->unload( nullptr );
        auto bytes = [&] {
            std::ifstream input( numericPath, std::ios::binary );
            return std::string( std::istreambuf_iterator<char>( input ), {} );
        };
        const auto beforeRead = bytes();
        check( !beforeRead.empty() && readAuthored() && bytes() == beforeRead,
               "A reopened numeric-domain load must preserve the authored SQLite bytes" );

        const auto legacyRoot = folder / "legacy-domain";
        std::filesystem::create_directory( legacyRoot );
        app->setMediaPath( "legacy-domain" );
        domain = make_ptr<DatabaseManager>();
        domain->loadFromFile( ( legacyRoot / "AssetDatabase.db" ).u8string().c_str() );
        bound = dynamic_cast<IParameterizedDatabase *>( domain->getDatabase().get() );
        check( bound != nullptr &&
               sql( "CREATE TABLE configured_actors(id TEXT PRIMARY KEY,name TEXT,parent_id TEXT);"
                    "CREATE TABLE attribs(actor_id TEXT,title TEXT,value TEXT)" ) != nullptr &&
               sql( "INSERT INTO configured_actors VALUES(?,'Legacy Model','0')", { StringUtil::make_upper( uuid ) } ) != nullptr &&
               sql( "INSERT INTO attribs VALUES(?,'uuid',?)", { StringUtil::make_upper( uuid ), uuid } ) != nullptr,
               "Seed a legacy text-ID root and its identical UUID metadata" );
        auto legacy = resources->loadAssetProperties( StringUtil::parseUUID( uuid ) );
        check( legacy && legacy->getChildren().size() == 1 &&
               legacy->getChildren()[0]->getProperty( "id" ) == StringUtil::make_upper( uuid ) &&
               legacy->getChildren()[0]->getProperty( "uuid" ) == uuid,
               "Legacy text IDs retain their row spelling while exposing the canonical UUID" );
        check( sql( "ALTER TABLE configured_actors ADD COLUMN uuid TEXT" ) != nullptr &&
               sql( "UPDATE configured_actors SET uuid=?", { uuid } ) != nullptr &&
               resources->loadAssetProperties( StringUtil::parseUUID( uuid ) ),
               "Consistent UUID column and attribute metadata preserve existing text row IDs" );
        domain->unload( nullptr );
        std::cout << "PASS: read-only domain assets, authored numeric root UUIDs, legacy text IDs, headless model load, bounded tree traversal and identity rejection\n";
    }
}
