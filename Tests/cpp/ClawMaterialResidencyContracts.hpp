#pragma once

#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawMaterialManager.hpp>
#include <WPGraphics/ClawMaterialResidencyRetry.hpp>
#include <Workphone/Graphics/MaterialPass.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/IO/FileSystem.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <cstdio>

namespace claw_material_residency_contracts
{
    using namespace workphone;
    using namespace workphone::render;

    inline void require( bool value, const char *message )
    {
        if( !value ) throw std::runtime_error( message );
    }

    class InspectionMaterial final : public ClawMaterial
    {
    public:
        mutable u32 propertyReads = 0;
        bool rejectInspection = false;

        SmartPtr<Properties> getProperties() const override
        {
            ++propertyReads;
            if( rejectInspection ) throw std::runtime_error( "injected residency inspection failure" );
            return ClawMaterial::getProperties();
        }
    };

    class InspectionManager final : public ClawMaterialManager
    {
    public:
        void retain( const SmartPtr<IMaterial> &material ) { m_materials.emplace_back( material ); }
    };

    class InspectionFiles final : public FileSystem
    {
    public:
        u32 opens = 0;
        using FileSystem::open;
        SmartPtr<workphone::IStream> open( const String &path, bool input, bool binary, bool truncate,
                               bool ignorePath, bool ignoreCase ) override
        {
            ++opens;
            return FileSystem::open( path, input, binary, truncate, ignorePath, ignoreCase );
        }
    };

    template <class Fixture> struct InspectionFixture
    {
        Fixture &fixture;
        SmartPtr<IFileSystem> previousFiles;
        SmartPtr<InspectionFiles> files = make_ptr<InspectionFiles>();
        SmartPtr<InspectionManager> manager = make_ptr<InspectionManager>();
        std::filesystem::path path;
        String file, document;

        explicit InspectionFixture( Fixture &owner ) : fixture( owner ),
            previousFiles( owner.app->getFileSystem() ),
            path( owner.root / "automatic_residency_inspection.mat" ),
            file( path.generic_string().c_str() )
        {
            auto properties = make_ptr<Properties>();
            properties->setProperty( "materialType", 0 );
            document = DataUtil::toString( properties.get() );
            writeSource();
            fixture.app->setFileSystem( files );
            manager->load( nullptr );
        }

        ~InspectionFixture()
        {
            manager->unload( nullptr );
            manager = nullptr;
            fixture.app->setFileSystem( previousFiles );
            std::error_code ignored;
            std::filesystem::remove( path, ignored );
        }

        void writeSource() const
        {
            std::ofstream output( path );
            output << document.c_str();
            require( output.good(), "automatic residency source fixture is writable" );
        }

        SmartPtr<InspectionMaterial> load()
        {
            auto material = make_ptr<InspectionMaterial>();
            material->setResourceManager( manager );
            material->setName( "automatic_residency_inspection" );
            material->setFilePath( file );
            manager->retain( material );
            material->load( nullptr );
            require( material->isLoaded(), "inspection material loads the real file-backed graph" );
            return material;
        }

        void resetCounts( InspectionMaterial &material )
        {
            material.propertyReads = 0;
            files->opens = 0;
        }
    };

    inline void runRetryDeadline()
    {
        using Retry = workphone::render::detail::MaterialResidencyRetry;
        Retry retry;
        const auto start = Retry::Clock::time_point{} + std::chrono::seconds( 10 );
        require( retry.ready( start ), "a fresh material has no automatic inspection delay" );
        retry.defer( start );
        require( !retry.ready( start ) &&
                     !retry.ready( start + std::chrono::seconds( 1 ) - Retry::Clock::duration( 1 ) ),
                 "failed inspection waits through the instant before its retry deadline" );
        for( u32 frame = 0; frame < 100; ++frame )
            require( !retry.ready( start + std::chrono::milliseconds( frame ) ),
                     "skipped frames keep the original retry deadline" );
        require( retry.ready( start + std::chrono::seconds( 1 ) ) &&
                     retry.ready( start + std::chrono::seconds( 2 ) ),
                 "automatic inspection becomes eligible at the exact deadline and stays eligible" );
        retry.defer( start + std::chrono::seconds( 2 ) );
        retry.reset();
        require( retry.ready( start ), "unload/reset clears a previous failed-inspection delay" );
    }

