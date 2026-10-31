#include <Workphone/Workphone.hpp>
#include <Workphone/Core/LogManagerDefault.hpp>
#include <Workphone/Database/CatalogResourceAdapter.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/ResourcePackageBuildProfile.hpp>
#include <WPGraphics/Resources/TerrainResource.hpp>
#include <WPGraphics/Resources/FoliageResource.hpp>
#if WP_PACKAGE_AUDIO
#include <WPAudio/AudioResourceCompiler.hpp>
#endif
#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#include <WPLua/LuaScriptCompiler.hpp>
#include <WPSQLite/WPSQLite.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <set>
#include <fstream>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <Workphone/System/ResourcePackageProfile.hpp>
#include <Workphone/System/ResourcePackageRecovery.hpp>
#include <Workphone/System/ResourcePackageHistory.hpp>
#ifdef _WIN32
#include <Windows.h>
#include <Psapi.h>
#endif

namespace
{
    using namespace workphone;
    using namespace workphone::resource;

    // Windows keeps these input files immutable through compilation/admission.
    // Absolute installation paths do not participate in the derived identity.
    class CookerModulePins
    {
    public:
        String identity;
#ifdef _WIN32
        struct File { HANDLE value=INVALID_HANDLE_VALUE; ~File(){if(value!=INVALID_HANDLE_VALUE)CloseHandle(value);} };
        std::vector<std::unique_ptr<File>> files;
        std::map<String,std::filesystem::path> baseline;
        static std::map<String,std::filesystem::path> inventory()
        {
            HMODULE modules[512]{}; DWORD bytes=0;
            if(!EnumProcessModules(GetCurrentProcess(),modules,sizeof(modules),&bytes)||bytes>sizeof(modules))
                throw std::runtime_error("Cannot capture cooker module inventory");
            wchar_t executable[32768]{};
            const auto executableLength=GetModuleFileNameW(nullptr,executable,32768);
            if(!executableLength||executableLength>=32768) throw std::runtime_error("Cannot locate cooker executable");
            const auto directory=std::filesystem::canonical(executable).parent_path();
            std::map<String,std::filesystem::path> result;
            for(size_t i=0;i<bytes/sizeof(HMODULE);++i)
            {
                wchar_t name[32768]{}; const auto length=GetModuleFileNameW(modules[i],name,32768);
                if(!length||length>=32768) throw std::runtime_error("Cannot locate a loaded cooker module");
                auto path=std::filesystem::canonical(std::filesystem::path(name));
                String label=StringUtil::make_lower(path.filename().u8string().c_str());
                const bool known=label=="workphone.dll"||label=="wpgraphics.dll"||label=="wplua.dll"||
                    label=="wpaudio.dll"||label=="wpsqlite.dll"||label.find("d3dcompiler_")==0||
                    label=="dxcompiler.dll"||label=="dxil.dll";
                if(known || std::filesystem::equivalent(path.parent_path(),directory))
                    if(!result.emplace(label,path).second) throw std::runtime_error("Aliased cooker module names");
            }
            return result;
        }
        void capture()
        {
            baseline=inventory(); u64 combined=hashString("workphone-cooker-modules-v1");
            for(const auto &entry:baseline)
            {
                auto file=std::make_unique<File>();
                file->value=CreateFileW(entry.second.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
                if(file->value==INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot pin a cooker module input");
                files.push_back(std::move(file));
                u64 hash=0,size=0; String error;
                if(!CompiledResourceIO::hashFile(entry.second.u8string().c_str(),hash,size,error)) throw std::runtime_error(error.c_str());
                combined=hashString(entry.first,combineHash(combined,entry.first.size()));
                combined=combineHash(combineHash(combined,size),hash);
            }
            identity=String(std::to_string(combined).c_str());
        }
        void verify() const
        {
            if(baseline!=inventory()) throw std::runtime_error("Cooker module set changed; retry with a new captured identity");
        }
#else
        void capture(){throw std::runtime_error("Automatic package toolchain capture is currently supported on Windows only");}
        void verify() const {}
#endif
    };

    int run(const Array<String> &input)
    {
        if(input.size()==5 && (input[1]=="--prune-jobs" || input[1]=="--apply-prune-jobs"))
        {
            auto number = [](const String &value, u64 maximum) -> u64 {
                if(value.empty()||value.size()>10||value.find_first_not_of("0123456789")!=String::npos)
                    throw std::runtime_error("Retention limits must be decimal integers");
                const auto parsed=std::stoull(value.c_str());
                if(parsed>maximum) throw std::runtime_error("Retention limit exceeds its supported bound");
                return parsed;
            };
            ResourcePackageHistoryResult result; String error;
            if(!pruneResourcePackageHistory(input[2],static_cast<u32>(number(input[3],4096)),
                number(input[4],1024ull*1024*1024),input[1]=="--prune-jobs",{},result,error))
                throw std::runtime_error(error.c_str());
            std::cout << "Selected " << result.selected.size() << "; removed " << result.removed.size()
                << "; skipped " << result.skipped.size() << "; warnings " << result.warnings.size()
                << "; retained bytes " << result.retainedBytes << '\n';
            for(const auto &id:result.selected) std::cout << id << '\n';
            for(const auto &warning:result.warnings) std::cerr << warning << '\n';
            return 0;
        }
        if(input.size() == 3 && input[1] == "--recover-jobs")
        {
            ResourcePackageRecoveryResult recovery;
            String error;
            if(!recoverResourcePackageJobs(input[2], recovery, error)) throw std::runtime_error(error.c_str());
            std::cout << "Recovered " << recovery.recovered.size() << "; locked " << recovery.skipped.size()
                << "; warnings " << recovery.warnings.size() << ". Publication outcomes are unknown; inspect outputs before retrying.\n";
            for(const auto &path : recovery.recovered) std::cout << path << '\n';
            for(const auto &warning : recovery.warnings) std::cerr << warning << '\n';
            return 0;
        }
        if(input.size() >= 5 && input[1] == "--verify")
        {
            ResourceSystem runtime(nullptr, nullptr);
            RuntimeResourceConfig config;
            const auto directory = std::filesystem::absolute(std::filesystem::u8path(input[2].c_str()));
            config.compiledRoot = directory.u8string().c_str();
            config.manifestPath = (directory / "runtime.wprm").u8string().c_str();
            config.target = input[3];
            String error;
            if(!runtime.initializeRuntime(config, error))
                throw std::runtime_error(error.c_str());
            for(size_t i = 4; i < input.size(); ++i)
                if(!runtime.loadUUID(input[i], error))
                    throw std::runtime_error(error.c_str());
            std::cout << "Verified pinned runtime payload closures by UUID.\n";
            return 0;
        }
        auto args = input;
        String previewPath;
        if(args.size() >= 4 && args[1] == "--preview-profile")
        {
            previewPath = args[3];
            if(previewPath.empty() || previewPath.find('\0') != String::npos)
                throw std::runtime_error("Preview requires a new report filename");
            args.erase(args.begin() + 3);
            args[1] = "--profile";
        }
        std::function<bool()> cancelled = [] { return false; };
#ifdef _WIN32
        if(args.size() == 5 && args[1] == "--profile" && args[3] == "--cancel-handle")
        {
            const auto text = std::string(args[4].c_str());
            size_t used = 0;
            const auto number = std::stoull(text, &used);
            if(used != text.size() || number == 0 || number > UINTPTR_MAX)
                throw std::runtime_error("Invalid cancellation handle");
            const auto event = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(number));
            DWORD flags = 0;
            if(!GetHandleInformation(event, &flags)) throw std::runtime_error("Cancellation handle was not inherited");
            cancelled = [event] {
                const auto wait = WaitForSingleObject(event, 0);
                if(wait == WAIT_FAILED) throw std::runtime_error("Cannot inspect cancellation event");
                return wait == WAIT_OBJECT_0;
            };
            args.erase(args.begin() + 3, args.end());
        }
#endif
        String savePath;
        if(args.size() == 3 && args[1] == "--profile")
            args = loadPackageProfile(args[2]);
        else if(args.size() >= 3 && args[1] == "--save-profile")
        {
            savePath = args[2];
            args.erase(args.begin() + 1, args.begin() + 3);
        }
        if(args.size() < 10 || (args.size() - 6) % 2 != 0)
        {
            std::cerr << "Usage: WorkphoneResourcePackage <source-root> <catalog> <cooked-root> "
                         "<new-output-directory> <target> --map <catalog-type=extension> --root <uuid> "
                         "[--map <type=extension>] [--root <uuid>] "
                         "[--build-profile <name> --toolchain <identity> [--build-settings <JSON-object>]]\n"
                         "       WorkphoneResourcePackage --profile <profile.json>\n"
                         "       WorkphoneResourcePackage --prune-jobs <project> <keep-count> <max-bytes>\n"
                         "       WorkphoneResourcePackage --apply-prune-jobs <project> <keep-count> <max-bytes>\n"
                         "       WorkphoneResourcePackage --recover-jobs <project-root>\n"
                         "       WorkphoneResourcePackage --preview-profile <profile.json> <new-report.json>\n"
                         "       WorkphoneResourcePackage --verify <package-directory> <target> <uuid> [uuid...]\n"
                         "       WorkphoneResourcePackage --save-profile <new-profile.json> <positional arguments above>\n";
            return 2;
        }
        Array<CatalogResourceAdapter::TypeMapping> mappings;
        Array<String> uuids;
        std::set<String> mappedTypes, rootIdentities;
        for(size_t i = 6; i < args.size(); i += 2)
        {
            if(args[i] == "--root")
            {
                auto identity = StringUtil::make_lower(args[i + 1]);
                if(!rootIdentities.insert(identity).second)
                    throw std::runtime_error("Duplicate package root UUID");
                uuids.push_back(args[i + 1]);
            }
            else if(args[i] == "--map")
            {
                const auto split = args[i + 1].find('=');
                if(split == String::npos || split == 0 || split + 1 == args[i + 1].size())
                    throw std::runtime_error("Expected --map catalog-type=extension");
                const auto type = args[i + 1].substr(0, split);
                const ResourceTypeID extension(args[i + 1].substr(split + 1));
                if(!extension.isValid() || !mappedTypes.insert(type).second)
                    throw std::runtime_error("Invalid or duplicate package type mapping");
                mappings.push_back({type, extension});
            }
            else if(args[i]=="--build-profile" || args[i]=="--toolchain" || args[i]=="--build-settings") {}
            else
                throw std::runtime_error("Unknown package option");
        }
        const auto buildProfile=packageBuildProfile(args);
        if(mappings.empty() || uuids.empty() || args[5].empty())
            throw std::runtime_error("At least one mapping, root UUID and explicit target are required");
        if(!savePath.empty())
        {
            savePackageProfile(savePath, args);
            std::cout << "Saved package profile; no assets compiled or exported.\n";
            return 0;
        }
        if(cancelled()) return 3;
        // Never create an authoring database because a CLI path was misspelled.
        if(!std::filesystem::is_regular_file(std::filesystem::u8path(args[2].c_str())))
            throw std::runtime_error("Catalog must be an existing regular file");
        auto catalog = make_ptr<AssetDatabaseManager>();
        if(!catalog->setProjectRoot(args[1]))
            throw std::runtime_error("Invalid project source root");
        catalog->loadFromFile(args[2]);
        auto registry = std::make_shared<ResourceCompilerRegistry>();
        String error;
        if(!registry->registerCompiler(std::make_shared<render::GraphicsResourceCompiler>(), &error) ||
           !registry->registerCompiler(std::make_shared<LuaScriptCompiler>(), &error) ||
           !registry->registerCompiler(std::make_shared<render::TerrainResourceCompiler>(), &error) ||
           !registry->registerCompiler(std::make_shared<render::FoliageResourceCompiler>(), &error))
            throw std::runtime_error(error.c_str());
        auto resources = std::make_shared<ResourceSystem>(registry, std::make_shared<ResourceCompilationDatabase>());
#if WP_PACKAGE_AUDIO
        if(!registry->registerCompiler(std::make_shared<audio::AudioResourceCompiler>(),&error))
            throw std::runtime_error(error.c_str());
#endif
        ResourceSystemConfig config;
        config.sourceRoot = args[1];
        config.compiledRoot = args[3];
        config.target = args[5];
        config.buildProfile = buildProfile;
        CookerModulePins modulePins;
        if(!buildProfile.key().empty()) {
            modulePins.capture();
            config.buildProfile.toolchain=String(std::to_string(buildProfile.toolchain.size()).c_str())+":"+
                buildProfile.toolchain+":modules-v1:"+modulePins.identity;
        }
        if(!resources->initialize(config, error))
            throw std::runtime_error(error.c_str());
        CatalogResourceAdapter adapter(catalog, resources, config.sourceRoot, mappings);
        Array<CatalogResourceAdapter::Request> roots;
        // Resolve every root before cooking so malformed identities fail early.
        for(const auto &uuid : uuids)
        {
            CatalogResourceAdapter::Request request;
            if(!adapter.resolve(uuid, request, error))
                throw std::runtime_error(error.c_str());
            roots.push_back(request);
        }
        CompilationOptions options;
        options.packagedBuild = true;
        for(const auto &request : roots)
        {
            if(cancelled()) return 3;
            const auto report = adapter.compile(request, options);
            for(const auto &message : report.messages)
                std::cerr << message << '\n';
            if(!report.succeeded())
                throw std::runtime_error("Package compilation failed; no output directory published");
        }
        if(cancelled()) return 3;
        if(!buildProfile.key().empty()) modulePins.verify();
        if(!previewPath.empty())
        {
            Array<RuntimePackagePreviewEntry> entries;
            if(!adapter.previewRuntimePackage(roots, entries, error))
                throw std::runtime_error(error.c_str());
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            auto string = [&](const String &value) { writer.String(value.c_str(), static_cast<rapidjson::SizeType>(value.size())); };
            writer.StartObject();
            writer.Key("version"); writer.Uint(1);
            writer.Key("target"); string(config.target);
            writer.Key("entries"); writer.StartArray();
            for(const auto &entry : entries)
            {
                writer.StartObject();
                writer.Key("uuid"); string(entry.uuid);
                writer.Key("resourceId"); string(entry.resourceId.str());
                writer.Key("root"); writer.Bool(entry.root);
                writer.Key("payloadBytes"); writer.Uint64(entry.payloadBytes);
                writer.Key("installDependencies"); writer.StartArray();
                for(const auto &child : entry.installDependencies) string(child.str());
                writer.EndArray(); writer.EndObject();
            }
            writer.EndArray(); writer.EndObject();
            if(buffer.GetSize() > 8 * 1024 * 1024) throw std::runtime_error("Closure preview exceeds 8 MiB report limit");
            if(cancelled()) return 3;
            namespace fs = std::filesystem;
            const auto report = fs::absolute(fs::u8path(previewPath.c_str()));
            const auto temporary = report.parent_path() / fs::u8path((".wp-preview-" + StringUtil::getUUID()).c_str());
            struct Cleanup { fs::path path; ~Cleanup() { std::error_code ec; fs::remove(path, ec); } } cleanup{temporary};
            {
                std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
                stream.write(buffer.GetString(), buffer.GetSize());
                stream.close();
                if(!stream) throw std::runtime_error("Cannot write closure preview report");
            }
            std::error_code ec;
            fs::create_hard_link(temporary, report, ec);
            if(ec) throw std::runtime_error("Cannot publish closure preview report exclusively");
            std::cout << "Previewed admitted install closure; package output was not touched.\n";
            return 0;
        }
        if(!adapter.writeRuntimePackage(args[4], roots, error))
            throw std::runtime_error(error.c_str());
        std::cout << "Published catalog-admitted runtime resource directory.\n";
        return 0;
    }

