#pragma once
#include "CityInventoryBudgetContracts.hpp"
#include <WPProcedural/ProceduralModel.hpp>
#include <WPProcedural/CProceduralTerrain.hpp>
#include <WPProcedural/CRoad.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>

namespace procedural_mesh_builder_budget_contracts
{
    using namespace city_inventory_budget_contracts;
    inline void triangle( ProceduralMesh &mesh, float x = 0 )
    {
        mesh.addVertex( Vector3F( x,0,0 ) ); mesh.addVertex( Vector3F( x+1,0,0 ) );
        mesh.addVertex( Vector3F( x,0,1 ) ); mesh.addTriangle( 0,1,2 ); mesh.computeBoundingBox();
    }
    class RawAllocationFailureRule : public ModelRule
    {
    public:
        bool fail = true;
        void build( ProceduralMesh &mesh ) const override
        {
            // Models an ordinary heap failure without a Source admission denial.
            if( fail ) throw std::bad_alloc();
            BoxRule( Vector3F( 1,1,1 ), Vector3F( 8,0,0 ) ).build( mesh );
        }
    };
    class LegacySwallowingRule : public ModelRule
    {
    public:
        void build( ProceduralMesh &mesh ) const override
        {
            try { BoxRule().build( mesh ); }
            catch( const std::bad_alloc & ) {}
        }
    };
    class CountingTerrain : public CProceduralTerrain
    {
    public:
        mutable int reads = 0;
        MeshGeneratorDefault *generator = nullptr;
        mutable bool stop = false;
        Array<f32> getHeightData() const override
        {
            ++reads;
            if( stop ) { stop = false; generator->clear(); }
            return CProceduralTerrain::getHeightData();
        }
    };
    class ClearingRoad : public CRoad
    {
    public:
        MeshGeneratorDefault *generator = nullptr;
        mutable int reads = 0;
        Array<SmartPtr<IRoadNode>> getRoadNodes() const override
        {
            ++reads; generator->clear();
            return {};
        }
    };
    struct Runtime
    {
        SmartPtr<core::IApplicationManager> previous = core::IApplicationManager::instancePtr();
        SmartPtr<core::ApplicationManager> app = make_ptr<core::ApplicationManager>();
        SmartPtr<FactoryManager> factories = make_ptr<FactoryManager>();
        Runtime()
        {
            factories->load( nullptr ); app->setFactoryManager( factories );
            app->setMeshManager( make_ptr<MeshManager>() ); core::IApplicationManager::setInstance( app );
        }
        ~Runtime()
        {
            app->setMeshManager( nullptr ); app->setFactoryManager( nullptr );
            core::IApplicationManager::setInstance( previous );
        }
    };
    inline void run()
    {
        Budget budget;
        {
            ProceduralMesh source, target;
            source.vertices.reserve( 17 ); source.indices.reserve( 19 ); triangle( source ); triangle( target, 8 );
            const auto baseline = budget.used();
            const auto vertices = sourceBudgetStorageBytes( 17*sizeof( ProceduralVertex ), alignof( ProceduralVertex ) );
            budget.set( baseline + vertices );
            denied( [&]{ target = source; } );
            check( target.vertices[0].position.x == 8 && target.boundsValid && budget.used() == baseline,
                   "Late procedural index copy denial preserves all geometry and bounds" );
            budget.set( budget.original );
            ProceduralMesh copy( source );
            check( copy.vertices.capacity() == 17 && copy.indices.capacity() == 19 &&
                   copy.vertices.get_allocator().isSourceBudgeted(), "Procedural mesh copies admit spare capacity" );
            ProceduralMesh compact;
            compact.vertices.reserve( 3 ); compact.indices.reserve( 3 ); triangle( compact );
            compact.append( compact, Vector3F( 2,0,0 ) );
            check( compact.vertices.size() == 6 && compact.indices.size() == 6 && compact.indices[3] == 3 &&
                   compact.vertices[3].position.x == 2, "Procedural self-append survives buffer relocation" );
            ProceduralMesh partial;
            partial.vertices.reserve( 3 ); partial.indices.reserve( 2 );
            for( const auto &vertex : source.vertices ) partial.addVertex( vertex );
            budget.set( budget.used() );
            denied( [&]{ partial.addTriangle( 0,1,2 ); } );
            check( partial.indices.empty(), "Procedural triangle denial publishes no partial index triple" );
            denied( []{ auto mesh = std::make_unique<ProceduralMesh>(); } );
            compact.releaseStorage(); copy.releaseStorage();
            check( compact.vertices.capacity() == 0 && copy.indices.capacity() == 0,
                   "Procedural mesh storage physically retires at the cap" );
            budget.set( budget.original );
        }
        {
            ProceduralModelComponent source, target;
            source.rootRule = std::allocate_shared<BoxRule>( SourceBudgetAllocator<BoxRule>{} );
            target.rootRule = std::allocate_shared<BoxRule>( SourceBudgetAllocator<BoxRule>{} );
            source.bakedMesh.vertices.reserve( 17 ); source.bakedMesh.indices.reserve( 19 );
            triangle( source.bakedMesh, 3 ); triangle( target.bakedMesh, 10 ); target.dirty = false;
            const auto oldRule = target.rootRule;
            const auto baseline = budget.used();
            budget.set( baseline + sourceBudgetStorageBytes( 17*sizeof( ProceduralVertex ), alignof( ProceduralVertex ) ) );
            denied( [&]{ target = source; } );
            check( target.rootRule == oldRule && !target.dirty && target.bakedMesh.boundsValid &&
                   target.bakedMesh.vertices[0].position.x == 10 && target.bakedMesh.indices.size() == 3 &&
                   budget.used() == baseline,
                   "Late component mesh-copy denial preserves the rule, geometry and dirty status" );
            budget.set( budget.original ); target = source;
            check( target.rootRule == source.rootRule && target.dirty &&
                   target.bakedMesh.vertices.capacity() == 17 && target.bakedMesh.indices.capacity() == 19,
                   "Component assignment retries with admitted mesh spare capacity and shared rule identity" );
            budget.set( budget.used() );
            denied( []{ auto component = std::make_unique<ProceduralModelComponent>(); } );
            budget.set( budget.original );
        }
        {
            ProceduralModelComponent component;
            component.rootRule = std::allocate_shared<BoxRule>( SourceBudgetAllocator<BoxRule>{} );
            ProceduralModelSystem system; system.bake( component );
            check( !component.dirty && component.bakedMesh.hasGeometry(), "Procedural model fixture bakes" );
            const auto old = component.bakedMesh.vertices[0].position;
            component.dirty = true;
            budget.set( budget.used() );
            system.bake( component );
            check( component.dirty && component.bakedMesh.vertices[0].position == old && component.bakedMesh.hasGeometry(),
                   "Rule admission denial cannot publish a partial bake or clear retry status" );
            budget.set( budget.original ); system.bake( component );
            check( !component.dirty && component.bakedMesh.hasGeometry(), "Procedural model baking retries" );

            component.rootRule = std::allocate_shared<LegacySwallowingRule>( SourceBudgetAllocator<LegacySwallowingRule>{} );
            component.dirty = true;
            budget.set( budget.used() );
            system.bake( component );
            check( component.dirty && component.bakedMesh.vertices[0].position == old,
                   "Legacy swallowed admission denial preserves the last bake and its retry status" );
            budget.set( budget.original ); system.bake( component );
            check( !component.dirty && component.bakedMesh.hasGeometry(), "Legacy swallowed-denial baking retries" );

            auto composite = std::allocate_shared<CompositeRule>( SourceBudgetAllocator<CompositeRule>{} );
            auto first = std::allocate_shared<BoxRule>( SourceBudgetAllocator<BoxRule>{}, Vector3F( 1,1,1 ), Vector3F( 4,0,0 ) );
            auto last = std::allocate_shared<RawAllocationFailureRule>( SourceBudgetAllocator<RawAllocationFailureRule>{} );
            composite->add( first ); composite->add( last ); component.rootRule = composite; component.dirty = true;
            const auto denials = sourceBudgetDenialCount();
            system.bake( component );
            check( sourceBudgetDenialCount() == denials && component.dirty &&
                   component.bakedMesh.vertices.size() == 8 && component.bakedMesh.indices.size() == 36 &&
                   component.bakedMesh.vertices[0].position == old,
                   "Heap failure after a successful composite child preserves the last bake without an admission-denial epoch" );
            last->fail = false; system.bake( component );
            check( !component.dirty && component.bakedMesh.vertices.size() == 16 &&
                   component.bakedMesh.indices.size() == 72 && component.bakedMesh.vertices[0].position.x == 3.5f,
                   "Composite baking retries the complete rule tree after an ordinary heap failure" );

            CompositeRule graph;
            auto child = std::allocate_shared<BoxRule>( SourceBudgetAllocator<BoxRule>{} ); graph.add( child );
            budget.set( budget.used() );
            denied( [&]{ CompositeRule snapshot( graph ); } );
            check( graph.children.size() == 1, "Denied rule graph snapshot preserves shared children" );
            budget.set( budget.original );
        }
        {
            Runtime runtime;
            auto terrain = make_ptr<CountingTerrain>();
            terrain->setHeightmapResolution( Vector2I( 2,2 ) ); terrain->setSize( Vector3F( 1,1,1 ) );
            Array<f32> heights; heights.resize( 4 ); terrain->setHeightData( heights );
            auto generator = make_ptr<MeshGeneratorDefault>(); terrain->generator = generator.get();
            auto old = generator->generateFromTerrain( terrain );
            check( old && old->getNumSubMeshes() == 1, "CPU-only mesh generator fixture produces native buffers" );
            const auto reads = terrain->reads;
            budget.set( budget.used() );
            check( !generator->generateFromTerrain( terrain ) && terrain->reads == reads &&
                   generator->getLastGeneratedMesh() == old,
                   "Output inventory denial precedes producers and retains the last native mesh" );
            budget.set( budget.original );
            check( generator->generateFromTerrain( terrain ) != nullptr, "Mesh generation retries after slot admission" );
            auto retained = generator->getAllGeneratedMeshes();
            check( retained.size() == 2 && retained.get_allocator().isSourceBudgeted(), "Mesh caller inventory snapshots remain admitted" );
            terrain->stop = true;
            check( !generator->generateFromTerrain( terrain ) && generator->getAllGeneratedMeshes().empty() &&
                   generator->getGenerationStatus() == "Ready", "Reentrant clear cancels stale mesh publication" );
            auto road = make_ptr<ClearingRoad>(); road->generator = generator.get();
            Array<SmartPtr<IProceduralObject>> objects{ Allocator<SmartPtr<IProceduralObject>>( sourceBudget ) };
            objects.push_back( road );
            check( !generator->generateFromObjects( objects ) && road->reads == 1 &&
                   !generator->getLastGeneratedMesh() && generator->getAllGeneratedMeshes().empty() &&
                   generator->getGenerationStatus() == "Ready" && generator->getGenerationProgress() == 0,
                   "Road callback cancellation preserves Ready status when the cancelled producer returns no nodes" );
            Array<Vector3<real_Num>> positions, normals;
            Array<Vector2<real_Num>> uvs;
            Array<u32> indices;
            positions.push_back( Vector3<real_Num>( 0,0,0 ) ); positions.push_back( Vector3<real_Num>( 1,0,0 ) );
            positions.push_back( Vector3<real_Num>( 0,0,1 ) ); normals.resize( 3 ); uvs.resize( 3 );
            indices.push_back( 0 ); indices.push_back( 1 ); indices.push_back( 2 );
            for( const u64 extra : { 0ull,128ull,256ull,512ull,1024ull,2048ull,4096ull,8192ull } )
            {
                budget.set( budget.used() + extra );
                SmartPtr<IMesh> native;
                try { native = MeshUtil::createMesh( positions, normals, uvs, indices ); }
                catch( const std::exception & ) {}
                if( native )
                {
                    check( native->getNumSubMeshes() == 1, "Admitted native mesh creation is complete" );
                    native->unload( nullptr );
                }
                budget.set( budget.original );
            }
            budget.set( budget.used() ); generator->unload( nullptr );
            check( generator->getAllGeneratedMeshes().empty() && retained[0]->getNumSubMeshes() == 1,
                   "Full-cap mesh inventory retirement does not unload externally retained results" );
            budget.set( budget.original );
        }
    }
}
