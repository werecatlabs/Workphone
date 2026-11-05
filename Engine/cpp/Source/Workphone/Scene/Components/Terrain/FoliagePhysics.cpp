#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/FoliagePageStream.hpp>
#include <Workphone/Scene/Components/Terrain/FoliagePhysics.hpp>
#include <Workphone/Scene/Components/Terrain/FoliageAuthoring.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Interface/Physics/IRigidStatic3.hpp>
#include <Workphone/Interface/Physics/IBoxShape3.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <algorithm>
#include <cmath>
#include <map>
#include <limits>
#include <queue>
#include <set>

namespace workphone::scene
{
    namespace
    {
        bool finite( const Vector3<real_Num> &v )
        {
            return std::isfinite( v.x ) && std::isfinite( v.y ) && std::isfinite( v.z );
        }

        Vector3<real_Num> point( const Transform3<real_Num> &t, const Vector3<real_Num> &p )
        {
            return t.getPosition() + t.getOrientation() * ( p * t.getScale() );
        }

        Vector3<real_Num> realPoint( const Vector3F &v )
        {
            return { v.x, v.y, v.z };
        }

        bool fail( String &error, const char *s )
        {
            error = s;
            return false;
        }

        bool validPlacement( const Transform3<real_Num> &placement )
        {
            const auto &scale = placement.getScale();
            const auto &q = placement.getOrientation();
            const double norm =
                double( q.w ) * q.w + double( q.x ) * q.x + double( q.y ) * q.y + double( q.z ) * q.z;
            return finite( placement.getPosition() ) && finite( scale ) && scale.x > 0 &&
                   scale.y > 0 && scale.z > 0 && std::isfinite( norm ) && std::abs( norm - 1 ) <= .001;
        }

        bool collidable( const render::FoliageSpecies &species )
        {
            return species.collisionRadius > 0 && species.kind != render::FoliageKind::Grass &&
                   species.kind != render::FoliageKind::GroundCover;
        }

        bool activeInstance( const render::FoliageInstance &instance )
        {
            return !( instance.flags & ( render::FoliageRemoved | render::FoliagePromoted ) );
        }
    }  // namespace

    struct FoliagePhysics::State
    {
        struct Body
        {
            SmartPtr<physics::IRigidStatic3> rigid;
            SmartPtr<physics::IBoxShape3> shape;
        };

        struct Source
        {
            StreamBudget budget;
            render::FoliageSnapshot data;
            Transform3<real_Num> placement;
            std::map<render::FoliageCellKey, Array<u32>> cells;
            std::shared_ptr<const render::FoliageStreamSource> paged;
            std::unique_ptr<render::FoliagePageStream> stream;
            std::map<render::FoliageCellKey, std::shared_ptr<const render::FoliagePageData>> pages;
            f32 maximumDistance = 0;

            bool loadNearby( const Array<render::FoliageCellKey> &wanted, String &error )
            {
                // Old native bodies remain live while new pages load. Their geometry
                // is self-contained, so an out-of-range page lease can retire now.
                for( auto it = pages.begin(); it != pages.end(); )
                {
                    if( !std::binary_search( wanted.begin(), wanted.end(), it->first ) )
                    {
                        it = pages.erase( it );
                    }
                    else
                    {
                        ++it;
                    }
                }
                const bool admitted = stream->setDemand( wanted, error );
                render::FoliagePageResult result;
                while( stream->take( result ) )
                {
                    // Ready results from an old demand may be holding the
                    // result budget that the new demand needs to retry.
                    if( !std::binary_search( wanted.begin(), wanted.end(), result.cell ) )
                    {
                        continue;
                    }
                    if( result.page )
                    {
                        pages[result.cell] = std::move( result.page );
                    }
                    else if( error.empty() )
                    {
                        error = result.error.empty() ? "Foliage collision page failed to load."
                                                     : result.error;
                    }
                }
                return admitted && error.empty() && pages.size() == wanted.size();
            }
        };

