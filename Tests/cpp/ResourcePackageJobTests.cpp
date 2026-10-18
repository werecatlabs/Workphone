#include <Workphone/Workphone.hpp>
#include <Workphone/System/PackageManager.hpp>
#include <Workphone/System/ResourcePackageProfile.hpp>
#include <Workphone/System/JobQueue.hpp>
#include <Workphone/System/ThreadPool.hpp>
#include <Workphone/Core/LogManagerDefault.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <WPLua/LuaManager.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <luabind/luabind.hpp>
#include <WPSQLite/WPSQLite.hpp>
#include <filesystem>
#include <future>
#include <iostream>
#include <stdexcept>

using namespace workphone;
namespace fs = std::filesystem;
namespace
{
    void require(bool value, const char *message) { if(!value) throw std::runtime_error(message); }
    class Queue final : public JobQueue
    {
    public:
        SmartPtr<IJob> captured;
        bool isRunning() const override { return true; }
        void addJob(SmartPtr<IJob> job) override { captured = job; }
    };
    class Workers final : public ThreadPool
    {
    public:
        u32 getNumThreads() const override { return 1; }
    };
    void run(const fs::path &project, const fs::path &original)
    {
        auto app = core::IApplicationManager::instance();
        app->setProjectPath(project.u8string().c_str());
        app->setRunning(true);
        auto queue = make_ptr<Queue>();
        app->setJobQueue(queue);
        app->setThreadPool(make_ptr<Workers>());
        auto manager = make_ptr<PackageManager>();
        app->setPackageManager(manager);
        {
            LuaManager lua;
            lua.load(nullptr);
            luabind::globals(lua.getLuaState())["packages"] = SmartPtr<IPackageManager>(manager);
            require(lua.executeSource(
                "packages:setResourcePackageProfile('shipping.json'); "
                "assert(packages:getResourcePackageProfile()=='shipping.json'); "
                "assert(not packages:isResourcePackageBusy()); "
                "assert(packages:getResourcePackageStatus()=='idle'); "
                "assert(packages:getResourcePackageLogPath()==''); "
                "packages:cancelResourcePackage()", "=package-binding-contract"), lua.getLastDiagnostic().c_str());
            lua.unload(nullptr);
        }
        const auto base = loadPackageProfile(original.u8string().c_str());
        auto configure = [&](const char *name) {
            auto arguments = base;
            const auto output = project / name;
            arguments[4] = output.u8string().c_str();
            const auto profile = project / (std::string(name) + ".json");
            savePackageProfile(profile.u8string().c_str(), arguments);
            manager->setResourcePackageProfile(profile.u8string().c_str());
            return profile;
        };
        configure("cancelled-output");
        require(manager->startResourcePackage() && manager->isResourcePackageBusy(), "Queue package request");
        require(!manager->startResourcePackage(), "Reject overlapping manager builds");
        manager->cancelResourcePackage();
        queue->captured->execute();
        require(manager->getResourcePackageStatus() == "cancelled" && !fs::exists(project / "cancelled-output"), "Queued cancellation prevents publication");
        const auto profile = configure("success-output");
        require(manager->startResourcePackage(), "Queue retry after cancellation");
        // Change the selected profile after queueing. Worker must use the captured request.
        const auto moved = project / "profile-renamed-after-queue.json";
        fs::rename(profile, moved);
        auto job = queue->captured;
        auto future = std::async(std::launch::async, [job]() mutable {
            Thread::setCurrentThreadId(Thread::ThreadId::WorkerThread);
            job->execute();
        });
        const auto unrelated = std::async(std::launch::async, [] { return 42; });
        require(unrelated.wait_for(std::chrono::seconds(1)) == std::future_status::ready, "Package job does not wait for unrelated work");
        require(future.wait_for(std::chrono::seconds(60)) == std::future_status::ready, "Package child completes");
        future.get();
        require(manager->getResourcePackageStatus() == "succeeded" && !manager->isResourcePackageBusy(), "Successful child reports completion");
        require(fs::is_regular_file(project / "success-output" / "runtime.wprm"), "Success publishes checked runtime manifest");
        require(fs::is_regular_file(fs::u8path(manager->getResourcePackageLogPath().c_str())), "Capture owned child log");
        fs::rename(moved, profile);
        require(manager->startResourcePackage(), "Queue occupied destination failure");
        auto failureJob = queue->captured;
        std::async(std::launch::async, [failureJob]() mutable {
            Thread::setCurrentThreadId(Thread::ThreadId::WorkerThread);
            failureJob->execute();
        }).get();
        require(manager->getResourcePackageStatus() == "failed", "Nonzero child exit propagates failure");
        require(fs::is_regular_file(project / "success-output" / "runtime.wprm"), "Failed retry preserves last package");
        configure("switch-output");
        require(manager->startResourcePackage(), "Queue before project switch");
        const auto other = project / "other-project";
        fs::create_directory(other);
        app->setProjectPath(other.u8string().c_str());
        queue->captured->execute();
        require(!fs::exists(project / "switch-output"), "Project switch cancels queued old-session work");
        require(!manager->startResourcePackage(), "Reject profile bound to another project");
        app->setProjectPath(project.u8string().c_str());
        configure("unload-output");
        require(manager->startResourcePackage(), "Queue before manager unload");
        manager->unload(nullptr);
        queue->captured->execute();
        require(!fs::exists(project / "unload-output"), "Manager unload cancels queued work");
        for(const auto &entry : fs::directory_iterator(project / ".workphone" / "package-jobs"))
            require(entry.path().extension() == ".log", "Finished requests remove immutable profile snapshots");
        queue->captured = nullptr;
        app->setPackageManager(nullptr);
        app->setJobQueue(nullptr);
        app->setRunning(false);
        std::cout << "PASS: native package jobs, immutable requests, cancellation, failure, retry and project isolation\n";
    }
}
int wmain(int argc, wchar_t **argv)
{
    if(argc != 3 && argc != 5) return 2;
    TypeManager types;
    types.load(); TypeManager::setInstance(&types);
    auto app = make_ptr<core::ApplicationManager>();
    core::IApplicationManager::setInstance(app);
    app->setLogManager(make_ptr<LogManagerDefault>());
    auto factories = make_ptr<FactoryManager>();
    factories->load(nullptr); app->setFactoryManager(factories);
    app->setFileSystem(make_ptr<FileSystem>());
    auto sqlite = make_ptr<SQLitePlugin>(); sqlite->load(nullptr);
    int code = 0;
    try
    {
        if(argc == 5 && std::wstring(argv[1]) == L"--consume")
        {
            resource::ResourceSystem runtime(nullptr, nullptr);
            resource::RuntimeResourceConfig config;
            config.compiledRoot = fs::path(argv[2]).u8string().c_str();
            config.manifestPath = (fs::path(argv[2]) / "runtime.wprm").u8string().c_str();
            config.target = StringUtil::toUTF16to8(argv[3]);
            String error;
            require(runtime.initializeRuntime(config, error), error.c_str());
            auto root = runtime.loadUUID(StringUtil::toUTF16to8(argv[4]), error);
            require(root != nullptr, error.c_str());
            LuaManager lua;
            lua.load(nullptr);
            require(lua.executeSource("package.path=''; package.cpath=''", "=source-free-search"), "Disable authoring module fallback");
            require(lua.loadScriptResource(root), lua.getLastDiagnostic().c_str());
            require(lua.executeSource("assert(require('helper').value==42)", "=source-free-consumer"), lua.getLastDiagnostic().c_str());
            lua.unload(nullptr);
            std::cout << "PASS: source-free packaged Lua consumer and require dependency\n";
        }
        else run(fs::path(argv[1]), fs::path(argv[2]));
    }
    catch(const std::exception &error) { std::cerr << error.what() << '\n'; code = 1; }
    app->setPackageManager(nullptr);
    app->setJobQueue(nullptr);
    app->setThreadPool(nullptr);
    app->setRunning(false);
    sqlite->unload(nullptr); sqlite = nullptr;
    app->setFileSystem(nullptr); app->setFactoryManager(nullptr);
    factories->unload(nullptr); factories = nullptr;
    core::IApplicationManager::setInstance(nullptr); app = nullptr;
    TypeManager::setInstance(nullptr); types.unload();
    return code;
}
