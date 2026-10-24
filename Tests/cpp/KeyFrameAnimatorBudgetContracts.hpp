#pragma once

#include <Workphone/Animation/Animator.hpp>
#include <Workphone/Animation/KeyFrameAnimator3.hpp>
#include <Workphone/Animation/KeyFrameAnimator2.hpp>
#include <Workphone/Animation/KeyFrameTransform3.hpp>
#include <Workphone/Math/LinearSpline3.hpp>
#include <Workphone/Math/LinearSpline2.hpp>
#include <Workphone/Math/RotationalSpline3.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include "MeshObjectBodyBudgetContracts.hpp"
#include <cmath>
#include <stdexcept>

namespace keyframe_animator_budget_contracts
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
                     "keyframe animator capacity applies" );
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

    template<class T> class LinearProbe : public LinearSpline3<T>
    {
    public:
        auto &times() { return this->m_times; }
        auto &points() { return this->m_points; }
        auto &tangents() { return this->m_tangents; }
    };
    template<class T> class RotationalProbe : public RotationalSpline3<T>
    {
    public:
        auto &points() { return this->m_points; }
        auto &tangents() { return this->m_tangents; }
    };
    template<class T> class Linear2Probe : public LinearSpline2<T>
    {
    public:
        auto &times() { return this->m_times; }
        auto &points() { return this->m_points; }
        auto &tangents() { return this->m_tangents; }
    };

    inline void bodies()
    {
        // Warm shared object metadata independently from all admitted wrappers.
        { auto a = make_ptr<KeyFrameAnimator3>(); auto b = make_ptr<KeyFrameAnimator3>();
          LinearSpline3F c; RotationalSpline3F d; LinearSpline3D e; RotationalSpline3D f; }
        mesh_object_body_budget_contracts::direct<Animator>();
        mesh_object_body_budget_contracts::factory<Animator>();
        mesh_object_body_budget_contracts::pooled<Animator>();
        mesh_object_body_budget_contracts::direct<LinearSpline3F>();
        mesh_object_body_budget_contracts::factory<LinearSpline3F>();
        mesh_object_body_budget_contracts::pooled<LinearSpline3F>();
        mesh_object_body_budget_contracts::direct<RotationalSpline3F>();
        mesh_object_body_budget_contracts::factory<RotationalSpline3F>();
        mesh_object_body_budget_contracts::pooled<RotationalSpline3F>();
        mesh_object_body_budget_contracts::direct<LinearSpline3D>();
        mesh_object_body_budget_contracts::direct<RotationalSpline3D>();
        mesh_object_body_budget_contracts::direct<KeyFrameAnimator2>();
        mesh_object_body_budget_contracts::factory<KeyFrameAnimator2>();
        mesh_object_body_budget_contracts::pooled<KeyFrameAnimator2>();
        mesh_object_body_budget_contracts::direct<LinearSpline2<f32>>();
        mesh_object_body_budget_contracts::factory<LinearSpline2<f32>>();
        mesh_object_body_budget_contracts::pooled<LinearSpline2<f32>>();
        mesh_object_body_budget_contracts::direct<LinearSpline2<f64>>();
        Capacity budget;
        auto baseline = budget.used();
        const auto body = bodyBytes<KeyFrameAnimator3>(), current = bodyBytes<KeyFrameTransform3>();
        for( auto headroom : { body - 1, body + current - 1 } )
        {
            budget.set( baseline + headroom );
            denied( [] { auto object = make_ptr<KeyFrameAnimator3>(); },
                    "animator body and initial sampled-key body require separate admission" );
            require( budget.used() == baseline, "partial animator construction retires wrapper and current-key body" );
        }
        budget.set( baseline + body + current );
        auto object = make_ptr<KeyFrameAnimator3>();
        require( budget.used() == baseline + body + current,
                 "empty animator owns exactly its wrapper and one sampled-key body" );
        auto escaped = object->getCurrentKeyFrame();
        require( escaped && escaped->getReferences() == 2, "sampled key has one animator and one escaped owner" );
        const auto denials = sourceBudgetDenialCount();
        object->unload( nullptr ); object = nullptr;
        require( budget.used() == baseline + current && sourceBudgetDenialCount() == denials,
                 "at-cap wrapper retirement preserves only externally pinned sampled-key body" );
        escaped = nullptr;
        require( budget.used() == baseline, "last sampled-key owner retires the initial intrusive reference" );
        budget.restore();
        {
            FactoryTemplate<KeyFrameAnimator3> factory;
            factory.load( nullptr ); factory.setPool( nullptr );
            const auto empty = budget.used();
            const auto heapBody = sourceBudgetStorageBytes( sizeof( KeyFrameAnimator3 ), alignof( KeyFrameAnimator3 ) );
            const auto inventories = arrayBytes<KeyFrameAnimator3 *>( 1 ) + arrayBytes<SmartPtr<ISharedObject>>( 1 );
            budget.set( empty + heapBody + current - 1 );
            denied( [&] { auto value = factory.make_ptr<KeyFrameAnimator3>(); },
                    "factory partial constructor denial retires borrowed heap body" );
            require( budget.used() == empty, "failed factory animator publishes no owner inventory" );
            budget.set( empty + heapBody + current + inventories );
            auto value = factory.make_ptr<KeyFrameAnimator3>();
            require( value && budget.used() == empty + heapBody + current + inventories,
                     "factory animator charges actual heap alignment, sampled body, and pointer inventories" );
            value = nullptr;
            require( budget.used() == empty + inventories, "factory last owner retires animator and initial sampled key" );
            factory.unload( nullptr );
            require( budget.used() == empty, "factory inventory capacity retires at cap" );
        }
        budget.restore(); baseline = budget.used();
        {
            auto pool = make_ptr<Pool<KeyFrameAnimator3>>(); pool->setNextSize( 2 );
            auto slot = pool->allocate_object();
            const auto retained = budget.used();
            require( slot && retained >= baseline + sourceBudgetStorageBytes(
                         2 * sizeof( KeyFrameAnimator3 ), alignof( KeyFrameAnimator3 ) ),
                     "animator pool includes physical unused object slots" );
            budget.set( retained + current - 1 );
            denied( [&] { auto value = new( slot.get() ) KeyFrameAnimator3; },
                    "pooled constructor must admit its separate sampled-key body" );
            require( budget.used() == retained, "failed pooled constructor preserves reusable borrowed slot" );
            budget.set( retained + current );
            auto value = new( slot.get() ) KeyFrameAnimator3;
            require( budget.used() == retained + current, "pool placement does not double-charge animator wrapper" );
            value->~KeyFrameAnimator3(); pool->free_object( value );
            require( budget.used() == retained, "pooled animator destructor retires sampled key at cap" );
            pool = nullptr;
            require( budget.used() == baseline, "pool teardown retires animator slab and inventory capacity" );
        }
    }

    inline void keyframes()
    {
        Capacity budget;
        KeyFrameAnimator3 animator;
        auto sample = animator.getCurrentKeyFrame();
        auto first = make_ptr<KeyFrameTransform3>();
        auto second = make_ptr<KeyFrameTransform3>();
        first->setTime( 0 ); first->setPosition( { 0, 0, 0 } ); first->setScale( { 1, 1, 1 } );
        second->setTime( 2 ); second->setPosition( { 10, 0, 0 } ); second->setScale( { 3, 3, 3 } );
        second->setOrientation( Quaternion<real_Num>( 0, 0, 1, 0 ) );
        Array<SmartPtr<KeyFrameTransform3>> old; old.push_back( first );
        animator.setKeyFrames( old );
        Array<SmartPtr<KeyFrameTransform3>> input;
        input.reserve( 19 ); input.push_back( second ); input.push_back( nullptr );
        input.push_back( first ); input.push_back( first );
        const auto resident = budget.used();
        const auto pointer = animator.getKeyFrames().data();
        budget.set( resident + arrayBytes<SmartPtr<KeyFrameTransform3>>( 19 ) - 1 );
        denied( [&] { animator.setKeyFrames( input ); }, "bulk keyframe setter must admit caller spare capacity before publication" );
        require( budget.used() == resident && animator.getKeyFrames().data() == pointer &&
                     animator.getKeyFrames().size() == 1 && animator.getCurrentKeyFrame() == sample,
                 "refused bulk setter preserves inventory, aliases and current sample" );
        budget.restore(); animator.setKeyFrames( input );
        require( animator.getKeyFrames().capacity() == 19 && animator.getKeyFrames().size() == 3 &&
                     animator.getKeyFrames()[0] == first && animator.getKeyFrames()[1] == first &&
                     animator.getKeyFrames()[2] == second && animator.getKeyFrames().get_allocator().isSourceBudgeted(),
                 "bulk import keeps aliases and spare capacity, filters nulls, and sorts once" );
        const auto imported = budget.used();
        budget.set( imported );
        denied( [&] { animator.setKeyFrames( animator.getKeyFrames() ); }, "self-import still requires independent staged storage" );
        denied( [&] { auto copy = animator.getKeyFrames(); }, "escaped keyframe snapshots require admission" );
        require( budget.used() == imported, "denied self-import and snapshot retain no partial capacity" );
        budget.restore();
        auto retained = animator.getKeyFrames();
        require( retained.capacity() == 19 && budget.used() == imported + arrayBytes<SmartPtr<KeyFrameTransform3>>( 19 ),
                 "escaped keyframe snapshots charge actual spare capacity" );
        Array<SmartPtr<KeyFrameTransform3>> pair;
        pair.reserve( 11 ); pair.push_back( second ); pair.push_back( first );
        animator.addKeyFrames( pair );
        require( animator.getKeyFrames().size() == 2 && animator.getKeyFrames().capacity() == 11,
                 "bulk add retains the established replacement semantics" );
        animator.setAnimationLength( 2 ); animator.setAnimationTime( 1 );
        budget.set( budget.used() );
        const auto noGrowth = sourceBudgetDenialCount();
        animator.update();
        require( animator.getCurrentKeyFrame() == sample && sample->getPosition().x == 5 &&
                     sample->getScale().x == 2 && sample->getOrientation() == second->getOrientation(),
                 "allocation-free playback preserves sampled position, scale and orientation behavior" );
        animator.start(); animator.pause();
        require( animator.isPaused() && !animator.isPlaying(), "pause preserves playback state at cap" );
        animator.resume(); require( animator.isPlaying(), "resume preserves playback state at cap" );
        animator.stop();
        require( !animator.isPlaying() && animator.getAnimationTime() == 0 && sourceBudgetDenialCount() == noGrowth,
                 "playback state changes need no new Source admission" );
        unsigned completions = 0;
        animator.setOnCompleteCallback( [&completions] { ++completions; } );
        animator.start(); animator.setAnimationTime( 2 ); animator.update();
        require( !animator.isPlaying() && animator.isFinished() && completions == 1,
                 "forward completion invokes its retained callback exactly once" );
        animator.setLoop( true ); animator.start(); animator.setAnimationTime( 2 ); animator.update();
        require( animator.isPlaying() && animator.getAnimationTime() == 0 && completions == 1,
                 "looping restarts without completing the animation" );
        animator.stop(); animator.setLoop( false ); animator.setReverse( true );
        animator.start(); animator.setAnimationTime( 0 ); animator.update();
        require( !animator.isPlaying() && completions == 2 && sourceBudgetDenialCount() == noGrowth,
                 "reverse completion preserves allocation-free playback and callback behavior" );
        animator.setReverse( false ); animator.setOnCompleteCallback( nullptr );
        budget.restore(); animator.setKeyFrames( old ); animator.update();
        require( animator.getCurrentKeyFrame() == first, "single-key playback keeps its established authored-key alias" );
        bool completed = false;
        animator.setOnCompleteCallback( [&completed] { completed = true; } );
        animator.start();
        const auto beforeUnload = budget.used();
        budget.set( beforeUnload );
        const auto beforeDenials = sourceBudgetDenialCount();
        animator.unload( nullptr );
        require( animator.getKeyFrames().capacity() == 0 && !animator.getCurrentKeyFrame() &&
                     !animator.isPlaying() && !completed && retained[2] == second &&
                     budget.used() == beforeUnload - arrayBytes<SmartPtr<KeyFrameTransform3>>( 1 ) &&
                     sourceBudgetDenialCount() == beforeDenials,
                 "at-cap unload retires only owned key capacity while escaped keys/samples survive" );
        const auto empty = budget.used();
        budget.set( empty + arrayBytes<SmartPtr<KeyFrameTransform3>>( 11 ) + bodyBytes<KeyFrameTransform3>() - 1 );
        denied( [&] { animator.setKeyFrames( pair ); }, "reuse after unload must admit a new sampled-key body before publishing keys" );
        require( budget.used() == empty && animator.getKeyFrames().empty() && !animator.getCurrentKeyFrame(),
                 "reinitialization denial retires staged key inventory" );
        budget.restore(); animator.setKeyFrames( pair );
        animator.setAnimationTime( 1 ); animator.update();
        require( animator.getCurrentKeyFrame()->getPosition().x == 5, "animator can be reused after unload and retry" );
    }

    inline void linearSplines()
    {
        Capacity budget;
        LinearProbe<f32> spline;
        spline.addPoint( { -1, 0, 0 } );
        Array<f32> keys; keys.reserve( 17 ); keys.push_back( 0 ); keys.push_back( 1 ); keys.push_back( 2 );
        Array<Vector3F> positions; positions.reserve( 23 );
        positions.push_back( { 0, 0, 0 } ); positions.push_back( { 2, 0, 0 } ); positions.push_back( { 4, 0, 0 } );
        const auto resident = budget.used();
        const auto pointer = spline.points().data();
        const auto times = arrayBytes<f32>( 17 ), points = arrayBytes<Vector3F>( 23 ), tangents = arrayBytes<Vector3F>( 4 );
        for( auto headroom : { times - 1, times + points - 1, times + points + tangents - 1 } )
        {
            budget.set( resident + headroom );
            denied( [&] { spline.setPoints( keys, positions ); }, "spline time, point and tangent inventories require separate admission" );
            require( budget.used() == resident && spline.getNumPoints() == 1 && spline.points().data() == pointer &&
                         spline.getPoint( 0 ).x == -1 && spline.times().empty() && spline.tangents().empty(),
                     "partial spline setter denial preserves every published inventory" );
        }
        budget.restore(); spline.setPoints( keys, positions );
        require( spline.times().capacity() == 17 && spline.points().capacity() == 23 && spline.tangents().capacity() == 4 &&
                     spline.times().get_allocator().isSourceBudgeted() && spline.points().get_allocator().isSourceBudgeted() &&
                     spline.tangents().get_allocator().isSourceBudgeted() &&
                     budget.used() == resident - arrayBytes<Vector3F>( 1 ) + times + points + tangents,
                 "spline import preserves caller spare capacity and admits derived tangent storage" );
        budget.set( budget.used() );
        const auto denials = sourceBudgetDenialCount();
        spline.updatePoint( 1, { 3, 0, 0 } ); spline.recalcTangents();
        require( spline.interpolate( 0.5f ).x == 3 && sourceBudgetDenialCount() == denials,
                 "existing spline points and tangent recomputation work without new capacity" );
        spline.addPoint( { 6, 0, 0 } );
        const auto fourPoints = budget.used();
        const auto oldTangents = spline.tangents().data();
        budget.set( fourPoints + arrayBytes<Vector3F>( 8 ) - 1 );
        denied( [&] { spline.addPoint( { 8, 0, 0 } ); }, "tangent growth is admitted before otherwise-spare point append" );
        require( budget.used() == fourPoints && spline.getNumPoints() == 4 && spline.tangents().data() == oldTangents,
                 "failed tangent growth cannot expose an extra spline point" );
        budget.restore(); spline.addPoint( { 8, 0, 0 } );
        Array<Pair<f32, Vector3F>> additions; additions.reserve( 13 );
        additions.push_back( { 3, { 10, 0, 0 } } ); additions.push_back( { 4, { 12, 0, 0 } } );
        const auto beforeAppend = budget.used();
        budget.set( beforeAppend + times + points - 1 );
        denied( [&] { spline.setPoints( additions ); }, "paired append stages both time and point inventories" );
        require( budget.used() == beforeAppend && spline.getNumPoints() == 5 && spline.times().size() == 3,
                 "paired-append refusal leaves both inventories intact" );
        budget.restore(); spline.setPoints( additions );
        require( spline.getNumPoints() == 7 && spline.times().size() == 5 && spline.getPoint( 6 ).x == 12,
                 "paired point setter preserves existing bulk-append behavior" );
        const auto withSpline = budget.used();
        auto snapshot = spline.points();
        require( budget.used() == withSpline + points, "caller spline snapshot retains actual point spare capacity" );
        const auto owned = arrayBytes<f32>( spline.times().capacity() ) + arrayBytes<Vector3F>( spline.points().capacity() ) +
                           arrayBytes<Vector3F>( spline.tangents().capacity() );
        budget.set( budget.used() );
        const auto beforeClear = budget.used(), beforeClearDenials = sourceBudgetDenialCount();
        spline.clear();
        require( spline.getNumPoints() == 0 && spline.times().empty() && spline.tangents().empty() && budget.used() == beforeClear,
                 "logical spline clear keeps physical spare storage charged" );
        spline.unload( nullptr );
        require( spline.points().capacity() == 0 && spline.times().capacity() == 0 && spline.tangents().capacity() == 0 &&
                     budget.used() == beforeClear - owned && snapshot.size() == 7 && sourceBudgetDenialCount() == beforeClearDenials,
                 "at-cap spline unload retires owned arrays without invalidating escaped snapshots" );
    }

    inline void splineCopies()
    {
        Capacity budget;
        LinearProbe<f32> source, target;
        Array<f32> keys; keys.reserve( 7 ); keys.push_back( 0 ); keys.push_back( 1 );
        Array<Vector3F> points; points.reserve( 9 ); points.push_back( { 0, 0, 0 } ); points.push_back( { 4, 0, 0 } );
        source.setPoints( keys, points ); target.addPoint( { -1, 0, 0 } );
        const auto resident = budget.used();
        const auto timeBytes = arrayBytes<f32>( 7 ), pointBytes = arrayBytes<Vector3F>( 9 ), tangentBytes = arrayBytes<Vector3F>( 2 );
        const auto targetPointer = target.points().data(), sourcePointer = source.points().data();
        auto targetHandle = target.getHandle();
        for( auto headroom : { timeBytes - 1, timeBytes + pointBytes - 1, timeBytes + pointBytes + tangentBytes - 1 } )
        {
            budget.set( resident + headroom );
            denied( [&] { static_cast<LinearSpline3F &>( target ) = source; }, "spline assignment admits all payload arrays before commit" );
            require( budget.used() == resident && target.points().data() == targetPointer && target.getPoint( 0 ).x == -1 &&
                         source.points().data() == sourcePointer && target.getHandle() == targetHandle,
                     "copy-assignment denial preserves both payload and object identity" );
        }
        budget.restore(); static_cast<LinearSpline3F &>( target ) = source;
        require( target.getHandle() == targetHandle && target.getHandle() != source.getHandle() &&
                     target.points().data() != source.points().data() && target.getPoint( 1 ).x == 4,
                 "spline assignment owns independent arrays without copying shared-object metadata" );
        const auto beforeCopy = budget.used();
        {
            LinearSpline3F copy( source );
            require( copy.getHandle() != source.getHandle() && copy.getPoint( 1 ).x == 4 &&
                         budget.used() == beforeCopy + timeBytes + pointBytes + tangentBytes,
                     "spline copy constructor duplicates only admitted capacities and fresh object identity" );
        }
        require( budget.used() == beforeCopy && source.getPoint( 1 ).x == 4,
                 "retiring copied spline cannot retire source metadata or arrays" );
    }

    inline void rotationalSplines()
    {
        Capacity budget;
        RotationalProbe<f32> spline;
        const auto first = QuaternionF::identity();
        const auto second = QuaternionF( std::sqrt( .5f ), 0, std::sqrt( .5f ), 0 );
        spline.addPoint( first );
        const auto resident = budget.used(), pairBytes = arrayBytes<QuaternionF>( 2 );
        const auto pointer = spline.points().data();
        for( auto headroom : { pairBytes - 1, 2 * pairBytes - 1 } )
        {
            budget.set( resident + headroom );
            denied( [&] { spline.addPoint( second ); }, "rotation spline admits tangent and point growth before publication" );
            require( budget.used() == resident && spline.getNumPoints() == 1 && spline.points().data() == pointer && spline.tangents().empty(),
                     "rotation-spline denial preserves old point inventory and absent tangents" );
        }
        budget.restore(); spline.addPoint( second );
        const auto middle = spline.interpolate( .5f );
        require( spline.interpolate( 0.0f ) == first && spline.interpolate( 1.0f ) == second &&
                     std::isfinite( middle.W() ) && middle != first && middle != second,
                 "rotation spline retains endpoint and interior SQUAD behavior" );
        spline.points().reserve( 13 ); spline.setAutoCalculate( false ); spline.addPoint( first );
        const auto manual = budget.used();
        budget.set( manual + arrayBytes<QuaternionF>( 4 ) - 1 );
        denied( [&] { spline.recalcTangents(); }, "manual tangent rebuild is independently admitted" );
        require( budget.used() == manual && spline.tangents().size() == 2 && spline.getNumPoints() == 3,
                 "denied manual rebuild preserves previous tangents and manual points" );
        budget.restore(); spline.recalcTangents(); spline.setAutoCalculate( true );
        RotationalProbe<f32> copy;
        const auto beforeCopy = budget.used();
        const auto pointBytes = arrayBytes<QuaternionF>( 13 ), tangentBytes = arrayBytes<QuaternionF>( 4 );
        budget.set( beforeCopy + pointBytes + tangentBytes - 1 );
        denied( [&] { static_cast<RotationalSpline3F &>( copy ) = spline; }, "rotation spline assignment stages both arrays" );
        require( budget.used() == beforeCopy && copy.getNumPoints() == 0,
                 "failed rotation copy assignment leaves target empty" );
        budget.restore(); static_cast<RotationalSpline3F &>( copy ) = spline;
        require( copy.getHandle() != spline.getHandle() && copy.points().capacity() == 13 && copy.tangents().capacity() == 4 &&
                     copy.points().get_allocator().isSourceBudgeted() && copy.tangents().get_allocator().isSourceBudgeted(),
                 "rotation copies retain admitted spare capacity and independent object identity" );
        budget.set( budget.used() );
        const auto denials = sourceBudgetDenialCount();
        spline.updatePoint( 1, second ); spline.recalcTangents();
        const auto beforeClear = budget.used();
        spline.clear();
        require( budget.used() == beforeClear && spline.getNumPoints() == 0,
                 "rotation clear keeps spare capacity charged" );
        spline.unload( nullptr );
        require( budget.used() == beforeClear - pointBytes - tangentBytes && copy.getNumPoints() == 3 &&
                     sourceBudgetDenialCount() == denials,
                 "rotation unload frees only its own arrays at the cap" );
    }

    inline void keyframes2D()
    {
        Capacity budget;
        KeyFrameAnimator2 animator;
        require( !animator.getCurrentKeyFrame(), "2D current sample remains lazy at construction" );
        auto first = make_ptr<KeyFrameTransform2>(), second = make_ptr<KeyFrameTransform2>();
        first->setTime( 0 ); first->setPosition( { 0, 0 } ); first->setScale( { 1, 1 } ); first->setRotation( 0 );
        second->setTime( 2 ); second->setPosition( { 10, 4 } ); second->setScale( { 3, 5 } ); second->setRotation( 90 );
        Array<SmartPtr<KeyFrameTransform2>> old; old.push_back( first ); animator.setKeyFrames( old );
        Array<SmartPtr<KeyFrameTransform2>> input; input.reserve( 17 );
        input.push_back( second ); input.push_back( first ); input.push_back( nullptr );
        const auto resident = budget.used();
        const auto pointer = animator.getKeyFrames().data();
        budget.set( resident + arrayBytes<SmartPtr<KeyFrameTransform2>>( 17 ) - 1 );
        denied( [&] { animator.setKeyFrames( input ); }, "2D replacement admits complete caller spare capacity" );
        require( budget.used() == resident && animator.getKeyFrames().data() == pointer &&
                     animator.getKeyFrames().size() == 1 && !animator.getCurrentKeyFrame(),
                 "2D replacement refusal preserves the old inventory and lazy sample" );
        budget.restore(); animator.setKeyFrames( input );
        require( animator.getKeyFrames().capacity() == 17 && animator.getKeyFrames().size() == 2 &&
                     animator.getKeyFrames()[0] == first && animator.getKeyFrames()[1] == second &&
                     animator.getKeyFrames().get_allocator().isSourceBudgeted() && !animator.getCurrentKeyFrame(),
                 "2D replacement filters nulls, sorts once and retains Source spare capacity without sampling" );
        const auto beforeAppend = budget.used();
        budget.set( beforeAppend + arrayBytes<SmartPtr<KeyFrameTransform2>>( 19 ) - 1 );
        denied( [&] { animator.addKeyFrames( animator.getKeyFrames() ); }, "2D self-append stages independent old and imported owners" );
        require( budget.used() == beforeAppend && animator.getKeyFrames().size() == 2,
                 "2D bulk-append refusal publishes no partial keys" );
        budget.restore(); animator.addKeyFrames( animator.getKeyFrames() );
        require( animator.getKeyFrames().size() == 4 && animator.getKeyFrames().capacity() == 19 &&
                     animator.getKeyFrames()[0] == first && animator.getKeyFrames()[1] == first &&
                     animator.getKeyFrames()[2] == second && animator.getKeyFrames()[3] == second,
                 "2D bulk add preserves its existing append semantics and duplicate aliases" );
        animator.setKeyFrames( input );
        animator.setAnimationLength( 2 ); animator.start(); animator.pause(); animator.setAnimationTime( 1 );
        const auto beforeSample = budget.used();
        budget.set( beforeSample + bodyBytes<KeyFrameTransform2>() - 1 );
        denied( [&] { animator.update(); }, "lazy 2D sample body must be admitted before frame-time mutation" );
        require( budget.used() == beforeSample && !animator.getCurrentKeyFrame() && animator.isPaused() &&
                     animator.getAnimationTime() == 1,
                 "refused first 2D sample preserves current pointer and playback time/state" );
        budget.set( beforeSample + bodyBytes<KeyFrameTransform2>() ); animator.update();
        auto sample = animator.getCurrentKeyFrame();
        require( sample && sample != first && sample != second && sample->getPosition().x == 5 &&
                     sample->getPosition().y == 2 && sample->getScale().x == 2 && sample->getScale().y == 3 &&
                     sample->getRotation() == 45 && sample->getTime() == 0,
                 "2D paused-active sampling retains position, scale, angle and current-key time semantics" );
        const auto denials = sourceBudgetDenialCount();
        animator.update(); animator.stop(); animator.setAnimationTime( 2 ); animator.update();
        require( sample->getPosition().x == 5 && sourceBudgetDenialCount() == denials,
                 "stopped 2D updates do not change the retained sample or allocate" );
        denied( [&] { auto snapshot = animator.getKeyFrames(); }, "escaped 2D inventories require Source capacity" );
        budget.restore();
        auto retained = animator.getKeyFrames();
        require( retained.capacity() == 17 && retained.get_allocator().isSourceBudgeted(),
                 "2D caller snapshots preserve admitted spare capacity" );
        animator.setKeyFrames( old ); animator.start(); animator.pause(); animator.update();
        require( animator.getCurrentKeyFrame() == first, "single-key 2D sampling preserves the authored-key alias" );
        const auto beforeUnload = budget.used();
        budget.set( beforeUnload );
        const auto teardownDenials = sourceBudgetDenialCount();
        animator.unload( nullptr );
        require( !animator.getCurrentKeyFrame() && animator.getKeyFrames().capacity() == 0 && !animator.isPlaying() &&
                     !animator.isPaused() && retained[1] == second && sample->getPosition().x == 5 &&
                     budget.used() == beforeUnload - arrayBytes<SmartPtr<KeyFrameTransform2>>( 1 ) &&
                     sourceBudgetDenialCount() == teardownDenials,
                 "at-cap 2D unload releases owned inventory while external keys and sampled body survive" );
        budget.restore(); animator.setKeyFrames( input );
        require( !animator.getCurrentKeyFrame(), "2D reimport after unload preserves lazy sampling" );
    }

    inline void linearSplines2D()
    {
        Capacity budget;
        Linear2Probe<f32> spline;
        Array<Vector2F> initial; initial.push_back( { -1, 0 } ); spline.setPoints( initial );
        Array<f32> keys; keys.reserve( 11 ); keys.push_back( 0 ); keys.push_back( 1 );
        Array<Vector2F> positions; positions.reserve( 13 ); positions.push_back( { 0, 1 } ); positions.push_back( { 4, 2 } );
        const auto resident = budget.used();
        const auto pointer = spline.points().data();
        const auto times = arrayBytes<f32>( 11 ), points = arrayBytes<Vector3F>( 14 ), tangents = arrayBytes<Vector3F>( 4 );
        for( auto headroom : { times - 1, times + points - 1, times + points + tangents - 1 } )
        {
            budget.set( resident + headroom );
            denied( [&] { spline.setPoints( keys, positions ); }, "2D spline admits time, converted point and tangent arrays before commit" );
            require( budget.used() == resident && spline.points().data() == pointer && spline.getNumPoints() == 1 &&
                         spline.points()[0].x == -1 && spline.times().empty() && spline.tangents().empty(),
                     "2D spline partial setter refusal preserves all prior inventories" );
        }
        budget.restore(); spline.setPoints( keys, positions );
        require( spline.times().capacity() == 11 && spline.points().capacity() == 14 && spline.tangents().capacity() == 4 &&
                     spline.getNumPoints() == 3 && spline.points()[1].y == 1 && spline.points()[2].z == 0 &&
                     spline.times().get_allocator().isSourceBudgeted() && spline.points().get_allocator().isSourceBudgeted() &&
                     budget.used() == resident - arrayBytes<Vector3F>( 1 ) + times + points + tangents,
                 "2D key/point setter preserves existing point-append behavior and actual converted spare capacity" );
        require( spline.interpolate( 0.0f ) == Vector2F( -1, 0 ) && spline.interpolate( 1.0f ) == Vector2F( 4, 2 ),
                 "2D Hermite spline retains endpoint sampling after automatic tangent calculation" );
        const auto beforeAppend = budget.used();
        const auto largerPoints = arrayBytes<Vector3F>( 16 ), largerTangents = arrayBytes<Vector3F>( 8 );
        for( auto headroom : { largerPoints - 1, largerPoints + largerTangents - 1 } )
        {
            budget.set( beforeAppend + headroom );
            denied( [&] { spline.setPoints( positions ); }, "uniform 2D append stages converted positions and derived tangent growth" );
            require( budget.used() == beforeAppend && spline.getNumPoints() == 3 && spline.points().capacity() == 14,
                     "denied 2D append preserves point count and physical inventory" );
        }
        budget.restore(); spline.setPoints( positions );
        Array<Pair<f32, Vector2F>> additions; additions.reserve( 7 ); additions.push_back( { 2, { 8, 3 } } );
        const auto paired = budget.used();
        budget.set( paired + times + largerPoints - 1 );
        denied( [&] { spline.setPoints( additions ); }, "paired 2D import stages both index and position ownership" );
        require( budget.used() == paired && spline.getNumPoints() == 5 && spline.times().size() == 2,
                 "paired 2D refusal cannot publish one inventory ahead of the other" );
        budget.restore(); spline.setPoints( additions );
        require( spline.getNumPoints() == 6 && spline.times().size() == 3 && spline.points()[5].x == 8,
                 "paired 2D imports preserve append semantics" );
        Linear2Probe<f32> target;
        const auto source = budget.used();
        const auto payload = times + largerPoints + largerTangents;
        for( auto headroom : { times - 1, times + largerPoints - 1, payload - 1 } )
        {
            budget.set( source + headroom );
            denied( [&] { static_cast<LinearSpline2<f32> &>( target ) = spline; }, "2D spline assignment stages every owned array" );
            require( budget.used() == source && target.getNumPoints() == 0,
                     "partial 2D spline assignment denial leaves target empty" );
        }
        budget.restore(); static_cast<LinearSpline2<f32> &>( target ) = spline;
        require( target.getHandle() != spline.getHandle() && target.points().data() != spline.points().data() &&
                     target.times().capacity() == 11 && target.points().capacity() == 16 && target.tangents().capacity() == 8,
                 "2D spline assignment owns admitted spare capacity and independent object identity" );
        const auto beforeCopy = budget.used();
        {
            LinearSpline2<f32> copy( spline );
            require( copy.getHandle() != spline.getHandle() && copy.interpolate( 1.0f ) == Vector2F( 8, 3 ) &&
                         budget.used() == beforeCopy + payload,
                     "2D spline copy constructor admits its payload without sharing base metadata" );
        }
        require( budget.used() == beforeCopy, "copied 2D spline destruction retires its exact payload" );
        spline.setAutoCalculate( false );
        Array<Vector2F> manual; manual.reserve( 5 );
        manual.push_back( { 10, 4 } ); manual.push_back( { 12, 5 } ); manual.push_back( { 14, 6 } );
        spline.setPoints( manual );
        const auto beforeRecalc = budget.used();
        budget.set( beforeRecalc + arrayBytes<Vector3F>( 16 ) - 1 );
        denied( [&] { spline.recalcTangents(); }, "2D manual tangent rebuild requires its own growth admission" );
        require( budget.used() == beforeRecalc && spline.getNumPoints() == 9 && spline.tangents().size() == 6,
                 "refused 2D manual rebuild leaves prior tangent data unchanged" );
        budget.restore(); spline.recalcTangents();
        auto retained = spline.points();
        const auto owned = times + largerPoints + arrayBytes<Vector3F>( 16 );
        const auto withSnapshot = budget.used();
        budget.set( withSnapshot );
        const auto denials = sourceBudgetDenialCount();
        spline.recalcTangents(); spline.clear();
        require( budget.used() == withSnapshot && spline.times().empty() && spline.tangents().empty(),
                 "2D logical clear retains charged physical spare storage" );
        spline.unload( nullptr );
        require( spline.times().capacity() == 0 && spline.points().capacity() == 0 && spline.tangents().capacity() == 0 &&
                     budget.used() == withSnapshot - owned && retained.size() == 9 && target.getNumPoints() == 6 &&
                     sourceBudgetDenialCount() == denials,
                 "2D spline unload retires only its owned arrays and preserves escaped snapshots at cap" );
    }

    inline void callbackRestart2D()
    {
        Capacity budget;
        KeyFrameAnimator2 animator;
        auto first = make_ptr<KeyFrameTransform2>(), second = make_ptr<KeyFrameTransform2>();
        first->setTime( 0 ); first->setPosition( { 0, 0 } );
        second->setTime( 2 ); second->setPosition( { 10, 4 } );
        Array<SmartPtr<KeyFrameTransform2>> initial; initial.push_back( first );
        Array<SmartPtr<KeyFrameTransform2>> restarted; restarted.reserve( 7 );
        restarted.push_back( first ); restarted.push_back( second );
        animator.setKeyFrames( initial ); animator.setAnimationLength( 1 );
        unsigned callbacks = 0;
        animator.setOnCompleteCallback( [&] {
            ++callbacks;
            animator.setKeyFrames( restarted ); animator.setAnimationLength( 2 );
            animator.start(); animator.setAnimationTime( 1 );
        } );
        animator.start(); animator.setAnimationTime( 1 );
        const auto before = budget.used();
        const auto pointer = animator.getKeyFrames().data();
        budget.set( before + bodyBytes<KeyFrameTransform2>() - 1 );
        denied( [&] { animator.update(); }, "callback restart sample must be admitted before completion callback mutation" );
        require( callbacks == 0 && !animator.getCurrentKeyFrame() && animator.isPlaying() &&
                     animator.getAnimationTime() == 1 && animator.getKeyFrames().data() == pointer &&
                     animator.getKeyFrames().size() == 1 && budget.used() == before,
                 "cold single-key sample denial cannot consume completion or publish callback changes" );
        budget.set( before + bodyBytes<KeyFrameTransform2>() + arrayBytes<SmartPtr<KeyFrameTransform2>>( 7 ) );
        animator.update();
        auto sample = animator.getCurrentKeyFrame();
        require( callbacks == 1 && animator.isPlaying() && animator.getAnimationTime() == 1 && sample &&
                     sample != first && sample != second && sample->getPosition().x == 5 && sample->getPosition().y == 2 &&
                     animator.getKeyFrames().size() == 2 && animator.getKeyFrames().capacity() == 7,
                 "callback restart retry publishes new keys and consumes the pre-admitted sampled body" );
        budget.restore();
        KeyFrameAnimator2 inactive;
        inactive.setKeyFrames( restarted );
        inactive.setOnCompleteCallback( [&callbacks] { ++callbacks; } );
        budget.set( budget.used() );
        const auto denials = sourceBudgetDenialCount();
        inactive.update();
        require( !inactive.getCurrentKeyFrame() && callbacks == 1 && sourceBudgetDenialCount() == denials,
                 "inactive cold callbacks do not force sample allocation" );
        budget.restore(); inactive.setKeyFrames( initial );
        inactive.start(); inactive.pause();
        budget.set( budget.used() );
        const auto pausedDenials = sourceBudgetDenialCount();
        inactive.update();
        require( inactive.getCurrentKeyFrame() == first && callbacks == 1 && sourceBudgetDenialCount() == pausedDenials,
                 "paused single-key callbacks preserve allocation-free authored-key alias sampling" );
    }

    inline void run()
    { bodies(); keyframes(); linearSplines(); splineCopies(); rotationalSplines(); keyframes2D(); linearSplines2D(); callbackRestart2D(); }
}