        SmartPtr<physics::IPhysicsManager> manager;
        SmartPtr<physics::IPhysicsScene3> scene;
        std::shared_ptr<Source> source, pending;
        std::map<u64, Body> bodies;
        StreamBudget budget;
        u32 capacity = 512;
        FoliageCollisionStats stats;

        void retire( Body &b )
        {
            if( b.rigid )
            {
                scene->removeActor( b.rigid );
                if( b.shape )
                {
                    b.rigid->removeShape( b.shape, false );
                }
                manager->removePhysicsBody( b.rigid );
            }
            if( b.shape )
            {
                manager->removeCollisionShape( b.shape );
            }
            b = {};
            ++stats.retired;
        }
    };

    FoliagePhysics::FoliagePhysics( SmartPtr<physics::IPhysicsManager> manager,
                                    SmartPtr<physics::IPhysicsScene3> scene, u32 maximumColliders ) :
        m_state( std::make_unique<State>() )
    {
        m_state->manager = manager;
        m_state->scene = scene;
        m_state->capacity = std::min( maximumColliders, 2048u );
    }

    FoliagePhysics::~FoliagePhysics()
    {
        clear();
    }

    void FoliagePhysics::clear()
    {
        clearActive();
        m_state->pending.reset();
    }

    void FoliagePhysics::clearActive()
    {
        auto &s = *m_state;
        for( auto &entry : s.bodies )
        {
            s.retire( entry.second );
        }
        s.bodies.clear();
        s.source.reset();
        s.budget.reset();
        s.stats.active = s.stats.eligible = s.stats.omitted = s.stats.visitedCells = 0;
        s.stats.reservedBytes = 0;
    }

