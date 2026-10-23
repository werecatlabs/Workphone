#ifndef WP_SCENE_WORLD_HPP
#define WP_SCENE_WORLD_HPP

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace workphone::scene
{
    struct WorldCell
    {
        String id, file;
        Vector3F minimum, maximum;
        u64 key = 0, bytes = 0, decodedBytes = 0;
        // A detail/proxy pair has one spatial owner. Proxies never own gameplay
        // identities and are excluded from the spatial index's detail entries.
        u64 detailKey = 0, proxyKey = 0;
        u32 actors = 0;
        double distanceSquared( const Vector3F &position ) const;
    };

    struct WorldManifest
    {
        static constexpr u32 version = 2;
        Array<WorldCell> cells;
        float cellSize = 256;
        String generation;
    };

    struct WorldCookOptions
    {
        float cellSize = 256;
        u32 maximumActorsPerCell = 128;
        u64 maximumCellBytes = 4ull << 20;
        String assetRoot;
        // Explicit authoring conversion, limited to render-only static candidates.
        // Scripted, animated, physical, or externally referenced groups stay persistent.
        bool convertRenderOnlyLeaves = false;
        // Deliberately coarse skyline preview: one batched bounds-box mesh per
        // detailed cell. Output must be below assetRoot for portable references.
        bool generateHlod = false;
    };

    /** Offline conversion only. Runtime reads the manifest and demanded payloads.
     * The persistent layer is cell zero and always participates in admission.
     * Cell payloads preserve UUIDs and the portable binary Properties format. */
    class WPCore_API SceneWorld
    {
    public:
        static bool cook( const Properties &scene, const String &directory,
                          const WorldCookOptions &options, String &manifestPath, String &error );
        static bool readManifest( const String &path, WorldManifest &manifest, String &error );
        static SmartPtr<Properties> readCell( const String &manifestPath, const WorldCell &cell,
                                              String &error );
        static u64 keyFor( const String &text );
    };
}
#endif
