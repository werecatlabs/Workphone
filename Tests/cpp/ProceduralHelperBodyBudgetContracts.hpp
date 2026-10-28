#pragma once
#include "CityInventoryBudgetContracts.hpp"
#include <WPProcedural/WPProceduralPipeline.hpp>
#include <WPProcedural/WPTextureForge.hpp>
#include <WPProcedural/WPBuildingGenerator.hpp>
#include <WPProcedural/CTextureForge.hpp>
#include <Workphone/System/FactoryTemplate.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/Scene/Components/ProceduralSurfaceTexture.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <memory>

namespace procedural_helper_body_budget_contracts
{
    using namespace city_inventory_budget_contracts;

    inline bool sameMesh( const ProceduralMesh &a, const ProceduralMesh &b )
    {
        if( a.vertices.size() != b.vertices.size() || a.indices.size() != b.indices.size() ||
            a.boundsValid != b.boundsValid ) return false;
        for( size_t i = 0; i < a.vertices.size(); ++i )
        {
            const auto &x = a.vertices[i], &y = b.vertices[i];
            if( x.position != y.position || x.normal != y.normal || x.uv != y.uv || x.mask != y.mask ) return false;
        }
        for( size_t i = 0; i < a.indices.size(); ++i ) if( a.indices[i] != b.indices[i] ) return false;
        return true;
    }

    inline bool sameMeshes( const Array<ProceduralMesh> &a, const Array<ProceduralMesh> &b )
    {
        if( a.size() != b.size() ) return false;
        for( size_t i = 0; i < a.size(); ++i ) if( !sameMesh( a[i], b[i] ) ) return false;
        return true;
    }

    inline bool sameBuilding( const BuildingResult &a, const BuildingResult &b )
    {
        return a.position == b.position && a.wallKeyVariant == b.wallKeyVariant &&
               sameMesh( a.body, b.body ) && sameMesh( a.roof, b.roof ) && sameMesh( a.parapet, b.parapet ) &&
               sameMeshes( a.windows, b.windows ) && sameMeshes( a.details, b.details ) &&
               sameMeshes( a.interiors, b.interiors );
    }

    template<class Buffer> inline bool sameValues( const Buffer &a, const Buffer &b )
    {
        if( a.size() != b.size() ) return false;
        for( size_t i = 0; i < a.size(); ++i ) if( a[i] != b[i] ) return false;
        return true;
    }

    inline bool sameTexture( const TextureBuffer &a, const TextureBuffer &b )
    { return a.width == b.width && a.height == b.height && sameValues( a.pixels, b.pixels ); }

    inline bool sameBake( const SurfaceBakeResult &a, const SurfaceBakeResult &b )
    {
        return sameTexture( a.albedo, b.albedo ) && sameTexture( a.normal, b.normal ) &&
               sameTexture( a.orm, b.orm ) && a.height.width == b.height.width &&
               a.height.height == b.height.height && sameValues( a.height.values, b.height.values );
    }

    inline BuildingSpec buildingSpec()
    {
        BuildingSpec spec; spec.floors = 1; spec.width = 4; spec.depth = 4;
        return spec;
    }

    inline SurfaceBakeParams bakeParams()
    { SurfaceBakeParams params; params.size = 8; return params; }

    template<class T> inline std::unique_ptr<T> admittedBody( Budget &budget, u32 seed )
    {
        static_assert( std::is_base_of_v<SourceBudgetObject, T> );
        const auto baseline = budget.used();
        const auto bytes = sourceBudgetStorageBytes( sizeof( T ), alignof( std::max_align_t ) );
        budget.set( baseline + bytes - 1 );
        denied( [&]{ auto value = std::make_unique<T>( seed ); } );
        check( budget.used() == baseline, "One-byte-short helper body admission publishes no object or lease" );
        budget.set( baseline + bytes );
        auto value = std::make_unique<T>( seed );
        check( budget.used() == baseline + bytes,
               "Procedural helper scalar construction charges exactly one body including all embedded values" );
        return value;
    }

