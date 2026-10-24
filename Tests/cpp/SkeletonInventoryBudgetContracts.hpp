#pragma once

#include <Workphone/Mesh/MeshSkeleton.hpp>
#include <Workphone/Mesh/Bone.hpp>
#include <Workphone/Mesh/LinkedSkeletonAnimationSource.hpp>
#include <Workphone/Animation/Animation.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include "MeshObjectBodyBudgetContracts.hpp"
#include <stdexcept>

namespace skeleton_inventory_budget_contracts
{
    using namespace workphone;
    inline void require( bool condition, const char *message )
    { if( !condition ) throw std::runtime_error( message ); }
    struct Capacity
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        static constexpr auto source = size_t( StreamMemoryClass::SourceCpu );
        u64 original = scheduler->stats().limits[source];
        u64 used() const { return scheduler->stats().used[source]; }
        void set( u64 value )
        {
            String error;
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, value, error ),
                     "skeleton capacity applies" );
        }
        void restore() { set( original ); }
        ~Capacity() { restore(); }
    };
    template<class F> inline void denied( F operation, const char *message )
    {
        bool rejected = false;
        try { operation(); } catch( const std::bad_alloc & ) { rejected = true; }
        require( rejected, message );
    }
    template<class T> inline u64 arrayBytes( size_t capacity )
    { return capacity ? sourceBudgetStorageBytes( capacity * sizeof( T ), alignof( T ) ) : 0; }
    template<class T> inline u64 bodyBytes()
    { return sourceBudgetStorageBytes( sizeof( T ), alignof( std::max_align_t ) ); }

    class SkeletonProbe : public MeshSkeleton
    {
    public:
        using BoneIndex = decltype( m_bonesByName );
        using AnimationIndex = decltype( m_animationsByName );
        auto &bones() { return m_bones; }
        auto &animations() { return m_animations; }
        size_t boneIndexSize() const { return m_bonesByName.size(); }
        size_t animationIndexSize() const { return m_animationsByName.size(); }
        void setLink( SmartPtr<LinkedSkeletonAnimationSource> value ) { m_linker = value; }
        void retainProbe( SmartPtr<IBone> bone, SmartPtr<IAnimation> animation )
        {
            m_bones.push_back( bone ); m_animations.push_back( animation );
            m_bonesByName.emplace( "probe", bone );
            m_animationsByName.emplace( "probe", animation );
        }
    };
    class BoneProbe : public Bone
    {
    public:
        auto &children() { return m_children; }
    };

    inline void warm()
    {
        // Warm manager-owned inventories separately from each object's payload.
        SkeletonProbe skeleton;
        for( unsigned i = 0; i < 8; ++i ) skeleton.createBone( i );
        skeleton.createAnimation( "warmone", 1 );
        skeleton.createAnimation( "warmtwo", 2 );
        auto parent = make_ptr<Bone>();
        parent->createChild( 1 );
        auto link = make_ptr<LinkedSkeletonAnimationSource>( "warm", 1 );
    }

    inline void bodies()
    {
        mesh_object_body_budget_contracts::direct<Bone>();
        mesh_object_body_budget_contracts::factory<Bone>();
        mesh_object_body_budget_contracts::pooled<Bone>();
        Capacity budget;
        u64 boneHead = 0, animationHead = 0;
        auto baseline = budget.used();
        { SkeletonProbe::BoneIndex index; boneHead = budget.used() - baseline; }
        { SkeletonProbe::AnimationIndex index; animationHead = budget.used() - baseline; }
        require( budget.used() == baseline, "standalone index sentinels physically retire" );
        const auto body = bodyBytes<MeshSkeleton>();
        for( auto admitted : { body - 1, body + animationHead - 1,
                               body + animationHead + boneHead - 1 } )
        {
            budget.set( baseline + admitted );
            denied( [] { auto skeleton = make_ptr<MeshSkeleton>(); },
                    "skeleton constructor denies body and independent name-index stages" );
            require( budget.used() == baseline, "partial skeleton construction fully unwinds" );
        }
        budget.set( baseline + body + animationHead + boneHead );
        auto skeleton = make_ptr<MeshSkeleton>();
        require( budget.used() == baseline + body + animationHead + boneHead,
                 "skeleton body and both physical map sentinels charge exactly" );
        const auto denials = sourceBudgetDenialCount();
        skeleton->unload( nullptr );
        require( budget.used() == baseline + body + animationHead + boneHead &&
                     sourceBudgetDenialCount() == denials,
                 "empty unload retains existing map sentinels without replacement admission" );
        skeleton = nullptr;
        require( budget.used() == baseline, "skeleton destruction retires sentinels and wrapper" );
        budget.restore();

        {
            FactoryTemplate<MeshSkeleton> producer;
            producer.load( nullptr );
            producer.setPool( nullptr );
            const auto empty = budget.used();
            const auto factoryBody = sourceBudgetStorageBytes( sizeof( MeshSkeleton ), alignof( MeshSkeleton ) );
            const auto inventories = arrayBytes<MeshSkeleton *>( 1 ) + arrayBytes<SmartPtr<ISharedObject>>( 1 );
            budget.set( empty + factoryBody + animationHead + boneHead - 1 );
            denied( [&] { auto candidate = producer.make_ptr<MeshSkeleton>(); },
                    "factory skeleton constructor admits both indexes before inventory publication" );
            require( budget.used() == empty, "factory constructor refusal retires body and partial index metadata" );
            budget.set( empty + factoryBody + animationHead + boneHead + inventories );
            auto instance = producer.make_ptr<MeshSkeleton>();
            require( instance && budget.used() == empty + factoryBody + animationHead + boneHead + inventories,
                     "factory skeleton charges its actual body, map heads and inventories exactly" );
            instance = nullptr;
            require( budget.used() == empty + inventories, "final factory skeleton owner retires both index sentinels" );
            producer.unload( nullptr );
            require( budget.used() == empty, "factory skeleton teardown retires retained inventory capacity at cap" );
        }
        budget.restore();
        baseline = budget.used();
        {
            auto pool = make_ptr<Pool<MeshSkeleton>>();
            pool->setNextSize( 2 );
            auto slot = pool->allocate_object();
            const auto retained = budget.used();
            const auto slab = sourceBudgetStorageBytes( 2 * sizeof( MeshSkeleton ), alignof( MeshSkeleton ) );
            require( slot && retained >= baseline + slab,
                     "skeleton pool admits unused slots with its complete physical slab" );
            budget.set( retained + animationHead + boneHead - 1 );
            denied( [&] { auto candidate = new( slot.get() ) MeshSkeleton; },
                    "borrowed pool-slot skeleton construction still admits both index sentinels" );
            require( budget.used() == retained, "denied pooled constructor frees partial indexes and preserves borrowed slot" );
            budget.set( retained + animationHead + boneHead );
            auto instance = new( slot.get() ) MeshSkeleton;
            require( budget.used() == retained + animationHead + boneHead,
                     "pooled construction adds index heads without a second body charge" );
            instance->~MeshSkeleton();
            pool->free_object( instance );
            require( budget.used() == retained, "pooled skeleton destruction retains only its physical slab and pool metadata" );
            pool = nullptr;
            require( budget.used() == baseline, "pool retirement physically releases skeleton slots and pool inventories at cap" );
        }
        budget.restore();

        const String name = "linked";
        baseline = budget.used();
        budget.set( baseline + bodyBytes<LinkedSkeletonAnimationSource>() - 1 );
        denied( [&] { auto link = make_ptr<LinkedSkeletonAnimationSource>( name, 2 ); },
                "linked-source wrapper is independently admitted" );
        require( budget.used() == baseline, "denied linked-source body leaves no charge" );
    }

    template<class Index> inline u64 nodeBytes( const String &name )
    {
        Capacity budget;
        Index index;
        const auto baseline = budget.used();
        index.emplace( name, nullptr );
        return budget.used() - baseline;
    }

    inline void namedInventories()
    {
        Capacity budget;
        const String oldName = "old", newName = "new", clipName = "clip";
        const auto boneNode = nodeBytes<SkeletonProbe::BoneIndex>( newName );
        const auto animationNode = nodeBytes<SkeletonProbe::AnimationIndex>( clipName );
        SkeletonProbe skeleton;
        skeleton.bones().reserve( 1 );
        skeleton.animations().reserve( 1 );
        auto old = skeleton.createBone( oldName, 7 );
        const auto baseline = budget.used();
        const auto bone = mesh_object_body_budget_contracts::managerBytes<Bone>();
        const auto grownOwners = arrayBytes<SmartPtr<IBone>>( 2 );
        for( auto headroom : { bone - 1, bone + boneNode - 1,
                               bone + boneNode + grownOwners - 1 } )
        {
            budget.set( baseline + headroom );
            denied( [&] { skeleton.createBone( newName, 19 ); },
                    "bone body, name node, and owner growth each require admission" );
            require( budget.used() == baseline && skeleton.getBones().size() == 1 &&
                         skeleton.getBones().capacity() == 1 && skeleton.boneIndexSize() == 1 &&
                         skeleton.getBone( oldName ) == old && !skeleton.hasBone( newName ),
                     "named bone failure rolls back both index and strong inventory" );
        }
        budget.restore();
        auto added = skeleton.createBone( newName, 19 );
        require( added && added->getBoneHandle() == 19 && skeleton.getBone( newName ) == added &&
                     skeleton.getBones().get_allocator().isSourceBudgeted(),
                 "named bone retry preserves handle, lookup and Source policy" );

        // Duplicate names keep the earlier inventory owner while redirecting
        // the name alias to the last complete bone, matching existing behavior.
        const auto beforeReplacement = budget.used();
        budget.set( beforeReplacement + bone + arrayBytes<SmartPtr<IBone>>( 4 ) - 1 );
        denied( [&] { skeleton.createBone( newName, 23 ); },
                "duplicate-name owner growth may be refused" );
        require( budget.used() == beforeReplacement && skeleton.getBone( newName ) == added &&
                     skeleton.getBones().size() == 2,
                 "refused replacement preserves the previous named alias" );
        budget.restore();
        auto replacement = skeleton.createBone( newName, 23 );
        require( skeleton.getBone( newName ) == replacement && skeleton.getBones()[1] == added &&
                     skeleton.getBones().size() == 3,
                 "successful duplicate alias retains every earlier bone owner" );

        // Animation construction has its own admitted map sentinels. Measure
        // that payload once so the last gate targets this skeleton's index node.
        const auto beforeAnimation = budget.used();
        auto candidate = core::IApplicationManager::instance()->getFactoryManager()->make_ptr<Animation>();
        const auto animation = budget.used() - beforeAnimation;
        candidate = nullptr;
        require( budget.used() == beforeAnimation, "measured clip candidate fully retires" );
        budget.set( beforeAnimation + animation + animationNode - 1 );
        denied( [&] { skeleton.createAnimation( clipName, 3 ); },
                "clip name-index admission follows complete clip construction" );
        require( budget.used() == beforeAnimation && skeleton.getAnimations().empty() &&
                     skeleton.animationIndexSize() == 0 && !skeleton.hasAnimation( clipName ),
                 "clip index refusal leaves no partially published clip" );
        budget.restore();
        auto clip = skeleton.createAnimation( clipName, 3 );
        budget.set( budget.used() );
        const auto noGrowth = sourceBudgetDenialCount();
        require( skeleton.createAnimation( clipName, 8 ) == clip && clip->getLength() == 3 &&
                     skeleton.hasBone( oldName ) && skeleton.hasAnimation( clipName ) &&
                     skeleton.getBone( newName ) == replacement,
                 "existing clip and name lookups work with no headroom" );
        skeleton.setBlendMode( SkeletonAnimationBlendMode::ANIMBLEND_CUMULATIVE );
        require( sourceBudgetDenialCount() == noGrowth,
                 "lookup and blend edits do not attempt Source admission" );
        budget.restore();

        skeleton.bones().reserve( 23 );
        skeleton.animations().reserve( 11 );
        const auto resident = budget.used();
        budget.set( resident );
        denied( [&] { auto copy = skeleton.getBones(); }, "escaped bone inventories require their spare capacity" );
        denied( [&] { auto copy = skeleton.getAnimations(); }, "escaped clip inventories require their spare capacity" );
        budget.restore();
        auto bones = skeleton.getBones();
        auto clips = skeleton.getAnimations();
        const auto snapshots = arrayBytes<SmartPtr<IBone>>( 23 ) + arrayBytes<SmartPtr<IAnimation>>( 11 );
        require( budget.used() == resident + snapshots && bones.capacity() == 23 && clips.capacity() == 11 &&
                     bones.get_allocator().isSourceBudgeted() && clips.get_allocator().isSourceBudgeted(),
                 "caller-retained inventories charge their actual imported spare capacity" );
        budget.set( budget.used() );
        const auto beforeUnload = sourceBudgetDenialCount();
        skeleton.unload( nullptr );
        require( skeleton.getBones().capacity() == 0 && skeleton.getAnimations().capacity() == 0 &&
                     skeleton.boneIndexSize() == 0 && skeleton.animationIndexSize() == 0 &&
                     bones[1] == added && clips[0] == clip && sourceBudgetDenialCount() == beforeUnload,
                 "at-cap skeleton unload retires inventories while escaped snapshots remain valid" );
    }

    inline void hierarchy()
    {
        Capacity budget;
        BoneProbe parent;
        parent.children().reserve( 1 );
        auto first = parent.createChild( 3, Vector3<real_Num>( 1, 2, 3 ) );
        const auto baseline = budget.used();
        const auto body = mesh_object_body_budget_contracts::managerBytes<Bone>();
        for( auto headroom : { body - 1, body + arrayBytes<SmartPtr<IBone>>( 2 ) - 1 } )
        {
            budget.set( baseline + headroom );
            denied( [&] { parent.createChild( 9 ); }, "child body and parent inventory growth require independent admission" );
            require( budget.used() == baseline && parent.children().size() == 1 &&
                         parent.children()[0] == first && first->getParent().get() == &parent,
                     "child admission failure leaves hierarchy and parent backlink unchanged" );
        }
        budget.restore();
        auto second = parent.createChild( 9, Vector3<real_Num>( 4, 5, 6 ) );
        parent.children().reserve( 29 );
        const auto resident = budget.used();
        budget.set( resident );
        denied( [&] { auto children = parent.getChildren(); }, "child snapshot cannot escape without physical admission" );
        budget.restore();
        auto children = parent.getChildren();
        require( children.capacity() == 29 && children.get_allocator().isSourceBudgeted() &&
                     budget.used() == resident + arrayBytes<SmartPtr<IBone>>( 29 ),
                 "child snapshots preserve actual spare capacity under Source policy" );
        budget.set( budget.used() );
        const auto denials = sourceBudgetDenialCount();
        parent.setPosition( Vector3<real_Num>( 7, 8, 9 ) );
        parent.setBindingPose();
        parent.setPosition( Vector3<real_Num>::zero() );
        first->setPosition( Vector3<real_Num>::zero() );
        second->setPosition( Vector3<real_Num>::zero() );
        parent.setManuallyControlled( true );
        parent.reset();
        require( parent.getPosition().x == 7 && first->getPosition().y == 2 &&
                     second->getPosition().z == 6 && parent.isManuallyControlled(),
                 "inline transforms and recursive binding/reset retain hierarchy behavior at cap" );
        const auto withSnapshot = budget.used();
        parent.unload( nullptr );
        require( parent.children().capacity() == 0 && !first->getParent() && !second->getParent() &&
                     children[0] == first && children[1] == second &&
                     budget.used() == withSnapshot - arrayBytes<SmartPtr<IBone>>( 29 ) &&
                     sourceBudgetDenialCount() == denials,
                 "unload detaches weak parent links and physically retires only its own child array" );
        budget.restore();
        auto heapParent = make_ptr<Bone>();
        auto escaped = heapParent->createChild( 17 );
        heapParent = nullptr;
        require( !escaped->getParent(), "retained child cannot hold a dangling parent after destruction" );
    }

    inline void linkedSource()
    {
        Capacity budget;
        SkeletonProbe skeleton;
        auto other = make_ptr<MeshSkeleton>();
        auto link = make_ptr<LinkedSkeletonAnimationSource>( "linked-rig", 2.5f, other );
        skeleton.setLink( link );
        auto clip = skeleton.createAnimation( "local", 1 );
        const auto baseline = budget.used();
        budget.set( baseline );
        const auto denials = sourceBudgetDenialCount();
        const LinkedSkeletonAnimationSource *returned = nullptr;
        require( skeleton.getAnimation( "local", &returned ) == clip && returned == link.get() &&
                     link->pSkeleton == other && link->scale == 2.5f,
                 "linked-source output and pinned skeleton retain their existing semantics" );
        skeleton.unload( nullptr );
        require( link->pSkeleton == other, "escaped linked-source owner retains its target skeleton" );
        link->unload( nullptr );
        require( !link->pSkeleton && link->skeletonName.empty() && sourceBudgetDenialCount() == denials,
                 "at-cap linked-source unload drops name storage and strong target pin without admission" );
    }

    inline void retirementCallbacks()
    {
        struct RetiredBone : Bone
        {
            SkeletonProbe *owner = nullptr;
            bool *observed = nullptr;
            ~RetiredBone()
            {
                *observed = owner->getBones().empty() && owner->getAnimations().empty() &&
                            owner->boneIndexSize() == 0 && owner->animationIndexSize() == 0;
            }
        };
        struct RetiredAnimation : Animation
        {
            SkeletonProbe *owner = nullptr;
            bool *observed = nullptr;
            ~RetiredAnimation()
            {
                *observed = owner->getBones().empty() && owner->getAnimations().empty() &&
                            owner->boneIndexSize() == 0 && owner->animationIndexSize() == 0;
            }
        };
        struct RetiredChild : Bone
        {
            BoneProbe *owner = nullptr;
            bool *observed = nullptr;
            ~RetiredChild() { *observed = owner->children().empty(); }
        };
        Capacity budget;
        SkeletonProbe skeleton;
        BoneProbe parent;
        bool boneDetached = false, animationDetached = false, childrenDetached = false;
        auto bone = make_ptr<RetiredBone>(); bone->owner = &skeleton; bone->observed = &boneDetached;
        auto animation = make_ptr<RetiredAnimation>();
        animation->owner = &skeleton; animation->observed = &animationDetached;
        skeleton.retainProbe( bone, animation );
        bone = nullptr; animation = nullptr;
        auto child = make_ptr<RetiredChild>(); child->owner = &parent; child->observed = &childrenDetached;
        parent.children().push_back( child ); child = nullptr;
        budget.set( budget.used() );
        const auto denials = sourceBudgetDenialCount();
        skeleton.unload( nullptr ); parent.unload( nullptr );
        require( boneDetached && animationDetached && childrenDetached &&
                     sourceBudgetDenialCount() == denials,
                 "final-reference callbacks observe a fully detached skeleton and hierarchy generation at cap" );
    }

    inline void run()
    {
        warm(); bodies(); namedInventories(); hierarchy(); linkedSource(); retirementCallbacks();
    }
}
