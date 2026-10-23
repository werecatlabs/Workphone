#pragma once

#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawMaterialManager.hpp>
#include <Workphone/Graphics/MaterialPass.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
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

    /** Reuse the real factory/state/file services of MaterialLoadSequenceTests. */
    template <class Fixture> void run( Fixture &fixture )
    {
        auto manager = make_ptr<ClawMaterialManager>();
        manager->load( nullptr );
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
        auto scheduler = WorldStreamingScheduler::shared();
        const auto source = size_t( StreamMemoryClass::SourceCpu );
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
        auto drawVersion = identity->getRenderState();
        require( drawVersion && manager->releaseUnusedResources( 0 ) == 0,
                 "zero-sized sweeps do no work" );
        require( manager->releaseUnusedResources( 1 ) == 1 && !identity->isLoaded(),
                 "a sole cached file-backed material releases its graph" );
        require( identity->getHandle()->getUUIDAsString() == uuid && identity->getFilePath() == file &&
                     identity->getFileSystemId() == fileId && identity->getName() == name,
                 "material retirement preserves UUID, name, path and filesystem identity" );
        require( scheduler->stats().used[source] >= baseline.used[source] + sizeof( MaterialRenderState ),
                 "a retained draw version remains charged after material eviction" );
        std::weak_ptr<const MaterialRenderState> retiredWeak = drawVersion;
        drawVersion.reset();
        require( retiredWeak.expired() && scheduler->stats().used[source] == baseline.used[source],
                 "final draw retirement returns SourceCpu reservations while weak caches remain" );
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
        manager->unload( nullptr );
        manager = nullptr;
        require( scheduler->stats().used[source] == baseline.used[source],
                 "material manager teardown returns all resident descriptor and snapshot leases" );
        std::puts( "Material residency, graph pins, dirty authoring, budget denial/retry and retained draw leases passed." );
    }
}