    template<class T> inline void retireBody( Budget &budget, std::unique_ptr<T> &value )
    {
        const auto before = budget.used();
        const auto bytes = sourceBudgetStorageBytes( sizeof( T ), alignof( std::max_align_t ) );
        budget.set( before ); value.reset();
        check( budget.used() == before - bytes, "Procedural helper destruction retires its sole body lease at the cap" );
        budget.set( budget.original );
    }

    inline void adapterBodyAdmission( Budget &budget )
    {
        static_assert( std::is_base_of_v<SourceBudgetObject, CTextureForge> );
        const auto baseline = budget.used();
        const auto bytes = sourceBudgetStorageBytes( sizeof( CTextureForge ), alignof( std::max_align_t ) );
        budget.set( baseline + bytes - 1 );
        denied( []{
            auto memory = CTextureForge::operator new( sizeof( CTextureForge ) );
            CTextureForge::operator delete( memory );
        } );
        check( budget.used() == baseline, "Denied texture adapter scalar body leaves no lease" );
        budget.set( baseline + bytes );
        // Isolate body allocation from reference metadata and any existing
        // engine factory pools; the embedded forge is part of this one body.
        auto release = []( void *memory ) { CTextureForge::operator delete( memory ); };
        std::unique_ptr<void, decltype( release )> memory(
            CTextureForge::operator new( sizeof( CTextureForge ) ), release );
        check( budget.used() == baseline + bytes, "Texture adapter scalar storage admits exactly one body including its embedded forge" );
        budget.set( budget.used() ); memory.reset();
        check( budget.used() == baseline, "Texture adapter scalar body retires through matching Source deletion at the cap" );
        budget.set( budget.original );

        FactoryTemplate<CTextureForge> factory;
        const auto factoryBaseline = budget.used();
        const auto factoryBytes = sourceBudgetStorageBytes( sizeof( CTextureForge ), alignof( CTextureForge ) );
        budget.set( factoryBaseline + factoryBytes - 1 );
        denied( [&]{ auto slot = factory.allocateMemory(); factory.freeMemory( slot ); } );
        check( budget.used() == factoryBaseline, "Denied texture adapter factory body leaves no lease" );
        budget.set( factoryBaseline + factoryBytes );
        auto freeSlot = [&]( void *slot ) { factory.freeMemory( slot ); };
        std::unique_ptr<void, decltype( freeSlot )> slot( factory.allocateMemory(), freeSlot );
        check( budget.used() == factoryBaseline + factoryBytes, "Texture adapter factory storage admits its exact physical body alignment" );
        budget.set( budget.used() ); slot.reset();
        check( budget.used() == factoryBaseline, "Texture adapter factory body retires through matching Source free at the cap" );
        budget.set( budget.original );
    }

    struct SurfaceRuntime
    {
        SmartPtr<core::IApplicationManager> previous = core::IApplicationManager::instancePtr();
        SmartPtr<core::ApplicationManager> app = make_ptr<core::ApplicationManager>();
        SmartPtr<FactoryManager> factories = make_ptr<FactoryManager>();
        SmartPtr<FactoryTemplate<CTextureForge>> producer = make_ptr<FactoryTemplate<CTextureForge>>();
        TaskId previousTask = Thread::getCurrentTask();
        SurfaceRuntime()
        {
            factories->load( nullptr ); app->setFactoryManager( factories );
            producer->setObjectTypeName( "ITextureForge" );
            producer->setObjectTypeHash( StringUtil::getHash64( "ITextureForge" ) );
            factories->addFactory( producer ); producer->setPool( nullptr );
            check( factories->getFactoryByName( "ITextureForge" ) == producer &&
                   producer->isObjectDerivedFromByInfo( ITextureForge::typeInfo() ) && !producer->hasPool(),
                   "Surface fixture registers the real texture adapter factory without a pre-admitted pool" );
            core::IApplicationManager::setInstance( app );
            Thread::setCurrentTask( TaskId::Application );
        }
        ~SurfaceRuntime()
        {
            factories->unload( nullptr ); app->setFactoryManager( nullptr );
            core::IApplicationManager::setInstance( previous ); Thread::setCurrentTask( previousTask );
        }
    };

