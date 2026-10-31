#include <Workphone/System/ResourceSystem.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <WPGraphics/Resources/TerrainResource.hpp>
#include <WPGraphics/Resources/FoliageResource.hpp>
#include <algorithm>
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
    constexpr const char *terrainUUID = "2a6d7957-45ab-4bbb-9d96-91f7a9c1a003";
    constexpr const char *foliageUUID = "2a6d7957-45ab-4bbb-9d96-91f7a9c1a004";
    constexpr const char *speciesUUID = "2a6d7957-45ab-4bbb-9d96-91f7a9c1a005";
    constexpr const char *pageUUID = "2a6d7957-45ab-4bbb-9d96-91f7a9c1a006";
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
        auto runtime = std::make_shared<ResourceSystem>(nullptr, nullptr);
        RuntimeResourceConfig config;
        config.compiledRoot = root.u8string().c_str();
        config.manifestPath = (root / "runtime.wprm").u8string().c_str();
        config.target = argc == 4 ? std::filesystem::path(argv[3]).u8string().c_str() : expectedTarget;
        config.maxPayloadBytes = 1u << 20;
        config.maxClosurePayloadBytes = 2u << 20;
        config.maxClosureResources = 6;
        String error;
        if(!runtime->initializeRuntime(config, error))
            throw std::runtime_error(error.c_str());
        require(!runtime->registry(), "Runtime mount acquired an authoring compiler registry");
        auto material = runtime->loadUUID(materialUUID, error);
        if(!material)
            throw std::runtime_error(error.c_str());
        auto texture = runtime->loadUUID(textureUUID, error);
        if(!texture)
            throw std::runtime_error(error.c_str());
        require(material->header.resourceId == ResourceID("data://surface.matres") &&
                    texture->header.resourceId == ResourceID("data://albedo.texres"),
                "Catalog UUID bindings changed");
        require(material->dependencies.size() == 1 && material->dependencies[0] == texture &&
                    runtime->load(texture->header.resourceId, error) == texture,
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
        auto terrain = runtime->loadUUID(terrainUUID, error);
        require(terrain && terrain->header.resourceId == ResourceID("data://ground.terrain") &&
                    terrain->dependencies.size() == 1 && terrain->dependencies[0] == texture,
                "Terrain UUID and shared texture generation changed");
        auto ground = render::decodeCookedTerrainResource(terrain, expectedTarget, true, error);
        if(!ground)
            throw std::runtime_error(error.c_str());
        const auto &heightfield = ground->data.terrain;
        const Array<f32> heights = {0,1,2,2,3,4,4,5,6};
        require(ground->data.target == expectedTarget && ground->data.packagedBuild &&
                    heightfield.dimensions == Vector2I(3,3) && heightfield.spacing == Vector2F(2,4) &&
                    heightfield.origin == Vector2F(-2,-4) && heightfield.heightScale == .5f &&
                    heightfield.heights == heights && heightfield.holes == Array<u8>({0,0,0,1}) &&
                    heightfield.layers[0].albedo == "data://albedo.texres" &&
                    heightfield.layers[0].uvScale == Vector2F(.5f,.25f) &&
                    heightfield.layers[0].roughness == .625f && heightfield.layers[0].metallic == .125f &&
                    heightfield.appearance.wetness == .25f && heightfield.surfaceMasks.size() == 9 &&
                    ground->textures.size() == 1 && ground->textures[0].levels[0].bgra == image.levels[0].bgra,
                "Cooked terrain samples, appearance or shared texture values changed");
        require(ground->data.tiles.size() == 1 && ground->data.tiles[0].first == Vector2I(0,0) &&
                    ground->data.tiles[0].last == Vector2I(2,2) && ground->data.tiles[0].hasHoles &&
                    ground->data.tiles[0].minimum == Vector3F(-2,0,-4) &&
                    ground->data.tiles[0].maximum == Vector3F(2,3,4),
                "Cooked terrain tile metadata changed");
        render::TerrainMeshData terrainMesh;
        if(!render::buildTerrainTile(heightfield, ground->data.tiles[0], 0, terrainMesh, error))
            throw std::runtime_error(error.c_str());
        require(terrainMesh.positions.size() == 8 && terrainMesh.indices.size() == 18 &&
                    std::find(terrainMesh.positions.begin(), terrainMesh.positions.end(), Vector3F(0,1.5f,0)) != terrainMesh.positions.end() &&
                    std::find(terrainMesh.positions.begin(), terrainMesh.positions.end(), Vector3F(2,3,4)) == terrainMesh.positions.end(),
                "Cooked terrain mesh did not preserve its hole and height samples");
        auto brokenTerrain = *terrain;
        brokenTerrain.payload[0] ^= 1;
        render::CookedTerrainData rejectedTerrain;
        require(!render::decodeCookedTerrain(brokenTerrain, rejectedTerrain, error) && !error.empty() &&
                    !render::decodeCookedTerrainResource(terrain, "wrong-target", true, error) &&
                    !render::decodeCookedTerrainResource(terrain, expectedTarget, false, error),
                "Terrain decoder accepted corruption or incompatible target/build mode");

        auto world = runtime->loadUUID(foliageUUID, error);
        auto species = runtime->loadUUID(speciesUUID, error);
        auto pageResource = runtime->loadUUID(pageUUID, error);
        require(world && species && pageResource &&
                    world->header.resourceId == ResourceID("data://forest.folworld") &&
                    species->header.resourceType == ResourceTypeID("folspec") &&
                    pageResource->header.resourceType == ResourceTypeID("folpage") &&
                    species->dependencies.size() == 1 && species->dependencies[0] == texture,
                "Foliage UUID bindings and shared texture generation changed");
        auto forest = render::decodeCookedFoliageWorld(runtime, world->header.resourceId, expectedTarget, true, error);
        if(!forest)
            throw std::runtime_error(error.c_str());
        require(forest->source.snapshot && forest->source.snapshot->instances.empty() &&
                    forest->source.snapshot->species.size() == 1 && forest->source.cells.size() == 1 &&
                    forest->source.cells[0] == render::FoliageCellKey{0,0} &&
                    forest->source.pageInstanceCounts.size() == 1 && forest->source.pageInstanceCounts[0] == 2 &&
                    forest->source.snapshot->pageSize == 16 &&
                    forest->textures.at("data://albedo.texres").levels[0].bgra == image.levels[0].bgra,
                "Cooked foliage directory or texture values changed");
        const auto &bush = *forest->source.snapshot->species[0];
        require(bush.id == "installed-sdk-bush" && bush.kind == render::FoliageKind::Bush &&
                    bush.boundsMinimum == Vector3F(-.5f,0,-.25f) &&
                    bush.boundsMaximum == Vector3F(.5f,1,.25f) && bush.materials.size() == 1 &&
                    bush.materials[0].albedo == "data://albedo.texres" &&
                    bush.materials[0].roughness == .625f && bush.materials[0].metalness == .125f,
                "Cooked foliage species metadata changed");
        for(const auto &lod : bush.lods)
            require(lod.positions.size() == 3 && lod.positions[2] == Vector3F(0,1,0) &&
                        lod.normals[0] == Vector3F(0,0,1) && lod.uvs[2] == Vector2F(.5f,0) &&
                        lod.indices == Array<u32>({0,1,2}) && lod.sections.size() == 1 &&
                        lod.sections[0].indexCount == 3 && lod.sections[0].material == 0,
                    "Cooked foliage LOD geometry changed");
        const StreamCancellation notCancelled(nullptr);
        auto foliagePage = forest->source.load({0,0}, notCancelled, error);
        if(!foliagePage)
            throw std::runtime_error(error.c_str());
        require(foliagePage->cell == render::FoliageCellKey{0,0} && foliagePage->instances.size() == 2,
                "Cooked foliage page did not load its exact instance count");
        for(size_t index = 0; index < foliagePage->instances.size(); ++index)
        {
            const auto &instance = foliagePage->instances[index];
            require(instance.id == 21 + index && instance.species == 0 &&
                        instance.position == Vector3F(f32(index+1),0,f32(index+1)) &&
                        instance.scale == Vector3F(1,1,1) && instance.seed == 101 + index &&
                        instance.flags == render::FoliageManual,
                    "Cooked foliage instance values changed");
        }
        require(!forest->source.load({7,8}, notCancelled, error) && !error.empty() &&
                    !render::decodeCookedFoliageWorld(runtime, world->header.resourceId, "wrong-target", true, error) &&
                    !render::decodeCookedFoliageWorld(runtime, world->header.resourceId, expectedTarget, false, error),
                "Foliage decoder accepted a missing page or incompatible target/build mode");

        ResourceID unknown("data://must-be-cleared.texres");
        require(!runtime->resolveRuntimeUUID("2a6d7957-45ab-4bbb-9d96-91f7a9c1afff", unknown, error) &&
                    !unknown.isValid() && !error.empty(), "Unknown UUID accepted or retained stale output");
        runtime->shutdown();
        require(!runtime->isInitialized() && material->dependencies[0] == texture &&
                    render::decodeCookedTexture(*texture, image, error) &&
                    ground->data.terrain.heights == heights && foliagePage->instances[1].id == 22,
                "Caller-owned cooked resources did not survive shutdown");
        modules();
        std::cout << "Installed cooked content: catalog UUID material/texture/terrain/foliage closure, exact mips/tiles/species/pages and shutdown passed\n";
        return 0;
    }
    catch(const std::exception &error)
    {
        std::cerr << "Cooked content rejected: " << error.what() << '\n';
        return 10;
    }
}
