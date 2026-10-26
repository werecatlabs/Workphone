#pragma once
#include "MaterialSceneCpuProfile.h"

// Opt-in validation support for the real sample scene. The main function owns
// this bounded observer until the application and TypeManager have retired.
#include "ReviewEvidence.h"
#include <array>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <sstream>
#if defined( _WIN32 )
#    include "../../../Tests/cpp/MaterialHeapProfile.hpp"
#    include <evntprov.h>
#    pragma comment( lib, "advapi32.lib" )
#endif
#if WP_GRAPHICS_SYSTEM_CLAW
#    include <WPGraphics/ClawHammerSystem.hpp>
#endif

namespace workphone::advanced
{
    class ProductionSceneProfile
    {
    public:
        enum class Mode { Memory, Cpu };
        static constexpr size_t SampleCapacity = 1024;
        static constexpr size_t FrameCapacity = 8192;
        struct Workload
        {
            uint64_t actors = 0, materials = 0, textures = 0, meshes = 0;
            uint64_t materialReferences = 0, textureReferences = 0, meshReferences = 0;
            uint64_t meshSections = 0, vertices = 0, indices = 0, triangles = 0;
            uint64_t authoredTexturePayloadBytes = 0, treeLodGroups = 0;
            // FNV-1a over ordered position/normal/index values, without pointers,
            // names, uninitialized vertex padding, or generated resource IDs.
            uint64_t geometryDigest = 14695981039346656037ull;
            uint64_t circuitDigest = 14695981039346656037ull;
        };

        ProductionSceneProfile( std::string path, Mode mode ) : m_path( std::move( path ) ), m_mode( mode )
        {
#if defined( _WIN32 )
            LARGE_INTEGER frequency{};
            if( !QueryPerformanceFrequency( &frequency ) || frequency.QuadPart <= 0 )
                throw std::runtime_error( "Scene profile QPC is unavailable." );
            m_frequency = uint64_t( frequency.QuadPart );
            checkpoint( "process_bootstrap_before_type_manager" );
            // Registration does not start a trace, require elevation or change
            // OS policy. A collector must explicitly enable this provider.
            static const GUID provider = { 0xbb4d8875, 0x7b47, 0x4edd,
                { 0xa3, 0x25, 0x06, 0x38, 0x5e, 0x59, 0xdf, 0x1c } };
            m_eventRegistrationStatus = EventRegister( &provider, nullptr, nullptr, &m_provider );
#else
            throw std::runtime_error( "Production scene profiling currently requires Windows." );
#endif
        }
        ~ProductionSceneProfile()
        {
#if defined( _WIN32 )
            if( m_provider ) EventUnregister( m_provider );
#endif
        }
        ProductionSceneProfile( const ProductionSceneProfile & ) = delete;
        ProductionSceneProfile &operator=( const ProductionSceneProfile & ) = delete;

        void configure( uint32_t seed, int quality, uint32_t width, uint32_t height,
                        const std::string &view, bool orbit, bool foliage, bool audio, bool effects,
                        double seconds, const std::string &replay, const std::string &plugins,
                        int forcedTreeLod, int forcedVehicleLod )
        {
            m_seed = seed; m_quality = quality; m_width = width; m_height = height;
            m_view = view; m_orbit = orbit; m_foliage = foliage; m_audio = audio; m_effects = effects;
            m_requestedSeconds = seconds; m_replay = replay; m_plugins = plugins;
            m_forcedTreeLod = forcedTreeLod; m_forcedVehicleLod = forcedVehicleLod;
        }
        void setWorkload( const Workload &workload ) { m_workload = workload; }
        static void hashBytes( uint64_t &digest, const void *data, size_t count )
        {
            const auto bytes = static_cast<const unsigned char *>( data );
            for( size_t i = 0; i < count; ++i ) { digest ^= bytes[i]; digest *= 1099511628211ull; }
        }

