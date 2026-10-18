#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Jobs/ResourcePackageJob.hpp>
#include <Workphone/System/ResourcePackageProfile.hpp>
#include <filesystem>
#include <stdexcept>
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
        auto complete = [&](const String &status, const String &diagnostic, int code) {
            std::lock_guard<std::mutex> lock(request.mutex);
            request.status = status;
            request.diagnostic = diagnostic;
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
            fs::create_directories(snapshot.parent_path());
            savePackageProfile(request.snapshotPath, request.arguments);
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
            auto command = quote(executable.wstring()) + L" --profile " + quote(snapshot.wstring()) +
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
                WaitForSingleObject(child.value, INFINITE);
                throw std::runtime_error("Cannot assign package child lifetime");
            }
            if(ResumeThread(thread.value) == static_cast<DWORD>(-1))
                throw std::runtime_error("Cannot start package child");
            for(;;)
            {
                if(request.cancel) SetEvent(cancelEvent.value);
                const auto wait = WaitForSingleObject(child.value, 50);
                if(wait == WAIT_OBJECT_0) break;
                if(wait != WAIT_TIMEOUT) throw std::runtime_error("Cannot wait for package child");
            }
            DWORD exitCode = 1;
            if(!GetExitCodeProcess(child.value, &exitCode)) throw std::runtime_error("Cannot read package exit status");
            const String message = exitCode == 0 ? "Published checked resource directory" :
                (exitCode == 3 ? "Cancelled before package publication" : "Package failed; inspect the job log");
            complete(exitCode == 0 ? "succeeded" : (exitCode == 3 ? "cancelled" : "failed"), message, static_cast<int>(exitCode));
#else
            throw std::runtime_error("Asynchronous resource packaging is currently supported on Windows only");
#endif
        }
        catch(const std::exception &error) { complete("failed", error.what(), 1); }
        std::error_code ignored;
        std::filesystem::remove(std::filesystem::u8path(request.snapshotPath.c_str()), ignored);
    }
}
