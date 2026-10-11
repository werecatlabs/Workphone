#pragma once
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
        struct Restore
        {
            SmartPtr<core::IApplicationManager> app;
            String media;
            ~Restore() { app->setMediaPath( media ); }
        } restore{app,oldMedia};
        const auto root = folder / "domain-media";
        std::filesystem::create_directories( root );
        app->setMediaPath( root.u8string().c_str() );
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
        check( sql( "INSERT INTO configured_actors(id,uuid,name,parent_id) VALUES(10,?,'Root',0)", {uuid} ) != nullptr &&
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
        std::cout << "PASS: read-only domain assets, explicit UUID mapping, numeric descendant traversal, complete attributes, cycle/ambiguity rejection and read rollback\n";
    }
}
