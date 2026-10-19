#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Jobs/ResourcePackageJob.hpp>
#include <Workphone/System/ResourcePackageProfile.hpp>
#include <Workphone/System/ResourcePackageRecovery.hpp>
#include <filesystem>
#include <stdexcept>
#include <fstream>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace workphone
{
    namespace
    {
#ifdef _WIN32
        struct PackageProcessHandle
        {
            HANDLE value = nullptr;
            ~PackageProcessHandle() { if(value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
        };
        HANDLE requestLock(const std::filesystem::path &snapshot)
        {
            auto path = snapshot;
            path += ".lock";
            return CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, nullptr,
                CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
        }
        bool redirected(const std::filesystem::path &path)
        {
            const auto attributes = GetFileAttributesW(path.c_str());
            return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
        }
        constexpr char requestOwnerMagic[] = "WORKPHONE_PACKAGE_REQUEST_V1\n";
        std::filesystem::path ownerPath(const std::filesystem::path &snapshot)
        {
            auto path = snapshot.parent_path() / snapshot.stem();
            path += ".owner";
            return path;
        }
        bool ownedSnapshot(const std::filesystem::path &snapshot)
        {
            const auto path = ownerPath(snapshot);
            std::error_code ec;
            if(redirected(path) || !std::filesystem::is_regular_file(std::filesystem::symlink_status(path, ec)) || ec)
                return false;
            std::ifstream marker(path, std::ios::binary);
            char bytes[sizeof(requestOwnerMagic)]{};
            marker.read(bytes, sizeof(bytes));
            return std::string(bytes, static_cast<size_t>(marker.gcount())) == requestOwnerMagic;
        }
        bool requestName(const std::string &name)
        {
            if(name.size() != 41 || name.substr(36) != ".json") return false;
            bool nonzero = false;
            for(size_t i = 0; i < 36; ++i)
            {
                if(i == 8 || i == 13 || i == 18 || i == 23)
                { if(name[i] != '-') return false; continue; }
                const auto c = name[i];
                if(!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return false;
                nonzero = nonzero || c != '0';
            }
            return nonzero;
        }
        std::wstring quote(const std::wstring &text)
        {
            std::wstring result = L"\"";
            size_t slashes = 0;
            for(auto c : text)
            {
                if(c == L'\\') { ++slashes; continue; }
                if(c == L'"') result.append(slashes * 2 + 1, L'\\');
                else result.append(slashes, L'\\');
                slashes = 0;
                result += c;
            }
            result.append(slashes * 2, L'\\');
            return result + L'"';
        }
#endif
    }
    bool recoverResourcePackageJobs(const String &projectRoot,
        ResourcePackageRecoveryResult &result, String &error)
    {
        result = {}; error.clear();
        try
        {
#ifdef _WIN32
            namespace fs = std::filesystem;
            if(projectRoot.empty()) throw std::runtime_error("Recovery requires an existing project root");
            const auto project = fs::canonical(fs::u8path(projectRoot.c_str()));
            if(!fs::is_directory(project)) throw std::runtime_error("Recovery project must be a directory");
            const auto storage = project / ".workphone";
            const auto jobs = storage / "package-jobs";
            if(redirected(storage) || redirected(jobs))
                throw std::runtime_error("Recovery refuses redirected package-job storage");
            if(!fs::exists(jobs)) return true;
            if(!fs::is_directory(jobs)) throw std::runtime_error("Package-job storage must be a directory");
            // Capture names before renaming: iterator mutation must not hide entries.
            Array<fs::path> candidates;
            size_t visited = 0;
            for(const auto &entry : fs::directory_iterator(jobs))
            {
                if(visited++ == 4096)
                { result.warnings.push_back("Recovery scan reached 4096 directory entries; additional history was not inspected"); break; }
                if(requestName(entry.path().filename().u8string())) candidates.push_back(entry.path());
            }
            for(const auto &snapshot : candidates)
            {
                if(redirected(snapshot) || !fs::is_regular_file(fs::symlink_status(snapshot)))
                { result.warnings.push_back("Recovery preserved a non-regular snapshot: " + String(snapshot.u8string().c_str())); continue; }
                PackageProcessHandle lock{requestLock(snapshot)};
                if(lock.value == INVALID_HANDLE_VALUE)
                { result.skipped.push_back(snapshot.u8string().c_str()); continue; }
                try
                {
                    if(!ownedSnapshot(snapshot))
                        throw std::runtime_error("Legacy/unmarked snapshot preserved for manual inspection");
                    const auto arguments = loadPackageProfile(snapshot.u8string().c_str());
                    if(!fs::equivalent(project, fs::u8path(arguments[1].c_str())))
                        throw std::runtime_error("Snapshot source root does not match recovery project");
                    auto archived = jobs / snapshot.stem();
                    archived += ".interrupted.json";
                    if(!MoveFileExW(snapshot.c_str(), archived.c_str(), MOVEFILE_WRITE_THROUGH))
                        throw std::runtime_error("Cannot archive snapshot exclusively; existing history is preserved");
                    result.recovered.push_back(archived.u8string().c_str());
                }
                catch(const std::exception &exception)
                { result.warnings.push_back(String(snapshot.u8string().c_str()) + ": " + exception.what()); }
            }
            return true;
#else
            throw std::runtime_error("Package-job recovery is currently supported on Windows only");
#endif
        }
        catch(const std::exception &exception) { error = exception.what(); return false; }
    }

    ResourcePackageJob::ResourcePackageJob(std::shared_ptr<ResourcePackageJobState> state) : m_request(std::move(state)) {}
    void ResourcePackageJob::stop()
    {
        m_request->cancel = true;
        {
            std::lock_guard<std::mutex> lock(m_request->mutex);
            if(m_request->status == "queued")
            {
                m_request->status = "cancelled";
                m_request->diagnostic = "Cancelled before execution";
                m_request->exitCode = 3;
                m_request->finished = true;
                setState(IJob::State::Finish);
            }
        }
        Job::stop();
    }
    void ResourcePackageJob::execute()
    {
        auto &request = *m_request;
        String recoveryNotice;
        bool ownsSnapshot = false;
        bool ownsMarker = false;
        bool retainSnapshot = false;
#ifdef _WIN32
        // Retained until after normal snapshot cleanup; completion can race a new job.
        PackageProcessHandle ownedRequest;
#endif
        auto complete = [&](const String &status, const String &diagnostic, int code) {
            std::lock_guard<std::mutex> lock(request.mutex);
            request.status = status;
            request.diagnostic = diagnostic + recoveryNotice;
            request.exitCode = code;
            request.finished = true;
            setState(IJob::State::Finish);
        };
        {
            std::lock_guard<std::mutex> lock(request.mutex);
            if(request.finished || request.status != "queued") return;
            request.status = "running";
        }
        if(request.cancel) { complete("cancelled", "Cancelled before execution", 3); return; }
        if(Thread::getCurrentThreadId() == Thread::ThreadId::Primary)
        {
            complete("failed", "Package process cannot execute on the primary thread", 1);
            return;
        }
        try
        {
#ifdef _WIN32
            namespace fs = std::filesystem;
            const auto snapshot = fs::u8path(request.snapshotPath.c_str());
            ResourcePackageRecoveryResult recovery;
            String recoveryError;
            if(!recoverResourcePackageJobs(request.projectRoot, recovery, recoveryError))
                throw std::runtime_error(recoveryError.c_str());
            if(!recovery.recovered.empty() || !recovery.skipped.empty() || !recovery.warnings.empty())
            {
                recoveryNotice = "; interrupted-request recovery: " + StringUtil::toString(recovery.recovered.size()) +
                    " archived, " + StringUtil::toString(recovery.skipped.size()) + " locked, " +
                    StringUtil::toString(recovery.warnings.size()) + " warnings. Inspect .workphone/package-jobs";
            }
            fs::create_directories(snapshot.parent_path());
            ownedRequest.value = requestLock(snapshot);
            if(ownedRequest.value == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot own package request lifetime");
            {
                PackageProcessHandle marker{CreateFileW(ownerPath(snapshot).c_str(), GENERIC_WRITE,
                    FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr)};
                if(marker.value == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create request ownership marker exclusively");
                ownsMarker = true;
                DWORD written = 0;
                if(!WriteFile(marker.value, requestOwnerMagic, sizeof(requestOwnerMagic)-1, &written, nullptr) ||
                    written != sizeof(requestOwnerMagic)-1 || !FlushFileBuffers(marker.value))
                    throw std::runtime_error("Cannot persist package request ownership marker");
            }
            savePackageProfile(request.snapshotPath, request.arguments);
            ownsSnapshot = true;
            PackageProcessHandle cancelEvent{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
            if(!cancelEvent.value) throw std::runtime_error("Cannot create package cancellation event");
            // Only these three handles may be inherited by the package child.
            if(!SetHandleInformation(cancelEvent.value, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT))
                throw std::runtime_error("Cannot share package cancellation event");
            SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
            PackageProcessHandle output{CreateFileW(fs::u8path(request.logPath.c_str()).c_str(), GENERIC_WRITE,
                                      FILE_SHARE_READ, &security, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr)};
            PackageProcessHandle input{CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     &security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
            if(output.value == INVALID_HANDLE_VALUE || input.value == INVALID_HANDLE_VALUE)
                throw std::runtime_error("Cannot open package job log");
            PackageProcessHandle group{CreateJobObjectW(nullptr, nullptr)};
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
            limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            if(!group.value || !SetInformationJobObject(group.value, JobObjectExtendedLimitInformation, &limits, sizeof(limits)))
                throw std::runtime_error("Cannot own package child process");
            SIZE_T attributeBytes = 0;
            InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeBytes);
            std::vector<unsigned char> storage(attributeBytes);
            auto attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
            if(!InitializeProcThreadAttributeList(attributes, 1, 0, &attributeBytes))
                throw std::runtime_error("Cannot initialize package process attributes");
            struct Attributes { LPPROC_THREAD_ATTRIBUTE_LIST value; ~Attributes() { DeleteProcThreadAttributeList(value); } } cleanup{attributes};
            HANDLE inherited[] = {output.value, input.value, cancelEvent.value};
            if(!UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherited, sizeof(inherited), nullptr, nullptr))
                throw std::runtime_error("Cannot restrict package inherited handles");
            const auto executable = fs::u8path(request.executable.c_str());
            auto command = quote(executable.wstring()) +
                (request.previewPath.empty() ? L" --profile " : L" --preview-profile ") + quote(snapshot.wstring());
            if(!request.previewPath.empty()) command += L" " + quote(fs::u8path(request.previewPath.c_str()).wstring());
            command +=
                L" --cancel-handle " + std::to_wstring(reinterpret_cast<uintptr_t>(cancelEvent.value));
            STARTUPINFOEXW startup{};
            startup.StartupInfo.cb = sizeof(startup);
            startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
            startup.StartupInfo.hStdOutput = output.value;
            startup.StartupInfo.hStdError = output.value;
            startup.StartupInfo.hStdInput = input.value;
            startup.lpAttributeList = attributes;
            PROCESS_INFORMATION process{};
            if(!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
                               CREATE_NO_WINDOW | CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT,
                               nullptr, executable.parent_path().c_str(), &startup.StartupInfo, &process))
                throw std::runtime_error("Cannot launch WorkphoneResourcePackage beside the Editor");
            PackageProcessHandle child{process.hProcess}, thread{process.hThread};
            if(!AssignProcessToJobObject(group.value, child.value))
            {
                TerminateProcess(child.value, 1);
                WaitForSingleObject(child.value, 5000);
                throw std::runtime_error("Cannot assign package child lifetime");
            }
            if(ResumeThread(thread.value) == static_cast<DWORD>(-1))
                throw std::runtime_error("Cannot start package child");
            const auto started = GetTickCount64();
            ULONGLONG cancellationStarted = 0;
            bool cancellationPending = false, expired = false, forced = false;
            for(;;)
            {
                const auto wait = WaitForSingleObject(child.value, 50);
                if(wait == WAIT_OBJECT_0) break;
                if(wait != WAIT_TIMEOUT)
                { retainSnapshot = true; throw std::runtime_error("Cannot wait for package child"); }
                const auto now = GetTickCount64();
                if(!cancellationPending && (request.cancel || (request.maximumRunMilliseconds &&
                    now - started >= request.maximumRunMilliseconds)))
                {
                    expired = !request.cancel;
                    cancellationPending = true;
                    cancellationStarted = now;
                    {
                        std::lock_guard<std::mutex> lock(request.mutex);
                        request.status = expired ? "timing out" : "cancelling";
                        request.diagnostic = String(expired ? "Child deadline reached" : "Cancellation requested") +
                            (request.cancelGraceMilliseconds ? "; waiting for cooperative exit before forced interruption" :
                                "; forced interruption is disabled");
                    }
                    if(!SetEvent(cancelEvent.value))
                    { retainSnapshot = true; throw std::runtime_error("Cannot signal package cancellation"); }
                }
                if(cancellationPending && request.cancelGraceMilliseconds &&
                    now - cancellationStarted >= request.cancelGraceMilliseconds)
                {
                    // Recheck before escalation: a completed publication wins.
                    if(WaitForSingleObject(child.value, 0) == WAIT_OBJECT_0) break;
                    retainSnapshot = true;
                    if(!TerminateJobObject(group.value, 4))
                        throw std::runtime_error("Cannot terminate unresponsive package child");
                    forced = true;
                    if(WaitForSingleObject(child.value, 5000) != WAIT_OBJECT_0)
                        throw std::runtime_error("Package child termination could not be confirmed");
                    break;
                }
            }
            DWORD exitCode = 1;
            if(!GetExitCodeProcess(child.value, &exitCode))
            { retainSnapshot = true; throw std::runtime_error("Cannot read package exit status"); }
            if(forced || (exitCode != 0 && exitCode != 1 && exitCode != 3))
            {
                retainSnapshot = true;
                complete("interrupted", String(forced ? (expired ? "Package child deadline and grace period expired" :
                    "Package child cancellation grace period expired") : "Package child exited unexpectedly") +
                    "; publication outcome is unknown. Captured request retained; inspect output before retrying", static_cast<int>(exitCode));
            }
            else
            {
            const String message = exitCode == 0 ? (request.previewPath.empty() ? "Published checked resource directory" :
                "Previewed checked closure; package output was not touched") :
                (exitCode == 3 ? (expired ? "Child deadline reached; cancelled before package publication" :
                    "Cancelled before package publication") : "Package failed; inspect the job log");
            complete(exitCode == 0 ? "succeeded" : (exitCode == 3 ? (expired ? "timed out" : "cancelled") : "failed"), message, static_cast<int>(exitCode));
            }
#else
            throw std::runtime_error("Asynchronous resource packaging is currently supported on Windows only");
#endif
        }
        catch(const std::exception &error) { complete(retainSnapshot ? "interrupted" : "failed",
            String(error.what()) + (retainSnapshot ? "; publication outcome is unknown; captured request retained" : ""), 1); }
        std::error_code ignored;
        if(ownsSnapshot && !retainSnapshot) std::filesystem::remove(std::filesystem::u8path(request.snapshotPath.c_str()), ignored);
#ifdef _WIN32
        // Keep ownership evidence if snapshot cleanup failed. Recovery retains it
        // beside archived history; unmarked legacy requests are never archived.
        if(ownsMarker && !retainSnapshot && !ignored) std::filesystem::remove(ownerPath(std::filesystem::u8path(request.snapshotPath.c_str())), ignored);
#endif
    }
}