    class LegacySwallowingForge : public CTextureForge
    {
    public:
        SurfaceBakeResult bakeSurface( SurfaceTag tag, const SurfaceBakeParams &params ) override
        {
            try { return CTextureForge::bakeSurface( tag, params ); }
            catch( const std::bad_alloc & ) { return {}; }
        }
    };

    inline void componentRetry( Budget &budget, u32 seed, u32 replacement )
    {
        SurfaceRuntime runtime;
        auto component = make_ptr<scene::ProceduralSurfaceTexture>();
        auto properties = component->getProperties();
        properties->setProperty( "Resolution", u32( 16 ) ); properties->setProperty( "Seed", seed );
        component->setProperties( properties );
        check( component->regenerate(), "Surface component fixture bakes through the registered real factory" );
        SurfaceBakeParams params; params.size = 16;
        WPTextureForge reference( seed );
        const auto expected = reference.bakeSurface( SurfaceTag::Concrete, params );
        check( sameBake( component->getResult(), expected ), "Surface component fixture retains a complete seeded result" );
        const auto albedo = component->getResult().albedo.data();
        const auto normal = component->getResult().normal.data();
        const auto orm = component->getResult().orm.data();
        const auto height = component->getResult().height.values.data();
        component->setTextureForge( nullptr );
        check( runtime.producer->getInstanceObjects().empty(), "Cleared service retires the prior factory-owned texture adapter" );
        const auto baseline = budget.used(); budget.set( baseline );
        const auto denials = sourceBudgetDenialCount();
        check( !component->regenerate() && sourceBudgetDenialCount() != denials && budget.used() == baseline &&
               runtime.producer->getInstanceObjects().empty() &&
               component->getResult().albedo.data() == albedo && component->getResult().normal.data() == normal &&
               component->getResult().orm.data() == orm && component->getResult().height.values.data() == height &&
               sameBake( component->getResult(), expected ),
               "Cold texture adapter admission failure returns false and preserves every previous output buffer" );
        budget.set( budget.original );
        String error; component->getProperties()->getPropertyValue( "Generation Error", error );
        check( error == "Out of memory", "Surface component reports memory denial with an allocation-free short diagnostic" );
        component->update();
        check( runtime.producer->getInstanceObjects().size() == 1 && sameBake( component->getResult(), expected ),
               "Application update retries denied cold service creation without requiring another property edit" );
        budget.set( budget.used() );
        const auto cleanDenials = sourceBudgetDenialCount(); component->update();
        check( sourceBudgetDenialCount() == cleanDenials && sameBake( component->getResult(), expected ),
               "Successful regeneration clears dirty status so a full-cap update does not attempt another bake" );
        budget.set( budget.original );

        properties->setProperty( "Seed", replacement ); component->setProperties( properties );
        const auto payloadBaseline = budget.used(); budget.set( payloadBaseline );
        check( !component->regenerate() && budget.used() == payloadBaseline && sameBake( component->getResult(), expected ),
               "Genuine texture payload denial also preserves the component's last complete result" );
        budget.set( budget.original ); component->update();
        WPTextureForge replacementReference( replacement );
        const auto changed = replacementReference.bakeSurface( SurfaceTag::Concrete, params );
        check( sameBake( component->getResult(), changed ), "Application update retries denied texture payload admission with the requested new seed" );

        auto legacy = make_ptr<LegacySwallowingForge>(); component->setTextureForge( legacy );
        const auto legacyBaseline = budget.used(); budget.set( legacyBaseline );
        check( !component->regenerate() && budget.used() == legacyBaseline && sameBake( component->getResult(), changed ),
               "A legacy service swallowing Source denial cannot publish an empty result over the component's last complete bake" );
        budget.set( budget.original ); component->update();
        check( sameBake( component->getResult(), changed ), "Component retry recovers after a legacy service swallowed admission denial" );
        budget.set( budget.used() );
        const auto finalDenials = sourceBudgetDenialCount(); component->update();
        check( sourceBudgetDenialCount() == finalDenials, "Recovered legacy-service bake clears component retry status" );
        budget.set( budget.original );
    }

