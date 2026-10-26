#include <Workphone/Core/Properties.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <WPGraphics/ClawCamera.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include "material_contracts.hpp"

namespace
{
    // Use the engine's normal registry lifecycle before constructing typed objects.
    struct RuntimeTypes
    {
        workphone::TypeManager manager;
        RuntimeTypes()
        {
            manager.load();
            workphone::TypeManager::setInstance(&manager);
        }
        ~RuntimeTypes()
        {
            manager.unload();
            workphone::TypeManager::setInstance(nullptr);
        }
    };
}

int main()
{
    RuntimeTypes runtime;
    workphone::Properties properties;
    properties.setProperty("name", workphone::String("installed C++ SDK"));
    properties.setProperty("count", workphone::s32(42));
    if(properties.getProperty("name") != "installed C++ SDK" ||
       properties.getPropertyAsInt("count") != 42)
        return 1;
    workphone::render::Material material;
    if(material.getBlendMode() != 0 || material.getCullMode() != 2)
        return 2;
    workphone::render::ClawCamera camera;
    camera.setNearClipDistance(0.25f);
    if(camera.getNearClipDistance() != 0.25f)
        return 3;
    try
    {
        installed_material_contracts::run();
    }
    catch(const std::exception &error)
    {
        std::cerr << "Installed material contract failed: " << error.what() << '\n';
        return 7;
    }
    const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if(snapshot == INVALID_HANDLE_VALUE)
        return 4;
    MODULEENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if(!Module32FirstW(snapshot, &entry))
    {
        CloseHandle(snapshot);
        return 5;
    }
    do
    {
        char path[4096]{};
        if(!WideCharToMultiByte(CP_UTF8, 0, entry.szExePath, -1, path, sizeof(path), nullptr, nullptr))
        {
            CloseHandle(snapshot);
            return 6;
        }
        std::cout << "module=" << path << '\n';
    } while(Module32NextW(snapshot, &entry));
    CloseHandle(snapshot);
    std::cout << "Installed C++ SDK: properties, material layouts/typed handles/schema migration, "
                 "immutable publication, backend record generations and Claw camera passed\n";
    return 0;
}
