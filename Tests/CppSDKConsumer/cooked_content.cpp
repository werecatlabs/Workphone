#include <Workphone/System/ResourceSystem.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <windows.h>
#include <tlhelp32.h>

namespace
{
    using namespace workphone;
    using namespace workphone::resource;
    constexpr const char *materialUUID = "2a6d7957-45ab-4bbb-9d96-91f7a9c1a001";
    constexpr const char *textureUUID = "2a6d7957-45ab-4bbb-9d96-91f7a9c1a002";
    constexpr const char *expectedTarget = "installed-cpp-sample";

    void require(bool value, const char *message)
    {
        if(!value)
            throw std::runtime_error(message);
    }

    void modules()
    {
        const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
        require(snapshot != INVALID_HANDLE_VALUE, "Module snapshot failed");
        MODULEENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        if(!Module32FirstW(snapshot, &entry))
        {
            CloseHandle(snapshot);
            throw std::runtime_error("Module enumeration failed");
        }
        do
        {
            char path[4096]{};
            if(!WideCharToMultiByte(CP_UTF8, 0, entry.szExePath, -1, path, sizeof(path), nullptr, nullptr))
            {
                CloseHandle(snapshot);
                throw std::runtime_error("Module path conversion failed");
            }
            std::cout << "module=" << path << '\n';
        } while(Module32NextW(snapshot, &entry));
        CloseHandle(snapshot);
    }
}

int wmain(int argc, wchar_t **argv)
{
    if((argc != 2 && argc != 4) || (argc == 4 && std::wstring(argv[2]) != L"--target"))
        return 2;
    try
    {
        const auto root = std::filesystem::path(argv[1]);
        ResourceSystem runtime(nullptr, nullptr);
        RuntimeResourceConfig config;
        config.compiledRoot = root.u8string().c_str();
        config.manifestPath = (root / "runtime.wprm").u8string().c_str();
        config.target = argc == 4 ? std::filesystem::path(argv[3]).u8string().c_str() : expectedTarget;
        config.maxPayloadBytes = 1u << 20;
        config.maxClosurePayloadBytes = 2u << 20;
        config.maxClosureResources = 2;
        String error;
        if(!runtime.initializeRuntime(config, error))
            throw std::runtime_error(error.c_str());
        require(!runtime.registry(), "Runtime mount acquired an authoring compiler registry");
        auto material = runtime.loadUUID(materialUUID, error);
        if(!material)
            throw std::runtime_error(error.c_str());
        auto texture = runtime.loadUUID(textureUUID, error);
        if(!texture)
            throw std::runtime_error(error.c_str());
        require(material->header.resourceId == ResourceID("data://surface.matres") &&
                    texture->header.resourceId == ResourceID("data://albedo.texres"),
                "Catalog UUID bindings changed");
        require(material->dependencies.size() == 1 && material->dependencies[0] == texture &&
                    runtime.load(texture->header.resourceId, error) == texture,
                "Pinned dependency cache identity changed");
        render::CookedMaterialData surface;
        if(!render::decodeCookedMaterial(*material, surface, error))
            throw std::runtime_error(error.c_str());
        require(surface.target == expectedTarget && surface.packagedBuild &&
                    surface.baseColour.r == .75f && surface.baseColour.g == .5f &&
                    surface.baseColour.b == .25f && surface.baseColour.a == 1.f &&
                    surface.metalness == .125f && surface.roughness == .625f &&
                    surface.textureId == texture->header.resourceId,
                "Typed material values changed");
        render::CookedTextureData image;
        if(!render::decodeCookedTexture(*texture, image, error))
            throw std::runtime_error(error.c_str());
        require(image.target == expectedTarget && image.packagedBuild && image.levels.size() == 3,
                "Cooked texture target/mips changed");
        for(size_t level = 0; level < image.levels.size(); ++level)
        {
            const auto &mip = image.levels[level];
            const auto side = 4u >> level;
            require(mip.width == side && mip.height == side && mip.bgra.size() == side * side * 4,
                    "Cooked mip dimensions changed");
            for(size_t pixel = 0; pixel < mip.bgra.size(); pixel += 4)
                require(mip.bgra[pixel] == 32 && mip.bgra[pixel + 1] == 96 &&
                            mip.bgra[pixel + 2] == 224 && mip.bgra[pixel + 3] == 255,
                        "Cooked mip pixel values changed");
        }
        ResourceID unknown("data://must-be-cleared.texres");
        require(!runtime.resolveRuntimeUUID("2a6d7957-45ab-4bbb-9d96-91f7a9c1afff", unknown, error) &&
                    !unknown.isValid() && !error.empty(), "Unknown UUID accepted or retained stale output");
        runtime.shutdown();
        require(!runtime.isInitialized() && material->dependencies[0] == texture &&
                    render::decodeCookedTexture(*texture, image, error),
                "Caller-owned cooked resources did not survive shutdown");
        modules();
        std::cout << "Installed cooked content: catalog UUID material/texture closure, exact mips and shutdown passed\n";
        return 0;
    }
    catch(const std::exception &error)
    {
        std::cerr << "Cooked content rejected: " << error.what() << '\n';
        return 10;
    }
}