        void checkpoint( const char *phase )
        {
#if defined( _WIN32 )
            const auto start = qpc();
            if( m_sampleCount == SampleCapacity ) { ++m_droppedSamples; return; }
            auto &sample = m_samples[m_sampleCount];
            sample.phase = phase;
            sample.qpc = start;
            sample.measuredFrames = m_frames;
            sample.heapCaptured = m_mode == Mode::Memory;
            if( sample.heapCaptured )
            {
                sample.heap = material_heap_profile::capture();
                ++m_heapWalks;
                if( m_measuring ) ++m_heapWalksDuringMeasurement;
                m_heapPeaks.observe( sample.heap );
            }
            ++m_sampleCount;
            wchar_t event[96]{};
            size_t length = 0;
            while( phase[length] && length + 1 < sizeof( event ) / sizeof( event[0] ) )
            { event[length] = wchar_t( static_cast<unsigned char>( phase[length] ) ); ++length; }
            marker( event );
            sample.observerTicks = qpc() - start;
            m_observerTicks += sample.observerTicks;
#else
            (void)phase;
#endif
        }

        void beginMeasurement()
        {
#if defined( _WIN32 )
            checkpoint( "scene_warmed_before_measurement" );
            m_materialCpu.begin();
            marker( L"scene_profile_measurement_begin" );
            processTimes( m_cpuKernelBegin, m_cpuUserBegin );
            m_beginQpc = m_lastFrameQpc = qpc();
            m_measurementThreadId = GetCurrentThreadId();
            m_nextMemoryQpc = m_beginQpc + m_frequency * 5;
            m_observerTicksAtBegin = m_observerTicks;
            m_measuring = true;
#endif
        }

        void frame()
        {
#if defined( _WIN32 )
            if( !m_measuring ) return;
            const auto start = qpc();
            const auto interval = start - m_lastFrameQpc;
            m_lastFrameQpc = start;
            m_frameIntervals[m_frames % FrameCapacity] = interval;
            ++m_frames;
            m_intervalTotalTicks += interval;
            if( interval < m_intervalMinTicks ) m_intervalMinTicks = interval;
            if( interval > m_intervalMaxTicks ) m_intervalMaxTicks = interval;
            m_observerTicks += qpc() - start;
            // Memory mode is deliberately unsuitable as a CPU timing comparison.
            // Walk only after the frame, before the next actual render iteration.
            if( m_mode == Mode::Memory && start >= m_nextMemoryQpc )
            {
                checkpoint( "periodic_warmed_scene" );
                m_nextMemoryQpc = qpc() + m_frequency * 5;
            }
#endif
        }

        void endMeasurement()
        {
#if defined( _WIN32 )
            if( !m_measuring ) return;
            m_endQpc = qpc();
            m_materialCpu.end();
            processTimes( m_cpuKernelEnd, m_cpuUserEnd );
            m_measuring = false;
            m_measuredObserverTicks = m_observerTicks - m_observerTicksAtBegin;
            marker( L"scene_profile_measurement_end" );
            checkpoint( "measurement_end_before_readback" );
#endif
        }

#if WP_GRAPHICS_SYSTEM_CLAW
        void observeTelemetry( const render::ClawHammerSystem::SceneProfileSnapshot &value )
        {
            if( !m_measuring || value.serial == m_lastTelemetrySerial ) return;
            m_lastTelemetrySerial = value.serial;
            m_lastTelemetry = value;
            ++m_telemetryFrames;
            m_graphicsUpdateNs += value.graphicsUpdateNs;
            m_graphicsPrepareNs += value.graphicsPrepareNs;
            m_nativeFrameNs += value.nativeFrameNs;
            m_preparationTotalNs += value.preparationTotalNs;
            m_mainPrepareNs += value.mainPrepareNs;
            m_shadowDeriveNs += value.shadowDeriveNs;
            m_materialPrepareNs += value.materialPrepareNs;
            m_materialPreparations += value.materialPreparations;
            m_materialStorageAllocations += value.materialStorageAllocations;
            m_materialStorageReusedViews += value.materialStorageReusedViews;
            m_failedPasses += value.failedPasses;
        }
#endif

