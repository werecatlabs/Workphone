#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/TransformSystem.hpp>
#include <Workphone/Math/Math.hpp>
#include <Workphone/Thread/ScopedLock.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <limits>

namespace workphone::scene
{
    namespace
    {
        Transform3<real_Num> makeTransform( const Vector3<real_Num> &position,
                                            const Quaternion<real_Num> &orientation,
                                            const Vector3<real_Num> &scale )
        {
            return Transform3<real_Num>( position, orientation, scale );
        }
    }  // namespace

    TransformSystem &TransformSystem::instance()
    {
        static TransformSystem system;
        return system;
    }

    TransformSystem::Handle TransformSystem::createTransform()
    {
        ScopedLock lock( &m_mutex, true );

        u32 index = 0;
        if( m_freeIndices.empty() )
        {
            index = static_cast<u32>( m_active.size() );
            appendSlot();
        }
        else
        {
            index = m_freeIndices.back();
            m_freeIndices.pop_back();
            resetSlot( index );
        }

        m_active[index] = 1;
        ++m_activeCount;
        return { index, m_generations[index] };
    }

    void TransformSystem::destroyTransform( Handle handle )
    {
        ScopedLock lock( &m_mutex, true );
        if( !containsUnlocked( handle ) )
        {
            return;
        }

        resetSlot( handle.index );
        m_active[handle.index] = 0;
        ++m_generations[handle.index];
        if( m_generations[handle.index] == 0 )
        {
            m_generations[handle.index] = 1;
        }

        m_freeIndices.push_back( handle.index );
        --m_activeCount;
    }

    bool TransformSystem::contains( Handle handle ) const
    {
        ScopedLock lock( &m_mutex, false );
        return containsUnlocked( handle );
    }

    size_t TransformSystem::getActiveCount() const
    {
        ScopedLock lock( &m_mutex, false );
        return m_activeCount;
    }

    size_t TransformSystem::getSlotCount() const
    {
        ScopedLock lock( &m_mutex, false );
        return m_active.size();
    }

    void TransformSystem::reserve( size_t count )
    {
        ScopedLock lock( &m_mutex, true );
        m_localPositions.reserve( count );
        m_localOrientations.reserve( count );
        m_localScales.reserve( count );
        m_worldPositions.reserve( count );
        m_worldOrientations.reserve( count );
        m_worldScales.reserve( count );
        m_actors.reserve( count );
        m_tasks.reserve( count );
        m_flags.reserve( count );
        m_frameTimes.reserve( count );
        m_frameDeltaTimes.reserve( count );
        m_referenceCounts.reserve( count );
        m_generations.reserve( count );
        m_active.reserve( count );
        m_freeIndices.reserve( count );
    }

    Transform3<real_Num> TransformSystem::getLocalTransform( Handle handle ) const
    {
        ScopedLock lock( &m_mutex, false );
        WP_ASSERT( containsUnlocked( handle ) );
        if( !containsUnlocked( handle ) )
        {
            return Transform3<real_Num>::identity();
        }

        return makeTransform( m_localPositions[handle.index], m_localOrientations[handle.index],
                              m_localScales[handle.index] );
    }

    void TransformSystem::setLocalTransform( Handle handle, const Transform3<real_Num> &transform )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( !containsUnlocked( handle ) )
        {
            return;
        }