    template <class Fixture> void runAutomaticRetry( Fixture &fixture )
    {
        runRetryDeadline();
        InspectionFixture<Fixture> inspection( fixture );
        auto material = inspection.load();
        auto *identity = material.get();
        const auto oldRoughness = material->getRoughness();
        material->setRoughness( oldRoughness == 0.75f ? 0.25f : 0.75f );
        WeakPtr<IMaterialTechnique> graphPin = material->getTechnique( 0 );
        material = nullptr;
        inspection.resetCounts( *identity );
        require( inspection.manager->releaseUnusedResourcesOnUpdate() == 0 &&
                     identity->propertyReads == 0 && inspection.files->opens == 0,
                 "externally retained graph nodes skip serialization and source I/O" );
        graphPin = nullptr;
        require( inspection.manager->releaseUnusedResourcesOnUpdate( 0 ) == 0 &&
                     identity->propertyReads == 0 && inspection.files->opens == 0,
                 "zero automatic candidates perform no material inspection" );
        require( inspection.manager->releaseUnusedResourcesOnUpdate() == 0 && identity->isLoaded() &&
                     identity->propertyReads == 1 && inspection.files->opens == 0,
                 "first automatic inspection detects unsaved edits before any source I/O" );
        for( u32 frame = 0; frame < 100; ++frame )
            require( inspection.manager->releaseUnusedResourcesOnUpdate() == 0,
                     "dirty materials stay resident across repeated automatic sweeps" );
        require( identity->propertyReads == 1 && inspection.files->opens == 0,
                 "repeated idle sweeps rebuild no dirty document and open no source" );
        identity->setRoughness( oldRoughness );
        require( inspection.manager->releaseUnusedResources() == 1 && !identity->isLoaded() &&
                     identity->propertyReads == 2 && inspection.files->opens == 1,
                 "forced reclamation bypasses retry delay and rechecks restored authored values" );

        identity->load( nullptr );
        inspection.resetCounts( *identity );
        require( inspection.manager->releaseUnusedResourcesOnUpdate() == 1 && !identity->isLoaded() &&
                     identity->propertyReads == 1 && inspection.files->opens == 1,
                 "a clean reloaded material retires on its first automatic sweep" );

        identity->load( nullptr );
        const auto beforeScheme = identity->getRenderState();
        const auto oldScheme = identity->getTechnique( 0 )->getScheme();
        identity->getTechnique( 0 )->setScheme( oldScheme + 1 );
        require( identity->getRenderState() == beforeScheme,
                 "technique metadata edits can preserve the selected render snapshot" );
        inspection.resetCounts( *identity );
        require( inspection.manager->releaseUnusedResourcesOnUpdate() == 0 && identity->isLoaded() &&
                     identity->propertyReads == 1 && inspection.files->opens == 0,
                 "complete authoring comparison preserves edits beyond render-state revisions" );
        identity->getTechnique( 0 )->setScheme( oldScheme );
        require( inspection.manager->releaseUnusedResources() == 1 && !identity->isLoaded(),
                 "restored non-rendering metadata immediately becomes reclaimable in a forced sweep" );

        identity->load( nullptr );
        std::filesystem::remove( inspection.path );
        inspection.resetCounts( *identity );
        require( inspection.manager->releaseUnusedResourcesOnUpdate() == 0 && identity->isLoaded() &&
                     identity->propertyReads == 1 && inspection.files->opens > 0,
                 "a missing source preserves the last clean material graph" );
        const auto failedSourceOpens = inspection.files->opens;
        for( u32 frame = 0; frame < 100; ++frame )
            inspection.manager->releaseUnusedResourcesOnUpdate();
        require( identity->propertyReads == 1 && inspection.files->opens == failedSourceOpens,
                 "missing sources defer both serialization and filesystem retries" );
        inspection.writeSource();
        require( inspection.manager->releaseUnusedResources() == 1 && !identity->isLoaded(),
                 "a restored source permits forced reclamation immediately" );

        identity->load( nullptr );
        inspection.resetCounts( *identity );
        identity->rejectInspection = true;
        require( inspection.manager->releaseUnusedResourcesOnUpdate() == 0 && identity->isLoaded(),
                 "authoring inspection errors preserve the resident graph" );
        for( u32 frame = 0; frame < 100; ++frame )
            inspection.manager->releaseUnusedResourcesOnUpdate();
        require( identity->propertyReads == 1 && inspection.files->opens == 0,
                 "failed authoring inspection is not retried on every idle update" );
        identity->rejectInspection = false;
        require( inspection.manager->releaseUnusedResources() == 1 && !identity->isLoaded(),
                 "forced inspection retries immediately after a temporary error clears" );
        std::puts( "Automatic material residency retry, complete authoring checks and source I/O counters passed." );
    }