        void setValidationEvidence( std::string evidence, std::string evidencePath,
                                    bool passed, bool nativePassed )
        {
            m_validationEvidence = std::move( evidence );
            m_validationEvidencePath = std::move( evidencePath );
            m_validationPassed = passed;
            m_nativePassed = nativePassed;
        }
        bool invariantsPassed() const
        {
            bool telemetry = false;
#if WP_GRAPHICS_SYSTEM_CLAW
            telemetry = m_telemetryFrames > 0 && m_failedPasses == 0 && m_lastTelemetry.dx11Available &&
                m_lastTelemetry.dx11FramesLifetime > 0 && m_lastTelemetry.dx11DrawsLifetime > 0 &&
                m_lastTelemetry.nativeUpdateSerial == m_lastTelemetry.nativeRenderSerial &&
                m_lastTelemetry.completedPasses > 0 && m_lastTelemetry.dx11PresentFailuresLifetime == 0;
#endif
            return m_validationPassed && m_nativePassed && m_beginQpc > 0 && m_endQpc > m_beginQpc &&
                m_frames > 0 && m_droppedSamples == 0 && telemetry &&
                ( m_mode != Mode::Cpu || m_heapWalksDuringMeasurement == 0 );
        }

        // Reporting happens after every application/scene owner has retired.
        // CREATE_NEW also rejects an output that appeared after CLI validation.
        void write( int exitCode, const std::string &error )
        {
            if( m_measuring ) endMeasurement();
            std::ostringstream out;
            out.imbue( std::locale::classic() );
            out << std::setprecision( 17 )
                << "{\n\"schema\":\"workphone.production-scene-profile.v1\",\n"
                << "\"mode\":" << jsonString( m_mode == Mode::Memory ? "memory" : "cpu" )
                << ",\n\"sceneSystem\":\"ProceduralRaceScene\",\n\"productionLikeScene\":true"
                << ",\n\"headless\":false,\n\"activeWorkerThreads\":0"
                << ",\n\"cpuAccounting\":\"Windows guest process times; not PMU or CPU cache counters\""
                << ",\n\"cpuTimingAcceptance\":false,\n\"hardwareCacheAcceptance\":false"
                << ",\n\"gpuResourceBytesMeasured\":false,\n\"materialOnlyAttribution\":false"
                << ",\n\"processMemoryScope\":\"Whole guest process, including private driver/staging allocations; dedicated GPU residency is not measured\""
                << ",\n\"cpuTimingIncludesMemoryWalks\":" << ( m_mode == Mode::Memory ? "true" : "false" )
                << ",\n\"seed\":" << m_seed << ",\n\"quality\":" << m_quality
                << ",\n\"width\":" << m_width << ",\n\"height\":" << m_height
                << ",\n\"view\":" << jsonString( m_view ) << ",\n\"orbit\":" << boolean( m_orbit )
                << ",\n\"foliageWorld\":" << boolean( m_foliage ) << ",\n\"audio\":" << boolean( m_audio )
                << ",\n\"effects\":" << boolean( m_effects ) << ",\n\"requestedSeconds\":" << m_requestedSeconds
                << ",\n\"forcedTreeLod\":" << m_forcedTreeLod << ",\n\"forcedVehicleLod\":" << m_forcedVehicleLod
                << ",\n\"replay\":" << jsonString( m_replay ) << ",\n\"plugins\":" << jsonString( m_plugins )
                << ",\n\"exitCode\":" << exitCode << ",\n\"error\":" << jsonString( error )
                << ",\n\"runCompleted\":" << boolean( exitCode == 0 && invariantsPassed() )
                << ",\n\"profileInvariantsPassed\":" << boolean( invariantsPassed() )
                << ",\n\"renderValidationPassed\":" << boolean( m_validationPassed )
                << ",\n\"nativeFrameValidationPassed\":" << boolean( m_nativePassed )
                << ",\n\"processId\":" << processId()
                << ",\n\"measurementThreadId\":" << m_measurementThreadId
                << ",\n\"qpcFrequency\":" << m_frequency << ",\n\"measurementBeginQpc\":" << m_beginQpc
                << ",\n\"measurementEndQpc\":" << m_endQpc
                << ",\n\"measurementSeconds\":" << seconds( m_endQpc >= m_beginQpc ? m_endQpc - m_beginQpc : 0 )
                << ",\n\"processKernelSeconds\":" << double( m_cpuKernelEnd - m_cpuKernelBegin ) / 10000000.0
                << ",\n\"processUserSeconds\":" << double( m_cpuUserEnd - m_cpuUserBegin ) / 10000000.0
                << ",\n\"renderOwnerUpdatesMeasured\":" << m_frames
                << ",\n\"renderOwnerUpdateMeanMs\":" << ( m_frames ? seconds( m_intervalTotalTicks ) * 1000.0 / double( m_frames ) : 0.0 )
                << ",\n\"renderOwnerUpdateMinMs\":" << ( m_frames ? seconds( m_intervalMinTicks ) * 1000.0 : 0.0 )
                << ",\n\"renderOwnerUpdateMaxMs\":" << seconds( m_intervalMaxTicks ) * 1000.0
                << ",\n\"renderOwnerUpdateP95Last8192Ms\":" << intervalP95Ms()
                << ",\n\"intervalsRetained\":" << std::min<uint64_t>( m_frames, FrameCapacity )
                << ",\n\"intervalCapacity\":" << FrameCapacity
                << ",\n\"intervalsTruncated\":" << boolean( m_frames > FrameCapacity )
                << ",\n\"memorySampleCapacity\":" << SampleCapacity
                << ",\n\"droppedMemorySamples\":" << m_droppedSamples
                << ",\n\"heapWalkCount\":" << m_heapWalks
                << ",\n\"heapWalksDuringMeasurement\":" << m_heapWalksDuringMeasurement
                << ",\n\"observerFixedBytes\":" << sizeof( *this )
                << ",\n\"observerDynamicStringCapacityBytes\":" << dynamicStringCapacityBytes()
                << ",\n\"observerMeasurementMs\":" << seconds( m_measuredObserverTicks ) * 1000.0
                << ",\n\"observerAllCheckpointMs\":" << seconds( m_observerTicks ) * 1000.0
                << ",\n\"observerTimingScope\":\"QPC frame bookkeeping and explicit checkpoint calls; enabled engine telemetry timing overhead is not separately subtracted\""
                << ",\n\"sampledHeapBusyPeakBytes\":" << sampledHeapPeak()
                << ",\n\"transientHeapPeakMeasured\":false"
                << ",\n\"etwProviderGuid\":\"{BB4D8875-7B47-4EDD-A325-06385E59DF1C}\""
                << ",\n\"etwRegistrationStatus\":" << m_eventRegistrationStatus
                << ",\n\"etwLastWriteStatus\":" << m_eventWriteStatus
                << ",\n\"etwCaptureVerified\":false"
                << ",\n\"assetInventoryScope\":\"ProceduralRaceScene assets; sample root and camera are additional owners; resource aliases are deduplicated by pointer identity\""
                << ",\n\"geometryHashScope\":\"FNV-1a64 of ordered CPU position/normal/index values; not an asset file or texture content hash\""
                << ",\n\"workload\":{\"uniqueGeneratedActors\":" << m_workload.actors
                << ",\"uniqueMaterials\":" << m_workload.materials << ",\"uniqueTextures\":" << m_workload.textures
                << ",\"uniqueMeshes\":" << m_workload.meshes
                << ",\"materialReferences\":" << m_workload.materialReferences
                << ",\"textureReferences\":" << m_workload.textureReferences << ",\"meshReferences\":" << m_workload.meshReferences
                << ",\"meshSections\":" << m_workload.meshSections << ",\"vertices\":" << m_workload.vertices
                << ",\"indices\":" << m_workload.indices << ",\"triangles\":" << m_workload.triangles
                << ",\"authoredTexturePayloadBytes\":" << m_workload.authoredTexturePayloadBytes
                << ",\"treeLodGroups\":" << m_workload.treeLodGroups
                << ",\"orderedGeometryFnv1a64\":" << m_workload.geometryDigest
                << ",\"orderedCircuitFnv1a64\":" << m_workload.circuitDigest << "}";
            writeTelemetry( out );
            m_materialCpu.write( out );
            out << ",\n\"memorySamples\":[";
            for( size_t i = 0; i < m_sampleCount; ++i )
            {
                if( i ) out << ',';
                const auto &s = m_samples[i];
                out << "{\"phase\":" << jsonString( s.phase ) << ",\"qpc\":" << s.qpc
                    << ",\"measuredFrames\":" << s.measuredFrames
                    << ",\"heapCaptured\":" << boolean( s.heapCaptured )
                    << ",\"observerMs\":" << seconds( s.observerTicks ) * 1000.0;
#if defined( _WIN32 )
                if( s.heapCaptured )
                    out << ",\"heapBusyDataBytes\":" << s.heap.heapBusyDataBytes
                        << ",\"heapBusyOverheadBytes\":" << s.heap.heapBusyOverheadBytes
                        << ",\"heapBusyBytes\":" << s.heap.heapBusyBytes()
                        << ",\"heapFreeDataBytes\":" << s.heap.heapFreeDataBytes
                        << ",\"heapFreeOverheadBytes\":" << s.heap.heapFreeOverheadBytes
                        << ",\"heapBusyBlocks\":" << s.heap.heapBusyBlocks
                        << ",\"heapFreeBlocks\":" << s.heap.heapFreeBlocks
                        << ",\"heapRegions\":" << s.heap.heapRegionCount
                        << ",\"privateCommitBytes\":" << s.heap.privateCommitBytes
                        << ",\"processLifetimePeakPrivateCommitBytes\":" << s.heap.processLifetimePeakPrivateCommitBytes
                        << ",\"workingSetBytes\":" << s.heap.workingSetBytes
                        << ",\"processLifetimePeakWorkingSetBytes\":" << s.heap.processLifetimePeakWorkingSetBytes
                        << ",\"vmPrivateCommittedBytes\":" << s.heap.vmPrivateCommittedBytes
                        << ",\"vmPrivateReservedBytes\":" << s.heap.vmPrivateReservedBytes;
#endif
                out << '}';
            }
            out << "],\n\"renderValidation\":" << ( m_validationEvidence.empty() ? "null" : m_validationEvidence )
                << ",\n\"limitations\":[\"Real procedural game scene; not a shipped asset-scene certification\","
                   "\"CPU frame intervals include scheduling and Present; process CPU is guest accounting, not PMU\","
                   "\"Heap walks are checkpoint samples and never establish exact transient heap peaks\","
                   "\"Shared shader registry and runtime capacity can remain after application teardown; not a leak soak\","
                   "\"SceneAssets contains sample/component aliases; listed payload counts are not added to actual heap totals\","
                   "\"Per-view material table sums are work/workspace diagnostics, not deduplicated scene residency\","
                   "\"GPU memory, hardware cache improvement, visual approval and isolated material attribution remain unverified\"]\n}\n";
            writeExclusive( m_path, out.str() );
            if( !m_validationEvidencePath.empty() && !m_validationEvidence.empty() &&
                !writeReviewEvidence( m_validationEvidencePath, m_validationEvidence ) )
                throw std::runtime_error( "Could not write deferred rendering evidence." );
        }

