#ifndef WP_ASSET_PACKAGE_POLICY_CONTRACTS_HPP
#define WP_ASSET_PACKAGE_POLICY_CONTRACTS_HPP
#include <Workphone/Database/AssetPackagePolicy.hpp>
#include <Workphone/Database/MediaAssetCatalog.hpp>
#include <Workphone/Interface/Database/IParameterizedDatabase.hpp>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace asset_package_policy_contracts
{
    using namespace workphone;
    namespace fs = std::filesystem;
    inline void require( bool condition, const char *message )
    { if( !condition ) throw std::runtime_error(message); }
    inline SmartPtr<IDatabaseQuery> sql( SmartPtr<AssetDatabaseManager> catalog, const String &statement,
                                          const Array<String> &values = {} )
    {
        auto db = catalog->getDatabase();
        return dynamic_cast<IParameterizedDatabase *>(db.get())->queryBound(statement,values);
    }
    inline String scalar( SmartPtr<AssetDatabaseManager> catalog, const String &statement,
                           const Array<String> &values = {} )
    {
        auto row = sql(catalog,statement,values);
        require(row && !row->eof(),"Expected package policy scalar");
        return row->getFieldValue("value");
    }
    inline void write( const fs::path &path )
    {
        fs::create_directories(path.parent_path());
        std::ofstream output(path,std::ios::binary);
        output << "package policy fixture\n";
        require(output.good(),"Cannot write package policy fixture");
    }
    inline void run( const fs::path &folder )
    {
        const auto root = folder / "package-policy";
        fs::create_directories(root);
        const auto dbPath = root / "catalog.db";
        auto catalog = make_ptr<AssetDatabaseManager>();
        require(catalog->setProjectRoot(root.u8string().c_str()),"Bind package policy project root");
        catalog->loadFromFile(dbPath.u8string().c_str());
        const String uuid = StringUtil::getUUID();
        write(root / "asset.bin");
        require(sql(catalog,"INSERT INTO resources(uuid,path,path_key,type,kind) VALUES(?,'asset.bin','asset.bin','PolicyFixture','file')",{uuid}) != nullptr,
                "Seed package policy file");
        String error;
        AssetPackagePolicy policy;
        const auto before = scalar(catalog,"SELECT count(*) AS value FROM sqlite_master");
        require(getAssetPackagePolicy(catalog,uuid,policy,error) && policy.usage == AssetPackageUsage::Runtime &&
                !policy.explicitPolicy && scalar(catalog,"SELECT count(*) AS value FROM sqlite_master") == before,
                "Default project shipping inspection must not create schema");
        const auto initial = policy.entry;
        require(setAssetPackagePolicy(catalog,initial,AssetPackageUsage::EditorOnly,error),error.c_str());
        require(!catalog->isEntryCurrent(initial) &&
                !setAssetPackagePolicy(catalog,initial,AssetPackageUsage::Excluded,error),
                "Policy writes must invalidate old DTOs and reject stale setters");
        require(getAssetPackagePolicy(catalog,uuid,policy,error) && policy.explicitPolicy &&
                policy.usage == AssetPackageUsage::EditorOnly,"Persist explicit Editor-only policy");
        require(!setAssetPackagePolicy(catalog,policy.entry,static_cast<AssetPackageUsage>(99),error),
                "Invalid shipping enums must fail");
        require(setAssetPackagePolicy(catalog,policy.entry,AssetPackageUsage::Excluded,error),error.c_str());
        require(getAssetPackagePolicy(catalog,uuid,policy,error) && policy.usage == AssetPackageUsage::Excluded,
                "Persist excluded usage");
        const auto excluded = policy.entry;
        // A real independent backend changes policy and advances catalog revision.
        auto independent = core::IApplicationManager::instance()->getFactoryManager()->make_object<IDatabase>();
        independent->loadFromFile(dbPath.u8string().c_str());
        auto bound = dynamic_cast<IParameterizedDatabase *>(independent.get());
        require(bound && bound->queryBound("UPDATE wp_asset_package_usage SET usage='editor-only' WHERE uuid=?",{uuid}),
                "Independent policy update");
        require(!catalog->isEntryCurrent(excluded),"Independent policy writes must expire catalog DTOs");
        independent->close();
        require(getAssetPackagePolicy(catalog,uuid,policy,error) && policy.usage == AssetPackageUsage::EditorOnly,
                "Observe independent policy changes");

        auto reader = make_ptr<AssetDatabaseManager>();
        require(reader->setProjectRoot(root.u8string().c_str()) && reader->openReadOnly(dbPath.u8string().c_str(),error),
                "Open read-only package policy catalog");
        AssetPackagePolicy readPolicy;
        require(getAssetPackagePolicy(reader,uuid,readPolicy,error) && readPolicy.usage == policy.usage &&
                !setAssetPackagePolicy(reader,readPolicy.entry,AssetPackageUsage::Runtime,error),
                "Read-only policy inspection must work while editing fails");
        reader->unload(nullptr);
        const auto stale = policy.entry;
        catalog->unload(nullptr);
        catalog->loadFromFile(dbPath.u8string().c_str());
        require(!setAssetPackagePolicy(catalog,stale,AssetPackageUsage::Runtime,error) &&
                getAssetPackagePolicy(catalog,uuid,policy,error) && policy.usage == AssetPackageUsage::EditorOnly,
                "Reopen must preserve usage and reject prior sessions");

        require(sql(catalog,"CREATE TRIGGER injected_policy_fault BEFORE INSERT ON wp_asset_package_usage BEGIN SELECT RAISE(ABORT,'policy fault'); END") != nullptr,
                "Inject policy write fault");
        const auto revision = catalog->getCatalogGeneration();
        require(!setAssetPackagePolicy(catalog,policy.entry,AssetPackageUsage::Runtime,error) &&
                catalog->getCatalogGeneration() == revision &&
                getAssetPackagePolicy(catalog,uuid,policy,error) && policy.usage == AssetPackageUsage::EditorOnly,
                "Failed policy writes must retain usage and revision transactionally");
        require(sql(catalog,"DROP TRIGGER injected_policy_fault") != nullptr,"Clear policy write fault");

        const auto copied = catalog->performFileOperation(AssetDatabaseManager::FileOperation::Copy,"asset.bin","copy.bin");
        require(copied.succeeded,copied.error.c_str());
        const auto copyUUID = scalar(catalog,"SELECT uuid AS value FROM resources WHERE path='copy.bin'");
        require(copyUUID != uuid && getAssetPackagePolicy(catalog,copyUUID,readPolicy,error) &&
                readPolicy.usage == AssetPackageUsage::EditorOnly,"Copies must inherit source shipping policy");
        const auto moved = catalog->performFileOperation(AssetDatabaseManager::FileOperation::Move,"asset.bin","moved.bin");
        require(moved.succeeded && getAssetPackagePolicy(catalog,uuid,policy,error) && policy.usage == AssetPackageUsage::EditorOnly,
                "Move must retain UUID shipping policy");
        const auto removed = catalog->performFileOperation(AssetDatabaseManager::FileOperation::Delete,"moved.bin");
        require(removed.succeeded && !getAssetPackagePolicy(catalog,uuid,policy,error),"Quarantined files cannot be inspected as active package assets");
        require(catalog->undoFileOperation(removed.operationId).succeeded && getAssetPackagePolicy(catalog,uuid,policy,error) &&
                policy.usage == AssetPackageUsage::EditorOnly,"Restore must retain shipping policy with original UUID");
        require(catalog->undoFileOperation(copied.operationId).succeeded &&
                !getAssetPackagePolicy(catalog,copyUUID,readPolicy,error),"Copy undo must remove active copied identity");

        require(sql(catalog,"UPDATE wp_asset_package_schema SET version=2") != nullptr,"Seed unsupported policy schema");
        const auto operations = scalar(catalog,"SELECT count(*) AS value FROM wp_asset_operations");
        const auto changed = catalog->performFileOperation(AssetDatabaseManager::FileOperation::Copy,"moved.bin","blocked.bin");
        require(!changed.succeeded && !fs::exists(root / "blocked.bin") &&
                scalar(catalog,"SELECT count(*) AS value FROM wp_asset_operations") == operations,
                "Unsupported policy schema must block copy before filesystem/journal publication");
        AssetDatabaseManager::EntrySnapshot current;
        require(catalog->tryGetEntry(uuid,current) && !getAssetPackagePolicy(catalog,uuid,policy,error) &&
                !setAssetPackagePolicy(catalog,current,AssetPackageUsage::Runtime,error) &&
                scalar(catalog,"SELECT version AS value FROM wp_asset_package_schema") == "2",
                "Future policy schemas must fail without migration or writes");
        require(sql(catalog,"UPDATE wp_asset_package_schema SET version=1") != nullptr,"Restore supported policy schema");
        const auto trigger = scalar(catalog,"SELECT sql AS value FROM sqlite_master WHERE name='wp_asset_package_usage_UPDATE'");
        require(sql(catalog,"DROP TRIGGER wp_asset_package_usage_UPDATE") != nullptr,"Remove revision tracking");
        require(!getAssetPackagePolicy(catalog,uuid,policy,error),"Missing revision tracking must fail closed");
        require(sql(catalog,trigger) != nullptr,"Restore exact policy tracking");
        catalog->unload(nullptr);

        const auto mediaRoot = root / "media";
        write(mediaRoot / "Runtime" / "shared.lua");
        write(mediaRoot / "Editor" / "window.lua");
        auto media = openMediaAssetCatalog(mediaRoot.u8string().c_str(),error);
        require(media != nullptr,error.c_str());
        const auto mediaPage = media->queryEntries({});
        require(mediaPage.succeeded && mediaPage.entries.size() == 2,"Inspect media package fixture");
        for(const auto &entry : mediaPage.entries)
        {
            require(getAssetPackagePolicy(media,entry.uuid,policy,error) && policy.explicitPolicy,
                    "Bundled media must use its existing explicit policy");
            require(policy.usage == (entry.path.find("Runtime/")==0 ? AssetPackageUsage::Runtime : AssetPackageUsage::EditorOnly),
                    "Media runtime opt-in must remain explicit");
            require(!setAssetPackagePolicy(media,entry,AssetPackageUsage::Runtime,error),"Media shipping policy is read-only");
        }
        require(scalar(media,"SELECT count(*) AS value FROM sqlite_master WHERE name='wp_asset_package_schema'") == "0",
                "Media inspection must not create project shipping schema");
        media->unload(nullptr);
    }
}
inline void assetPackagePolicyContracts(const std::filesystem::path &folder)
{ asset_package_policy_contracts::run(folder); }
#endif
