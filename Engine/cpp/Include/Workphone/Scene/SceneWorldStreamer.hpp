#ifndef WP_SCENE_WORLD_STREAMER_HPP
#define WP_SCENE_WORLD_STREAMER_HPP
#include <Workphone/Scene/SceneWorld.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <memory>

namespace workphone::scene
{
    struct SceneWorldStreamOptions
    {
        float loadRadius = 512, retainRadius = 768;
        bool enableHlod = false;
        float proxyLoadRadius = 2048, proxyRetainRadius = 2304;
        u32 maximumResidentActors = 1024, maximumResidentCells = 32;
        u32 maximumPendingCells = 16, maximumInstallsPerUpdate = 1;
        double installMilliseconds = 2, minimumResidencySeconds = 2;
        u64 maximumDecodedCellBytes = 32ull << 20;
    };
    struct SceneWorldStreamStatistics
    {
        u32 residentCells = 0, residentActors = 0, pendingCells = 0, retiringObjects = 0;
        u64 cellReads = 0, installedCells = 0, evictedCells = 0, failedCells = 0;
        u64 residentSourceBytes = 0, manifestBytes = 0, installBudgetOvershoots = 0;
        u64 releasedMeshResources = 0;
        u64 releasedMaterialResources = 0, releasedTextureResources = 0;
        u32 residentProxyCells = 0, visibleProxyCells = 0;
        u64 representationSwitches = 0, hlodAdmissionFailures = 0;
        u64 graphicsRetryRequests = 0;
        u32 indexNodesVisited = 0, indexCellsTested = 0;
        double lastInstallMilliseconds = 0;
    };

    /** Owner-thread cell coordinator. CPU workers only read/decode independent
     * payloads. Actor installation/retirement uses the existing scene transaction.
     * Budget statistics cover participating payloads; legacy asset caches and
     * renderer allocations are reported separately, not advertised as capped. */
    class WPCore_API SceneWorldStreamer final
    {
    public:
        explicit SceneWorldStreamer( std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared() );
        ~SceneWorldStreamer();
        bool open( const String &manifest, SmartPtr<IGameScene> scene,
                   const SceneWorldStreamOptions &options, String &error );
        bool update( const Array<Vector3F> &observers, String &error );
        void close();
        SceneWorldStreamStatistics statistics() const;
        // Explicitly retain cells containing edited/gameplay actors. A pin fails
        // closed on unknown identity; moving actors are not silently destroyed.
        bool pinActor( const String &uuid );
        bool isOpen() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}
#endif