        m_localPositions[handle.index] = transform.getPosition();
        m_localOrientations[handle.index] = transform.getOrientation();
        m_localScales[handle.index] = transform.getScale();
    }

    Transform3<real_Num> TransformSystem::getWorldTransform( Handle handle ) const
    {
        ScopedLock lock( &m_mutex, false );
        WP_ASSERT( containsUnlocked( handle ) );
        if( !containsUnlocked( handle ) )
        {
            return Transform3<real_Num>::identity();
        }

        return makeTransform( m_worldPositions[handle.index], m_worldOrientations[handle.index],
                              m_worldScales[handle.index] );
    }

    void TransformSystem::setWorldTransform( Handle handle, const Transform3<real_Num> &transform )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( !containsUnlocked( handle ) )
        {
            return;
        }

        m_worldPositions[handle.index] = transform.getPosition();
        m_worldOrientations[handle.index] = transform.getOrientation();
        m_worldScales[handle.index] = transform.getScale();
    }

    void TransformSystem::getTransforms( Handle handle, Transform3<real_Num> &localTransform,
                                         Transform3<real_Num> &worldTransform ) const
    {
        ScopedLock lock( &m_mutex, false );
        WP_ASSERT( containsUnlocked( handle ) );
        if( !containsUnlocked( handle ) )
        {
            localTransform = Transform3<real_Num>::identity();
            worldTransform = Transform3<real_Num>::identity();
            return;
        }

        localTransform = makeTransform( m_localPositions[handle.index],
                                        m_localOrientations[handle.index], m_localScales[handle.index] );
        worldTransform = makeTransform( m_worldPositions[handle.index],
                                        m_worldOrientations[handle.index], m_worldScales[handle.index] );
    }

    void TransformSystem::setTransforms( Handle handle, const Transform3<real_Num> &localTransform,
                                         const Transform3<real_Num> &worldTransform )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( !containsUnlocked( handle ) )
        {
            return;
        }

        m_localPositions[handle.index] = localTransform.getPosition();
        m_localOrientations[handle.index] = localTransform.getOrientation();
        m_localScales[handle.index] = localTransform.getScale();
        m_worldPositions[handle.index] = worldTransform.getPosition();
        m_worldOrientations[handle.index] = worldTransform.getOrientation();
        m_worldScales[handle.index] = worldTransform.getScale();
    }

    Vector3<real_Num> TransformSystem::getLocalPosition( Handle handle ) const
    {
        ScopedLock lock( &m_mutex, false );
        WP_ASSERT( containsUnlocked( handle ) );
        return containsUnlocked( handle ) ? m_localPositions[handle.index] : Vector3<real_Num>::zero();
    }

    void TransformSystem::setLocalPosition( Handle handle, const Vector3<real_Num> &position )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( containsUnlocked( handle ) )
        {
            m_localPositions[handle.index] = position;
        }
    }

    Quaternion<real_Num> TransformSystem::getLocalOrientation( Handle handle ) const
    {
        ScopedLock lock( &m_mutex, false );
        WP_ASSERT( containsUnlocked( handle ) );
        return containsUnlocked( handle ) ? m_localOrientations[handle.index]
                                          : Quaternion<real_Num>::identity();
    }

    void TransformSystem::setLocalOrientation( Handle handle, const Quaternion<real_Num> &orientation )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( containsUnlocked( handle ) )
        {
            m_localOrientations[handle.index] = orientation;
        }
    }

    Vector3<real_Num> TransformSystem::getLocalScale( Handle handle ) const
    {
        ScopedLock lock( &m_mutex, false );
        WP_ASSERT( containsUnlocked( handle ) );
        return containsUnlocked( handle ) ? m_localScales[handle.index] : Vector3<real_Num>::unit();
    }

    void TransformSystem::setLocalScale( Handle handle, const Vector3<real_Num> &scale )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( containsUnlocked( handle ) )
        {
            m_localScales[handle.index] = scale;
        }
    }

    Vector3<real_Num> TransformSystem::getWorldPosition( Handle handle ) const
    {
        ScopedLock lock( &m_mutex, false );
        WP_ASSERT( containsUnlocked( handle ) );
        return containsUnlocked( handle ) ? m_worldPositions[handle.index] : Vector3<real_Num>::zero();
    }

    void TransformSystem::setWorldPosition( Handle handle, const Vector3<real_Num> &position )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( containsUnlocked( handle ) )
        {
            m_worldPositions[handle.index] = position;
        }
    }

    Quaternion<real_Num> TransformSystem::getWorldOrientation( Handle handle ) const
    {
        ScopedLock lock( &m_mutex, false );
        WP_ASSERT( containsUnlocked( handle ) );
        return containsUnlocked( handle ) ? m_worldOrientations[handle.index]
                                          : Quaternion<real_Num>::identity();
    }

    void TransformSystem::setWorldOrientation( Handle handle, const Quaternion<real_Num> &orientation )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( containsUnlocked( handle ) )
        {
            m_worldOrientations[handle.index] = orientation;
        }
    }

    Vector3<real_Num> TransformSystem::getWorldScale( Handle handle ) const
    {
        ScopedLock lock( &m_mutex, false );
        WP_ASSERT( containsUnlocked( handle ) );
        return containsUnlocked( handle ) ? m_worldScales[handle.index] : Vector3<real_Num>::unit();
    }

    void TransformSystem::setWorldScale( Handle handle, const Vector3<real_Num> &scale )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( containsUnlocked( handle ) )
        {
            m_worldScales[handle.index] = scale;
        }
    }

    void TransformSystem::updateWorldFromLocal( Handle handle,
                                                const Transform3<real_Num> *parentWorldTransform )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( !containsUnlocked( handle ) )
        {
            return;
        }

        const auto index = handle.index;
        if( parentWorldTransform )
        {
            const auto &parentPosition = parentWorldTransform->getPosition();
            const auto &parentOrientation = parentWorldTransform->getOrientation();
            const auto &parentScale = parentWorldTransform->getScale();

            m_worldScales[index] = parentScale * m_localScales[index];
            m_worldOrientations[index] = parentOrientation * m_localOrientations[index];
            m_worldPositions[index] =
                parentOrientation * ( parentScale * m_localPositions[index] ) + parentPosition;
        }
        else
        {
            m_worldPositions[index] = m_localPositions[index];
            m_worldOrientations[index] = m_localOrientations[index];
            m_worldScales[index] = m_localScales[index];
        }
    }

    void TransformSystem::updateLocalFromWorld( Handle handle,
                                                const Transform3<real_Num> *parentWorldTransform )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( !containsUnlocked( handle ) )
        {
            return;
        }

        const auto index = handle.index;
        if( parentWorldTransform )
        {
            m_localPositions[index] =
                parentWorldTransform->convertWorldToLocalPosition( m_worldPositions[index] );
            m_localOrientations[index] =
                parentWorldTransform->convertWorldToLocalOrientation( m_worldOrientations[index] );

            const auto &parentScale = parentWorldTransform->getScale();
            if( Math<real_Num>::Abs( parentScale.X() ) > std::numeric_limits<real_Num>::epsilon() )
            {
                m_localScales[index].X() = m_worldScales[index].X() / parentScale.X();
            }

            if( Math<real_Num>::Abs( parentScale.Y() ) > std::numeric_limits<real_Num>::epsilon() )
            {
                m_localScales[index].Y() = m_worldScales[index].Y() / parentScale.Y();
            }

            if( Math<real_Num>::Abs( parentScale.Z() ) > std::numeric_limits<real_Num>::epsilon() )
            {
                m_localScales[index].Z() = m_worldScales[index].Z() / parentScale.Z();
            }
        }
        else
        {
            m_localPositions[index] = m_worldPositions[index];
            m_localOrientations[index] = m_worldOrientations[index];
            m_localScales[index] = m_worldScales[index];
        }
    }

    IGameActor *TransformSystem::getActorPtr( Handle handle ) const
    {
        ScopedLock lock( &m_mutex, false );
        WP_ASSERT( containsUnlocked( handle ) );
        return containsUnlocked( handle ) ? m_actors[handle.index].get() : nullptr;
    }

    SmartPtr<IGameActor> TransformSystem::getActor( Handle handle ) const
    {
        ScopedLock lock( &m_mutex, false );
        WP_ASSERT( containsUnlocked( handle ) );
        return containsUnlocked( handle ) ? m_actors[handle.index].lock() : nullptr;
    }

    void TransformSystem::setActor( Handle handle, SmartPtr<IGameActor> actor )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( containsUnlocked( handle ) )
        {
            m_actors[handle.index] = actor;
        }
    }

    bool TransformSystem::getFlag( Handle handle, u8 flag ) const
    {
        ScopedLock lock( &m_mutex, false );
        WP_ASSERT( containsUnlocked( handle ) );
        return containsUnlocked( handle ) && ( m_flags[handle.index] & flag ) != 0;
    }

    void TransformSystem::setFlag( Handle handle, u8 flag, bool value )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( !containsUnlocked( handle ) )
        {
            return;
        }

        if( value )
        {
            m_flags[handle.index] = static_cast<u8>( m_flags[handle.index] | flag );
        }
        else
        {
            m_flags[handle.index] = static_cast<u8>( m_flags[handle.index] & static_cast<u8>( ~flag ) );
        }
    }

    u8 TransformSystem::getFlags( Handle handle ) const
    {
        ScopedLock lock( &m_mutex, false );
        WP_ASSERT( containsUnlocked( handle ) );
        return containsUnlocked( handle ) ? m_flags[handle.index] : 0;
    }

    void TransformSystem::setFlags( Handle handle, u8 flags )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( containsUnlocked( handle ) )
        {
            m_flags[handle.index] = flags;
        }
    }

    TaskId TransformSystem::getTask( Handle handle ) const
    {
        ScopedLock lock( &m_mutex, false );
        WP_ASSERT( containsUnlocked( handle ) );
        return containsUnlocked( handle ) ? m_tasks[handle.index] : TaskId::None;
    }

    void TransformSystem::setTask( Handle handle, TaskId task )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( containsUnlocked( handle ) )
        {
            m_tasks[handle.index] = task;
        }
    }

    time_interval TransformSystem::getFrameTime( Handle handle ) const
    {
        ScopedLock lock( &m_mutex, false );
        WP_ASSERT( containsUnlocked( handle ) );
        return containsUnlocked( handle ) ? m_frameTimes[handle.index] : 0.0;
    }

    time_interval TransformSystem::getFrameDeltaTime( Handle handle ) const
    {
        ScopedLock lock( &m_mutex, false );
        WP_ASSERT( containsUnlocked( handle ) );
        return containsUnlocked( handle ) ? m_frameDeltaTimes[handle.index] : 0.0;
    }

    void TransformSystem::setFrameTime( Handle handle, time_interval frameTime )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( containsUnlocked( handle ) )
        {
            m_frameTimes[handle.index] = frameTime;
        }
    }

    void TransformSystem::setFrameDeltaTime( Handle handle, time_interval frameDeltaTime )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( containsUnlocked( handle ) )
        {
            m_frameDeltaTimes[handle.index] = frameDeltaTime;
        }
    }

    void TransformSystem::setFrameTimes( Handle handle, time_interval frameTime,
                                         time_interval frameDeltaTime )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( containsUnlocked( handle ) )
        {
            m_frameTimes[handle.index] = frameTime;
            m_frameDeltaTimes[handle.index] = frameDeltaTime;
        }
    }

    s32 TransformSystem::getReferenceCount( Handle handle ) const
    {
        ScopedLock lock( &m_mutex, false );
        WP_ASSERT( containsUnlocked( handle ) );
        return containsUnlocked( handle ) ? m_referenceCounts[handle.index] : 0;
    }

    void TransformSystem::setReferenceCount( Handle handle, s32 count )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( containsUnlocked( handle ) )
        {
            m_referenceCounts[handle.index] = count;
        }
    }

    void TransformSystem::addReference( Handle handle )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( containsUnlocked( handle ) )
        {
            ++m_referenceCounts[handle.index];
        }
    }

    void TransformSystem::removeReference( Handle handle )
    {
        ScopedLock lock( &m_mutex, true );
        WP_ASSERT( containsUnlocked( handle ) );
        if( containsUnlocked( handle ) )
        {
            --m_referenceCounts[handle.index];
        }
    }

    bool TransformSystem::containsUnlocked( Handle handle ) const
    {
        return handle.isValid() && handle.index < m_active.size() && m_active[handle.index] != 0 &&
               m_generations[handle.index] == handle.generation;
    }

    void TransformSystem::appendSlot()
    {
        m_localPositions.push_back( Vector3<real_Num>::zero() );
        m_localOrientations.push_back( Quaternion<real_Num>::identity() );
        m_localScales.push_back( Vector3<real_Num>::unit() );
        m_worldPositions.push_back( Vector3<real_Num>::zero() );
        m_worldOrientations.push_back( Quaternion<real_Num>::identity() );
        m_worldScales.push_back( Vector3<real_Num>::unit() );
        m_actors.emplace_back();
        m_tasks.push_back( TaskId::None );
        m_flags.push_back( 0 );
        m_frameTimes.push_back( 0.0 );
        m_frameDeltaTimes.push_back( 0.0 );
        m_referenceCounts.push_back( 0 );
        m_generations.push_back( 1 );
        m_active.push_back( 0 );
    }

    void TransformSystem::resetSlot( u32 index )
    {
        m_localPositions[index] = Vector3<real_Num>::zero();
        m_localOrientations[index] = Quaternion<real_Num>::identity();
        m_localScales[index] = Vector3<real_Num>::unit();
        m_worldPositions[index] = Vector3<real_Num>::zero();
        m_worldOrientations[index] = Quaternion<real_Num>::identity();
        m_worldScales[index] = Vector3<real_Num>::unit();
        m_actors[index] = WeakPtr<IGameActor>();
        m_tasks[index] = TaskId::None;
        m_flags[index] = 0;
        m_frameTimes[index] = 0.0;
        m_frameDeltaTimes[index] = 0.0;
        m_referenceCounts[index] = 0;
    }
}  // namespace workphone::scene
