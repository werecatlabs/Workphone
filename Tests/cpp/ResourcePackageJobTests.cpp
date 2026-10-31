#include <Workphone/Workphone.hpp>
#include <Workphone/System/PackageManager.hpp>
#include <Workphone/System/ResourcePackageProfile.hpp>
#include <Workphone/System/JobQueue.hpp>
#include <Workphone/System/ThreadPool.hpp>
#include <Workphone/Core/LogManagerDefault.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/Jobs/ResourcePackageJob.hpp>
#include <Windows.h>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Database/ResourceDatabase.hpp>
#include <WPLua/LuaManager.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <luabind/luabind.hpp>
#include <WPSQLite/WPSQLite.hpp>
#include <filesystem>
#include <future>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <thread>

using namespace workphone;
namespace fs = std::filesystem;
namespace
{
    void require(bool value, const char *message) { if(!value) throw std::runtime_error(message); }
    struct EnvironmentValue
    {
        std::string name, previous;
        EnvironmentValue(const char *key, const char *value) : name(key)
        {
            if(const auto old = std::getenv(key)) previous = old;
            _putenv_s(key, value);
        }
        ~EnvironmentValue() { _putenv_s(name.c_str(), previous.c_str()); }
    };
    class Queue final : public JobQueue
    {
    public:
        SmartPtr<IJob> captured;
        bool accepting = true;
        bool isRunning() const override { return true; }
        void addJob(SmartPtr<IJob> job) override
        {
            if(!accepting) return;
            job->setState(IJob::State::Queue);
            captured = job;
        }
    };
    class Workers final : public ThreadPool
    {
    public:
        u32 getNumThreads() const override { return 1; }
        IThreadPool::State getState() const override { return IThreadPool::State::Start; }
    };
    void run(const fs::path &project, const fs::path &original)
    {
        EnvironmentValue maximum("WP_RESOURCE_PACKAGE_MAX_RUN_MS", "0");
        EnvironmentValue grace("WP_RESOURCE_PACKAGE_CANCEL_GRACE_MS", "30000");
        EnvironmentValue historyCount("WP_RESOURCE_PACKAGE_HISTORY_KEEP_COUNT", "64");
        EnvironmentValue historyBytes("WP_RESOURCE_PACKAGE_HISTORY_MAX_BYTES", "67108864");
        EnvironmentValue logBytes("WP_RESOURCE_PACKAGE_MAX_LOG_BYTES", "0");
        auto app = core::IApplicationManager::instance();
        app->setProjectPath(project.u8string().c_str());
        app->setRunning(true);
        auto queue = make_ptr<Queue>();
        app->setJobQueue(queue);
        auto manager = make_ptr<PackageManager>();
        app->setPackageManager(manager);
        {
            auto catalog = make_ptr<AssetDatabaseManager>();
            require(catalog->setProjectRoot(project.u8string().c_str()), "Capture picker project root");
            catalog->loadFromFile((project / "catalog.db").u8string().c_str());
            auto resources = make_ptr<ResourceDatabase>();
            resources->setDatabaseManager(catalog);
            app->setResourceDatabase(resources);
            LuaManager lua;
            lua.load(nullptr);
            luabind::globals(lua.getLuaState())["packages"] = SmartPtr<IPackageManager>(manager);
            require(lua.executeSource(R"(
                local found = packages:queryResourcePackageRoots('main.lua', 'script')
                assert(found.ok, found.error)
                assert(#found.entries == 1 and not found.hasMore)
                pickerRow = found.entries[1]
                assert(pickerRow.path == 'main.lua' and pickerRow.type == 'script')
                assert(type(pickerRow.generation) == 'string' and type(pickerRow.catalogInstance) == 'string')
                local picked = packages:selectResourcePackageRoot(pickerRow)
                assert(picked.ok and picked.uuid == pickerRow.uuid, picked.error)
                assert(#packages:queryResourcePackageRoots('main.lua', 'Texture').entries == 0)
                assert(#packages:queryResourcePackageRoots('%', '').entries == 0)
                local bounded = packages:queryResourcePackageRoots('picker-only/', '')
                assert(bounded.ok and #bounded.entries == 25 and bounded.hasMore)
                pickerCursor = bounded.next
                local tail = packages:queryResourcePackageRootsPage('picker-only/', '', pickerCursor)
                assert(tail.ok and #tail.entries == 5 and not tail.hasMore)
                local seen = {}
                for _, row in ipairs(bounded.entries) do seen[row.uuid] = true end
                for _, row in ipairs(tail.entries) do assert(not seen[row.uuid]); seen[row.uuid] = true end
                assert(not packages:queryResourcePackageRootsPage('', '', pickerCursor).ok)
                local generation = pickerCursor.generation
                pickerCursor.generation = '18446744073709551616'
                assert(not packages:queryResourcePackageRootsPage('picker-only/', '', pickerCursor).ok)
                pickerCursor.generation = generation
                assert(not packages:queryResourcePackageRootsPage('', '', {}).ok)
                assert(not packages:queryResourcePackageRoots(string.rep('x', 4097), '').ok)
                local generation = pickerRow.generation
                pickerRow.generation = '0'
                assert(not packages:selectResourcePackageRoot(pickerRow).ok)
                pickerRow.generation = generation
                pickerRow.extra = true
                assert(not packages:selectResourcePackageRoot(pickerRow).ok)
                pickerRow.extra = nil
            )", "=package-root-picker-contract"), lua.getLastDiagnostic().c_str());
            catalog->close();
            catalog->loadFromFile((project / "catalog.db").u8string().c_str());
            require(lua.executeSource("assert(not packages:selectResourcePackageRoot(pickerRow).ok); "
                "assert(not packages:queryResourcePackageRootsPage('picker-only/', '', pickerCursor).ok)",
                "=package-root-reopened-catalog"), lua.getLastDiagnostic().c_str());
            app->setProjectPath(original.u8string().c_str());
            require(lua.executeSource("assert(not packages:queryResourcePackageRoots('', '').ok); "
                "assert(not packages:selectResourcePackageRoot(pickerRow).ok)",
                "=package-root-project-isolation"), lua.getLastDiagnostic().c_str());
            app->setProjectPath(project.u8string().c_str());
            require(lua.executeSource(
                "packages:setResourcePackageProfile('shipping.json'); "
                "assert(packages:getResourcePackageProfile()=='shipping.json'); "
                "assert(not packages:isResourcePackageBusy()); "
                "assert(packages:getResourcePackageStatus()=='idle'); "
                "assert(packages:getResourcePackageLogPath()==''); "
                "packages:cancelResourcePackage()", "=package-binding-contract"), lua.getLastDiagnostic().c_str());
            require(lua.executeSource(R"(
                local loaded = packages:readResourcePackageProfile('shipping.json')
                assert(loaded.ok, loaded.error)
                local profile = loaded.profile
                assert(profile.target == 'pc' and #profile.roots == 1)
                assert(profile.mappings[1].catalogType == 'script')
                profile.buildProfile='desktop'
                profile.toolchain='test-toolchain-identity'
                profile.buildSettings='{"quality":"high","empty":""}'
                local profiled=packages:saveResourcePackageProfile('ui-profiled.json',profile)
                assert(profiled.ok,profiled.error)
                local roundtrip=packages:readResourcePackageProfile('ui-profiled.json')
                assert(roundtrip.ok and roundtrip.profile.buildProfile=='desktop')
                assert(roundtrip.profile.toolchain=='test-toolchain-identity')
                assert(roundtrip.profile.buildSettings:find('"empty":""',1,true))
                profile.buildSettings='{"duplicate":"a","duplicate":"b"}'
                assert(not packages:saveResourcePackageProfile('ui-duplicate-settings.json',profile).ok)
                profile.buildProfile=''; profile.toolchain=''; profile.buildSettings='{}'
                profile.target = 'ui-profile-contract'
                profile.output = '../ui-authored-output'
                profile.cookedRoot = './ui-cooked'
                local saved = packages:saveResourcePackageProfile('ui-authored.json', profile)
                assert(saved.ok, saved.error)
                assert(packages:getResourcePackageProfile() == saved.path)
                local reopened = packages:readResourcePackageProfile(saved.path)
                assert(reopened.ok and reopened.profile.target == 'ui-profile-contract')
                assert(not packages:saveResourcePackageProfile('ui-authored.json', profile).ok)
                profile.roots[2] = profile.roots[1]:upper()
                assert(not packages:saveResourcePackageProfile('duplicate-roots.json', profile).ok)
                profile.roots[2] = nil
                profile.mappings[2] = profile.mappings[1]
                assert(not packages:saveResourcePackageProfile('duplicate-mappings.json', profile).ok)
                profile.mappings[2] = nil
                profile.extra = true
                assert(not packages:saveResourcePackageProfile('unknown-field.json', profile).ok)
                profile.extra = nil
                profile.roots[3] = profile.roots[1]
                assert(not packages:saveResourcePackageProfile('sparse-roots.json', profile).ok)
                profile.roots[3] = nil
                local source = profile.sourceRoot
                profile.sourceRoot = '..'
                assert(not packages:saveResourcePackageProfile('wrong-project.json', profile).ok)
                profile.sourceRoot = source
                assert(not packages:readResourcePackageProfile('missing-profile.json').ok)
            )", "=package-profile-authoring-contract"), lua.getLastDiagnostic().c_str());
            app->setResourceDatabase(nullptr);
            resources->setDatabaseManager(nullptr);
            catalog->close();
            lua.unload(nullptr);
        }
        const auto base = loadPackageProfile(original.u8string().c_str());
        {
            const auto protectedPath = project / "user-owned-profile.json";
            { std::ofstream owned(protectedPath); owned << "Preserve this author-owned file"; }
            auto request = std::make_shared<ResourcePackageJobState>();
            request->arguments = base;
            request->projectRoot = project.u8string().c_str();
            request->snapshotPath = protectedPath.u8string().c_str();
            request->logPath = (project / "unused-conflict.log").u8string().c_str();
            request->executable = "must-not-launch";
            auto conflictJob = make_ptr<ResourcePackageJob>(request);
            std::async(std::launch::async, [conflictJob]() mutable {
                Thread::setCurrentThreadId(Thread::ThreadId::WorkerThread);
                conflictJob->execute();
            }).get();
            require(request->status == "failed", "Existing captured-profile path must fail exclusively");
            std::ifstream retained(protectedPath);
            std::string bytes((std::istreambuf_iterator<char>(retained)), std::istreambuf_iterator<char>());
            require(bytes == "Preserve this author-owned file", "Failed snapshot creation must never clean up an unowned profile");
            auto lockPath = protectedPath;
            lockPath += ".lock";
            require(!fs::exists(lockPath), "Failed snapshot request releases only its owned lock");
            auto markerPath = protectedPath.parent_path() / protectedPath.stem();
            markerPath += ".owner";
            require(!fs::exists(markerPath), "Failed snapshot request cleans only its newly created ownership marker");
        }
        auto configure = [&](const char *name) {
            auto arguments = base;
            const auto output = project.parent_path() / name;
            arguments[4] = output.u8string().c_str();
            const auto profile = project / (std::string(name) + ".json");
            savePackageProfile(profile.u8string().c_str(), arguments);
            manager->setResourcePackageProfile(profile.u8string().c_str());
            return profile;
        };
        configure("cancelled-output");
        require(!manager->startResourcePackage() && !queue->captured,
                "Missing worker pool fails without queueing work on the UI thread");
        app->setThreadPool(make_ptr<Workers>());
        for(const auto bad : {"", "-1", "1", "1000ms", "86400001", "18446744073709551616"})
        {
            EnvironmentValue invalid("WP_RESOURCE_PACKAGE_MAX_RUN_MS", bad);
            // An empty environment value removes the override and uses the default.
            if(*bad) require(!manager->startResourcePackage() && !queue->captured,
                "Invalid execution limits fail before queueing a child");
        }
        {
            EnvironmentValue invalid("WP_RESOURCE_PACKAGE_CANCEL_GRACE_MS", "600001");
            require(!manager->startResourcePackagePreview() && !queue->captured,
                "Invalid cancellation grace fails before queueing a preview");
        }
        for(const auto key : {"WP_RESOURCE_PACKAGE_HISTORY_KEEP_COUNT", "WP_RESOURCE_PACKAGE_HISTORY_MAX_BYTES", "WP_RESOURCE_PACKAGE_MAX_LOG_BYTES"})
        {
            EnvironmentValue invalid(key, "4294967295");
            require(!manager->startResourcePackage() && !queue->captured, "Invalid resource budgets fail before queueing");
        }
        queue->accepting = false;
        require(!manager->startResourcePackage() && !manager->isResourcePackageBusy(),
                "A dropped queue request reports failure rather than staying busy");
        queue->accepting = true;
        require(manager->startResourcePackage() && manager->isResourcePackageBusy(), "Queue package request");
        require(!manager->startResourcePackage(), "Reject overlapping manager builds");
        manager->cancelResourcePackage();
        queue->captured->execute();
        require(manager->getResourcePackageStatus() == "cancelled" && !fs::exists(project.parent_path() / "cancelled-output"), "Queued cancellation prevents publication");
        configure("stopped-output");
        require(manager->startResourcePackage(), "Queue before worker queue shutdown");
        queue->captured->stop();
        require(!manager->isResourcePackageBusy() && manager->getResourcePackageStatus() == "cancelled",
                "Queue shutdown finalizes an unstarted request");
        queue->captured->execute();
        require(!fs::exists(project.parent_path() / "stopped-output"), "Stopped job cannot execute later");
        manager->setResourcePackageProfile((project / "ui-authored.json").u8string().c_str());
        require(manager->startResourcePackage(), "Queue profile authored through native Lua controls");
        auto authoredJob = queue->captured;
        std::async(std::launch::async, [authoredJob]() mutable {
            Thread::setCurrentThreadId(Thread::ThreadId::WorkerThread);
            authoredJob->execute();
        }).get();
        require(manager->getResourcePackageStatus() == "succeeded", "Build UI-authored target/output profile");
        {
            resource::ResourceSystem authoredRuntime(nullptr, nullptr);
            resource::RuntimeResourceConfig authoredConfig;
            authoredConfig.compiledRoot = (project.parent_path() / "ui-authored-output").u8string().c_str();
            authoredConfig.manifestPath = (project.parent_path() / "ui-authored-output" / "runtime.wprm").u8string().c_str();
            authoredConfig.target = "ui-profile-contract";
            String error;
            require(authoredRuntime.initializeRuntime(authoredConfig, error), error.c_str());
            require(authoredRuntime.loadUUID(base.back(), error) != nullptr, "Authored roots/target/output reach the actual runtime manifest");
        }
        for(const auto *rejected : {"duplicate-roots.json", "duplicate-mappings.json", "unknown-field.json", "sparse-roots.json", "wrong-project.json"})
            require(!fs::exists(project / rejected), "Rejected profile edits must not create files");
        configure("preview-output");
        require(manager->startResourcePackagePreview(), "Queue closure preview through owned worker");
        require(!manager->startResourcePackage(), "Preview excludes overlapping build");
        manager->cancelResourcePackage();
        queue->captured->execute();
        require(manager->getResourcePackageStatus() == "cancelled" && manager->getResourcePackagePreviewPath().empty(), "Cancelled preview exposes no report");
        require(manager->startResourcePackagePreview(), "Retry closure preview after cancellation");
        auto previewJob = queue->captured;
        std::async(std::launch::async, [previewJob]() mutable {
            Thread::setCurrentThreadId(Thread::ThreadId::WorkerThread);
            previewJob->execute();
        }).get();
        require(manager->getResourcePackageStatus() == "succeeded", "Preview process succeeds");
        require(!fs::exists(project.parent_path() / "preview-output"), "Preview does not publish package output");
        require(fs::is_regular_file(fs::u8path(manager->getResourcePackagePreviewPath().c_str())), "Successful preview retains full JSON report");
        {
            LuaManager lua;
            lua.load(nullptr);
            luabind::globals(lua.getLuaState())["packages"] = SmartPtr<IPackageManager>(manager);
            require(lua.executeSource(R"(
                local report = packages:readResourcePackagePreview()
                assert(report.ok, report.error)
                assert(report.total == 127 and #report.entries == 100 and report.target == 'pc')
                local byId = {}
                for _, entry in ipairs(report.entries) do byId[entry.resourceId] = entry end
                assert(byId['data://main.lua'].root and not byId['data://helper.lua'].root)
                assert(byId['data://helper.lua'].includedBy == 'data://main.lua')
                assert(tonumber(report.totalPayloadBytes) > 0)
                local seen, count = {}, 0
                for page=0,5 do
                    local slice = packages:readResourcePackagePreviewPage(page)
                    assert(slice.ok, slice.error)
                    assert(slice.page == page and slice.total == 127 and slice.path == report.path)
                    assert(slice.hasPrevious == (page > 0) and slice.hasMore == (page < 5))
                    assert(#slice.entries == (page == 5 and 2 or 25))
                    for _, row in ipairs(slice.entries) do
                        assert(not seen[row.resourceId])
                        seen[row.resourceId] = true; count = count+1
                        if not row.root then assert(row.includedBy == 'data://main.lua') end
                    end
                end
                assert(count == 127 and seen['data://pages/item124.lua'])
                for _, page in ipairs({-1,6,164,2147483647}) do
                    assert(not packages:readResourcePackagePreviewPage(page).ok)
                end
            )", "=package-preview-report-contract"), lua.getLastDiagnostic().c_str());
            const auto reportPath = fs::u8path(manager->getResourcePackagePreviewPath().c_str());
            auto retainedReport = reportPath;
            retainedReport += ".retained";
            fs::rename(reportPath, retainedReport);
            require(lua.executeSource("assert(not packages:readResourcePackagePreview().ok)", "=missing-preview-report"), lua.getLastDiagnostic().c_str());
            { std::ofstream invalid(reportPath); invalid << "{\"version\":1}"; }
            require(lua.executeSource("assert(not packages:readResourcePackagePreview().ok)", "=malformed-preview-report"), lua.getLastDiagnostic().c_str());
            fs::remove(reportPath);
            fs::rename(retainedReport, reportPath);
            const auto other = project / "preview-other-project";
            fs::create_directory(other);
            app->setProjectPath(other.u8string().c_str());
            require(lua.executeSource("assert(not packages:readResourcePackagePreview().ok)", "=preview-project-isolation"), lua.getLastDiagnostic().c_str());
            app->setProjectPath(project.u8string().c_str());
            lua.unload(nullptr);
        }
        manager->setResourcePackageProfile("missing-preview-profile.json");
        require(!manager->startResourcePackagePreview() && manager->getResourcePackagePreviewPath().empty(),
            "A rejected preview request cannot expose the previous successful report");
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
        require(fs::is_regular_file(project.parent_path() / "success-output" / "runtime.wprm"), "Success publishes checked runtime manifest");
        require(fs::is_regular_file(fs::u8path(manager->getResourcePackageLogPath().c_str())), "Capture owned child log");
        fs::rename(moved, profile);
        require(manager->startResourcePackage(), "Queue occupied destination failure");
        auto failureJob = queue->captured;
        std::async(std::launch::async, [failureJob]() mutable {
            Thread::setCurrentThreadId(Thread::ThreadId::WorkerThread);
            failureJob->execute();
        }).get();
        require(manager->getResourcePackageStatus() == "failed", "Nonzero child exit propagates failure");
        require(fs::is_regular_file(project.parent_path() / "success-output" / "runtime.wprm"), "Failed retry preserves last package");
        configure("switch-output");
        require(manager->startResourcePackage(), "Queue before project switch");
        const auto other = project / "other-project";
        fs::create_directory(other);
        app->setProjectPath(other.u8string().c_str());
        queue->captured->execute();
        require(!fs::exists(project.parent_path() / "switch-output"), "Project switch cancels queued old-session work");
        require(!manager->startResourcePackage(), "Reject profile bound to another project");
        app->setProjectPath(project.u8string().c_str());
        configure("unload-output");
        require(manager->startResourcePackage(), "Queue before manager unload");
        manager->unload(nullptr);
        queue->captured->execute();
        require(!fs::exists(project.parent_path() / "unload-output"), "Manager unload cancels queued work");
        for(const auto &entry : fs::directory_iterator(project / ".workphone" / "package-jobs"))
            require(entry.path().extension() == ".log" || entry.path().filename().u8string().find(".preview.json") != std::string::npos ||
                entry.path().filename().u8string().find(".completed.json") != std::string::npos,
                "Finished requests retain only logs and owned history metadata");
        queue->captured = nullptr;
        app->setPackageManager(nullptr);
        app->setJobQueue(nullptr);
        app->setRunning(false);
        std::cout << "PASS: native package jobs, immutable requests, cancellation, failure, retry and project isolation\n";
    }
}
namespace
{
    void watchdogContracts(const fs::path &project, const fs::path &profile, const fs::path &executable)
    {
        const auto jobs = project / ".workphone" / "package-jobs";
        const auto marker = project / "crash-child.pid";
        for(int mode : {1,2,3,4,6,7,8,5})
        {
            fs::remove(marker);
            auto request = std::make_shared<ResourcePackageJobState>();
            request->arguments = loadPackageProfile(profile.u8string().c_str());
            request->projectRoot = project.u8string().c_str();
            request->executable = executable.u8string().c_str();
            const String id = "d08c0000-0000-4000-8000-00000000000" + StringUtil::toString(mode);
            const auto snapshot = jobs / fs::u8path((id + ".json").c_str());
            request->snapshotPath = snapshot.u8string().c_str();
            request->logPath = (jobs / fs::u8path((id + ".log").c_str())).u8string().c_str();
            request->maximumRunMilliseconds = mode == 2 ? 0 : 2000;
            request->cancelGraceMilliseconds = 500;
            request->maximumLogBytes = mode==6 || mode==7 ? 4096 : (mode==8 ? 0 : 32*1024*1024);
            auto job = make_ptr<ResourcePackageJob>(request);
            auto future = std::async(std::launch::async, [job]() mutable {
                Thread::setCurrentThreadId(Thread::ThreadId::WorkerThread);
                job->execute();
            });
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
            while(!fs::exists(marker) && !request->finished && std::chrono::steady_clock::now() < deadline)
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            if(!fs::exists(marker)) request->cancel = true;
            require(fs::exists(marker), "Watchdog child started and published its test PID");
            HANDLE child = nullptr;
            if(mode != 5 && mode != 7 && mode != 8)
            {
                DWORD pid = 0; std::ifstream input(marker); input >> pid;
                child = OpenProcess(SYNCHRONIZE, FALSE, pid);
            }
            if(mode == 2) request->cancel = true;
            const auto ready = future.wait_for(std::chrono::seconds(10)) == std::future_status::ready;
            if(!ready) request->cancel = true;
            require(ready, "Package child supervision finishes within a bounded wait");
            future.get();
            if(mode != 5 && mode != 7 && mode != 8)
            {
                const auto exited = child && WaitForSingleObject(child, 1000) == WAIT_OBJECT_0;
                if(child) CloseHandle(child);
                require(exited, "Supervised child has exited before the job completes");
            }
            const bool retained = mode == 1 || mode == 2 || mode == 5 || mode == 6;
            require(fs::exists(snapshot) == retained, "Only uncertain outcomes retain captured requests");
            if(retained)
            {
                require(request->status == "interrupted" && request->diagnostic.find("unknown") != String::npos,
                    "Forced stops and crashes expose an uncertain publication outcome");
            }
            else if(mode == 3)
                require(request->status == "timed out" && request->exitCode == 3,
                    "Deadline cancellation is distinct from forced interruption");
            else
                require(request->status == "succeeded" && request->exitCode == 0,
                    "Successful child completion wins over a pending deadline");
            if(mode==6 || mode==7)
            {
                const auto path=fs::u8path(request->logPath.c_str());
                require(fs::file_size(path)==4096,"Diagnostic file never exceeds its captured byte budget");
                std::ifstream input(path,std::ios::binary);
                const std::string bytes((std::istreambuf_iterator<char>(input)),std::istreambuf_iterator<char>());
                require(bytes.find("[log truncated by package job]")!=std::string::npos,"Truncation remains visible after process exit");
            }
            if(mode==8) require(fs::file_size(fs::u8path(request->logPath.c_str()))==16384,"Explicit zero log budget disables truncation");
        }
        std::cout << "PASS: watchdog deadline, forced cancellation, cooperative timeout, success race and unexpected child exit\n";
    }
}
int wmain(int argc, wchar_t **argv)
{
    if(argc >= 3 && std::wstring(argv[1]) == L"--profile")
    {
        // Deliberately blocking test child: the production job still owns its
        // process handles, inheritance restrictions and Windows Job Object.
        const auto snapshot = fs::path(argv[2]);
        const auto marker=snapshot.parent_path().parent_path().parent_path() / "crash-child.pid";
        auto pendingMarker=marker;pendingMarker+=".tmp";
        {
            std::ofstream pid(pendingMarker,std::ios::trunc);
            pid << GetCurrentProcessId();pid.close();
            if(!pid)return 19;
        }
        // Existence is the parent's readiness signal. Publish only a closed,
        // complete PID file so it cannot observe an empty/partial process ID.
        fs::rename(pendingMarker,marker);
        const auto name = snapshot.stem().u8string();
        if(name == "d08c0000-0000-4000-8000-000000000003" || name == "d08c0000-0000-4000-8000-000000000004")
        {
            if(argc != 5) return 19;
            const auto event = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(std::stoull(argv[4])));
            if(WaitForSingleObject(event, 10000) != WAIT_OBJECT_0) return 19;
            return name.back() == '3' ? 3 : 0;
        }
        if(name == "d08c0000-0000-4000-8000-000000000005") return 17;
        if(name == "d08c0000-0000-4000-8000-000000000006" ||
           name == "d08c0000-0000-4000-8000-000000000007" || name == "d08c0000-0000-4000-8000-000000000008")
        {
            const std::string bytes(16384,'x'); DWORD written=0;
            do { if(!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr)) return 19; }
            while(name.back()=='6');
            return 0;
        }
        Sleep(INFINITE);
        return 1;
    }
    if(argc != 3 && argc != 4 && argc != 5) return 2;
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
        if(argc == 4 && std::wstring(argv[1]) == L"--watchdog")
            watchdogContracts(fs::canonical(argv[2]), fs::path(argv[3]), fs::absolute(argv[0]));
        else if(argc == 4 && (std::wstring(argv[1]) == L"--crash-owner" || std::wstring(argv[1]) == L"--run-profile"))
        {
            const bool crashOwner = std::wstring(argv[1]) == L"--crash-owner";
            const auto project = fs::canonical(argv[2]);
            auto request = std::make_shared<ResourcePackageJobState>();
            request->arguments = loadPackageProfile(fs::path(argv[3]).u8string().c_str());
            request->projectRoot = project.u8string().c_str();
            const auto executable = fs::absolute(argv[0]);
            request->executable = (crashOwner ? executable : executable.parent_path() / "WorkphoneResourcePackage.exe").u8string().c_str();
            const auto jobs = project / ".workphone" / "package-jobs";
            const String id = crashOwner ? String("0c01438b-6391-4c51-8383-62f8f06388de") : StringUtil::getUUID();
            request->snapshotPath = (jobs / fs::u8path((id + ".json").c_str())).u8string().c_str();
            request->logPath = (jobs / fs::u8path((id + ".log").c_str())).u8string().c_str();
            Thread::setCurrentThreadId(Thread::ThreadId::WorkerThread);
            auto job = make_ptr<ResourcePackageJob>(request);
            job->execute();
            if(crashOwner) throw std::runtime_error("Crash-owner fixture must be killed while its child is running");
            require(request->status == "succeeded", request->diagnostic.c_str());
            std::cout << request->diagnostic << '\n';
        }
        else if(argc == 5 && std::wstring(argv[1]) == L"--consume")
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
