#ifndef WP_SHARED_SCENE_INVENTORY_BUDGET_TESTS_HPP
#define WP_SHARED_SCENE_INVENTORY_BUDGET_TESTS_HPP

namespace
{
    class SharedInventoryProbe : public ComponentSystem
    {
    public:
        using ComponentSystem::getObject;
        bool refuseDerived = false;
        size_t componentCapacity() const { return m_components.capacity(); }
        size_t stateCapacity() const { return m_loadingStates.capacity(); }
    protected:
        void reserveData( size_t ) override
        {
            if( refuseDerived ) throw std::bad_alloc();
        }
    };

    class SceneQueueBudgetProbe : public GameScene
    {
    public:
        size_t playCount() const { return m_playQueue.size(); }
        size_t editCount() const { return m_editQueue.size(); }
        size_t phaseCount() const { return m_updateObjects.size(); }
    };
}

BOOST_AUTO_TEST_CASE( source_list_slabs_directories_copies_and_moves_retain_exact_ownership )
{
    TestGuard guard; CoordinatorBudget budget;
    const auto baseline = budget.used();
    budget.set( baseline );
    {
        List<int> ordinary( 2, GrowthPolicy::Fixed, 1 );
        ordinary.push_back( 1 ); ordinary.push_back( 2 );
        BOOST_CHECK_THROW( ordinary.push_back( 3 ), std::length_error );
        auto copy = ordinary;
        copy.erase( copy.begin() ); copy.push_back( 3 );
        BOOST_CHECK_EQUAL( copy.front(), 2 ); BOOST_CHECK_EQUAL( copy.back(), 3 );
        BOOST_CHECK( !copy.isSourceBudgeted() );
        BOOST_CHECK_EQUAL( budget.used(), baseline );
    }
    unsigned rejected = 0, admitted = 0;
    for( u64 headroom = 0; headroom <= 2048; headroom += 32 )
    {
        budget.set( baseline + headroom );
        {
            List<int> candidate( sourceBudget );
            try { candidate.reserve( 32 ); candidate.push_back( 7 ); ++admitted; }
            catch( const std::bad_alloc & ) { ++rejected; }
        }
        BOOST_CHECK_EQUAL( budget.used(), baseline );
    }
    BOOST_REQUIRE( rejected && admitted );
    budget.restore();
    {
        List<int> values( sourceBudget ); values.reserve( 19 ); values.push_back( 42 );
        const auto owned = budget.used();
        List<int> foreign; foreign.push_back( 9 );
        budget.set( owned );
        BOOST_CHECK_THROW( (void)List<int>( values ).size(), std::bad_alloc );
        BOOST_CHECK_THROW( values = foreign, std::bad_alloc );
        BOOST_CHECK_EQUAL( values.front(), 42 );
        BOOST_CHECK_EQUAL( values.capacity(), 19u );
        BOOST_CHECK_EQUAL( budget.used(), owned );
        List<int> moved( std::move( values ) );
        BOOST_CHECK( moved.isSourceBudgeted() );
        BOOST_CHECK( values.empty() );
        BOOST_CHECK_EQUAL( budget.used(), owned );
        moved.clear();
        BOOST_CHECK_EQUAL( budget.used(), owned );
        moved.push_back( 11 ); // Reuse retained nodes at the exact cap.
        BOOST_CHECK_EQUAL( moved.front(), 11 );
        budget.restore();
        values = foreign;
        BOOST_CHECK( values.isSourceBudgeted() );
        BOOST_CHECK_EQUAL( values.front(), 9 );
        budget.set( budget.used() );
    }
    BOOST_CHECK_EQUAL( budget.used(), baseline );
}

