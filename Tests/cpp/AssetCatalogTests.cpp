#include <Workphone/Workphone.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Interface/Database/IParameterizedDatabase.hpp>
#include <Workphone/Scene/Directors/ResourceDirector.hpp>
#include <Workphone/System/Resource.hpp>
#include <Workphone/Core/LogManagerDefault.hpp>
#include <WPSQLite/WPSQLite.hpp>
#include <filesystem>
#include <iostream>
#include <thread>
#include <atomic>
#include <stdexcept>

namespace workphone
{
    class CatalogTestResource : public Resource<IResource>
    {
    public:
        WP_CLASS_REGISTER_DECL;
    };
    WP_CLASS_REGISTER_DERIVED(workphone, CatalogTestResource, Resource<IResource>);
}
using namespace workphone;
namespace
{
    void require(bool value, const char *message)
    {
        if(!value) throw std::runtime_error(message);
    }
    SmartPtr<CatalogTestResource> asset(const String &path)
    {
        auto result = make_ptr<CatalogTestResource>();
        result->getHandle()->setUUID(StringUtil::getUUID());
        result->setFilePath(path);
        return result;
    }
    String entryId(SmartPtr<IBuildDirector> entry)
    {
        auto director = dynamic_pointer_cast<scene::ResourceDirector>(entry);
        require(director != nullptr,"Expected catalog ResourceDirector");
        return director->getResourceUUID();
    }
    void contracts(AssetDatabaseManager &catalog, const std::filesystem::path &folder)
    {
        auto db = catalog.getDatabase();
        auto bound = dynamic_cast<IParameterizedDatabase *>(db.get());
        require(bound != nullptr,"SQLite parameterized backend is mandatory; cannot skip");
        catalog.create();
        require(!catalog.getResourceEntryFromPath("missing.mesh"),"Missing lookup must return null");
        require(!catalog.getResourceEntry("missing-id"),"Missing UUID must return null");
        auto count = bound->queryBound("SELECT count(*) AS n FROM resources",{});
        require(count && count->getFieldValueAsInt("n") == 0,"Lookup must not persist an asset");

        const String quoted = "characters/hero's '; DROP TABLE resources;--.mesh";
        auto first = asset(quoted);
        const auto firstId = first->getHandle()->getUUIDAsString();
        catalog.addResourceEntry(first); catalog.addResourceEntry(first);
        require(entryId(catalog.getResourceEntryFromPath(quoted)) == firstId,"Quoted path must round-trip as bound data");
        count = bound->queryBound("SELECT count(*) AS n FROM resources",{});
        require(count && count->getFieldValueAsInt("n") == 1,"Repeated insert must be idempotent");
        auto detached = dynamic_pointer_cast<scene::ResourceDirector>(catalog.getResourceEntry(firstId));
        detached->setResourcePath("tampered");
        require(catalog.getResourceEntryFromPath(quoted) != nullptr,"Detached director cannot mutate authoritative catalog");

        auto conflict = asset(quoted);
        catalog.addResourceEntry(conflict);
        require(!catalog.hasResourceById(conflict->getHandle()->getUUIDAsString()),"Path collision cannot replace identity");
        auto second = asset("characters/second.mesh"); catalog.addResourceEntry(second);
        first->setFilePath(second->getFilePath()); catalog.updateResourceEntry(first);
        require(entryId(catalog.getResourceEntryFromPath(quoted)) == firstId,"Failed rename must retain original row");
        first->setFilePath("characters/renamed.mesh");
        first->setFileSystemId(workphone::UUID::generate());
        catalog.updateResourceEntry(first);
        require(!catalog.getResourceEntryFromPath(quoted),"Rename must remove old path");
        require(entryId(catalog.getResourceEntryFromPath(first->getFilePath())) == firstId,"Rename with unresolved filesystem ID preserves UUID");

        auto a = make_ptr<scene::ResourceDirector>(); a->getHandle()->setUUID(StringUtil::getUUID());
        auto b = make_ptr<scene::ResourceDirector>(); b->getHandle()->setUUID(StringUtil::getUUID());
        catalog.addResourceEntry(a); catalog.addResourceEntry(b);
        require(!catalog.getResourceEntryFromPath("scene"),"Shared scene path lookup must reject ambiguity");
        catalog.removeResourceEntry(a);
        require(!catalog.hasResourceById(a->getHandle()->getUUIDAsString()) && catalog.hasResourceById(b->getHandle()->getUUIDAsString()),"Scene deletion must target just its UUID");
        catalog.removeResourceEntryFromPath("scene");
        require(catalog.hasResourceById(b->getHandle()->getUUIDAsString()),"Path deletion must never remove shared scene rows");

        first->setFilePath(second->getFilePath()); catalog.removeResourceEntry(first);
        require(catalog.getResourceEntryFromPath(second->getFilePath()) != nullptr,"Stale object path cannot broaden UUID deletion");
        catalog.removeResourceEntryFromPath(second->getFilePath());
        require(!catalog.hasResourceById(second->getHandle()->getUUIDAsString()),"Explicit file path deletion must remove matching asset");

        auto persisted = asset("textures/persist.tex"); catalog.addResourceEntry(persisted);
        const auto persistedId = persisted->getHandle()->getUUIDAsString();
        std::atomic<bool> readsOkay{true};
        auto reader = [&] {
            try
            {
                for(int i=0;i<100;++i)
                    if(entryId(catalog.getResourceEntry(persistedId)) != persistedId) readsOkay=false;
            }
            catch(...) { readsOkay=false; }
        };
        std::thread one(reader), two(reader); one.join(); two.join();
        require(readsOkay,"Concurrent catalog reads must retain identity");
        catalog.loadFromFile(StringW((folder / "other.db").wstring().c_str()));
        require(!catalog.hasResourceById(persistedId),"Wide-string switch must select database B");
        catalog.loadFromFile(String((folder / "catalog.db").string().c_str()));
        require(entryId(catalog.getResourceEntryFromPath(persisted->getFilePath())) == persistedId,"Reopen/switch must preserve persistent identity");
        db = catalog.getDatabase(); bound = dynamic_cast<IParameterizedDatabase *>(db.get());
        require(bound && !bound->queryBound("INSERT INTO resources(uuid,path,type) VALUES(?,?,?)",{persistedId,"different.path","Test"}),"Unique UUID index must reject duplicates");
        auto later = asset("later.mesh"); catalog.addResourceEntry(later);
        require(catalog.hasResourceById(later->getHandle()->getUUIDAsString()),"Failed mutation must not strand transaction");
        require(!catalog.getResourceEntryFromPath(String(1025,'x')),"Oversized catalog key must reject cleanly");
        catalog.destroy();
        require(!catalog.getDatabase(),"Destroy must release connection");
        catalog.loadFromFile(String((folder / "catalog.db").string().c_str()));
        require(catalog.hasResourceById(persistedId),"Destroy must preserve persisted catalog data");
        catalog.clearDatabase();
        require(!catalog.hasResourceById(persistedId),"Explicit clear must remove rows");
    }
}
int main()
{
    TypeManager types; types.load(); TypeManager::setInstance(&types);
    auto app = make_ptr<core::ApplicationManager>(); core::IApplicationManager::setInstance(app);
    app->setLogManager(make_ptr<LogManagerDefault>());
    auto factories = make_ptr<FactoryManager>(); factories->load(nullptr); app->setFactoryManager(factories);
    app->setFileSystem(make_ptr<FileSystem>());
    auto plugin = make_ptr<SQLitePlugin>(); plugin->load(nullptr);
    const auto temp = std::filesystem::temp_directory_path();
    const auto folder = temp / (std::string("workphone_catalog_") + StringUtil::getUUID().c_str());
    std::filesystem::create_directories(folder);
    int result = 0;
    {
        auto catalog = make_ptr<AssetDatabaseManager>();
        try
        {
            catalog->loadFromFile(String((folder / "catalog.db").string().c_str()));
            require(catalog->getDatabase() && catalog->getDatabase()->isLoaded(),"Real SQLite backend must open");
            contracts(*catalog,folder);
            std::cout << "PASS: catalog identity, parameter binding, scoped deletion, rollback, detached lookups, concurrency and persistence\n";
        }
        catch(const std::exception &error) { std::cerr << "FAIL: " << error.what() << '\n'; result=1; }
        catalog->unload(nullptr); catalog=nullptr;
    }
    plugin->unload(nullptr); plugin=nullptr;
    app->setFileSystem(nullptr); app->setFactoryManager(nullptr);
    factories->unload(nullptr); factories=nullptr;
    core::IApplicationManager::setInstance(nullptr); app=nullptr;
    TypeManager::setInstance(nullptr); types.unload();
    if(folder.parent_path() == temp && folder.filename().string().find("workphone_catalog_") == 0)
        std::filesystem::remove_all(folder);
    return result;
}
