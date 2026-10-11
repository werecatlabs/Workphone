#include <Workphone/Database/AssetDocumentSave.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Graphics/MaterialManager.hpp>
#include <Workphone/Graphics/MaterialTechnique.hpp>
#include <Workphone/Graphics/MaterialPass.hpp>
#include <Workphone/Graphics/MaterialTexture.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <Workphone/Scene/GamePrefab.hpp>
#include <Workphone/Scene/GamePrefabManager.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
namespace
{
    void assetDocumentSaveContracts( SmartPtr<AssetDatabaseManager> catalog,
                                     const std::filesystem::path &root )
    {
        namespace fs = std::filesystem;
        auto write = []( const fs::path &path, const Array<u8> &bytes ) {
            std::ofstream output( path, std::ios::binary | std::ios::trunc );
            output.write( reinterpret_cast<const char *>( bytes.data() ), bytes.size() );
            require( static_cast<bool>( output ), "Write document failure fixture" );
        };
        auto read = []( const fs::path &path ) {
            std::ifstream input( path, std::ios::binary );
            return std::vector<u8>( std::istreambuf_iterator<char>( input ), {} );
        };
        auto encode = []( const String &text ) {
            Array<u8> bytes;
            bytes.resize( text.size() );
            std::copy( text.begin(), text.end(), bytes.begin() );
            return bytes;
        };
        auto data = make_ptr<Properties>();
        data->setName( "authored" );
        data->setProperty( "name", "literal name" );
        data->setProperty( "value", "literal value" );
        auto target = asset( "atomic-target.bin" );
        catalog->addResourceEntry( target );
        write( root / "atomic-target.bin", { 1, 0, 255 } );
        const auto id = target->getHandle()->getUUIDAsString();
        data->setProperty( "asset", id, Properties::resourceStr );
        auto &field = data->getPropertyObject( "asset" );
        field.setAttribute( "resourceType", "workphone::CatalogTestResource" );
        field.setAttribute( "referenceRequired", "false" );
        field.setAttribute( "label", "Texture & <optional>" );
        field.setReadOnly( true );
        for( const auto format : { DataFormat::JSON, DataFormat::XML } )
        {
            const auto text = DataUtil::toString( data.get(), true, format );
            auto parsed = make_ptr<Properties>();
            DataUtil::parse( text, parsed.get(), format );
            const auto restored = parsed->getPropertyObject( "asset" );
            require(
                restored.getValue() == id && restored.getTypeName() == Properties::resourceStr &&
                    restored.getAttribute( "resourceType" ) == "workphone::CatalogTestResource" &&
                    restored.getAttribute( "referenceRequired" ) == "false" && restored.isReadOnly() &&
                    restored.getAttribute( "label" ) == "Texture & <optional>" &&
                    parsed->getProperty( "name" ) == "literal name" &&
                    parsed->getProperty( "value" ) == "literal value",
                "Authored JSON/XML must round-trip resource policy and ordinary name/value fields" );
        }
        String error, installed;
        const auto original = encode( DataUtil::toString( data.get(), true ) );
        require( saveAssetDocument( "documents/atomic.material", data, original, installed, error ),
                 "Publish staged project document" );
        const auto file = root / "documents/atomic.material";
        require( read( file ) == std::vector<u8>( original.begin(), original.end() ),
                 "Install byte-exact document" );
        auto edges = queryAssetReferences( catalog, "documents/atomic.material", false );
        require( edges.succeeded && edges.edges.size() == 1 && !edges.edges[0].required &&
                     edges.edges[0].sourceState == "ready",
                 "Shared authored save must complete optional typed references" );
        data->getPropertyObject( "asset" ).setAttribute( "referenceRequired", "true" );
        const auto changed = encode( DataUtil::toString( data.get(), true ) );
        require( sql( catalog->getDatabase(),
                      "CREATE TRIGGER fail_document_begin BEFORE INSERT ON wp_asset_reference_documents "
                      "BEGIN SELECT RAISE(ABORT,'begin fault'); END" ) != nullptr,
                 "Inject pre-publication graph failure" );
        require(
            !saveAssetDocument( "documents/atomic.material", data, changed, installed, error ) &&
                installed.empty() &&
                read( file ) == std::vector<u8>( original.begin(), original.end() ) &&
                !queryAssetReferences( catalog, "documents/atomic.material", false ).edges[0].required,
            "Failed graph preparation must retain original file and previous graph" );
        require( sql( catalog->getDatabase(), "DROP TRIGGER fail_document_begin" ) != nullptr,
                 "Remove begin fault" );
#if defined WP_PLATFORM_WIN32
        auto held = CreateFileW( file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL, nullptr );
        require( held != INVALID_HANDLE_VALUE, "Hold destination without delete sharing" );
        const auto accepted =
            saveAssetDocument( "documents/atomic.material", data, changed, installed, error );
        CloseHandle( held );
        require( !accepted && read( file ) == std::vector<u8>( original.begin(), original.end() ),
                 "Locked destination must preserve last complete bytes" );
        require( !catalog
                      ->performFileOperation( AssetDatabaseManager::FileOperation::Delete,
                                              "atomic-target.bin" )
                      .succeeded,
                 "Failed replacement must leave a pending fence" );
        require( reindexAssetDocument( catalog, "documents/atomic.material", error ),
                 "Repair pending graph from unchanged installed bytes" );
        require( !queryAssetReferences( catalog, "documents/atomic.material", false ).edges[0].required,
                 "Reindex must read installed optional policy rather than failed-save intentions" );
#endif
        require( sql( catalog->getDatabase(),
                      "CREATE TRIGGER fail_document_ready BEFORE UPDATE ON wp_asset_reference_documents "
                      "WHEN NEW.state='ready' BEGIN SELECT RAISE(ABORT,'ready fault'); END" ) != nullptr,
                 "Inject post-publication graph failure" );
        require( !saveAssetDocument( "documents/atomic.material", data, changed, installed, error ) &&
                     read( file ) == std::vector<u8>( changed.begin(), changed.end() ) &&
                     queryAssetReferences( catalog, "documents/atomic.material", false )
                             .edges[0]
                             .sourceState == "pending",
                 "Failure after replacement retains complete new document and pending fence" );
        require( !reindexAssetDocument( catalog, "documents/atomic.material", error ) &&
                     queryAssetReferences( catalog, "documents/atomic.material", false )
                             .edges[0]
                             .sourceState == "pending",
                 "Reindex transaction failure must retain the prior pending revision" );
        require( sql( catalog->getDatabase(), "DROP TRIGGER fail_document_ready" ) != nullptr,
                 "Remove ready fault" );
        const auto timestamp = fs::last_write_time( file );
        require(
            reindexAssetDocument( catalog, "documents/atomic.material", error ) &&
                fs::last_write_time( file ) == timestamp &&
                queryAssetReferences( catalog, "documents/atomic.material", false ).edges[0].required,
            "Successful reindex repairs required policy without rewriting source bytes" );
        for( const auto &invalid :
             { String( "{\"asset\":\"legacy-unknown\",\"children\":[]}" ),
               String( "{\"__wp_properties_v1\":[],\"__wp_properties_v1\":[],\"handle_name\":\"\","
                       "\"children\":[]}" ),
               String( "{\"__wp_properties_v1\":[],\"handle_name\":\"\",\"asset\":\"untyped\","
                       "\"children\":[]}" ),
               String( "{\"__wp_properties_v1\":[],\"handle_name\":\"\",\"children\":[]} trailing" ),
               String( 129, '[' ) + String( 129, ']' ) } )
        {
            write( file, encode( invalid ) );
            require( !reindexAssetDocument( catalog, "documents/atomic.material", error ) &&
                         queryAssetReferences( catalog, "documents/atomic.material", false )
                             .edges[0]
                             .required,
                     "Ambiguous/malformed reindex must leave previous graph intact" );
        }
        write( file, changed );
        require( reindexAssetDocument( catalog, "documents/atomic.material", error ),
                 "Restore valid fixture graph" );
        const auto xml = encode( DataUtil::toString( data.get(), true, DataFormat::XML ) );
        write( root / "existing.fbscenexml", xml );
        require( reindexAssetDocument( catalog, "existing.fbscenexml", error ) &&
                     queryAssetReferences( catalog, "existing.fbscenexml", false ).edges.size() == 1,
                 "Index existing versioned XML reference metadata" );
        for( const auto &invalid :
             { String( "<Root><Properties Name=\"\" PropertyMetadataVersion=\"2\" /></Root>" ),
               String( "<!DOCTYPE Root><Root><Properties Name=\"\" PropertyMetadataVersion=\"1\" "
                       "/></Root>" ),
               String( "<Root><Properties Name=\"&#0;\" PropertyMetadataVersion=\"1\" /></Root>" ),
               String( "<Root><Properties Name=\"\" PropertyMetadataVersion=\"1\"><Property Name=\"a\" "
                       "Type=\"string\" readOnly=\"false\">x</Property><Property Name=\"a\" "
                       "Type=\"string\" readOnly=\"false\">y</Property></Properties></Root>" ) } )
        {
            write( root / "existing.fbscenexml", encode( invalid ) );
            require( !reindexAssetDocument( catalog, "existing.fbscenexml", error ) &&
                         queryAssetReferences( catalog, "existing.fbscenexml", false ).edges.size() == 1,
                     "Future/duplicate/entity XML must preserve the prior graph" );
        }
        write( root / "existing.fbscenexml", xml );
        require( reindexAssetDocument( catalog, "existing.fbscenexml", error ), "Repair XML fixture" );
        auto readOnly = make_ptr<AssetDatabaseManager>();
        require( readOnly->setProjectRoot( root.u8string().c_str() ) &&
                     readOnly->openReadOnly( ( root / "catalog.db" ).u8string().c_str(), error ) &&
                     !reindexAssetDocument( readOnly, "documents/atomic.material", error ),
                 "Document reindex cannot mutate a read-only mount" );
        readOnly->unload( nullptr );
        const auto binary = PropertiesBinarySerializer::serialize( *data );
        write( root / "existing.fbscenebin", binary );
        require( reindexAssetDocument( catalog, "existing.fbscenebin", error ) &&
                     queryAssetReferences( catalog, "existing.fbscenebin", false ).edges.size() == 1,
                 "Index existing portable binary metadata" );
        require( !reindexAssetDocument(
                     catalog, ( root.parent_path() / "outside.fbscene" ).u8string().c_str(), error ) &&
                     !reindexAssetDocument( catalog, ".workphone/catalog.db", error ),
                 "Reject out-of-project/private reindex sources" );
        require( !saveAssetDocument( ".workphone/forbidden.fbscene", data, changed, installed, error ) &&
                     !fs::exists( root / ".workphone/forbidden.fbscene" ),
                 "Authored saves cannot write private catalog storage" );
        auto app = core::IApplicationManager::instance();
        auto resources = dynamic_pointer_cast<ResourceDatabase>( app->getResourceDatabase() );
        auto media = openMediaAssetCatalog( ( root / "media" ).u8string().c_str(), error,
                                            ( root / "media/catalog.db" ).u8string().c_str() );
        require( media && resources->setMediaCatalog( media ),
                 "Mount bundled-media fixture for authored saves" );
        const auto originalMedia = read( root / "media/icon.png" );
        require( !saveAssetDocument( ( root / "media/icon.png" ).u8string().c_str(), data, changed,
                                     installed, error ) &&
                     read( root / "media/icon.png" ) == originalMedia,
                 "Shared save must reject bundled media even inside project root" );
        AssetDatabaseManager::EntryQuery mediaQuery;
        auto mediaRows = media->queryEntries( mediaQuery );
        auto mediaData = make_ptr<Properties>();
        mediaData->setProperty( "icon", mediaRows.entries[0].uuid, Properties::resourceStr );
        mediaData->getPropertyObject( "icon" ).setAttribute( "resourceType", "Texture" );
        require(
            saveAssetDocumentText( "documents/media.material", mediaData,
                                   DataUtil::toString( mediaData.get(), true ), error ) &&
                reindexAssetDocument( catalog, "documents/media.material", error ) &&
                queryAssetReferences( catalog, mediaRows.entries[0].uuid, true, "media" ).edges.size() ==
                    2,
            "Shared save and active-project reindex must retain bundled-media domain ownership" );
        resources->setMediaCatalog( nullptr );
        media->unload( nullptr );
        require(
            !saveAssetDocument( "../outside-authored-save.fbscene", data, changed, installed, error ) &&
                !fs::exists( root.parent_path() / "outside-authored-save.fbscene" ),
            "Relative save paths cannot escape the captured project root" );
        const auto exportPath = root.parent_path() / "explicit-export.fbscene";
        require( saveAssetDocument( exportPath.u8string().c_str(), data, changed, installed, error ) &&
                     fs::exists( exportPath ),
                 "Explicit absolute exports remain supported" );
        // Exercise actual material graph serialization and all three save entry points.
        auto texture = make_ptr<render::Texture>();
        texture->getHandle()->setUUID( StringUtil::getUUID() );
        texture->setFilePath( "material-texture.bin" );
        catalog->addResourceEntry( texture );
        write( root / "material-texture.bin", { 2, 4, 8 } );
        auto unit = make_ptr<render::MaterialTexture>();
        unit->setTexture( texture );
        auto pass = make_ptr<render::MaterialPass>();
        pass->addTextureUnit( unit );
        auto technique = make_ptr<render::MaterialTechnique>();
        technique->addPass( pass );
        auto material = make_ptr<render::Material>();
        material->addTechnique( technique );
        material->saveToFile( "documents/direct.material" );
        material->setFilePath( "documents/saved.material" );
        material->save();
        auto materials = make_ptr<render::MaterialManager>();
        materials->saveToFile( "documents/manager.material", material );
        for( const auto *name :
             { "documents/direct.material", "documents/saved.material", "documents/manager.material" } )
        {
            auto refs = queryAssetReferences( catalog, name, false );
            require( refs.succeeded && refs.edges.size() == 1 &&
                         refs.edges[0].targetUuid == texture->getHandle()->getUUIDAsString() &&
                         refs.edges[0].expectedType == "Texture" && refs.edges[0].diagnostic.empty(),
                     "Production material saves must capture texture UUID/type" );
            require( reindexAssetDocument( catalog, name, error ),
                     "Reindex persisted material texture metadata" );
        }
        // The prefab-resource save path previously delegated to a no-op base method.
        auto prefabs = make_ptr<scene::GamePrefabManager>();
        prefabs->setDataFormat( DataFormat::JSON );
        auto prefab =
            dynamic_pointer_cast<scene::IGamePrefab>( prefabs->create( StringUtil::getUUID() ) );
        require( prefab != nullptr, "Create prefab through production manager" );
        prefab->setData( data );
        prefabs->saveToFile( "documents/resource.prefab", prefab );
        require(
            fs::exists( root / "documents/resource.prefab" ) &&
                queryAssetReferences( catalog, "documents/resource.prefab", false ).edges.size() == 1,
            "Production prefab-resource save must write stored Properties and graph" );
        prefab->setFilePath( "documents/saved.prefab" );
        prefab->save();
        require( reindexAssetDocument( catalog, "documents/saved.prefab", error ) &&
                     queryAssetReferences( catalog, "documents/saved.prefab", false ).edges.size() == 1,
                 "Prefab save must persist stored Properties through the shared save path" );
        auto actor = make_ptr<scene::GameActor>();
        prefabs->savePrefab( "documents/actor.prefab", actor );
        require( fs::exists( root / "documents/actor.prefab" ) &&
                     reindexAssetDocument( catalog, "documents/actor.prefab", error ),
                 "Production actor prefab saves must install reindexable documents" );
        for( const auto &entry : fs::recursive_directory_iterator( root ) )
            require( entry.path().filename().u8string().find( ".workphone-save-" ) == std::string::npos,
                     "Completed and failed document saves must clean owned staging files" );
        std::cout << "PASS: staged authored saves, graph failure fences, typed JSON/XML persistence, "
                     "safe reindex, material and prefab routes\n";
    }
}  // namespace
