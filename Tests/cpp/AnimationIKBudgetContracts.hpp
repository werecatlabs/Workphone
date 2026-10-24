#pragma once
#include <Workphone/Animation/IK/AnimationIKSystem.hpp>
#include <Workphone/Animation/Animation.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Mesh/Bone.hpp>
#include <Workphone/Mesh/MeshSkeleton.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Scene/Components/Animator.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <memory>
#include <optional>
#include <stdexcept>

namespace animation_ik_budget_contracts
{
    using namespace workphone;
    using namespace workphone::animation;
    inline void require( bool value, const char *message )
    { if( !value ) throw std::runtime_error( message ); }
    struct Capacity
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        static constexpr size_t source = size_t( StreamMemoryClass::SourceCpu );
        u64 original = scheduler->stats().limits[source];
        u64 used() const { return scheduler->stats().used[source]; }
        void set( u64 value )
        {
            String error;
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, value, error ),
                     "IK SourceCpu capacity applies" );
        }
        void restore() { set( original ); }
        ~Capacity() { restore(); }
    };
    template<class F> void denied( F &&function, const char *message )
    {
        bool rejected = false;
        try { function(); } catch( const std::bad_alloc & ) { rejected = true; }
        require( rejected, message );
    }
    template<class T> u64 arrayBytes( size_t count )
    { return count ? sourceBudgetStorageBytes( count * sizeof( T ), alignof( T ) ) : 0; }
    inline Transform3<real_Num> target( real_Num x, real_Num y )
    { return { Vector3<real_Num>( x, y, 0 ), Quaternion<real_Num>::identity() }; }
    inline TwoBoneIKConstraint constraint( const String &id )
    {
        TwoBoneIKConstraint value;
        value.id = id; value.effectorBone = "hand";
        value.targetTransform = target( 1, 1 );
        value.settings.matchTargetOrientation = false;
        value.settings.poleTarget = Vector3<real_Num>::unitZ();
        return value;
    }
    class BoneFixture : public Bone
    {
    public:
        void attach( const SmartPtr<BoneFixture> &child )
        {
            m_children.push_back( child );
            auto mutableChild = child;
            mutableChild->m_parent = this;
        }
    };
    class SkeletonFixture : public MeshSkeleton
    {
    public:
        void publish( const String &name, SmartPtr<IBone> bone )
        { m_bones.push_back( bone ); m_bonesByName.emplace( name, bone ); }
    };
    struct Chain
    {
        SmartPtr<SkeletonFixture> skeleton = make_ptr<SkeletonFixture>();
        SmartPtr<BoneFixture> root = make_ptr<BoneFixture>();
        SmartPtr<BoneFixture> mid = make_ptr<BoneFixture>();
        SmartPtr<BoneFixture> hand = make_ptr<BoneFixture>();
        Chain()
        {
            mid->setPosition( Vector3<real_Num>::unitX() );
            hand->setPosition( Vector3<real_Num>::unitX() );
            root->attach( mid ); mid->attach( hand );
            skeleton->publish( "root", root ); skeleton->publish( "mid", mid );
            skeleton->publish( "hand", hand );
        }
        bool untouched() const
        {
            return root->getOrientation() == Quaternion<real_Num>::identity() &&
                   mid->getOrientation() == Quaternion<real_Num>::identity() &&
                   hand->getOrientation() == Quaternion<real_Num>::identity();
        }
        void reset()
        {
            root->setOrientation( Quaternion<real_Num>::identity() );
            mid->setOrientation( Quaternion<real_Num>::identity() );
            hand->setOrientation( Quaternion<real_Num>::identity() );
        }
    };
    inline void inventoryAndProperties( Capacity &budget )
    {
        AnimationIKSystem system;
        auto first = constraint( "first" );
        auto second = constraint( "second" );
        const auto empty = budget.used();
        budget.set( empty );
        denied( [&] { system.addConstraint( first ); }, "IK constraint inventory needs SourceCpu admission" );
        require( system.getConstraints().empty() && budget.used() == empty,
                 "Denied constraint growth preserves the empty producer" );
        budget.restore();
        require( system.addConstraint( first ) && system.addConstraint( second ), "IK constraint insertion retries" );
        require( system.getConstraints().get_allocator().isSourceBudgeted(), "Constraint inventory carries Source policy" );

        auto update = first;
        update.effectorBone.assign( CharacterPoolAllocator<char>::MaxPooledBlockBytes + 1, 'U' );
        update.targetTransform = target( 0, 1 );
        const auto resident = budget.used();
        const auto original = system.getConstraints().data();
        budget.set( resident );
        denied( [&] { system.updateConstraint( update ); }, "Constraint replacement stages oversized strings before publication" );
        require( budget.used() == resident && system.getConstraints().data() == original &&
                     system.findConstraint( "first" )->effectorBone == "hand" &&
                     system.findConstraint( "first" )->targetTransform == first.targetTransform,
                 "Denied replacement preserves both owning strings and all numeric settings" );
        denied( [&] { auto copy = system.getConstraints(); }, "Escaped constraint arrays retain shared admission" );
        const auto noAdmission = sourceBudgetDenialCount();
        require( system.setTarget( "first", target( 0.5f, 1 ) ) && system.setWeight( "first", 0.5f ) &&
                     system.setPoleTarget( "first", Vector3<real_Num>::unitY() ),
                 "Numeric IK edits update without allocating" );
        require( system.removeConstraint( system.getConstraints().front().id ) &&
                     system.getConstraints().size() == 1 && system.getConstraints()[0].id == "second" &&
                     sourceBudgetDenialCount() == noAdmission,
                 "Aliased first-element removal shifts existing constraints at capacity" );
        budget.restore();
        std::optional<Array<TwoBoneIKConstraint>> detached( system.getConstraints() );
        detached->reserve( 17 );
        const auto beforeClear = budget.used();
        budget.set( beforeClear );
        const auto clearDenials = sourceBudgetDenialCount();
        system.clear();
        require( sourceBudgetDenialCount() == clearDenials && budget.used() < beforeClear &&
                     system.getConstraints().capacity() == 0 && detached->capacity() == 17 &&
                     detached->front().id == "second", "Full-cap clear releases producer storage while a spare snapshot survives" );
        detached.reset();
        budget.restore();

        AnimationIKSystem authored;
        require( authored.addConstraint( first ), "Property fixture first constraint inserts" );
        auto oversized = second;
        oversized.id.assign( CharacterPoolAllocator<char>::MaxPooledBlockBytes + 1, 'P' );
        require( authored.addConstraint( oversized ), "Property fixture oversized second constraint inserts" );
        auto properties = authored.getProperties();
        require( system.addConstraint( second ), "Property replacement previous state exists" );
        const auto oldConstraints = system.getConstraints().data();
        u64 importBytes = 0, childBytes = 0;
        {
            const auto before = budget.used();
            AnimationIKSystem imported;
            require( imported.setProperties( properties ), "Property fixture complete import succeeds" );
            importBytes = budget.used() - before;
        }
        {
            const auto before = budget.used();
            auto children = properties->getChildrenByName( "constraint" );
            childBytes = budget.used() - before;
        }
        const auto beforeImport = budget.used();
        budget.set( beforeImport + importBytes + childBytes - 1 );
        denied( [&] { system.setProperties( properties ); }, "A partial property import must admit the complete replacement" );
        require( budget.used() == beforeImport && system.getConstraints().data() == oldConstraints &&
                     system.getConstraints().size() == 1 && system.getConstraints()[0].id == "second",
                 "Denied property import unwinds parsed constraints and preserves the previous inventory" );
        budget.restore();
        require( system.setProperties( properties ) && system.getConstraints().size() == 2 &&
                     system.getConstraints()[1].id == oversized.id, "Complete property import retries with normalized Source storage" );

        scene::Animator animator;
        auto animatorProperties = make_ptr<Properties>();
        animatorProperties->addChild( properties );
        const auto beforeAnimatorImport = budget.used();
        budget.set( beforeAnimatorImport );
        denied( [&] { animator.setProperties( animatorProperties ); }, "Animator stages IK properties before playback mutations" );
        require( animator.getApplyOnPropertyChange() && animator.getIKSystem().getConstraints().empty() &&
                     budget.used() == beforeAnimatorImport,
                 "Rejected animator IK properties preserve callback policy and previous constraints" );
        budget.restore();

        Chain chain;
        animator.setSkeleton( chain.skeleton );
        require( animator.getIKSystem().addConstraint( constraint( "previous" ) ),
                 "Late animator property failure has an existing constraint generation" );
        AnimationIKSystem shortAuthored;
        require( shortAuthored.addConstraint( first ) && shortAuthored.addConstraint( second ),
                 "Short-ID animator import fixture is valid" );
        auto shortProperties = shortAuthored.getProperties();
        auto lateProperties = make_ptr<Properties>();
        lateProperties->addChild( shortProperties );
        u64 shortImportBytes = 0, shortChildBytes = 0;
        {
            const auto before = budget.used();
            AnimationIKSystem imported;
            require( imported.setProperties( shortProperties ), "Late denial import measurement succeeds" );
            shortImportBytes = budget.used() - before;
        }
        {
            const auto before = budget.used();
            auto children = shortProperties->getChildrenByName( "constraint" );
            shortChildBytes = budget.used() - before;
        }
        const auto previousIK = animator.getIKSystem().getConstraints().data();
        const auto beforeLateImport = budget.used();
        budget.set( beforeLateImport + shortImportBytes + shortChildBytes );
        denied( [&] { animator.setProperties( lateProperties ); },
                "Post-import result preparation may deny after candidate IK properties have staged completely" );
        require( budget.used() == beforeLateImport && animator.getApplyOnPropertyChange() && chain.untouched() &&
                     animator.getIKSystem().getConstraints().data() == previousIK &&
                     animator.getIKSystem().getConstraints().size() == 1 &&
                     animator.getIKSystem().getConstraints()[0].id == "previous",
                 "Late Animator property denial restores the previous IK owner and callback policy" );
        budget.restore();
    }
    inline void stagedSolves( Capacity &budget )
    {
        Chain chain;
        AnimationIKSystem system;
        require( system.addConstraint( constraint( "first" ) ), "Solve fixture first constraint inserts" );
        auto oversized = constraint( "second" );
        oversized.id.assign( CharacterPoolAllocator<char>::MaxPooledBlockBytes + 1, 'S' );
        require( system.addConstraint( oversized ), "Solve fixture second constraint inserts" );
        u64 stringBytes = 0;
        {
            const auto before = budget.used();
            String copy( oversized.id );
            stringBytes = budget.used() - before;
        }
        u64 preparedBytes = 0;
        {
            const auto before = budget.used();
            auto prepared = system.prepareSolve( chain.skeleton.get() );
            preparedBytes = budget.used() - before;
            require( prepared.getResults().size() == 2 && chain.untouched(),
                     "IK preparation captures complete outputs without solving any bones" );
        }
        const auto hierarchyBytes = arrayBytes<SmartPtr<IBone>>( 3 );
        const auto before = budget.used();
        for( const auto headroom : { arrayBytes<AnimationIKSolveResult>( 2 ) - 1,
                                    preparedBytes - hierarchyBytes - stringBytes,
                                    preparedBytes - 1 } )
        {
            budget.set( before + headroom );
            denied( [&] { auto results = system.solve( chain.skeleton.get() ); },
                    "Result inventory, later oversized ID and final ancestry scratch are independent admission gates" );
            require( budget.used() == before && chain.untouched(),
                     "Every partial solve admission unwinds before the first bone orientation is changed" );
        }
        budget.restore();
        auto prepared = system.prepareSolve( chain.skeleton.get() );
        const auto admitted = budget.used();
        budget.set( admitted );
        const auto denials = sourceBudgetDenialCount();
        require( system.setTarget( "first", target( -1, 1 ) ), "Prepared pass does not borrow mutable target settings" );
        system.solvePrepared( prepared );
        require( sourceBudgetDenialCount() == denials && budget.used() == admitted &&
                     prepared.getResults()[0].result.success && prepared.getResults()[1].result.success &&
                     prepared.getResults()[0].result.effectorError < 0.001f && !chain.untouched(),
                 "Prepared IK runs both constraints at the full cap with captured settings and no hidden ancestry allocation" );
        auto results = prepared.takeResults();
        require( sourceBudgetDenialCount() == denials && results.get_allocator().isSourceBudgeted() &&
                     results.capacity() == 2 && results[1].constraintId == oversized.id && budget.used() < admitted,
                 "Taking solve results transfers charged output ownership and retires command/scratch storage" );
        system.solvePrepared( prepared );
        budget.restore();
        std::optional<Array<AnimationIKSolveResult>> retained( results );
        retained->reserve( 9 );
        const auto retainedResident = budget.used();
        budget.set( retainedResident );
        const auto retainedDenials = sourceBudgetDenialCount();
        system.clear();
        results = makeSourceBudgetSnapshot<AnimationIKSolveResult>();
        require( sourceBudgetDenialCount() == retainedDenials && retained->capacity() == 9 &&
                     retained->back().constraintId == oversized.id && budget.used() < retainedResident,
                 "Retained result snapshots preserve oversized IDs and spare slots after producer teardown at capacity" );
        retained.reset();
        budget.restore();

        chain.reset();
        const auto directResident = budget.used();
        budget.set( directResident );
        denied( [&] { TwoBoneIKSolver::solve( chain.hand, target( 1, 1 ) ); },
                "The public single-chain solver admits its local hierarchy scratch" );
        require( chain.untouched() && budget.used() == directResident,
                 "Direct solver scratch denial occurs before any local bone writes" );
        budget.restore();
        Array<SmartPtr<IBone>> rawScratch;
        rawScratch.reserve( 11 );
        Transform3<real_Num> transform;
        require( TwoBoneIKSolver::getModelSpaceTransform( chain.hand, transform, rawScratch ) &&
                     rawScratch.get_allocator().isSourceBudgeted() && rawScratch.capacity() == 11,
                 "Borrowed raw model-transform scratch is normalized with its spare capacity" );

        require( system.addConstraint( constraint( "late" ) ), "Edited-hierarchy fixture constraint imports" );
        auto late = system.prepareSolve( chain.skeleton.get() );
        auto newParent = make_ptr<BoneFixture>();
        newParent->attach( chain.root );
        const auto beforeReparentedSolve = budget.used();
        budget.set( beforeReparentedSolve );
        denied( [&] { system.solvePrepared( late ); },
                "Prepared IK re-admits ancestry growth before applying any edited hierarchy" );
        require( chain.untouched() && budget.used() == beforeReparentedSolve &&
                     late.getResults()[0].result.status == TwoBoneIKStatus::NotSolved,
                 "Denied late ancestry growth preserves pending results and every bone orientation" );
        budget.restore();
        system.solvePrepared( late );
        require( late.getResults()[0].result.success, "Prepared hierarchy growth retries after admission" );
    }
    class ApplyingAnimation : public Animation
    {
    public:
        unsigned applied = 0;
        void apply( ISkeleton *skeleton, f32, f32, f32 ) override
        {
            ++applied;
            if( auto bone = skeleton->getBone( "root" ) )
                bone->setPosition( Vector3<real_Num>( 4, 0, 0 ) );
        }
    };
    inline void animatorPublication( Capacity &budget )
    {
        Chain chain;
        scene::Animator animator;
        auto animation = make_ptr<ApplyingAnimation>();
        animation->setLength( 1 );
        animator.setSkeleton( chain.skeleton );
        animator.addAnimation( SmartPtr<IAnimation>( animation ) );
        require( animator.getIKSystem().addConstraint( constraint( "first" ) ), "Animator IK fixture imports" );
        animator.onUpdate();
        require( animation->applied == 1 && animator.getLastIKSolveResults().size() == 1 &&
                     animator.getLastIKSolveResults().get_allocator().isSourceBudgeted(),
                 "Animator publishes admitted IK results after normal animation application" );
        chain.root->setPosition( Vector3<real_Num>::zero() );
        chain.reset();
        const auto outputs = animator.getLastIKSolveResults().data();
        const auto resident = budget.used();
        budget.set( resident );
        denied( [&] { animator.onUpdate(); }, "Animator pre-admits IK before any animation application" );
        require( animation->applied == 1 && chain.untouched() &&
                     chain.root->getPosition() == Vector3<real_Num>::zero() &&
                     animator.getLastIKSolveResults().data() == outputs && budget.used() == resident,
                 "Denied Animator IK output preserves skeleton, previous debug results and application count" );
        budget.restore();
        animator.onUpdate();
        require( animation->applied == 2 && animator.getLastIKSolveResults().size() == 1,
                 "Animator retries the complete animation and IK publication" );
        auto retained = animator.getLastIKSolveResults();
        const auto beforeUnload = budget.used();
        budget.set( beforeUnload );
        animator.unload( nullptr );
        require( animator.getIKSystem().getConstraints().capacity() == 0 &&
                     animator.getLastIKSolveResults().capacity() == 0 && retained.size() == 1 &&
                     budget.used() < beforeUnload, "Animator unload releases IK producer arrays while caller results survive" );
        budget.restore();
    }
    inline void bodies( Capacity &budget )
    {
        const auto before = budget.used();
        const auto body = sourceBudgetStorageBytes( sizeof( AnimationIKSystem ), alignof( std::max_align_t ) );
        budget.set( before + body - 1 );
        denied( [] { auto value = std::make_unique<AnimationIKSystem>(); }, "Heap IK system bodies require SourceCpu" );
        require( budget.used() == before, "Refused IK body leaves no allocation behind" );
        budget.set( before + body );
        auto value = std::make_unique<AnimationIKSystem>();
        require( budget.used() == before + body, "An empty IK wrapper charges exactly its physical body" );
        const auto denials = sourceBudgetDenialCount();
        value.reset();
        require( budget.used() == before && sourceBudgetDenialCount() == denials,
                 "IK wrapper destruction needs no admission at full capacity" );
        budget.restore();
        const auto preparedBody = sourceBudgetStorageBytes( sizeof( AnimationIKSystem::PreparedSolve ),
                                                           alignof( std::max_align_t ) );
        budget.set( before + preparedBody - 1 );
        denied( [] { auto value = std::make_unique<AnimationIKSystem::PreparedSolve>(); },
                "Heap prepared-solve wrapper bodies cannot bypass admission" );
        require( budget.used() == before, "Refused prepared-solve wrapper leaves no charge" );
        budget.restore();
    }
    inline void notificationRetirement( Capacity &budget )
    {
        struct LoadedApplication
        {
            SmartPtr<core::IApplicationManager> application = core::IApplicationManager::instance();
            LoadingState previous = application->getLoadingState();
            u8 flags = application->getObjectFlags();
            LoadedApplication()
            {
                application->setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, false );
                application->setLoadingState( LoadingState::Loaded );
                application->setObjectFlags( flags );
            }
            ~LoadedApplication()
            {
                application->setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, false );
                application->setLoadingState( previous );
                application->setObjectFlags( flags );
            }
        } loaded;
        Chain chain;
        scene::Animator animator;
        animator.setSkeleton( chain.skeleton );
        require( animator.getIKSystem().addConstraint( constraint( "retired" ) ),
                 "Notification retirement fixture has a resident IK constraint" );
        animator.onUpdate();
        auto constraints = animator.getIKSystem().getConstraints();
        auto results = animator.getLastIKSolveResults();
        const auto retiredBytes = arrayBytes<TwoBoneIKConstraint>( animator.getIKSystem().getConstraints().capacity() ) +
                                  arrayBytes<AnimationIKSolveResult>( animator.getLastIKSolveResults().capacity() );
        animator.setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, false );
        animator.setLoadingState( LoadingState::Loaded );
        animator.setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        animator.setObjectFlag( OBJECT_FLAG_LOADING_EVENTS, true );
        const auto resident = budget.used();
        budget.set( resident );
        const auto denials = sourceBudgetDenialCount();
        animator.unload( nullptr );
        require( animator.getLoadingState() == LoadingState::Unloaded &&
                     animator.getIKSystem().getConstraints().capacity() == 0 &&
                     animator.getLastIKSolveResults().capacity() == 0 &&
                     sourceBudgetDenialCount() >= denials + 2,
                 "Both refused loading notifications still complete Animator state and IK retirement" );
        require( budget.used() == resident - retiredBytes && constraints.size() == 1 && results.size() == 1 &&
                     constraints[0].id == "retired" && results[0].constraintId == "retired",
                 "Full-cap notification failure releases exact producer IK arrays while detached snapshots remain charged" );
        budget.restore();
    }
    inline void run()
    {
        Capacity budget;
        inventoryAndProperties( budget );
        stagedSolves( budget );
        animatorPublication( budget );
        notificationRetirement( budget );
        bodies( budget );
    }
}