    int execute(const Array<String> &args)
    {
        TypeManager types;
        types.load();
        TypeManager::setInstance(&types);
        auto app = make_ptr<core::ApplicationManager>();
        core::IApplicationManager::setInstance(app);
        app->setLogManager(make_ptr<LogManagerDefault>());
        auto factories = make_ptr<FactoryManager>();
        factories->load(nullptr);
        app->setFactoryManager(factories);
        app->setFileSystem(make_ptr<FileSystem>());
        auto plugin = make_ptr<SQLitePlugin>();
        plugin->load(nullptr);
        int result = 1;
        try { result = run(args); }
        catch(const std::exception &e) { std::cerr << "Package failed: " << e.what() << '\n'; }
        plugin->unload(nullptr);
        plugin = nullptr;
        app->setFileSystem(nullptr);
        app->setFactoryManager(nullptr);
        factories->unload(nullptr);
        factories = nullptr;
        core::IApplicationManager::setInstance(nullptr);
        app = nullptr;
        TypeManager::setInstance(nullptr);
        types.unload();
        return result;
    }
}

#ifdef _WIN32
int wmain(int argc, wchar_t **argv)
{
    workphone::Array<workphone::String> args;
    for(int i = 0; i < argc; ++i)
        args.push_back(workphone::StringUtil::toUTF16to8(argv[i]));
    return execute(args);
}
#else
int main(int argc, char **argv)
{
    workphone::Array<workphone::String> args;
    for(int i = 0; i < argc; ++i) args.push_back(argv[i]);
    return execute(args);
}
#endif