    template <class Fixture> void benchmark( Fixture &fixture )
    {
        InspectionFixture<Fixture> inspection( fixture );
        auto material = inspection.load();
        auto *identity = material.get();
        const auto oldRoughness = material->getRoughness();
        material->setRoughness( oldRoughness == 0.75f ? 0.25f : 0.75f );
        material = nullptr;
        constexpr u32 attempts = 1000;
        inspection.resetCounts( *identity );
        const auto forcedStart = std::chrono::steady_clock::now();
        for( u32 frame = 0; frame < attempts; ++frame )
            require( inspection.manager->releaseUnusedResources() == 0,
                     "unthrottled benchmark retains unsaved authoring" );
        const auto forcedEnd = std::chrono::steady_clock::now();
        const auto forcedReads = identity->propertyReads;
        const auto forcedOpens = inspection.files->opens;
        require( forcedReads == attempts && forcedOpens == 0,
                 "forced comparison rebuilds each document without unnecessary source probes" );
        inspection.resetCounts( *identity );
        const auto automaticStart = std::chrono::steady_clock::now();
        for( u32 frame = 0; frame < attempts; ++frame )
            require( inspection.manager->releaseUnusedResourcesOnUpdate() == 0,
                     "automatic benchmark retains unsaved authoring" );
        const auto automaticEnd = std::chrono::steady_clock::now();
        require( identity->isLoaded() && inspection.files->opens == 0,
                 "automatic benchmark keeps dirty data without touching its source" );
        std::printf( "material_residency attempts=%u forced_ms=%.6f automatic_ms=%.6f "
                     "forced_document_reads=%u automatic_document_reads=%u "
                     "forced_source_opens=%u automatic_source_opens=%u\n", attempts,
                     std::chrono::duration<double, std::milli>( forcedEnd - forcedStart ).count(),
                     std::chrono::duration<double, std::milli>( automaticEnd - automaticStart ).count(),
                     forcedReads, identity->propertyReads, forcedOpens, inspection.files->opens );
    }