BOOST_AUTO_TEST_CASE( list_returning_actor_queries_keep_admitted_nodes_after_actor_teardown )
{
    TestGuard guard; CoordinatorBudget budget;
    auto actor = guard.sceneManager->createActor();
    auto child = guard.sceneManager->createActor();
    auto a = actor->addComponent<LODGroup>(); auto b = child->addComponent<LODGroup>();
    actor->addChild( child );
    auto direct = actor->getComponentsByTypePtr<LODGroup>();
    auto descendants = actor->getComponentsInChildrenPtr<LODGroup>();
    BOOST_CHECK( direct.isSourceBudgeted() && descendants.isSourceBudgeted() );
    BOOST_REQUIRE_EQUAL( direct.size(), 1u ); BOOST_CHECK( direct.front() == a.get() );
    BOOST_REQUIRE_EQUAL( descendants.size(), 1u ); BOOST_CHECK( descendants.front() == b.get() );
    budget.set( budget.used() );
    BOOST_CHECK_THROW( actor->getComponentsByTypePtr<LODGroup>(), std::bad_alloc );
    BOOST_CHECK_THROW( actor->getComponentsInChildrenPtr<LODGroup>(), std::bad_alloc );
    const auto held = budget.used();
    direct.clear(); descendants.clear();
    BOOST_CHECK_EQUAL( budget.used(), held );
    budget.restore();
    guard.sceneManager->destroyActor( actor );
    BOOST_CHECK( direct.empty() && descendants.empty() );
    const auto afterActor = budget.used();
    direct = List<LODGroup *>( sourceBudget ); descendants = List<LODGroup *>( sourceBudget );
    BOOST_CHECK_LT( budget.used(), afterActor );
}

BOOST_AUTO_TEST_CASE( shared_component_inventory_reserve_setters_and_unload_are_budgeted )
{
    TestGuard guard; CoordinatorBudget budget;
    auto system = make_ptr<SharedInventoryProbe>();
    auto component = make_ptr<LODGroup>();
    const auto baseline = budget.used();
    system->reserve( 4 );
    const auto slot = system->addComponent( component );
    BOOST_REQUIRE_LT( slot, 4u );
    const auto before = budget.used();
    system->refuseDerived = true;
    BOOST_CHECK_THROW( system->reserve( 32 ), std::bad_alloc );
    BOOST_CHECK_EQUAL( system->getSize(), 4u );
    BOOST_CHECK_EQUAL( system->componentCapacity(), 4u );
    BOOST_CHECK_EQUAL( system->stateCapacity(), 4u );
    BOOST_CHECK( system->getObject( slot ) == component );
    BOOST_CHECK_EQUAL( budget.used(), before );
    system->refuseDerived = false;
    unsigned rejected = 0; bool grew = false;
    for( u64 headroom = 0; headroom <= 4096; headroom += 32 )
    {
        budget.set( before + headroom );
        try { system->reserve( 32 ); grew = true; break; }
        catch( const std::bad_alloc & )
        {
            ++rejected;
            BOOST_CHECK_EQUAL( system->getSize(), 4u );
            BOOST_CHECK( system->getObject( slot ) == component );
            BOOST_CHECK_EQUAL( budget.used(), before );
        }
    }
    BOOST_REQUIRE( rejected && grew );
    budget.set( budget.used() );
    BOOST_CHECK_THROW( system->addDirtyComponent( component ), std::bad_alloc );
    BOOST_CHECK( !system->isDirty() );
    budget.restore(); system->addDirtyComponent( component );
    system->setDirty( false );
    budget.set( budget.used() );
    BOOST_CHECK_NO_THROW( system->addDirtyComponent( component ) ); // Duplicate uses no new slot.
    BOOST_CHECK( system->isDirty() );
    BOOST_CHECK_NO_THROW( system->removeDirtyComponent( component ) );
    budget.restore();
    Array<SmartPtr<IComponent>> foreign; foreign.reserve( 47 ); foreign.push_back( component );
    system->setComponents( foreign );
    system->setDirtyComponents( foreign );
    auto snapshot = system->getComponents();
    BOOST_CHECK( snapshot.get_allocator().isSourceBudgeted() );
    BOOST_CHECK_EQUAL( snapshot.capacity(), 47u );
    BOOST_CHECK( system->getDirtyComponents().get_allocator().isSourceBudgeted() );
    budget.set( budget.used() );
    BOOST_CHECK_THROW( system->setComponents( foreign ), std::bad_alloc );
    BOOST_CHECK_THROW( system->setDirtyComponents( foreign ), std::bad_alloc );
    BOOST_CHECK_THROW( system->getComponents(), std::bad_alloc );
    BOOST_CHECK_NO_THROW( system->removeDirtyComponent( component ) );
    BOOST_CHECK_NO_THROW( system->unload( nullptr ) );
    BOOST_CHECK_EQUAL( system->getSize(), 0u );
    BOOST_CHECK_EQUAL( snapshot.size(), 1u );
    snapshot = {};
    BOOST_CHECK_EQUAL( budget.used(), baseline );
    budget.restore();
    BOOST_CHECK_NO_THROW( system->reserve( 8 ) );
}