    private:
        struct Sample
        {
            const char *phase = "";
            uint64_t qpc = 0, measuredFrames = 0, observerTicks = 0;
            bool heapCaptured = false;
#if defined( _WIN32 )
            material_heap_profile::Snapshot heap;
#endif
        };
        static const char *boolean( bool value ) { return value ? "true" : "false"; }
        double seconds( uint64_t ticks ) const { return m_frequency ? double( ticks ) / double( m_frequency ) : 0; }
        static uint32_t processId()
        {
#if defined( _WIN32 )
            return GetCurrentProcessId();
#else
            return 0;
#endif
        }
        static uint64_t qpc()
        {
#if defined( _WIN32 )
            LARGE_INTEGER value{};
            if( !QueryPerformanceCounter( &value ) ) throw std::runtime_error( "Scene profile QPC failed." );
            return uint64_t( value.QuadPart );
#else
            return 0;
#endif
        }
        static void processTimes( uint64_t &kernel, uint64_t &user )
        {
#if defined( _WIN32 )
            FILETIME created{}, exited{}, kernelTime{}, userTime{};
            if( !GetProcessTimes( GetCurrentProcess(), &created, &exited, &kernelTime, &userTime ) )
                throw std::runtime_error( "Scene profile process CPU accounting failed." );
            kernel = ( uint64_t( kernelTime.dwHighDateTime ) << 32 ) | kernelTime.dwLowDateTime;
            user = ( uint64_t( userTime.dwHighDateTime ) << 32 ) | userTime.dwLowDateTime;
#else
            kernel = user = 0;
#endif
        }
        void marker( const wchar_t *phase )
        {
#if defined( _WIN32 )
            if( m_provider ) m_eventWriteStatus = EventWriteString( m_provider, 0, 0, phase );
#else
            (void)phase;
#endif
        }
        double intervalP95Ms() const
        {
            auto intervals = m_frameIntervals;
            const auto count = size_t( std::min<uint64_t>( m_frames, FrameCapacity ) );
            if( count == 0 ) return 0;
            std::sort( intervals.begin(), intervals.begin() + count );
            return seconds( intervals[( count * 95 + 99 ) / 100 - 1] ) * 1000.0;
        }
        uint64_t dynamicStringCapacityBytes() const
        {
            return m_path.capacity() + m_view.capacity() + m_replay.capacity() + m_plugins.capacity() +
                m_validationEvidence.capacity() + m_validationEvidencePath.capacity() + 6;
        }
        uint64_t sampledHeapPeak() const
        {
#if defined( _WIN32 )
            return m_heapPeaks.sampledHeapBusyBytes;
#else
            return 0;
#endif
        }
        void writeTelemetry( std::ostream &out ) const
        {
            out << ",\n\"telemetry\":{\"frames\":" << m_telemetryFrames
                << ",\"failedPasses\":" << m_failedPasses
                << ",\"graphicsUpdateNs\":" << m_graphicsUpdateNs
                << ",\"graphicsPrepareNs\":" << m_graphicsPrepareNs << ",\"nativeFrameNs\":" << m_nativeFrameNs
                << ",\"preparationTotalNs\":" << m_preparationTotalNs << ",\"mainPrepareNs\":" << m_mainPrepareNs
                << ",\"shadowDeriveNs\":" << m_shadowDeriveNs << ",\"materialPrepareNs\":" << m_materialPrepareNs
                << ",\"materialPreparations\":" << m_materialPreparations
                << ",\"materialStorageAllocations\":" << m_materialStorageAllocations
                << ",\"materialStorageReusedViews\":" << m_materialStorageReusedViews;
#if WP_GRAPHICS_SYSTEM_CLAW
            const auto &s = m_lastTelemetry;
            out << ",\"lastSerial\":" << s.serial << ",\"lastGeneration\":" << s.generation
                << ",\"nativeUpdateSerial\":" << s.nativeUpdateSerial << ",\"nativeRenderSerial\":" << s.nativeRenderSerial
                << ",\"completedPassesLastFrame\":" << s.completedPasses << ",\"skippedPassesLastFrame\":" << s.skippedPasses
                << ",\"registeredScenes\":" << s.registeredScenes << ",\"mainViewsLastFrame\":" << s.mainViews
                << ",\"shadowViewsLastFrame\":" << s.shadowViews
                << ",\"materialRecordsLastFrame\":" << s.materialRecords << ",\"materialBindingsLastFrame\":" << s.materialBindings
                << ",\"materialMeshesLastFrame\":" << s.materialMeshes
                << ",\"materialDenseCapacityBytesViewSumLastFrame\":" << s.materialDenseCapacityBytesSum
                << ",\"materialRetainedBytesViewSumLastFrame\":" << s.materialRetainedBytesSum
                << ",\"materialPeakBytesMaxLastFrame\":" << s.materialPeakBytesMax
                << ",\"cachePreparationsLifetime\":" << s.cachePreparationsLifetime
                << ",\"cacheStorageAllocationsLifetime\":" << s.cacheStorageAllocationsLifetime
                << ",\"directStorageAllocationsLifetime\":" << s.directStorageAllocationsLifetime
                << ",\"cacheStorageBytes\":" << s.cacheStorageBytes << ",\"cacheSlots\":" << s.cacheSlots
                << ",\"cacheResident\":" << s.cacheResident
                << ",\"dx11Available\":" << boolean( s.dx11Available )
                << ",\"dx11FramesLifetime\":" << s.dx11FramesLifetime
                << ",\"dx11DrawsLifetime\":" << s.dx11DrawsLifetime
                << ",\"dx11TrianglesLifetime\":" << s.dx11TrianglesLifetime
                << ",\"dx11MaterialUploadsLifetime\":" << s.dx11MaterialUploadsLifetime
                << ",\"dx11TransformUploadsLifetime\":" << s.dx11TransformUploadsLifetime
                << ",\"dx11StateBindingsLifetime\":" << s.dx11StateBindingsLifetime
                << ",\"dx11InstancesLifetime\":" << s.dx11InstancesLifetime
                << ",\"dx11GpuSamplesLifetime\":" << s.dx11GpuSamplesLifetime
                << ",\"dx11PresentAttemptsLifetime\":" << s.dx11PresentAttemptsLifetime
                << ",\"dx11PresentOccludedLifetime\":" << s.dx11PresentOccludedLifetime
                << ",\"dx11PresentFailuresLifetime\":" << s.dx11PresentFailuresLifetime
                << ",\"dx11IntervalMs\":" << s.dx11IntervalMs
                << ",\"dx11P95IntervalAvailable\":false,\"dx11P95IntervalMs\":null"
                << ",\"dx11CpuFrameMs\":" << s.dx11CpuFrameMs << ",\"dx11PresentMs\":" << s.dx11PresentMs
                << ",\"dx11GpuFrameMs\":" << s.dx11GpuFrameMs;
#endif
            out << '}';
        }
        static void writeExclusive( const std::string &path, const std::string &content )
        {
#if defined( _WIN32 )
            const auto file = CreateFileW( std::filesystem::path( path ).c_str(), GENERIC_WRITE, 0,
                                          nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr );
            if( file == INVALID_HANDLE_VALUE ) throw std::runtime_error( "Scene profile output exists or cannot be created." );
            const bool fits = content.size() <= uint64_t( MAXDWORD );
            DWORD written = 0;
            const bool ok = fits && WriteFile( file, content.data(), DWORD( content.size() ), &written, nullptr ) &&
                written == content.size() && FlushFileBuffers( file );
            CloseHandle( file );
            if( !ok ) throw std::runtime_error( "Scene profile output write failed." );
#else
            (void)path; (void)content;
            throw std::runtime_error( "Scene profile output requires Windows." );
#endif
        }