    bool FoliagePhysics::setSource( render::FoliageSnapshot source,
                                    const Transform3<real_Num> &placement, String &error )
    {
        error.clear();
        auto &s = *m_state;
        if( !source )
        {
            clear();
            return true;
        }
        if( s.pending && !s.pending->paged && source == s.pending->data &&
            placement == s.pending->placement )
        {
            return true;
        }
        if( s.source && !s.source->paged && source == s.source->data &&
            placement == s.source->placement )
        {
            s.pending.reset();
            return true;
        }
        if( !validPlacement( placement ) )
        {
            return fail( error, "Invalid foliage collision placement." );
        }
        if( !render::validateFoliageWorld( *source, error ) )
        {
            return false;
        }
        try
        {
            // Retained source plus a compact index of every collidable cell. Reservations
            // precede allocation, and are kept until the last native body has retired.
            auto sourceBudget = WorldStreamingScheduler::shared()->reserve(
                StreamMemoryClass::Collision,
                render::foliageSourceBytes( *source ) + source->instances.size() * sizeof( u32 ) * 2 +
                    render::foliageMaximumPages * 128ull,
                error );
            if( !sourceBudget )
            {
                return false;
            }
            std::map<render::FoliageCellKey, Array<u32>> cells;
            f32 maximumDistance = 0;
            for( u32 n = 0; n < source->instances.size(); ++n )
            {
                const auto &i = source->instances[n];
                const auto &species = *source->species[i.species];
                if( !activeInstance( i ) || !collidable( species ) )
                {
                    continue;
                }
                render::FoliageCellKey cell;
                if( !render::foliageCellForPosition( i.position, source->pageSize, cell, error ) )
                {
                    return false;
                }
                cells[cell].push_back( n );
                maximumDistance = std::max( maximumDistance, species.collisionDistance );
            }
            auto candidate = std::make_shared<State::Source>();
            candidate->data = std::move( source );
            candidate->placement = placement;
            candidate->cells = std::move( cells );
            candidate->budget = std::move( sourceBudget );
            candidate->maximumDistance = maximumDistance;
            s.pending = std::move( candidate );
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool FoliagePhysics::setPagedSource( std::shared_ptr<const render::FoliageStreamSource> source,
                                         const Transform3<real_Num> &placement, String &error )
    {
        error.clear();
        auto &s = *m_state;
        if( !source || !source->snapshot || !source->load ||
            source->cells.size() > render::foliageMaximumPages ||
            source->cells.size() != source->pageInstanceCounts.size() )
        {
            return fail( error, "Invalid paged foliage collision source." );
        }
        if( s.pending && source == s.pending->paged && placement == s.pending->placement )
        {
            return true;
        }
        if( s.source && source == s.source->paged && placement == s.source->placement )
        {
            s.pending.reset();
            return true;
        }
        if( !validPlacement( placement ) )
        {
            return fail( error, "Invalid foliage collision placement." );
        }
        if( !render::validateFoliageWorld( *source->snapshot, error ) )
        {
            return false;
        }
        try
        {
            auto candidate = std::make_shared<State::Source>();
            candidate->data = source->snapshot;
            candidate->placement = placement;
            candidate->paged = std::move( source );
            bool needsPages = false;
            for( const auto &species : candidate->data->species )
            {
                if( collidable( *species ) )
                {
                    needsPages = true;
                    candidate->maximumDistance =
                        std::max( candidate->maximumDistance, species->collisionDistance );
                }
            }
            const auto pageCount = needsPages ? candidate->paged->cells.size() : 0;
            candidate->budget = WorldStreamingScheduler::shared()->reserve(
                StreamMemoryClass::Collision, 4096 + pageCount * 128ull, error );
            if( !candidate->budget )
            {
                return false;
            }
            if( needsPages && pageCount )
            {
                candidate->stream = std::make_unique<render::FoliagePageStream>();
                if( !candidate->stream->setSource( *candidate->paged, error ) )
                {
                    return false;
                }
            }
            s.pending = std::move( candidate );
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool FoliagePhysics::update( const Array<Vector3F> &observers, String &error )
    {
        return update( observers, error, true );
    }

    bool FoliagePhysics::updateActive( const Array<Vector3F> &observers, String &error )
    {
        return update( observers, error, false );
    }

    bool FoliagePhysics::update( const Array<Vector3F> &observers, String &error,
                                  bool publishPending )
    {
        error.clear();
        auto &s = *m_state;
        if( !s.manager || !s.scene || !s.capacity )
        {
            return fail( error, "Foliage collision needs a physics manager, scene and capacity." );
        }
        if( observers.size() > 8 )
        {
            return fail( error, "Foliage collision supports at most eight observers." );
        }
        for( const auto &p : observers )
        {
            if( !finite( realPoint( p ) ) )
            {
                return fail( error, "Foliage observers must be finite." );
            }
        }
        const auto source = publishPending && s.pending ? s.pending : s.source;
        if( !source )
        {
            return true;
        }
        std::map<u64, State::Body> staged;
        StreamBudget stagingBudget;

        // Every manager-owned candidate must retire if preparation or publication
        // fails. This guard runs after any scene publication lock has been released.
        struct Rollback
        {
            State &state;
            std::map<u64, State::Body> &bodies;

            ~Rollback()
            {
                for( auto &entry : bodies )
                {
                    state.retire( entry.second );
                }
            }
        } rollback{ s, staged };

        try
        {
            struct Candidate
            {
                std::pair<double, u64> rank;
                const render::FoliageInstance *instance;

                bool operator<( const Candidate &other ) const { return rank < other.rank; }
            };
            std::priority_queue<Candidate> nearest;
            u32 eligible = 0, visitedCells = 0;
            // Local observer coordinates allow cell rejection even under translated,
            // rotated and nonuniformly scaled terrain placements.
            Array<Vector3<real_Num>> localObservers;
            const auto inverse = source->placement.getOrientation().inverse();
            const auto scale = source->placement.getScale();
            const double localRange =
                source->maximumDistance /
                std::min( { double( scale.x ), double( scale.y ), double( scale.z ) } );
            for( const auto &p : observers )
            {
                localObservers.push_back(
                    ( inverse * ( realPoint( p ) - source->placement.getPosition() ) ) / scale );
            }
            const auto nearby = [&]( render::FoliageCellKey cell ) {
                const double x = double( cell.x ) * source->data->pageSize,
                             z = double( cell.z ) * source->data->pageSize;
                for( const auto &p : localObservers )
                {
                    const double dx =
                        std::max( { x - p.x, 0.0, double( p.x ) - x - source->data->pageSize } );
                    const double dz =
                        std::max( { z - p.z, 0.0, double( p.z ) - z - source->data->pageSize } );
                    if( dx * dx + dz * dz <= localRange * localRange )
                    {
                        return true;
                    }
                }
                return false;
            };
            const auto consider = [&]( const render::FoliageInstance &i ) {
                const auto &species = *source->data->species[i.species];
                const auto centre = point( source->placement, realPoint( i.position ) );
                double distance = std::numeric_limits<double>::max();
                for( const auto &p : observers )
                {
                    const auto delta = centre - realPoint( p );
                    distance = std::min( distance, double( delta.lengthSquared() ) );
                }
                if( distance > double( species.collisionDistance ) * species.collisionDistance )
                {
                    return;
                }
                ++eligible;
                Candidate candidate{ { distance, i.id }, &i };
                if( nearest.size() < s.capacity )
                {
                    nearest.push( candidate );
                }
                else if( candidate < nearest.top() )
                {
                    nearest.pop();
                    nearest.push( candidate );
                }
            };
            if( source->stream )
            {
                Array<render::FoliageCellKey> demand;
                for( const auto cell : source->paged->cells )
                {
                    if( nearby( cell ) )
                    {
                        demand.push_back( cell );
                    }
                }
                std::sort( demand.begin(), demand.end() );
                if( !source->loadNearby( demand, error ) )
                {
                    return false;
                }
                visitedCells = u32( source->pages.size() );
                for( const auto &entry : source->pages )
                {
                    for( const auto &i : entry.second->instances )
                    {
                        if( activeInstance( i ) && collidable( *source->data->species[i.species] ) )
                        {
                            consider( i );
                        }
                    }
                }
            }
            else
            {
                for( const auto &cell : source->cells )
                {
                    if( nearby( cell.first ) )
                    {
                        ++visitedCells;
                        for( u32 index : cell.second )
                        {
                            consider( source->data->instances[index] );
                        }
                    }
                }
            }
            std::map<u64, const render::FoliageInstance *> wanted;
            while( !nearest.empty() )
            {
                const auto &candidate = nearest.top();
                if( !wanted.emplace( candidate.rank.second, candidate.instance ).second )
                {
                    return fail( error, "Duplicate foliage collision instance ID across pages." );
                }
                nearest.pop();
            }
            if( !wanted.empty() && !s.budget )
            {
                s.budget = WorldStreamingScheduler::shared()->reserve( StreamMemoryClass::Collision,
                                                                       s.capacity * 4096ull, error );
                if( !s.budget )
                {
                    return false;
                }
            }
            const bool replacing = source != s.source;
            u64 additional = 0;
            for( const auto &entry : wanted )
            {
                if( replacing || !s.bodies.count( entry.first ) )
                {
                    ++additional;
                }
            }
            if( additional )
            {
                stagingBudget = WorldStreamingScheduler::shared()->reserve( StreamMemoryClass::Collision,
                                                                            additional * 4096, error );
                if( !stagingBudget )
                {
                    return false;
                }
            }
            std::map<u64, State::Body> next;
            if( !replacing )
            {
                for( const auto &entry : s.bodies )
                {
                    if( wanted.count( entry.first ) )
                    {
                        next.emplace( entry );
                    }
                }
            }
            for( const auto &entry : wanted )
            {
                if( next.count( entry.first ) )
                {
                    continue;
                }
                const auto &i = *entry.second;
                const auto &species = *source->data->species[i.species];
                // Install the cleanup record before the first native allocation.
                auto &body = staged.emplace( entry.first, State::Body{} ).first->second;
                auto raw =
                    s.manager->addCollisionShapeByType( physics::IBoxShape3::typeInfo(), nullptr );
                body.shape = dynamic_pointer_cast<physics::IBoxShape3>( raw );
                if( !body.shape )
                {
                    if( raw )
                    {
                        s.manager->removeCollisionShape( raw );
                    }
                    return fail( error, "Backend rejected foliage trunk box." );
                }
                const Quaternion<real_Num> orientation( i.rotation.w, i.rotation.x, i.rotation.y,
                                                        i.rotation.z );
                const Vector3<real_Num> half( species.collisionRadius * i.scale.x,
                                              species.collisionHeight * i.scale.y * .5,
                                              species.collisionRadius * i.scale.z );
                const auto a = orientation * Vector3<real_Num>( half.x, 0, 0 ),
                           b = orientation * Vector3<real_Num>( 0, half.y, 0 ),
                           c = orientation * Vector3<real_Num>( 0, 0, half.z );
                const Vector3<real_Num> extent(
                    ( std::abs( a.x ) + std::abs( b.x ) + std::abs( c.x ) ) * scale.x * 2,
                    ( std::abs( a.y ) + std::abs( b.y ) + std::abs( c.y ) ) * scale.y * 2,
                    ( std::abs( a.z ) + std::abs( b.z ) + std::abs( c.z ) ) * scale.z * 2 );
                const auto centre =
                    point( source->placement,
                           realPoint( i.position ) + orientation * Vector3<real_Num>( 0, half.y, 0 ) );
                if( !finite( extent ) || !finite( centre ) ||
                    std::max( { extent.x, extent.y, extent.z, std::abs( centre.x ), std::abs( centre.y ),
                                std::abs( centre.z ) } ) > 1.e7 )
                {
                    return fail( error, "Foliage collision geometry exceeds the backend range." );
                }
                body.shape->setExtents( extent );
                body.rigid = s.manager->addRigidStatic(
                    Transform3<real_Num>( centre, source->placement.getOrientation() ) );
                if( !body.rigid )
                {
                    return fail( error, "Could not allocate foliage collision body." );
                }
                body.rigid->addShape( body.shape );
                if( body.rigid->getNumShapes() != 1 )
                {
                    return fail( error, "Backend rejected foliage collision shape attachment." );
                }
                next.emplace( entry.first, body );
                ++s.stats.created;
            }
            {
                // Native scene capacity is tested while all old actors remain live.
                // This requires temporary capacity for active + candidate bodies.
                // Scene mutations share one lock, so physics cannot observe a partial
                // candidate, and manager retirement occurs after releasing that lock.
                if( !s.scene->try_lock() )
                {
                    return fail( error, "Physics scene is busy; previous foliage collision retained." );
                }

                struct Unlock
                {
                    SmartPtr<physics::IPhysicsScene3> scene;

                    ~Unlock()
                    {
                        scene->unlock();
                    }
                } unlock{ s.scene };

                for( auto &entry : staged )
                {
                    s.scene->addActor( entry.second.rigid );
                    if( !s.scene->hasActor( entry.second.rigid ) )
                    {
                        for( auto &candidate : staged )
                        {
                            s.scene->removeActor( candidate.second.rigid );
                        }
                        return fail( error,
                                     "Physics scene capacity rejected staged foliage; previous "
                                     "collision retained." );
                    }
                }
                for( auto &entry : s.bodies )
                {
                    if( replacing || !next.count( entry.first ) )
                    {
                        s.scene->removeActor( entry.second.rigid );
                    }
                }
                s.bodies.swap( next );
                // 'next' now contains the previous generation, retired below.
            }
            staged.clear();
            auto retiredSource = s.source;
            if( replacing )
            {
                s.source = source;
                s.pending.reset();
            }
            for( auto &entry : next )
            {
                if( replacing || !s.bodies.count( entry.first ) )
                {
                    s.retire( entry.second );
                }
            }
            s.stats.active = u32( s.bodies.size() );
            s.stats.eligible = eligible;
            s.stats.visitedCells = visitedCells;
            s.stats.omitted = eligible - s.stats.active;
            s.stats.reservedBytes = ( s.budget ? s.budget->bytes() : 0 ) + source->budget->bytes();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    FoliageCollisionStats FoliagePhysics::getStats() const
    {
        auto stats = m_state->stats;
        stats.active = u32( m_state->bodies.size() );
        return stats;
    }

    SmartPtr<physics::IRigidStatic3> FoliagePhysics::getBody( u64 id ) const
    {
        auto it = m_state->bodies.find( id );
        return it == m_state->bodies.end() ? nullptr : it->second.rigid;
    }

    struct FoliageGameplay::State
    {
        SmartPtr<FoliageAuthoring> authoring;
        Acquire acquire;
        Release release;
        std::map<u64, SmartPtr<ISharedObject>> actors;
    };

    FoliageGameplay::FoliageGameplay( SmartPtr<FoliageAuthoring> authoring, Acquire acquire,
                                      Release release ) :
        m_state( std::make_unique<State>() )
    {
        m_state->authoring = authoring;
        m_state->acquire = std::move( acquire );
        m_state->release = std::move( release );
    }

    FoliageGameplay::~FoliageGameplay()
    {
        for( auto &entry : m_state->actors )
        {
            String error;
            m_state->authoring->setInstanceFlags( { entry.first }, render::FoliagePromoted, false,
                                                  error );
            try
            {
                m_state->release( entry.second );
            }
            catch( ... )
            {
            }
        }
    }

    bool FoliageGameplay::promote( u64 id, String &error )
    {
        error.clear();
        auto &s = *m_state;
        if( !s.authoring || !s.acquire || !s.release )
        {
            return fail( error, "Foliage promotion needs authoring and actor callbacks." );
        }
        if( s.actors.count( id ) )
        {
            return true;
        }
        if( s.actors.size() >= 256 )
        {
            return fail( error, "Foliage gameplay actor limit is 256." );
        }
        const auto source = s.authoring->getSnapshot();
        for( const auto &i : source->instances )
        {
            if( i.id == id )
            {
                if( i.flags & ( render::FoliageRemoved | render::FoliagePromoted ) )
                {
                    return fail( error, "Foliage instance is already removed or promoted." );
                }
                SmartPtr<ISharedObject> actor;
                try
                {
                    actor = s.acquire( i, error );
                    if( !actor )
                    {
                        return false;
                    }
                    s.actors.emplace( id, actor );
                    if( s.authoring->setInstanceFlags( { id }, render::FoliagePromoted, true, error ) )
                    {
                        return true;
                    }
                    s.actors.erase( id );
                    s.release( actor );
                    return false;
                }
                catch( const std::exception &e )
                {
                    s.actors.erase( id );
                    if( actor )
                    {
                        s.release( actor );
                    }
                    error = e.what();
                    return false;
                }
            }
        }
        return fail( error, "Unknown foliage instance ID." );
    }

    bool FoliageGameplay::demote( u64 id, String &error )
    {
        error.clear();
        auto &s = *m_state;
        auto it = s.actors.find( id );
        if( it == s.actors.end() )
        {
            return fail( error, "No gameplay actor owns that foliage instance." );
        }
        if( !s.authoring->setInstanceFlags( { id }, render::FoliagePromoted, false, error ) )
        {
            return false;
        }
        s.release( it->second );
        s.actors.erase( it );
        return true;
    }

    bool FoliageGameplay::remove( u64 id, String &error )
    {
        auto &s = *m_state;
        if( !s.authoring )
        {
            return fail( error, "Missing foliage authoring." );
        }
        if( !s.authoring->setInstanceFlags( { id }, render::FoliageRemoved, true, error ) )
        {
            return false;
        }
        auto it = s.actors.find( id );
        if( it != s.actors.end() )
        {
            s.release( it->second );
            s.actors.erase( it );
        }
        return true;
    }

    SmartPtr<ISharedObject> FoliageGameplay::getActor( u64 id ) const
    {
        auto it = m_state->actors.find( id );
        return it == m_state->actors.end() ? nullptr : it->second;
    }
}  // namespace workphone::scene