BOOST_AUTO_TEST_CASE( scene_update_table_load_replacement_registration_and_retirement_are_admitted )
{
    TestGuard guard; CoordinatorBudget budget;
    auto scene = make_ptr<SceneQueueBudgetProbe>();
    const auto baseline = budget.used();
    for( auto headroom : { 0u, 512u, 4096u } )
    {
        budget.set( baseline + headroom );
        BOOST_CHECK_NO_THROW( scene->load( nullptr ) );
        BOOST_CHECK( scene->getLoadingState() == LoadingState::Error );
        BOOST_CHECK_EQUAL( scene->phaseCount(), 0u );
        BOOST_CHECK_EQUAL( budget.used(), baseline );
    }
    budget.restore(); scene->load( nullptr );
    BOOST_REQUIRE( scene->isLoaded() );
    for( unsigned p = 0; p < unsigned( Thread::UpdateState::Count ); ++p )
        for( unsigned t = 0; t < unsigned( TaskId::Count ); ++t )
        {
            const auto &row = scene->getRegisteredObjects( Thread::UpdateState( p ), TaskId( t ) );
            BOOST_CHECK_GE( row.capacity(), WP_MAX_ACTORS );
            BOOST_CHECK( row.snapshot().get_allocator().isSourceBudgeted() );
        }
    auto actor = guard.sceneManager->createActor(); scene->addActor( actor );
    const auto loaded = budget.used();
    {
        auto *borrowed = &scene->getRegisteredObjects( Thread::UpdateState::Update, TaskId::Render );
        ISharedObject::ScopedLoadLock activeUpdate( scene.get() );
        scene->load( nullptr );
        BOOST_CHECK( borrowed == &scene->getRegisteredObjects( Thread::UpdateState::Update, TaskId::Render ) );
        BOOST_CHECK_EQUAL( budget.used(), loaded );
    }
    budget.set( loaded ); scene->load( nullptr );
    BOOST_CHECK( scene->isLoaded() );
    BOOST_CHECK_EQUAL( budget.used(), loaded );
    BOOST_CHECK_EQUAL( scene->getRegisteredObjects( Thread::UpdateState::Update, TaskId::Render ).size(), 1u );
    budget.restore();
    ConcurrentArray<SmartPtr<IGameActor>> foreign; foreign.reserve( 17 ); foreign.push_back( actor );
    scene->setRegisteredObjects( Thread::UpdateState::Update, TaskId::Render, foreign );
    BOOST_CHECK( scene->getRegisteredObjects( Thread::UpdateState::Update, TaskId::Render ).snapshot().get_allocator().isSourceBudgeted() );
    budget.set( budget.used() );
    BOOST_CHECK_THROW( scene->setRegisteredObjects( Thread::UpdateState::Update, TaskId::Render, foreign ), std::bad_alloc );
    BOOST_CHECK_EQUAL( scene->getRegisteredObjects( Thread::UpdateState::Update, TaskId::Render ).capacity(), 17u );
    BOOST_CHECK_NO_THROW( scene->unregisterAll( actor ) );
    BOOST_CHECK_NO_THROW( scene->unload( nullptr ) );
    budget.restore();
    guard.sceneManager->destroyActor( actor );
}

