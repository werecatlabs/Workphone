#include <Workphone/Workphone.hpp>
#include <Workphone/Core/LogManagerDefault.hpp>
#include <Workphone/Database/CatalogResourceAdapter.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#include <WPLua/LuaScriptCompiler.hpp>
#include <WPSQLite/WPSQLite.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace
{
    using namespace workphone;
    using namespace workphone::resource;

    int run(const Array<String> &args)
    {
        if(args.size() < 10 || (args.size() - 6) % 2 != 0)
        {
            std::cerr << "Usage: WorkphoneResourcePackage <source-root> <catalog> <cooked-root> "
                         "<new-output-directory> <target> --map <catalog-type=extension> --root <uuid> "
                         "[--map <type=extension>] [--root <uuid>]\n";
            return 2;
        }
        Array<CatalogResourceAdapter::TypeMapping> mappings;
        Array<String> uuids;
        for(size_t i = 6; i < args.size(); i += 2)
        {
            if(args[i] == "--root")
                uuids.push_back(args[i + 1]);
            else if(args[i] == "--map")
            {
                const auto split = args[i + 1].find('=');
                if(split == String::npos || split == 0 || split + 1 == args[i + 1].size())
                    throw std::runtime_error("Expected --map catalog-type=extension");
                mappings.push_back({args[i + 1].substr(0, split), ResourceTypeID(args[i + 1].substr(split + 1))});
            }
            else
                throw std::runtime_error("Unknown package option");
        }
        if(mappings.empty() || uuids.empty() || args[5].empty())
            throw std::runtime_error("At least one mapping, root UUID and explicit target are required");
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
           !registry->registerCompiler(std::make_shared<LuaScriptCompiler>(), &error))
            throw std::runtime_error(error.c_str());
        auto resources = std::make_shared<ResourceSystem>(registry, std::make_shared<ResourceCompilationDatabase>());
        ResourceSystemConfig config;
        config.sourceRoot = args[1];
        config.compiledRoot = args[3];
        config.target = args[5];
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
            const auto report = adapter.compile(request, options);
            for(const auto &message : report.messages)
                std::cerr << message << '\n';
            if(!report.succeeded())
                throw std::runtime_error("Package compilation failed; no output directory published");
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
