#pragma once

#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <memory>
#include <mutex>

namespace workphone::scene
{
    class IGameActor;
    class MeshRenderer;

    namespace procedural_scene
    {
        struct SceneAssets;

        struct CityStreamingOptions
        {
            f32 loadRadius = 300.f;
            f32 retainRadius = 420.f;
            u32 maximumResidentPatches = 32;
            u32 maximumPendingPatches = 8;
            u32 maximumInstallsPerUpdate = 1;
            u64 maximumPatchBytes = 24ull << 20;
        };

        struct CityStreamingStats
        {
            bool initialized = false;
            u32 totalPatches = 0, residentPatches = 0, pendingPatches = 0, retiringPatches = 0;
            u32 residentMeshes = 0, fallbackMeshes = 0;
            u64 installedPatches = 0, evictedPatches = 0, reloadedPatches = 0, failedPatches = 0;
            u64 meshPayloadEvictions = 0, meshPayloadReloads = 0;
            u64 nativeMeshUnloads = 0, nativeMeshLoads = 0;
            u64 residentSourceBytes = 0, sourceDiskBytes = 0;
        };

        /** Residency for the detail tiers of generated render-only building patches.
         * City actors, terrain, collision, materials and lowest-detail meshes remain owned by
         * the generated scene. Call mutations on the scene owner task with render publication
         * excluded. Close before clearing the generation. */
        class WPCore_API CityStreaming final
        {
        public:
            explicit CityStreaming( const CityStreamingOptions &options = {} );
            ~CityStreaming();

            bool initialize( SceneAssets &assets, SmartPtr<IGameActor> owner,
                             const String &cacheDirectory, String &error );
            /// Use the same active view as the building LOD groups. A missing camera preserves demand.
            bool updateFromActiveCamera( String &error );
            bool update( const Array<Vector3F> &observers, String &error );
            void close();
            CityStreamingStats stats() const;
            bool isInitialized() const;
            bool isStreamedOut( const MeshRenderer *renderer ) const;
            bool hasPending() const;

            CityStreaming( const CityStreaming & ) = delete;
            CityStreaming &operator=( const CityStreaming & ) = delete;

        private:
            struct Impl;
            CityStreamingOptions m_options;
            mutable std::mutex m_mutex;
            std::unique_ptr<Impl> m_impl;
        };
    }  // namespace race
}  // namespace workphone::scene