BOOST_AUTO_TEST_CASE( scene_registration_admission_precedes_actor_and_phase_publication )
{
    TestGuard guard; CoordinatorBudget budget;
    auto scene = make_ptr<SceneQueueBudgetProbe>(); scene->load( nullptr );
    auto actor = guard.sceneManager->createActor();
    ConcurrentArray<SmartPtr<IGameActor>> empty;
    for( auto phase : { Thread::UpdateState::PreUpdate, Thread::UpdateState::Update, Thread::UpdateState::PostUpdate } )
        scene->setRegisteredObjects( phase, TaskId::Application, empty );
    const auto baseline = budget.used();
    const auto firstRow = sourceBudgetStorageBytes( sizeof( SmartPtr<IGameActor> ), alignof( SmartPtr<IGameActor> ) );
    budget.set( baseline + firstRow );
    BOOST_CHECK_THROW( scene->registerUpdates( TaskId::Application, actor ), std::bad_alloc );
    for( auto phase : { Thread::UpdateState::PreUpdate, Thread::UpdateState::Update, Thread::UpdateState::PostUpdate } )
        BOOST_CHECK_EQUAL( scene->getRegisteredObjects( phase, TaskId::Application ).size(), 0u );
    BOOST_CHECK_EQUAL( budget.used(), baseline + firstRow ); // Retained, admitted spare capacity.
    budget.set( budget.used() );
    BOOST_CHECK_THROW( scene->addActor( actor ), std::bad_alloc );
    BOOST_CHECK( !actor->getScene() );
    BOOST_CHECK( !actor->getFlag( IGameActor::ActorFlagInScene ) );
    budget.restore();
    scene->addActor( actor );
    BOOST_CHECK_EQUAL( scene->getRegisteredObjects( Thread::UpdateState::Update, TaskId::Application ).size(), 1u );
    scene->removeActor( actor ); scene->unload( nullptr );
    guard.sceneManager->destroyActor( actor );
}

BOOST_AUTO_TEST_CASE( scene_play_edit_queue_denial_preserves_the_previous_mode_and_pending_actors )
{
    TestGuard guard; CoordinatorBudget budget;
    auto scene = make_ptr<SceneQueueBudgetProbe>(); scene->load( nullptr );
    auto actor = guard.sceneManager->createActor(); scene->addActor( actor );
    auto pool = guard.applicationManager->getThreadPool();
    const auto threads = pool->getNumThreads();
    guard.addCleanup( [pool, threads]() mutable { pool->setNumThreads( threads ); } );
    pool->setNumThreads( 1 );
    scene->setState( IGameScene::State::Play );
    BOOST_REQUIRE_EQUAL( scene->playCount(), 1u );
    budget.set( budget.used() );
    BOOST_CHECK_THROW( scene->setState( IGameScene::State::Edit ), std::bad_alloc );
    BOOST_CHECK( scene->getState() == IGameScene::State::Play );
    BOOST_CHECK_EQUAL( scene->playCount(), 1u ); BOOST_CHECK_EQUAL( scene->editCount(), 0u );
    budget.restore();
    scene->setState( IGameScene::State::Edit );
    BOOST_CHECK_EQUAL( scene->playCount(), 0u ); BOOST_CHECK_EQUAL( scene->editCount(), 1u );
    budget.set( budget.used() );
    BOOST_CHECK_NO_THROW( scene->unload( nullptr ) );
    BOOST_CHECK_EQUAL( scene->playCount(), 0u ); BOOST_CHECK_EQUAL( scene->editCount(), 0u );
    budget.restore(); guard.sceneManager->destroyActor( actor );
}

#endif