    /** Reuse the real factory/state/file services of MaterialLoadSequenceTests. */
    template <class Fixture> void run( Fixture &fixture )
    {
        auto scheduler = WorldStreamingScheduler::shared();
        const auto source = size_t( StreamMemoryClass::SourceCpu );
        const auto beforeManager = scheduler->stats().used[source];
        auto manager = make_ptr<ClawMaterialManager>();
        manager->load( nullptr );
        // Measure the manager before constructing caller-owned document and
        // String fixtures, whose retained character slabs are also budgeted.
        const auto managerInventoryBytes = scheduler->stats().used[source] - beforeManager;
        const auto path = fixture.root / "resident_city_surface.mat";
        const String file = path.generic_string().c_str();
        // Use the serialized scheme/pass/slot shape of authored .mat assets.
        // A bare materialType document intentionally has no authored texture
        // slots; getTextureUnit(0) cannot provide a facade for an absent slot.
        auto document = make_ptr<Properties>();
        document->setProperty( "materialType", 0 );
        auto technique = make_ptr<Properties>(); technique->setName( "schemes" );
        technique->setProperty( "scheme", 0 );
        auto pass = make_ptr<Properties>(); pass->setName( "passes" );
        auto slot = make_ptr<Properties>(); slot->setName( "textures" );
        pass->addChild( slot ); technique->addChild( pass ); document->addChild( technique );
        const auto sourceDocument = DataUtil::toString( document.get() );
        {
            std::ofstream output( path );
            output << sourceDocument.c_str();
            require( output.good(), "material residency source fixture is writable" );
        }
        const auto baseline = scheduler->stats();
        String error;
        auto material = dynamic_pointer_cast<ClawMaterial>( manager->loadFromFile( file ) );
        require( material && material->isLoaded(), "file-backed material loads through its manager" );
        require( scheduler->stats().used[source] > baseline.used[source],
                 "material authoring descriptors and render versions retain SourceCpu leases" );
        const auto uuid = material->getHandle()->getUUIDAsString();
        const auto name = material->getName();
        const auto fileId = material->getFileSystemId();
        auto *identity = material.get();
        require( manager->releaseUnusedResources() == 0,
                 "external material strong owners pin the resident graph" );
        WeakPtr<ClawMaterial> weakMaterial = material;
        material = nullptr;
        require( manager->releaseUnusedResources() == 0 && identity->isLoaded(),
                 "external material weak owners pin the resident graph" );
        weakMaterial = nullptr;
        {
            ISharedObject::ScopedLoadLock loadPin( identity );
            require( manager->releaseUnusedResources() == 0,
                     "explicit material load locks pin otherwise unowned residency" );
        }

        // Keep one detached immutable draw version while all authoring owners
        // disappear. It may outlive eviction without losing its CPU accounting.
        // A snapshot also owns admitted lease/control storage. Measure its full
        // footprint independently of the material graph and retained pool slots.
        const auto beforeSnapshotProbe = scheduler->stats().used[source];
        auto snapshotProbe = MaterialRenderState::create();
        const auto snapshotReservationBytes = scheduler->stats().used[source] - beforeSnapshotProbe;
        require( snapshotReservationBytes > sizeof( MaterialRenderState ),
                 "snapshot admission includes its payload and lease/control storage" );
        snapshotProbe.reset();
        require( scheduler->stats().used[source] == beforeSnapshotProbe,
                 "an independent snapshot releases its complete admitted footprint" );
        auto drawVersion = identity->getRenderState();
        require( drawVersion && manager->releaseUnusedResources( 0 ) == 0,
                 "zero-sized sweeps do no work" );
        for( const auto &texture : drawVersion->textures )
            require( !texture, "the retained residency snapshot has no separately owned texture dependencies" );
        require( manager->releaseUnusedResources( 1 ) == 1 && !identity->isLoaded(),
                 "a sole cached file-backed material releases its graph" );
        require( identity->getHandle()->getUUIDAsString() == uuid && identity->getFilePath() == file &&
                     identity->getFileSystemId() == fileId && identity->getName() == name,
                 "material retirement preserves UUID, name, path and filesystem identity" );
        require( scheduler->stats().used[source] >= baseline.used[source] + snapshotReservationBytes,
                 "a retained draw version remains charged after material eviction" );
        std::weak_ptr<const MaterialRenderState> retiredWeak = drawVersion;
        const auto beforeDrawRelease = scheduler->stats().used[source];
        drawVersion.reset();
        require( retiredWeak.expired() && scheduler->stats().used[source] + snapshotReservationBytes == beforeDrawRelease,
                 "final draw retirement returns its complete snapshot admission while cached wrappers and pool capacity remain charged" );
        material = dynamic_pointer_cast<ClawMaterial>( manager->loadResource( name ) );
        require( material.get() == identity && material->isLoaded(),
                 "normal manager reload rehydrates the same retired identity" );
        require( manager->createOrRetrieve( file ).first.get() == identity,
                 "normal path-based create-or-retrieve preserves the same cached material identity" );

        for( u32 kind = 0; kind < 3; ++kind )
        {
            SmartPtr<IMaterialNode> pin;
            if( kind == 0 ) pin = material->getTechnique( 0 );
            if( kind == 1 ) pin = material->getTechnique( 0 )->getPasses().front();
            if( kind == 2 )
            {
                auto pass = dynamic_pointer_cast<MaterialPass>( material->getTechnique( 0 )->getPasses().front() );
                pin = pass->getTextureUnit( 0 );
            }
            require( pin != nullptr, "material residency graph pin exists" );
            auto *nodeIdentity = pin.get();
            material = nullptr;
            require( manager->releaseUnusedResources() == 0,
                     "external technique, pass and texture-unit strong owners pin resident materials" );
            WeakPtr<IMaterialNode> weakPin = pin;
            pin = nullptr;
            require( manager->releaseUnusedResources() == 0,
                     "external technique, pass and texture-unit weak owners pin resident materials" );
            weakPin = nullptr;
            {
                ISharedObject::ScopedLoadLock loadPin( nodeIdentity );
                require( manager->releaseUnusedResources() == 0,
                         "explicit technique, pass and facade load locks pin material residency" );
            }
            material = dynamic_pointer_cast<ClawMaterial>( manager->getByName( name ) );
        }
        const auto oldRoughness = material->getRoughness();
        material->setRoughness( oldRoughness == 0.75f ? 0.25f : 0.75f );
        material = nullptr;
        require( manager->releaseUnusedResources() == 0 && identity->isLoaded(),
                 "unsaved authored material edits survive automatic sweeps" );
        material = dynamic_pointer_cast<ClawMaterial>( manager->getById( uuid ) );
        material->setRoughness( oldRoughness );
        material = nullptr;
        require( manager->releaseUnusedResources() == 1,
                 "restoring the source values makes an otherwise unused material reclaimable" );

        // Admission rejects before source parsing and never publishes an empty
        // loaded graph. The existing cached identity can retry once capacity returns.
        const auto retired = scheduler->stats();
        require( scheduler->setBudget( StreamMemoryClass::SourceCpu,
                                      retired.used[source] + 8 * sizeof( MaterialRenderState ), error ),
                 "tight material SourceCpu budget applies" );
        bool denied = false;
        try { manager->loadFromFile( file ); }
        catch( const std::exception & ) { denied = true; }
        require( denied && !identity->isLoaded(),
                 "material source admission denial leaves a retryable unloaded resource" );
        if( identity->getNumTechniques() != 0 || scheduler->stats().used[source] != retired.used[source] )
            std::fprintf(stderr, "denied graph: techniques=%u source=%llu before=%llu\n",
                identity->getNumTechniques(), static_cast<unsigned long long>(scheduler->stats().used[source]),
                static_cast<unsigned long long>(retired.used[source]));
        require( identity->getNumTechniques() == 0 && scheduler->stats().used[source] == retired.used[source],
                 "denial after graph allocation unwinds techniques and returns all snapshot reservations" );
        require( scheduler->setBudget( StreamMemoryClass::SourceCpu, baseline.limits[source], error ),
                 "material SourceCpu budget restores" );
        material = dynamic_pointer_cast<ClawMaterial>( manager->loadFromFile( file ) );
        require( material.get() == identity && material->isLoaded(),
                 "returning capacity permits manager file-load retry on the same resource" );
        material = nullptr;
        std::filesystem::remove( path );
        require( manager->releaseUnusedResources() == 0 && identity->isLoaded(),
                 "missing reload sources preserve the last resident graph" );
        auto procedural = dynamic_pointer_cast<ClawMaterial>( manager->create( "city_runtime_material" ) );
        procedural->load( nullptr );
        auto *proceduralIdentity = procedural.get();
        procedural = nullptr;
        require( manager->releaseUnusedResources() == 0 && proceduralIdentity->isLoaded(),
                 "generated materials with no reloadable source remain resident" );
        {
            std::ofstream output( path, std::ios::trunc );
        }
        require( manager->releaseUnusedResources() == 0 && identity->isLoaded(),
                 "an empty reload source preserves the last resident material graph" );
        {
            std::ofstream output( path );
            output << sourceDocument.c_str();
        }
        u32 roundRobinReleased = 0;
        for( u32 candidate = 0; candidate < 2; ++candidate )
            roundRobinReleased += manager->releaseUnusedResources( 1 );
        require( roundRobinReleased == 1 && !identity->isLoaded() && proceduralIdentity->isLoaded(),
                 "bounded round-robin scans reach reclaimable assets while preserving procedural neighbors" );
        proceduralIdentity->unload(nullptr);
        const auto beforeManagerUnload = scheduler->stats().used[source];
        manager->unload( nullptr );
        manager = nullptr;
        require( scheduler->stats().used[source] + managerInventoryBytes <= beforeManagerUnload,
                 "material manager teardown frees its inventory and wrappers while factory-owned retained capacity stays charged" );
        std::puts( "Material residency, graph pins, dirty authoring, budget denial/retry and retained draw leases passed." );
        runAutomaticRetry( fixture );
    }
}
