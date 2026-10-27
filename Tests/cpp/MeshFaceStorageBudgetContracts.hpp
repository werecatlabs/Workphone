#pragma once
#include "CityInventoryBudgetContracts.hpp"
#include <WPProcedural/MeshFace.hpp>
#include <Workphone/System/FactoryTemplate.hpp>

namespace mesh_face_storage_budget_contracts
{
    using namespace city_inventory_budget_contracts;
    template <class T> inline u64 bytes( const Array<T> &values )
    {
        return values.capacity() ?
            sourceBudgetStorageBytes( values.capacity() * sizeof( T ), alignof( T ) ) : 0;
    }

    inline void run()
    {
        static_assert( std::is_base_of_v<SourceBudgetObject, MeshFace> );
        Budget budget;
        {
            // Raw body checks isolate scalar/factory admission from the shared
            // object's separately admitted, reusable metadata-pool slabs.
            FactoryTemplate<MeshFace> factory;
            const auto before = budget.used();
            const auto scalarBytes = sourceBudgetStorageBytes( sizeof( MeshFace ), alignof( std::max_align_t ) );
            budget.set( before + scalarBytes - 1 );
            denied( []{ auto storage = MeshFace::operator new( sizeof( MeshFace ) ); MeshFace::operator delete( storage ); } );
            check( budget.used() == before, "Denied face body leaves no partial allocation" );
            budget.set( before + scalarBytes );
            auto scalar = MeshFace::operator new( sizeof( MeshFace ) );
            check( budget.used() == before + scalarBytes, "Scalar face body admits its complete physical allocation" );
            MeshFace::operator delete( scalar );
            check( budget.used() == before, "Scalar face body retires at the cap" );

            const auto factoryBytes = sourceBudgetStorageBytes( sizeof( MeshFace ), alignof( MeshFace ) );
            budget.set( before + factoryBytes - 1 );
            denied( [&]{ auto storage = factory.allocateMemory(); factory.freeMemory( storage ); } );
            budget.set( before + factoryBytes );
            auto storage = factory.allocateMemory();
            check( budget.used() == before + factoryBytes, "Factory raw face storage follows the shared body marker" );
            factory.freeMemory( storage );
            check( budget.used() == before, "Factory raw face storage releases without replacement admission" );
            budget.set( budget.original );

            { MeshFace warmMetadata; }
            const auto lifetimeBefore = budget.used();
            budget.set( lifetimeBefore + scalarBytes );
            std::unique_ptr<ISharedObject> object( new MeshFace );
            check( budget.used() == lifetimeBefore + scalarBytes,
                   "Complete scalar face lifetime reuses metadata slots while admitting its body" );
            const auto epoch = sourceBudgetDenialCount();
            object.reset();
            check( budget.used() == lifetimeBefore && sourceBudgetDenialCount() == epoch,
                   "Virtual base deletion selects the matching admitted face body release" );
            budget.set( budget.original );
        }
        {
            MeshFace face;
            check( face.Points.get_allocator().isSourceBudgeted() && face.UVs.get_allocator().isSourceBudgeted(),
                   "New face payloads retain the public Array types with SourceCpu policy" );
            face.Points.reserve( 5 ); face.Points.push_back( Vector3F( 1, 2, 3 ) );
            face.UVs.reserve( 7 ); face.UVs.push_back( Vector2F( .1f, .2f ) );
            const auto oldPoints = face.Points.data();
            const auto oldUVs = face.UVs.data();
            const auto oldBytes = bytes( face.Points ) + bytes( face.UVs );

            Array<Vector3F> points;
            points.reserve( 37 ); points.push_back( Vector3F( 4, 5, 6 ) );
            Array<Vector2F> uvs;
            uvs.reserve( 53 ); uvs.push_back( Vector2F( .7f, .8f ) );
            const auto pointBytes = bytes( points );
            const auto uvBytes = bytes( uvs );
            const auto before = budget.used();
            budget.set( before + pointBytes + uvBytes - 1 );
            denied( [&]{ face.setGeometry( points, uvs ); } );
            check( face.Points.data() == oldPoints && face.UVs.data() == oldUVs &&
                   face.Points.capacity() == 5 && face.UVs.capacity() == 7 &&
                   face.Points[0] == Vector3F( 1, 2, 3 ) && face.UVs[0] == Vector2F( .1f, .2f ) &&
                   budget.used() == before,
                   "Late UV admission failure preserves both old face payloads and retires staged points" );

            budget.set( before + pointBytes + uvBytes );
            face.setGeometry( points, uvs );
            check( face.Points == points && face.UVs == uvs &&
                   face.Points.capacity() == 37 && face.UVs.capacity() == 53 &&
                   face.Points.get_allocator().isSourceBudgeted() && face.UVs.get_allocator().isSourceBudgeted() &&
                   !points.get_allocator().isSourceBudgeted() && !uvs.get_allocator().isSourceBudgeted() &&
                   face.Points.data() != points.data() && face.UVs.data() != uvs.data() &&
                   budget.used() == before - oldBytes + pointBytes + uvBytes,
                   "Exact face import admits foreign spare capacity while leaving caller storage independent" );

            const auto resident = budget.used();
            budget.set( resident );
            denied( [&]{ auto snapshot = face.Points; } );
            check( budget.used() == resident && face.Points[0] == points[0],
                   "Denied caller snapshot preserves the face and leaves no copied storage" );
            budget.set( resident + pointBytes + uvBytes );
            auto pointSnapshot = face.Points;
            auto uvSnapshot = face.UVs;
            check( pointSnapshot.capacity() == 37 && uvSnapshot.capacity() == 53 &&
                   pointSnapshot.get_allocator().isSourceBudgeted() && uvSnapshot.get_allocator().isSourceBudgeted(),
                   "Caller snapshot retry retains complete admitted point and UV capacity" );
            const auto withSnapshots = budget.used();
            budget.set( withSnapshots );
            const auto epoch = sourceBudgetDenialCount();
            face.unload( nullptr );
            check( face.Points.empty() && face.UVs.empty() && !face.Points.capacity() && !face.UVs.capacity() &&
                   face.Points.get_allocator().isSourceBudgeted() && face.UVs.get_allocator().isSourceBudgeted() &&
                   pointSnapshot[0] == points[0] && uvSnapshot[0] == uvs[0] &&
                   budget.used() == withSnapshots - pointBytes - uvBytes && sourceBudgetDenialCount() == epoch,
                   "Full-cap face unload physically retires both buffers while retained snapshots survive" );
            budget.set( budget.original );

            Array<Vector3F> sparePoints; sparePoints.reserve( 13 );
            Array<Vector2F> spareUVs; spareUVs.reserve( 17 );
            face.setGeometry( sparePoints, spareUVs );
            check( face.Points.empty() && face.UVs.empty() && face.Points.capacity() == 13 && face.UVs.capacity() == 17,
                   "Empty foreign face arrays still import their physical spare capacity" );
            budget.set( budget.used() );
            face.unload( nullptr );
            check( !face.Points.capacity() && !face.UVs.capacity(), "Face unload releases empty spare buffers at the cap" );
            budget.set( budget.original );
        }
        {
            MeshFace source, destination;
            Array<Vector3F> points; points.reserve( 19 ); points.push_back( Vector3F( 7, 8, 9 ) );
            Array<Vector2F> uvs; uvs.reserve( 23 ); uvs.push_back( Vector2F( .3f, .4f ) );
            // Public fields remain compatible with legacy raw replacement. A
            // face copy/import must explicitly restore admitted ownership.
            source.Points = points; source.UVs = uvs;
            source.setName( "source face" ); source.setId( 91 );
            source.setLoadingState( LoadingState::Loaded );
            source.addReference(); source.addWeakReference(); source.lockLoad();
            source.next = &destination;
            {
                MeshFace copy( source );
                check( copy.Points == points && copy.UVs == uvs &&
                       copy.Points.capacity() == 19 && copy.UVs.capacity() == 23 &&
                       copy.Points.get_allocator().isSourceBudgeted() && copy.UVs.get_allocator().isSourceBudgeted() &&
                       copy.getReferences() == 1 && copy.getWeakReferences() == 0 &&
                       !copy.isLoadLocked() && copy.getLoadingState() == LoadingState::Allocated &&
                       !copy.next.load() && copy.getHandle() != source.getHandle() &&
                       source.getReferences() == 2 && source.getWeakReferences() == 1,
                       "Face copy imports foreign geometry into a fresh shared-object lifetime" );
                copy.setName( "copy face" );
                check( source.getName() == "source face", "Copied face metadata is independently owned" );
            }
            const auto beforeCopy = budget.used();
            budget.set( beforeCopy + bytes( points ) + bytes( uvs ) - 1 );
            denied( [&]{ MeshFace copy( source ); } );
            check( budget.used() == beforeCopy && source.getReferences() == 2 && source.getWeakReferences() == 1 &&
                   source.Points == points && source.UVs == uvs,
                   "Late UV copy-construction failure releases points and fresh metadata without corrupting source ownership" );
            budget.set( budget.original );
            source.unlockLoad(); source.removeWeakReference(); source.removeReference();
            check( source.getReferences() == 1 && source.getWeakReferences() == 0,
                   "Geometry copies do not alter source intrusive ownership" );

            destination.Points.reserve( 3 ); destination.Points.push_back( Vector3F( 1, 1, 1 ) );
            destination.UVs.reserve( 5 ); destination.UVs.push_back( Vector2F( .9f, .9f ) );
            destination.setName( "destination face" ); destination.setId( 43 );
            destination.setLoadingState( LoadingState::Loading );
            destination.addReference(); destination.addWeakReference(); destination.lockLoad();
            const auto oldPoints = destination.Points.data();
            const auto oldUVs = destination.UVs.data();
            const auto oldBytes = bytes( destination.Points ) + bytes( destination.UVs );
            const auto required = bytes( points ) + bytes( uvs );
            const auto before = budget.used();
            budget.set( before + required - 1 );
            denied( [&]{ destination = source; } );
            check( destination.Points.data() == oldPoints && destination.UVs.data() == oldUVs &&
                   destination.Points[0] == Vector3F( 1, 1, 1 ) && destination.UVs[0] == Vector2F( .9f, .9f ) &&
                   budget.used() == before, "Denied face assignment preserves both destination arrays" );
            budget.set( before + required );
            destination = source;
            check( destination.Points == points && destination.UVs == uvs &&
                   destination.Points.capacity() == 19 && destination.UVs.capacity() == 23 &&
                   destination.Points.get_allocator().isSourceBudgeted() && destination.UVs.get_allocator().isSourceBudgeted() &&
                   budget.used() == before - oldBytes + required &&
                   destination.getName() == "destination face" && destination.getId() == 43 &&
                   destination.getLoadingState() == LoadingState::Loading && destination.isLoadLocked() &&
                   destination.getReferences() == 2 && destination.getWeakReferences() == 1,
                   "Exact assignment retry publishes geometry together and preserves destination identity and ownership" );
            budget.set( budget.used() );
            const auto epoch = sourceBudgetDenialCount();
            destination = destination;
            check( sourceBudgetDenialCount() == epoch, "Face self assignment needs no admission" );
            destination.unlockLoad(); destination.removeWeakReference(); destination.removeReference();
            destination.unload( nullptr );
            check( destination.getReferences() == 1 && destination.getWeakReferences() == 0,
                   "Assigned face retains an independent reference-count lifetime" );
            budget.set( budget.original );
        }
    }
}
