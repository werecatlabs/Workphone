#pragma once
#include <WPLua/LuaManager.hpp>
#include <WPLua/LuaObjectData.hpp>
#include <WPLua/LuaScriptCompiler.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Database/ResourceDatabase.hpp>
#include <Workphone/Script/ProjectScriptPolicy.hpp>
#include <Workphone/System/StateManager.hpp>
#include <Workphone/System/TimerMT.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/Scene/Transform.hpp>
#include <Workphone/Scene/Components/Script.hpp>
#include <Workphone/Memory/FactoryUtil.hpp>
#include <WPSQLite/WPSQLite.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <stdexcept>

// Actual Script component -> runtime UUID -> compiled Lua closure -> object/update.
// No renderer, project source filesystem, catalog or cooking index is needed by
// the consumer. The minimal manager fixture follows the existing native lifecycle.
inline void runLuaRuntimeScriptComponentContracts()
{
    using namespace workphone;
    using namespace workphone::resource;
    namespace fs=std::filesystem;
    auto check=[](bool value,const char *message) { if(!value)throw std::runtime_error(message); };
    check(!core::IApplicationManager::instancePtr(),"runtime Script fixture requires the prior fixture to retire");
    const auto root=fs::temp_directory_path()/fs::u8path(("wp-runtime-script-"+StringUtil::getUUID()).c_str());
    check(!fs::exists(root),"runtime Script fixture directory must be new");
    fs::create_directories(root/"authoring"/"source"/"logic");
    struct Directory {
        fs::path root;
        ~Directory(){
            // Restore only this fixture's files so Windows can remove the
            // read-only package after both fresh runtime consumers retire.
            std::error_code error;
            fs::recursive_directory_iterator it(root,error),end;
            while(!error&&it!=end){
                std::error_code permissionError;
                if(it->is_regular_file(permissionError))
                    fs::permissions(it->path(),fs::perms::owner_write,fs::perm_options::add,permissionError);
                it.increment(error);
            }
            fs::remove_all(root,error);
        }
    } directory{root};
    struct Application {
        SmartPtr<core::ApplicationManager> app=make_ptr<core::ApplicationManager>();
        SmartPtr<FactoryManager> factories=make_ptr<FactoryManager>();
        SmartPtr<SQLitePlugin> sqlite=make_ptr<SQLitePlugin>();
        SmartPtr<TimerMT> timer=make_ptr<TimerMT>();
        SmartPtr<StateManager> states=make_ptr<StateManager>();
        SmartPtr<scene::GameManager> game=make_ptr<scene::GameManager>();
        Application() {
            core::IApplicationManager::setInstance(app);
            factories->load(nullptr);app->setFactoryManager(factories);
            FactoryUtil::addFactory<scene::Transform>();
            app->setFileSystem(make_ptr<FileSystem>());sqlite->load(nullptr);
            timer->load(nullptr);app->setTimer(timer);
            app->setStateManager(states);states->load(nullptr);
            app->setGameManager(game);game->load(nullptr);
        }
        ~Application() {
            // Component finalization must run while its VM and native services live.
            game->unload(nullptr);releaseProjectScriptManager();
            app->setGameManager(nullptr);game=nullptr;
            states->unload(nullptr);app->setStateManager(nullptr);states=nullptr;
            app->setTimer(nullptr);timer->unload(nullptr);timer=nullptr;
            app->setResourceDatabase(nullptr);sqlite->unload(nullptr);sqlite=nullptr;
            app->setFileSystem(nullptr);app->setFactoryManager(nullptr);
            factories->unload(nullptr);factories=nullptr;
            core::IApplicationManager::setInstance(nullptr);
        }
    } application;
    check(application.timer->isLoaded()&&application.states->isLoaded()&&application.game->isLoaded(),"runtime Script native fixture did not load");
    const auto source=root/"authoring"/"source";
    const auto cooked=root/"authoring"/"compiled";
    const auto package=root/"package";
    auto write=[&](const fs::path &path,const char *text) {
        std::ofstream file(path,std::ios::binary);file<<text;check(file.good(),"runtime Script fixture write failed");
    };
    write(source/"logic"/"Helper.lua","return { amount=42 }");
    write(source/"logic"/"Actor.lua.deps","data://logic/Helper.lua\n");
    write(source/"logic"/"Actor.lua",R"(
        local helper=require('logic.Helper')
        runtimeAssetLoads=(runtimeAssetLoads or 0)+1
        class 'RuntimeAssetActor'
        function RuntimeAssetActor:__init(owner)
            self.owner=owner
            runtimeConstructs=(runtimeConstructs or 0)+1
        end
        function RuntimeAssetActor:update()
            runtimeTicks=(runtimeTicks or 0)+helper.amount
        end
    )");
    write(source/"Wrong.txtres","not a Lua resource");
    const String actorUuid="d2c49e32-1e39-4971-82e8-a4b12402d401";
    const String wrongUuid="d2c49e32-1e39-4971-82e8-a4b12402d402";
    const String missingUuid="d2c49e32-1e39-4971-82e8-a4b12402d403";
    String error;
    {
        class WrongTypeCompiler final : public IResourceCompiler {
        public:
            String name() const override {return "RuntimeScriptWrongTypeFixture";}
            Array<CompilerOutput> outputs() const override {return {{ResourceTypeID("txtres"),1}};}
            CompilationStatus compile(const CompileContext &,std::ostream &output,Array<String> &) const override
            {output<<"wrong type";return output?CompilationStatus::Success:CompilationStatus::Failure;}
        };
        auto registry=std::make_shared<ResourceCompilerRegistry>();
        check(registry->registerCompiler(std::make_shared<LuaScriptCompiler>(),&error),"runtime Script compiler registration");
        check(registry->registerCompiler(std::make_shared<WrongTypeCompiler>(),&error),"runtime Script wrong-type compiler registration");
        ResourceSystem author(registry,std::make_shared<ResourceCompilationDatabase>());
        ResourceSystemConfig config;config.sourceRoot=source.u8string().c_str();config.compiledRoot=cooked.u8string().c_str();config.target="runtime-script-contract";
        const auto initialized=author.initialize(config,error);check(initialized,error.c_str());
        CompilationOptions options;options.packagedBuild=true;
        const ResourceID actor("data://logic/Actor.lua"),wrong("data://Wrong.txtres");
        check(author.compile(actor,options).succeeded(),"runtime Script compilation failed");
        check(author.compile(wrong,options).succeeded(),"runtime Script wrong-type compilation failed");
        const auto packaged=author.writeRuntimePackage(package.u8string().c_str(),{actor,wrong},{{actorUuid,actor},{wrongUuid,wrong}},error);
        check(packaged,error.c_str());
        author.shutdown();
        auto catalog=make_ptr<AssetDatabaseManager>();
        check(catalog->setProjectRoot(source.u8string().c_str()),"runtime Script catalog root");
        catalog->loadFromFile((root/"authoring"/"catalog.db").u8string().c_str());
        check(catalog->getDatabase()&&catalog->getDatabase()->isLoaded(),"runtime Script authoring catalog fixture");
        catalog->unload(nullptr);
    }
    check(fs::canonical(root/"authoring").parent_path()==fs::canonical(root),"runtime Script cleanup root escaped");
    fs::remove_all(root/"authoring");
    check(!fs::exists(source)&&!fs::exists(cooked)&&!fs::exists(root/"authoring"/"catalog.db"),"runtime Script authoring inputs survived removal");
    auto files=[&]() {
        std::map<fs::path,std::string> values;
        for(const auto &entry:fs::recursive_directory_iterator(package)) if(entry.is_regular_file()) {
            std::ifstream file(entry.path(),std::ios::binary);
            values.emplace(fs::relative(entry.path(),package),std::string(std::istreambuf_iterator<char>(file),{}));
        }
        return values;
    };
    const auto before=files();
    check(!before.empty(),"runtime Script package was empty");
    for(const auto &entry:fs::recursive_directory_iterator(package)) if(entry.is_regular_file()) {
        fs::permissions(entry.path(),fs::perms::owner_write|fs::perms::group_write|fs::perms::others_write,fs::perm_options::remove);
        check((fs::status(entry.path()).permissions()&(fs::perms::owner_write|fs::perms::group_write|fs::perms::others_write))==fs::perms::none,"runtime Script package fixture did not become read-only");
    }
    const auto loose=root/"loose.lua";write(loose,"runtimeLooseFallback=true");
    RuntimeResourceConfig mount;mount.compiledRoot=package.u8string().c_str();mount.manifestPath=(package/"runtime.wprm").u8string().c_str();mount.target="runtime-script-contract";
    class SpyDatabase final : public ResourceDatabase {
    public:
        mutable size_t catalogLookups=0;
        SmartPtr<IDatabaseManager> getDatabaseManager() const override {++catalogLookups;return {};}
    };
    auto vm=make_ptr<LuaManager>();
    for(size_t pass=0;pass<2;++pass) {
        // Reuse the manager, but require an explicit package remount for each
        // fresh native VM and Script owner after the previous VM was retired.
        const auto configured=vm->configureRuntimeScriptResources(mount,error);check(configured,error.c_str());
        vm->load(nullptr);check(vm->isLoaded()&&vm->getLuaState(),"runtime Script VM load");
        auto state=vm->getLuaState();setProjectScriptManager(vm);
        auto spy=make_ptr<SpyDatabase>();application.app->setResourceDatabase(spy);
        check(!vm->loadScriptAsset(missingUuid),"runtime Script missing UUID was accepted");
        check(!vm->loadScriptAsset(wrongUuid),"runtime Script wrong-type UUID was accepted");
        check(spy->catalogLookups==0,"runtime Script UUID miss consulted authoring catalog");
        application.app->setResourceDatabase(nullptr);
        check(vm->executeSource("assert(runtimeAssetLoads==nil and runtimeConstructs==nil)","=runtime-before-component"),"runtime Script asset executed before owner load");
        auto actor=application.game->createActor();check(actor!=nullptr,"runtime Script actor creation");
        auto component=actor->addComponent<scene::Script>();check(component!=nullptr,"runtime Script component creation");
        auto properties=make_ptr<Properties>();properties->setProperty("className","RuntimeAssetActor");
        properties->setProperty("scriptAssetUuid",actorUuid);properties->setProperty("updateInEditMode",true);
        component->setProperties(properties);
        auto retained=dynamic_pointer_cast<LuaObjectData>(component->getScriptData());
        check(retained&&retained->getLuaState()==state,"runtime Script owner did not create Lua instance through UUID");
        component->update();
        check(vm->loadScriptAsset("D2C49E32-1E39-4971-82E8-A4B12402D401"),"runtime Script uppercase UUID alias was rejected");
        check(vm->executeSource("assert(runtimeAssetLoads==1 and runtimeConstructs==1 and runtimeTicks==42); assert(package.path=='' and package.cpath==''); assert(not pcall(require,'missing'))", "=runtime-component-update"),"runtime Script update or compiled module policy failed");
        vm->loadScript(loose.u8string().c_str());check(vm->getError()&&vm->getLuaState()==state,"runtime Script loose source load was permitted");
        vm->loadScripts({loose.u8string().c_str()});check(vm->getError()&&vm->getLuaState()==state,"runtime Script source list was permitted");
        vm->configureScriptResources(nullptr,nullptr,false);check(vm->getError()&&vm->getLuaState()==state,"runtime Script authoring retarget was permitted");
        check(!vm->configureRuntimeScriptResources(mount,error)&&vm->getLuaState()==state,"active runtime Script VM was retargeted");
        vm->setLuaState(nullptr);check(vm->getError()&&vm->getLuaState()==state,"runtime Script raw VM replacement was permitted");
        Thread::setCurrentTask(TaskId::Application);vm->reloadScripts();vm->update();
        check(vm->getError()&&!vm->reloadPending()&&vm->getLuaState()==state,"runtime Script reload replaced or reopened the VM");
        component->update();
        check(vm->executeSource("assert(runtimeTicks==84 and runtimeLooseFallback==nil)","=runtime-preserved-owner"),"runtime Script rejection damaged active owner or used loose source");
        application.game->destroyActor(actor);actor=nullptr;
        check(!component->getScriptData()&&!retained->getLuaState(),"runtime Script unload retained Lua owner state");
        component=nullptr;retained=nullptr;
        releaseProjectScriptManager();check(!vm->getLuaState(),"runtime Script VM did not unload");
        spy->catalogLookups=0;application.app->setResourceDatabase(spy);
        vm->setError(false);vm->load(nullptr);
        check(vm->getError()&&!vm->isLoaded()&&!vm->getLuaState(),"unmounted runtime Script VM reopened without explicit remount");
        vm->setError(false);vm->loadScript(loose.u8string().c_str());
        check(vm->getError()&&!vm->getLuaState(),"unloaded runtime Script loose source route lost runtime policy");
        vm->setError(false);
        check(!vm->loadScriptAsset(actorUuid)&&vm->getError()&&!vm->getLuaState(),"unloaded runtime Script UUID route lost runtime policy");
        check(spy->catalogLookups==0,"unloaded runtime Script route consulted authoring catalog");
        application.app->setResourceDatabase(nullptr);
        check(files()==before,"runtime Script consumer modified package contents");
    }
}
