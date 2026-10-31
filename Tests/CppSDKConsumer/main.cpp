#include <Workphone/Core/Properties.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <WPGraphics/ClawCamera.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <algorithm>
#include <array>
#include <iostream>
#include <new>
#include <windows.h>
#include <tlhelp32.h>
#include "material_contracts.hpp"

namespace
{
    bool nativeUILayout()
    {
        // The exported constructor/destructor operate on caller-owned storage.
        // This exercises the real DLL's embedded native font-atlas layout, whose
        // public WORKPHONE_INCLUDE_* definitions must match the SDK interface.
        using Context = workphone::ui::ClawUIWorkphoneContext;
        constexpr size_t padding = alignof(Context) * 8;
        alignas(Context) std::array<unsigned char, sizeof(Context) + padding * 2> storage;
        storage.fill(0xa5);
        auto *context = new (storage.data() + padding) Context();
        const auto colour = Context::toWorkphoneColor(workphone::ColourF(1,0,0,1));
        const auto point = Context::toWorkphoneVec2(workphone::Vector2F(3,7));
        const bool valid = context->isValid() && context->getContext() && context->getMemory() &&
            context->getFontPixels() && context->getFontWidth() > 0 && context->getFontHeight() > 0 &&
            colour.r == 255 && colour.g == 0 && colour.b == 0 && colour.a == 255 &&
            point.x == 3 && point.y == 7;
        context->~Context();
        const auto guard = [](unsigned char value) { return value == 0xa5; };
        return valid && std::all_of(storage.begin(), storage.begin() + padding, guard) &&
            std::all_of(storage.end() - padding, storage.end(), guard);
    }

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
    if(!nativeUILayout())
    {
        std::cerr << "Installed native UI layout or font initialization failed\n";
        return 7;
    }
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
                 "immutable publication, backend record generations, Claw camera and native UI layout passed\n";
    return 0;
}