    inline void run()
    {
        Budget budget;
        constexpr u32 seed = 1337, replacement = 7331;
        const auto spec = buildingSpec(); const auto params = bakeParams();
        const Vector3<real_Num> origin( 1,2,3 );
        {
            auto textures = admittedBody<WPTextureForge>( budget, seed );
            check( textures->getSeed() == seed, "Admitted texture helper retains its requested seed" );
            budget.set( budget.original );
            WPTextureForge reference( seed );
            const auto actual = textures->bakeSurface( SurfaceTag::Concrete, params );
            const auto expected = reference.bakeSurface( SurfaceTag::Concrete, params );
            check( actual.albedo.width == params.size && !actual.albedo.pixels.empty() && sameBake( actual, expected ),
                   "Heap texture helper produces the same complete PBR bake as the existing seeded value path" );
            retireBody( budget, textures );
            check( !actual.albedo.pixels.empty() && sameBake( actual, expected ),
                   "Generated texture buffers remain valid after helper destruction" );
        }
        {
            auto buildings = admittedBody<WPBuildingGenerator>( budget, seed );
            budget.set( budget.original );
            WPBuildingGenerator reference( seed );
            const auto actual = buildings->generate( spec, origin );
            const auto expected = reference.generate( spec, origin );
            check( actual.body.hasGeometry() && !actual.windows.empty() && sameBuilding( actual, expected ),
                   "Heap building helper preserves complete seeded assembly output" );
            retireBody( budget, buildings );
            check( actual.body.hasGeometry() && sameBuilding( actual, expected ),
                   "Generated building assemblies retain ownership after helper destruction" );
        }
        {
            auto pipeline = admittedBody<WPProceduralPipeline>( budget, seed );
            const auto fullBody = budget.used(); budget.set( fullBody );
            pipeline->initialize(); pipeline->update( real_Num( .016 ) );
            check( pipeline->getSeed() == seed && pipeline->noise().getSeed() == seed &&
                   pipeline->textures().getSeed() == seed && pipeline->sky().getState().timeOfDay == 14 &&
                   budget.used() == fullBody,
                   "Pipeline initialization and embedded noise, texture, building and sky helpers add no nested body leases" );
            pipeline->setSeed( replacement );
            check( pipeline->getSeed() == replacement && pipeline->noise().getSeed() == replacement &&
                   pipeline->textures().getSeed() == replacement && budget.used() == fullBody,
                   "Pipeline reseeding replaces fixed embedded helper values at the full cap" );
            budget.set( budget.original );
            WPNoise noise( replacement ); WPTextureForge textures( replacement ); WPBuildingGenerator buildings( replacement );
            check( pipeline->noise().noise3( .31f,.73f,1.29f ) == noise.noise3( .31f,.73f,1.29f ) &&
                   sameBake( pipeline->textures().bakeSurface( SurfaceTag::Concrete, params ),
                             textures.bakeSurface( SurfaceTag::Concrete, params ) ) &&
                   sameBuilding( pipeline->buildings().generate( spec, origin ), buildings.generate( spec, origin ) ),
                   "Reseeded heap pipeline routes usable deterministic noise, PBR and building generation through its embedded helpers" );
            // Retire while initialized so the existing shutdown path is covered.
            retireBody( budget, pipeline );
        }
        {
            const auto baseline = budget.used(); budget.set( baseline );
            WPTextureForge textures( seed ), textureCopy( textures ), textureAssigned( replacement );
            textureAssigned = textureCopy;
            WPBuildingGenerator buildings( seed ), buildingCopy( buildings ), buildingAssigned( replacement );
            buildingAssigned = buildingCopy;
            WPProceduralPipeline pipeline( seed ); pipeline.initialize();
            WPProceduralPipeline pipelineCopy( pipeline ), pipelineAssigned( replacement );
            pipelineAssigned = pipelineCopy;
            pipelineAssigned.update( real_Num( .016 ) );
            check( textureCopy.getSeed() == seed && textureAssigned.getSeed() == seed &&
                   pipelineCopy.getSeed() == seed && pipelineAssigned.getSeed() == seed &&
                   budget.used() == baseline, "Stack construction, value copies and assignments add no procedural helper body leases" );
            pipeline.setSeed( replacement );
            check( pipelineCopy.getSeed() == seed && pipelineAssigned.noise().getSeed() == seed &&
                   budget.used() == baseline, "Copied pipeline values retain independent embedded seed state without heap ownership" );
            budget.set( budget.original );
            check( sameBake( textures.bakeSurface( SurfaceTag::Concrete, params ),
                             textureAssigned.bakeSurface( SurfaceTag::Concrete, params ) ) &&
                   sameBuilding( buildings.generate( spec, origin ), buildingAssigned.generate( spec, origin ) ) &&
                   sameBake( pipelineCopy.textures().bakeSurface( SurfaceTag::Concrete, params ),
                             textureCopy.bakeSurface( SurfaceTag::Concrete, params ) ) &&
                   sameBuilding( pipelineAssigned.buildings().generate( spec, origin ), buildingCopy.generate( spec, origin ) ),
                   "Stack and copied procedural helpers preserve deterministic output independently of scalar heap admission" );
            budget.set( budget.used() );
            pipeline.shutdown(); pipelineCopy.shutdown(); pipelineAssigned.shutdown();
            check( budget.used() == baseline, "Fixed-value helper shutdown requires no allocation at the full cap" );
            budget.set( budget.original );
        }
        adapterBodyAdmission( budget );
        {
            auto concrete = make_ptr<CTextureForge>();
            SmartPtr<ITextureForge> service = concrete;
            service->setSeed( seed );
            const auto actual = service->bakeSurface( SurfaceTag::Concrete, params );
            WPTextureForge reference( seed );
            const auto expected = reference.bakeSurface( SurfaceTag::Concrete, params );
            check( service->getSeed() == seed && !actual.albedo.pixels.empty() && sameBake( actual, expected ),
                   "Active texture interface preserves complete deterministic seeded PBR output" );
            const auto baseline = budget.used(); budget.set( baseline );
            service->setSeed( replacement );
            check( service->getSeed() == replacement && budget.used() == baseline,
                   "Texture interface reseeding replaces its embedded fixed-value forge without another body lease" );
            denied( [&]{ service->bakeSurface( SurfaceTag::Concrete, params ); } );
            check( sameBake( actual, expected ) && budget.used() == baseline,
                   "Denied interface bake leaves independently retained previous textures intact" );
            budget.set( budget.original );
            WPTextureForge replacementReference( replacement );
            check( sameBake( service->bakeSurface( SurfaceTag::Concrete, params ),
                             replacementReference.bakeSurface( SurfaceTag::Concrete, params ) ),
                   "Texture interface bake retries with deterministic replacement-seed output" );
            budget.set( budget.used() ); service = nullptr; concrete = nullptr;
            check( sameBake( actual, expected ), "Escaped texture buffers remain usable after full-cap interface-owner release" );
            budget.set( budget.original );
        }
        componentRetry( budget, seed, replacement );
    }
}
