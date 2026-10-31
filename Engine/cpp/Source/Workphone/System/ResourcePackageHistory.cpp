#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/ResourcePackageHistory.hpp>
#include <Workphone/System/ResourcePackageProfile.hpp>
#include <Workphone/System/ResourceID.hpp>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <filesystem>
#include <chrono>
#include <fstream>
#include <set>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace workphone
{
namespace
{
    namespace fs = std::filesystem;
    void check(bool value, const char *message) { if(!value) throw std::runtime_error(message); }
    bool validHistoryUuid(const std::string &value)
    {
        if(value.size() != 36) return false;
        bool nonzero = false;
        for(size_t i=0; i<36; ++i)
        {
            if(i==8 || i==13 || i==18 || i==23) { if(value[i]!='-') return false; }
            else { const auto c=value[i]; if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F'))) return false; nonzero |= c!='0'; }
        }
        return nonzero;
    }
#ifdef _WIN32
    struct File
    {
        HANDLE value = INVALID_HANDLE_VALUE;
        ~File() { if(value != INVALID_HANDLE_VALUE) CloseHandle(value); }
        File(const fs::path &path, DWORD access, DWORD creation=OPEN_EXISTING, DWORD flags=FILE_ATTRIBUTE_NORMAL)
        {
            value=CreateFileW(path.c_str(), access, FILE_SHARE_READ, nullptr, creation, flags|FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
            if(value!=INVALID_HANDLE_VALUE) {
                FILE_ATTRIBUTE_TAG_INFO info{};
                if(!GetFileInformationByHandleEx(value,FileAttributeTagInfo,&info,sizeof(info)) ||
                    (info.FileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)) {
                    CloseHandle(value); value=INVALID_HANDLE_VALUE;
                }
            }
        }
        File(const File&)=delete;
        File &operator=(const File&)=delete;
    };
    bool redirected(const fs::path &path)
    { const auto a=GetFileAttributesW(path.c_str()); return a!=INVALID_FILE_ATTRIBUTES && (a&FILE_ATTRIBUTE_REPARSE_POINT); }
    void regular(const fs::path &path)
    { check(!redirected(path) && fs::is_regular_file(fs::symlink_status(path)), "History member is not an ordinary file"); }
    bool present(const fs::path &path)
    {
        std::error_code ec; const auto status=fs::symlink_status(path,ec);
        check(!ec||ec==std::errc::no_such_file_or_directory,"Cannot inspect history evidence");
        return fs::exists(status);
    }
    fs::path folder(const String &root, fs::path &project)
    {
        check(!root.empty() && root.find('\0')==String::npos, "Invalid history project root");
        project=fs::canonical(fs::u8path(root.c_str()));
        check(fs::is_directory(project), "History project must be a directory");
        const auto storage=project/".workphone", jobs=storage/"package-jobs";
        check(!redirected(storage) && !redirected(jobs), "History refuses redirected job storage");
        return jobs;
    }
    u64 hash(File &file, u64 &size)
    {
        check(file.value!=INVALID_HANDLE_VALUE, "History member is open, unavailable or not deletable");
        LARGE_INTEGER length{};
        check(GetFileSizeEx(file.value,&length) && length.QuadPart>=0 && length.QuadPart<=512ll*1024*1024,
            "History member exceeds the 512 MiB inspection limit");
        size=static_cast<u64>(length.QuadPart);
        LARGE_INTEGER zero{}; check(SetFilePointerEx(file.value,zero,nullptr,FILE_BEGIN), "Cannot seek history member");
        u64 result=14695981039346656037ull, total=0;
        char bytes[65536]; DWORD count=0;
        do {
            check(ReadFile(file.value,bytes,sizeof(bytes),&count,nullptr)!=FALSE, "Cannot read history member");
            total+=count; check(total<=size, "History member changed during inspection");
            result=resource::hashBytes(bytes,count,result);
        } while(count);
        check(total==size,"History member changed during inspection"); return result;
    }
    std::string text(File &file, size_t maximum)
    {
        check(file.value!=INVALID_HANDLE_VALUE,"History metadata is open or unavailable");
        LARGE_INTEGER size{}, zero{};
        check(GetFileSizeEx(file.value,&size) && size.QuadPart>0 && size.QuadPart<=static_cast<LONGLONG>(maximum),"Invalid history metadata size");
        check(SetFilePointerEx(file.value,zero,nullptr,FILE_BEGIN),"Cannot seek history metadata");
        std::string result(static_cast<size_t>(size.QuadPart),'\0'); DWORD read=0;
        check(ReadFile(file.value,result.data(),static_cast<DWORD>(result.size()),&read,nullptr) && read==result.size(),"Cannot read history metadata");
        check(result.find('\0')==std::string::npos,"Invalid history metadata bytes"); return result;
    }
    void publish(const fs::path &path,const std::string &bytes)
    {
        // The destination is never replaced, including after interruption.
        File file(path,GENERIC_WRITE,CREATE_NEW);
        check(file.value!=INVALID_HANDLE_VALUE,"History metadata already exists or cannot be created");
        DWORD written=0;
        check(WriteFile(file.value,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr) &&
            written==bytes.size() && FlushFileBuffers(file.value),"Cannot persist history metadata");
    }
    struct Member { std::string name; u64 size=0, fingerprint=0; };
    struct Receipt { std::string id; fs::path path; u64 completed=0, bytes=0, fingerprint=0; Array<Member> files; };
    String string(const rapidjson::Value &object,const char *name)
    {
        check(object.IsObject()&&object.HasMember(name)&&object[name].IsString()&&
            object[name].GetStringLength()>0&&object[name].GetStringLength()<=4096,"Invalid history string");
        String value(object[name].GetString(),object[name].GetStringLength());
        check(value.find('\0')==String::npos,"Invalid history string"); return value;
    }
    Receipt readReceipt(const fs::path &path,File &file,const fs::path &project)
    {
        const auto bytes=text(file,16384); rapidjson::Document doc;
        doc.Parse<rapidjson::kParseValidateEncodingFlag|rapidjson::kParseIterativeFlag>(bytes.data(),bytes.size());
        check(!doc.HasParseError()&&doc.IsObject()&&doc.MemberCount()==7&&doc.HasMember("version")&&
            doc["version"].IsUint()&&doc["version"].GetUint()==1,"Invalid completion receipt");
        check(string(doc,"kind")=="workphone-package-job","Unknown completion receipt producer");
        Receipt result; result.id=string(doc,"id").c_str(); result.path=path;
        result.fingerprint=resource::hashBytes(bytes.data(),bytes.size());
        check(validHistoryUuid(result.id)&&path.filename()==fs::u8path(result.id+".completed.json"),"Invalid completion identity");
        check(fs::equivalent(project,fs::u8path(string(doc,"project").c_str())),"History receipt belongs to another project");
        const auto status=string(doc,"status");
        check(status=="succeeded"||status=="failed"||status=="cancelled"||status=="timed out","Uncertain outcomes are not prunable");
        check(doc.HasMember("completedMs")&&doc["completedMs"].IsUint64()&&doc.HasMember("files")&&
            doc["files"].IsArray()&&doc["files"].Size()>=1&&doc["files"].Size()<=2,"Invalid completion members");
        result.completed=doc["completedMs"].GetUint64(); result.bytes=bytes.size(); std::set<std::string> names;
        for(const auto &entry:doc["files"].GetArray())
        {
            check(entry.IsObject()&&entry.MemberCount()==3&&entry.HasMember("size")&&entry["size"].IsUint64()&&
                entry.HasMember("hash")&&entry["hash"].IsUint64(),"Invalid history member");
            Member member; member.name=string(entry,"name").c_str();
            check((member.name==result.id+".log"||member.name==result.id+".preview.json")&&names.insert(member.name).second,
                "History member path is not owned by this request");
            member.size=entry["size"].GetUint64(); member.fingerprint=entry["hash"].GetUint64();
            check(member.size<=512ull*1024*1024,"History member is too large"); result.bytes+=member.size;
            result.files.push_back(member);
        }
        check(names.count(result.id+".log")!=0,"Completion receipt requires its owned log"); return result;
    }
#endif
}

bool recordResourcePackageCompletion(const String &root,const String &snapshotPath,const String &logPath,
    const String &previewPath,const String &status,String &error)
{
    error.clear();
    try {
#ifdef _WIN32
        fs::path project; const auto jobs=folder(root,project), snapshot=fs::u8path(snapshotPath.c_str());
        const auto id=snapshot.stem().u8string();
        check(validHistoryUuid(id)&&snapshot.parent_path()==jobs,"Completion snapshot is outside owned job storage");
        check(fs::equivalent(project,fs::u8path(loadPackageProfile(snapshotPath)[1].c_str())),"Completion source root mismatch");
        check(status=="succeeded"||status=="failed"||status=="cancelled"||status=="timed out","Cannot complete an uncertain request");
        Array<fs::path> files{fs::u8path(logPath.c_str())};
        if(!previewPath.empty()&&status=="succeeded") files.push_back(fs::u8path(previewPath.c_str()));
        rapidjson::StringBuffer buffer; rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        writer.StartObject(); writer.Key("version"); writer.Uint(1); writer.Key("id"); writer.String(id.c_str());
        writer.Key("project"); writer.String(project.u8string().c_str()); writer.Key("status"); writer.String(status.c_str());
        writer.Key("completedMs"); writer.Uint64(static_cast<u64>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count())); writer.Key("files"); writer.StartArray();
        for(const auto &path:files) {
            const auto name=path.filename().u8string();
            check(path.parent_path()==jobs&&(name==id+".log"||name==id+".preview.json"),"Completion member is outside owned storage");
            regular(path); File file(path,GENERIC_READ); u64 size=0; const auto fingerprint=hash(file,size);
            writer.StartObject(); writer.Key("name"); writer.String(name.c_str()); writer.Key("size"); writer.Uint64(size);
            writer.Key("hash"); writer.Uint64(fingerprint); writer.EndObject();
        }
        writer.EndArray(); writer.Key("kind"); writer.String("workphone-package-job"); writer.EndObject();
        publish(jobs/fs::u8path(id+".completed.json"),std::string(buffer.GetString(),buffer.GetSize()));
        return true;
#else
        throw std::runtime_error("Package history is currently supported on Windows only");
#endif
    } catch(const std::exception &exception) { error=exception.what(); return false; }
}

bool pruneResourcePackageHistory(const String &root,u32 keepCount,u64 maximumBytes,bool dryRun,
    const String &protectedId,ResourcePackageHistoryResult &result,String &error)
{
    result={}; error.clear();
    try {
#ifdef _WIN32
        check(keepCount<=4096&&maximumBytes<=1024ull*1024*1024,"Invalid history retention limits");
        check(protectedId.empty()||validHistoryUuid(protectedId.c_str()),"Invalid protected request identity");
        fs::path project; const auto jobs=folder(root,project); if(!fs::exists(jobs)) return true;
        Array<Receipt> receipts; size_t visited=0;
        for(const auto &entry:fs::directory_iterator(jobs)) {
            if(visited++==4096) { result.warnings.push_back("History scan reached 4096 entries; quota accounting is incomplete"); break; }
            const auto name=entry.path().filename().u8string();
            if(name.size()!=51||name.substr(36)!=".completed.json") continue;
            try { regular(entry.path()); File file(entry.path(),GENERIC_READ);
                auto receipt=readReceipt(entry.path(),file,project); receipts.push_back(std::move(receipt)); }
            catch(const std::exception &exception) { result.warnings.push_back(String(name.c_str())+": "+exception.what()); }
        }
        std::sort(receipts.begin(),receipts.end(),[](const Receipt &a,const Receipt &b) {
            return a.completed!=b.completed ? a.completed>b.completed : a.id>b.id;
        });
        size_t kept=0;
        u64 protectedBytes=0;
        for(const auto &receipt:receipts) if(receipt.id==protectedId.c_str()) protectedBytes=receipt.bytes;
        for(const auto &receipt:receipts) {
            const bool protect=receipt.id==protectedId.c_str();
            if(protect) protectedBytes=0;
            if(protect||(kept+(protectedBytes?1:0)<keepCount && result.retainedBytes+receipt.bytes+protectedBytes<=maximumBytes)) {
                ++kept; result.retainedBytes+=receipt.bytes; continue;
            }
            try {
                const auto base=jobs/fs::u8path(receipt.id+".json"); auto lockPath=base; lockPath+=".lock";
                File lock(lockPath,GENERIC_READ|GENERIC_WRITE|DELETE,CREATE_NEW,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE);
                if(lock.value==INVALID_HANDLE_VALUE) { result.skipped.push_back(receipt.id.c_str()); result.retainedBytes+=receipt.bytes; continue; }
                check(!present(base)&&!present(jobs/fs::u8path(receipt.id+".interrupted.json")),"Uncertain/live request evidence is preserved");
                regular(receipt.path); File metadata(receipt.path,GENERIC_READ|DELETE);
                const auto current=readReceipt(receipt.path,metadata,project);
                check(current.fingerprint==receipt.fingerprint,"Completion receipt changed; preserved");
                u64 metadataSize=0; const auto receiptHash=hash(metadata,metadataSize);
                auto intentPath=jobs/fs::u8path(receipt.id+".pruning");
                const auto intent="WORKPHONE_PACKAGE_PRUNE_V1\n"+std::to_string(receiptHash)+"\n";
                bool resuming=false;
                if(present(intentPath)) { regular(intentPath); File existing(intentPath,GENERIC_READ);
                    check(text(existing,128)==intent,"Unknown prune intent is preserved"); resuming=true; }
                Array<std::unique_ptr<File>> members;
                for(const auto &member:current.files) {
                    const auto path=jobs/fs::u8path(member.name);
                    if(resuming&&!present(path)) continue;
                    regular(path); auto file=std::make_unique<File>(path,GENERIC_READ|DELETE); u64 size=0;
                    check(hash(*file,size)==member.fingerprint&&size==member.size,"History member was modified; preserved");
                    members.push_back(std::move(file));
                }
                result.selected.push_back(receipt.id.c_str());
                if(dryRun) { result.retainedBytes+=receipt.bytes; continue; }
                if(!resuming) publish(intentPath,intent);
                FILE_DISPOSITION_INFO disposition{TRUE};
                for(auto &file:members) check(SetFileInformationByHandle(file->value,FileDispositionInfo,&disposition,sizeof(disposition))!=FALSE,"Cannot prune history member; intent retained");
                members.clear();
                check(SetFileInformationByHandle(metadata.value,FileDispositionInfo,&disposition,sizeof(disposition))!=FALSE,"Cannot remove completion receipt; intent retained");
                result.removed.push_back(receipt.id.c_str());
                // Only this exact, validated intent is owned by this operation.
                File intentFile(intentPath,GENERIC_READ|DELETE);
                check(text(intentFile,128)==intent,"Prune intent changed; preserved");
                check(SetFileInformationByHandle(intentFile.value,FileDispositionInfo,&disposition,sizeof(disposition))!=FALSE,"Cannot remove completed prune intent");
            } catch(const std::exception &exception) { result.retainedBytes+=receipt.bytes;
                result.warnings.push_back(String(receipt.id.c_str())+": "+exception.what()); }
        }
        if(result.retainedBytes>maximumBytes) result.warnings.push_back("Protected or unprunable history exceeds the managed byte budget");
        if(!dryRun && receipts.size()-result.removed.size()>keepCount)
            result.warnings.push_back("Protected or unprunable history exceeds the managed count budget");
        return true;
#else
        throw std::runtime_error("Package history is currently supported on Windows only");
#endif
    } catch(const std::exception &exception) { error=exception.what(); return false; }
}
}