        std::string m_path, m_view, m_replay, m_plugins, m_validationEvidence, m_validationEvidencePath;
        Mode m_mode;
        Workload m_workload;
        std::array<Sample, SampleCapacity> m_samples{};
        std::array<uint64_t, FrameCapacity> m_frameIntervals{};
        size_t m_sampleCount = 0;
        uint64_t m_droppedSamples = 0, m_frames = 0, m_frequency = 0;
        MaterialSceneCpuProfile m_materialCpu;
        uint64_t m_beginQpc = 0, m_endQpc = 0, m_lastFrameQpc = 0, m_nextMemoryQpc = 0;
        uint64_t m_cpuKernelBegin = 0, m_cpuKernelEnd = 0, m_cpuUserBegin = 0, m_cpuUserEnd = 0;
        uint64_t m_intervalTotalTicks = 0, m_intervalMinTicks = ~uint64_t(0), m_intervalMaxTicks = 0;
        uint64_t m_observerTicks = 0, m_observerTicksAtBegin = 0, m_measuredObserverTicks = 0;
        uint64_t m_heapWalks = 0, m_heapWalksDuringMeasurement = 0;
        uint64_t m_telemetryFrames = 0, m_lastTelemetrySerial = 0;
        uint64_t m_graphicsUpdateNs = 0, m_graphicsPrepareNs = 0, m_nativeFrameNs = 0;
        uint64_t m_preparationTotalNs = 0, m_mainPrepareNs = 0, m_shadowDeriveNs = 0, m_materialPrepareNs = 0;
        uint64_t m_materialPreparations = 0, m_materialStorageAllocations = 0, m_materialStorageReusedViews = 0, m_failedPasses = 0;
        uint32_t m_seed = 0, m_width = 0, m_height = 0;
        uint32_t m_measurementThreadId = 0;
        int m_quality = 0, m_forcedTreeLod = -1, m_forcedVehicleLod = -1;
        double m_requestedSeconds = 0;
        bool m_measuring = false, m_orbit = false, m_foliage = false, m_audio = false, m_effects = false;
        bool m_validationPassed = false, m_nativePassed = false;
        uint32_t m_eventRegistrationStatus = 0, m_eventWriteStatus = 0;
#if defined( _WIN32 )
        material_heap_profile::Peaks m_heapPeaks;
        REGHANDLE m_provider = 0;
#endif
#if WP_GRAPHICS_SYSTEM_CLAW
        render::ClawHammerSystem::SceneProfileSnapshot m_lastTelemetry{};
#endif
    };
}
