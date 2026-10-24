#pragma once

#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/SubMesh.hpp>
#include <Workphone/Mesh/MeshPose.hpp>
#include <Workphone/Mesh/VertexBoneAssignment.hpp>
#include <Workphone/Animation/Animation.hpp>
#include <Workphone/Interface/System/IFactory.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IResourceManager.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include "MeshObjectBodyBudgetContracts.hpp"
#include <array>
#include <mutex>
#include <stdexcept>

namespace mesh_inventory_budget_contracts
{
    using namespace workphone;

    inline void require( bool condition, const char *message )
    {
        if( !condition ) throw std::runtime_error( message );
    }

    template <class T> u64 bytes( size_t capacity )
    {
        return capacity ? sourceBudgetStorageBytes( capacity * sizeof( T ), alignof( T ) ) : 0;
    }

    struct Capacity
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        static constexpr auto source = size_t( StreamMemoryClass::SourceCpu );
        u64 original = scheduler->stats().limits[source];
        u64 used() const { return scheduler->stats().used[source]; }
        void set( u64 capacity )
        {
            String error;
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, capacity, error ),
                     "mesh inventory test capacity applies" );
        }
        void restore() { set( original ); }
        ~Capacity() { restore(); }
    };

    template <class F> void denied( F &&operation, const char *message )
    {
        bool refused = false;
        try { operation(); }
        catch( const std::bad_alloc & ) { refused = true; }
        require( refused, message );
    }

    class MeshProbe : public Mesh
    {
    public:
        auto &subMeshes() { return m_subMeshes; }
        auto &animations() { return m_animations; }
        auto &bones() { return m_boneAssignments; }
        auto &poses() { return m_poses; }
    };

    class SubMeshProbe : public SubMesh
    {
    public:
        auto &bones() { return m_boneAssignments; }
    };

    template <class T> void warmFactory( size_t count )
    {
        auto manager = core::IApplicationManager::instance()->getFactoryManager();
        auto typeManager = TypeManager::instance();
        auto factory = manager->getFactoryByHash( typeManager->getHash( T::typeInfo() ) );
        // Match make_ptr<T>'s exact-type lookup. Some fixtures register only the
        // scene Animation component; the core animation and pose creators then
        // use direct construction and have no factory inventories to warm.
        if( !factory ) return;
        std::array<SmartPtr<ISharedObject>, 8> objects;
        require( count <= objects.size(), "mesh inventory fixture warm count fits" );
        for( size_t i = 0; i < count; ++i ) objects[i] = factory->make_object<ISharedObject>();
    }

    struct DetachedMeshManager
    {
        SmartPtr<core::IApplicationManager> application = core::IApplicationManager::instance();
        SmartPtr<IResourceManager> previous = application->getMeshManager();
        DetachedMeshManager() { application->setMeshManager( nullptr ); }
        ~DetachedMeshManager() { application->setMeshManager( previous ); }
    };

    inline void run()
    {
        // A failed array publication must not be confused with first-use growth
        // of the independent factory instance and teardown inventories.
        warmFactory<Animation>( 4 );
        warmFactory<MeshPose>( 4 );
        warmFactory<Mesh>( 2 );
        warmFactory<SubMesh>( 2 );
        Capacity capacity;
        auto mesh = make_ptr<MeshProbe>();
        auto submesh = make_ptr<SubMeshProbe>();
        auto second = make_ptr<SubMesh>();
        auto third = make_ptr<SubMesh>();
        std::array<SmartPtr<IVertexBoneAssignment>, 3> bones;
        std::array<SmartPtr<IMeshPose>, 3> poses;
        std::array<SmartPtr<IAnimation>, 3> animations;
        u64 poseConstruction = 0, animationConstruction = 0;
        const char *names[] = { "first", "second", "third" };
        for( size_t i = 0; i < bones.size(); ++i )
        {
            bones[i] = make_ptr<VertexBoneAssignment>( u32( i ), u16( i ), 1.f );
            const auto beforePose = capacity.used();
            poses[i] = make_ptr<MeshPose>( u16( i ), String() );
            poseConstruction = capacity.used() - beforePose;
            const auto beforeAnimation = capacity.used();
            auto animation = core::IApplicationManager::instance()->getFactoryManager()->make_ptr<Animation>();
            animation->setName( names[i] ); animation->setLength( 1.f );
            animations[i] = animation;
            animationConstruction = capacity.used() - beforeAnimation;
        }
        // Object bodies and empty pose/animation lookup tables have their own
        // admission contracts. Retain them in this array-inventory baseline.
        const auto baseline = capacity.used();
        require( mesh->subMeshes().get_allocator().isSourceBudgeted() &&
                 mesh->animations().get_allocator().isSourceBudgeted() &&
                 mesh->bones().get_allocator().isSourceBudgeted() &&
                 mesh->poses().get_allocator().isSourceBudgeted() &&
                 submesh->bones().get_allocator().isSourceBudgeted(),
                 "every mesh and submesh inventory uses SourceCpu allocation" );

        capacity.set( baseline );
        denied( [&] { mesh->addSubMesh( submesh ); }, "submesh publication refuses unadmitted capacity" );
        denied( [&] { mesh->addBoneAssignment( bones[0] ); }, "mesh bone publication propagates denial" );
        denied( [&] { mesh->addPose( poses[0] ); }, "mesh pose publication propagates denial" );
        denied( [&] { submesh->addBoneAssignment( bones[0] ); }, "submesh bone publication propagates denial" );
        capacity.set( baseline + animationConstruction );
        denied( [&] { mesh->createAnimation( "denied", 1.f ); }, "animation publication unwinds after body admission" );
        capacity.set( baseline + poseConstruction );
        denied( [&] { mesh->createPose( 0, "denied" ); }, "created pose publication unwinds after body admission" );
        require( mesh->getNumSubMeshes() == 0 && mesh->getNumAnimations() == 0 &&
                 mesh->getNumPoses() == 0 && mesh->bones().empty() && submesh->bones().empty() &&
                 capacity.used() == baseline,
                 "denied inventory publication preserves counts and physical ownership" );

        capacity.restore();
        mesh->addSubMesh( submesh ); mesh->addSubMesh( second ); mesh->addSubMesh( third );
        for( size_t i = 0; i < bones.size(); ++i )
        {
            mesh->addBoneAssignment( bones[i] ); mesh->addPose( poses[i] );
            submesh->addBoneAssignment( bones[i] );
            mesh->animations().push_back( animations[i] );
            require( bool( animations[i] ), "admitted animation publication succeeds" );
        }
        mesh->subMeshes().reserve( 7 ); mesh->animations().reserve( 11 );
        mesh->bones().reserve( 13 ); mesh->poses().reserve( 17 ); submesh->reserveBoneAssignments( 19 );
        const auto subBytes = bytes<SmartPtr<ISubMesh>>( 7 );
        const auto animationBytes = bytes<SmartPtr<IAnimation>>( 11 );
        const auto boneBytes = bytes<SmartPtr<IVertexBoneAssignment>>( 13 );
        const auto poseBytes = bytes<SmartPtr<IMeshPose>>( 17 );
        const auto subBoneBytes = bytes<SmartPtr<IVertexBoneAssignment>>( 19 );
        const auto ownerBytes = subBytes + animationBytes + boneBytes + poseBytes + subBoneBytes;
        require( capacity.used() == baseline + ownerBytes, "all five spare inventories charge physical storage exactly" );

        capacity.set( capacity.used() );
        denied( [&] { mesh->getSubMeshes(); }, "submesh caller copy needs admission" );
        denied( [&] { mesh->getBoneAssignments(); }, "mesh bone caller copy needs admission" );
        denied( [&] { submesh->getBoneAssignments(); }, "submesh bone caller copy needs admission" );
        denied( [&] { mesh->subMeshes().reserve( 8 ); }, "submesh spare growth needs admission" );
        denied( [&] { mesh->animations().reserve( 12 ); }, "animation spare growth needs admission" );
        denied( [&] { mesh->bones().reserve( 14 ); }, "bone spare growth needs admission" );
        denied( [&] { mesh->poses().reserve( 18 ); }, "pose spare growth needs admission" );
        denied( [&] { submesh->reserveBoneAssignments( 20 ); }, "submesh bone spare growth needs admission" );
        require( capacity.used() == baseline + ownerBytes, "failed copies and spare growth preserve owners" );

        capacity.restore();
        auto submeshes = mesh->getSubMeshes();
        auto meshBones = mesh->getBoneAssignments();
        auto subBones = submesh->getBoneAssignments();
        // Animation and pose APIs expose indexed elements rather than arrays;
        // copy the protected inventories to verify their allocator propagation.
        auto animationSnapshot = mesh->animations();
        auto poseSnapshot = mesh->poses();
        require( submeshes.capacity() == 7 && meshBones.capacity() == 13 && subBones.capacity() == 19 &&
                 animationSnapshot.capacity() == 11 && poseSnapshot.capacity() == 17,
                 "inventory copies preserve source spare capacity" );
        require( submeshes.get_allocator().isSourceBudgeted() && meshBones.get_allocator().isSourceBudgeted() &&
                 subBones.get_allocator().isSourceBudgeted() && animationSnapshot.get_allocator().isSourceBudgeted() &&
                 poseSnapshot.get_allocator().isSourceBudgeted(), "escaped copies retain SourceCpu policy" );
        require( capacity.used() == baseline + ownerBytes * 2, "escaped copies own distinct physical capacity" );

        capacity.set( capacity.used() );
        mesh->removeSubMesh( second ); mesh->removeSubMesh( submesh );
        mesh->removeBoneAssignment( bones[1] ); mesh->removeBoneAssignment( bones[0] );
        mesh->removeAnimation( animations[1] ); mesh->removeAnimation( animations[0] );
        mesh->removePose( poses[1] ); mesh->removePose( poses[0] );
        submesh->removeBoneAssignment( bones[1] ); submesh->removeBoneAssignment( bones[0] );
        require( mesh->getNumSubMeshes() == 1 && mesh->getSubMesh( 0 ) == third &&
                 mesh->getNumAnimations() == 1 && mesh->getAnimation( 0 ) == animations[2] &&
                 mesh->getNumPoses() == 1 && mesh->getPose( 0 ) == poses[2] &&
                 mesh->bones().size() == 1 && mesh->bones()[0] == bones[2] &&
                 submesh->bones().size() == 1 && submesh->bones()[0] == bones[2],
                 "at-capacity middle and first removals preserve remaining order" );
        mesh->removeAllSubMeshes(); mesh->removeAllAnimations(); mesh->removeAllPoses();
        require( capacity.used() == baseline + ownerBytes * 2, "erase and clear retain admitted spare capacity" );
        mesh->unload( nullptr ); submesh->unload( nullptr );
        require( capacity.used() == baseline + ownerBytes, "at-capacity unload physically retires every owner inventory" );
        require( submeshes.size() == 3 && meshBones.size() == 3 && subBones.size() == 3 &&
                 animationSnapshot.size() == 3 && poseSnapshot.size() == 3,
                 "escaped snapshots survive owner removals and teardown" );
        submeshes = makeSourceBudgetSnapshot<SmartPtr<ISubMesh>>();
        meshBones = makeSourceBudgetSnapshot<SmartPtr<IVertexBoneAssignment>>();
        subBones = makeSourceBudgetSnapshot<SmartPtr<IVertexBoneAssignment>>();
        animationSnapshot = makeSourceBudgetSnapshot<SmartPtr<IAnimation>>();
        poseSnapshot = makeSourceBudgetSnapshot<SmartPtr<IMeshPose>>();
        require( capacity.used() == baseline, "last escaped inventory owner retires physical charge" );
        capacity.restore();

        // Mesh clone iterates a retained Source snapshot of the source bones.
        // Its first one-slot destination and two-slot replacement overlap while
        // the second bone is appended. Refusal must discard the entire clone.
        mesh->addBoneAssignment( bones[0] ); mesh->addBoneAssignment( bones[1] );
        mesh->bones().reserve( 13 );
        const auto sourceBones = bytes<SmartPtr<IVertexBoneAssignment>>( 13 );
        const auto cloneBones = bytes<SmartPtr<IVertexBoneAssignment>>( 2 );
        const auto meshCloneBody = mesh_object_body_budget_contracts::managerBytes<Mesh>();
        const auto cloning = meshCloneBody + sourceBones + bytes<SmartPtr<IVertexBoneAssignment>>( 1 ) + cloneBones;
        const auto sourcePointer = mesh->bones().data();
        require( capacity.used() == baseline + sourceBones, "bone-only source owns its spare inventory" );
        capacity.set( baseline + sourceBones + cloning - 1 );
        denied( [&] { mesh->clone(); }, "second clone bone growth propagates one-byte-short admission" );
        require( capacity.used() == baseline + sourceBones && mesh->bones().size() == 2 &&
                 mesh->bones().capacity() == 13 && mesh->bones().data() == sourcePointer &&
                 mesh->bones()[0] == bones[0] && mesh->bones()[1] == bones[1],
                 "failed clone retires snapshot and candidate while preserving source inventory" );
        capacity.set( baseline + sourceBones + cloning );
        auto clone = mesh->clone();
        require( bool( clone ), "mesh clone retries with exact overlapping inventory capacity" );
        require( capacity.used() == baseline + sourceBones + meshCloneBody + cloneBones,
                 "completed mesh clone retains distinct two-element bone capacity" );
        auto clonedBones = clone->getBoneAssignments();
        require( clonedBones.size() == 2 && clonedBones.get_allocator().isSourceBudgeted() &&
                 clonedBones[0] == bones[0] && clonedBones[1] == bones[1],
                 "mesh clone preserves bone references in a Source-owned inventory" );
        clonedBones = makeSourceBudgetSnapshot<SmartPtr<IVertexBoneAssignment>>();
        capacity.set( capacity.used() );
        mesh->unload( nullptr ); clone->unload( nullptr ); clone = nullptr;
        require( capacity.used() == baseline, "at-capacity original and clone unload retire all bone storage" );

        capacity.restore();
        submesh->reserveBoneAssignments( 19 );
        submesh->addBoneAssignment( bones[0] ); submesh->addBoneAssignment( bones[1] );
        submesh->setUseSharedVertices( true );
        submesh->setRenderOperationType( RenderOperationType::OT_LINE_LIST );
        require( capacity.used() == baseline + subBoneBytes, "submesh clone source retains spare bone capacity" );
        const auto subCloneBody = mesh_object_body_budget_contracts::managerBytes<SubMesh>();
        capacity.set( baseline + subCloneBody + subBoneBytes * 2 - 1 );
        denied( [&] { submesh->clone(); }, "submesh bone snapshot needs complete clone admission" );
        require( capacity.used() == baseline + subBoneBytes && submesh->bones().size() == 2 &&
                 submesh->bones().capacity() == 19, "failed submesh clone preserves source bones and spare capacity" );
        capacity.set( baseline + subCloneBody + subBoneBytes * 2 );
        auto subClone = submesh->clone();
        require( bool( subClone ) && subClone->getUseSharedVertices() &&
                 subClone->getRenderOperationType() == RenderOperationType::OT_LINE_LIST &&
                 !subClone->getVertexBuffer() && !subClone->getIndexBuffer(),
                 "metadata-only submesh clone preserves flags and nullable geometry" );
        require( capacity.used() == baseline + subCloneBody + subBoneBytes * 2,
                 "submesh clone charges retained spare bone capacity exactly" );
        capacity.restore();
        auto retainedSubBones = subClone->getBoneAssignments();
        require( retainedSubBones.capacity() == 19 && retainedSubBones.size() == 2 &&
                 retainedSubBones.get_allocator().isSourceBudgeted() && retainedSubBones[0] == bones[0],
                 "submesh clone snapshot preserves Source policy, spare capacity and bone references" );
        capacity.set( capacity.used() );
        submesh->unload( nullptr ); subClone->unload( nullptr ); subClone = nullptr;
        require( capacity.used() == baseline + subBoneBytes, "submesh clone snapshot survives full-capacity teardown" );
        retainedSubBones = makeSourceBudgetSnapshot<SmartPtr<IVertexBoneAssignment>>();
        require( capacity.used() == baseline, "last cloned submesh bone snapshot retires physical storage" );
        capacity.restore();

        // Factory teardown can run after the application retires its mesh
        // manager. Keep unrelated event publication disabled for this focused
        // inventory/lock check; loading-notification denial has separate tests.
        mesh->subMeshes().reserve( 7 ); mesh->animations().reserve( 11 );
        mesh->bones().reserve( 13 ); mesh->poses().reserve( 17 );
        mesh->addBoneAssignment( bones[0] );
        const auto detachedBytes = subBytes + animationBytes + boneBytes + poseBytes;
        require( capacity.used() == baseline + detachedBytes, "detached shutdown fixture retains all mesh inventories" );
        const auto flags = mesh->getObjectFlags();
        mesh->setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, false );
        capacity.set( capacity.used() );
        const auto denialEpoch = sourceBudgetDenialCount();
        {
            DetachedMeshManager detached;
            {
                std::lock_guard<MeshProbe> lock( *mesh );
                const bool recursive = mesh->try_lock();
                if( recursive ) mesh->unlock();
                require( recursive, "detached mesh uses an allocation-free recursive fallback lock" );
            }
            mesh->unload( nullptr );
            require( mesh->getNumSubMeshes() == 0 && mesh->getNumAnimations() == 0 &&
                     mesh->getNumPoses() == 0 && mesh->bones().empty(),
                     "detached full-capacity shutdown clears every mesh inventory" );
            require( capacity.used() == baseline && sourceBudgetDenialCount() == denialEpoch,
                     "detached full-capacity shutdown retires storage without requesting Source admission" );
        }
        mesh->setObjectFlags( flags );
        require( bool( core::IApplicationManager::instance()->getMeshManager() ),
                 "detached shutdown fixture restores the application mesh manager" );
        capacity.restore();
    }
}
