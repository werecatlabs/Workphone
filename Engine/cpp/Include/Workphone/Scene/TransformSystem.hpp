#ifndef TransformSystem_h__
#define TransformSystem_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Memory/WeakPtr.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>
#include <Workphone/Thread/ScopedLock.hpp>

namespace workphone::scene
{
    /**
     * Owns transform data in structure-of-arrays storage.
     *
     * Handles remain stable while a Transform facade is alive. A generation prevents a
     * released slot from aliasing a later transform when the slot is reused.
     */
    class WPCore_API TransformSystem final
    {
    public:
        struct Handle
        {
            static constexpr u32 invalidIndex = static_cast<u32>( -1 );

            u32 index = invalidIndex;
            u32 generation = 0;

            bool isValid() const
            {
                return index != invalidIndex;
            }

            bool operator==( const Handle &other ) const
            {
                return index == other.index && generation == other.generation;
            }

            bool operator!=( const Handle &other ) const
            {
                return !( *this == other );
            }
        };

        /**
         * Non-owning view of the contiguous hot transform channels. A view is valid only
         * for the duration of the readData/writeData callback that supplied it.
         */
        struct ConstDataView
        {
            const Vector3<real_Num> *localPositions = nullptr;
            const Quaternion<real_Num> *localOrientations = nullptr;
            const Vector3<real_Num> *localScales = nullptr;
            const Vector3<real_Num> *worldPositions = nullptr;
            const Quaternion<real_Num> *worldOrientations = nullptr;
            const Vector3<real_Num> *worldScales = nullptr;
            const u8 *activeSlots = nullptr;
            const u32 *generations = nullptr;
            size_t slotCount = 0;
        };

        struct DataView
        {
            Vector3<real_Num> *localPositions = nullptr;
            Quaternion<real_Num> *localOrientations = nullptr;
            Vector3<real_Num> *localScales = nullptr;
            Vector3<real_Num> *worldPositions = nullptr;
            Quaternion<real_Num> *worldOrientations = nullptr;
            Vector3<real_Num> *worldScales = nullptr;
            const u8 *activeSlots = nullptr;
            const u32 *generations = nullptr;
            size_t slotCount = 0;
        };

        static TransformSystem &instance();

        TransformSystem( const TransformSystem &other ) = delete;
        TransformSystem &operator=( const TransformSystem &other ) = delete;

        Handle createTransform();
        void destroyTransform( Handle handle );

        bool contains( Handle handle ) const;
        size_t getActiveCount() const;
        size_t getSlotCount() const;
        void reserve( size_t count );

        /** Locks the storage once so systems can process whole channels in a batch. */
        template <class Visitor>
        void readData( Visitor visitor ) const
        {
            ScopedLock lock( &m_mutex, false );
            visitor( ConstDataView{ m_localPositions.data(), m_localOrientations.data(),
                                    m_localScales.data(), m_worldPositions.data(),
                                    m_worldOrientations.data(), m_worldScales.data(), m_active.data(),
                                    m_generations.data(), m_active.size() } );
        }

        /** Mutable batch access for transform systems; handle metadata remains read-only. */
        template <class Visitor>
        void writeData( Visitor visitor )
        {
            ScopedLock lock( &m_mutex, true );
            visitor( DataView{ m_localPositions.data(), m_localOrientations.data(), m_localScales.data(),
                               m_worldPositions.data(), m_worldOrientations.data(), m_worldScales.data(),
                               m_active.data(), m_generations.data(), m_active.size() } );
        }

        Transform3<real_Num> getLocalTransform( Handle handle ) const;
        void setLocalTransform( Handle handle, const Transform3<real_Num> &transform );

        Transform3<real_Num> getWorldTransform( Handle handle ) const;
        void setWorldTransform( Handle handle, const Transform3<real_Num> &transform );

        void getTransforms( Handle handle, Transform3<real_Num> &localTransform,
                            Transform3<real_Num> &worldTransform ) const;
        void setTransforms( Handle handle, const Transform3<real_Num> &localTransform,
                            const Transform3<real_Num> &worldTransform );

        Vector3<real_Num> getLocalPosition( Handle handle ) const;
        void setLocalPosition( Handle handle, const Vector3<real_Num> &position );

        Quaternion<real_Num> getLocalOrientation( Handle handle ) const;
        void setLocalOrientation( Handle handle, const Quaternion<real_Num> &orientation );

        Vector3<real_Num> getLocalScale( Handle handle ) const;
        void setLocalScale( Handle handle, const Vector3<real_Num> &scale );

        Vector3<real_Num> getWorldPosition( Handle handle ) const;
        void setWorldPosition( Handle handle, const Vector3<real_Num> &position );

        Quaternion<real_Num> getWorldOrientation( Handle handle ) const;
        void setWorldOrientation( Handle handle, const Quaternion<real_Num> &orientation );

        Vector3<real_Num> getWorldScale( Handle handle ) const;
        void setWorldScale( Handle handle, const Vector3<real_Num> &scale );

        /**
         * Updates one slot inside the central storage. Passing null represents a root
         * transform. These operations avoid constructing an object graph in the system.
         */
        void updateWorldFromLocal( Handle handle, const Transform3<real_Num> *parentWorldTransform );
        void updateLocalFromWorld( Handle handle, const Transform3<real_Num> *parentWorldTransform );

        IGameActor *getActorPtr( Handle handle ) const;
        SmartPtr<IGameActor> getActor( Handle handle ) const;
        void setActor( Handle handle, SmartPtr<IGameActor> actor );

        bool getFlag( Handle handle, u8 flag ) const;
        void setFlag( Handle handle, u8 flag, bool value );
        u8 getFlags( Handle handle ) const;
        void setFlags( Handle handle, u8 flags );

        TaskId getTask( Handle handle ) const;
        void setTask( Handle handle, TaskId task );

        time_interval getFrameTime( Handle handle ) const;
        time_interval getFrameDeltaTime( Handle handle ) const;
        void setFrameTime( Handle handle, time_interval frameTime );
        void setFrameDeltaTime( Handle handle, time_interval frameDeltaTime );
        void setFrameTimes( Handle handle, time_interval frameTime, time_interval frameDeltaTime );

        s32 getReferenceCount( Handle handle ) const;
        void setReferenceCount( Handle handle, s32 count );
        void addReference( Handle handle );
        void removeReference( Handle handle );

    private:
        TransformSystem() = default;
        ~TransformSystem() = default;

        bool containsUnlocked( Handle handle ) const;
        void appendSlot();
        void resetSlot( u32 index );

        mutable RecursiveSpinMutex m_mutex;

        // Hot transform channels are kept contiguous and independently iterable.
        Array<Vector3<real_Num>> m_localPositions;
        Array<Quaternion<real_Num>> m_localOrientations;
        Array<Vector3<real_Num>> m_localScales;
        Array<Vector3<real_Num>> m_worldPositions;
        Array<Quaternion<real_Num>> m_worldOrientations;
        Array<Vector3<real_Num>> m_worldScales;

        // Colder ownership and scheduling channels use the same slot index.
        Array<WeakPtr<IGameActor>> m_actors;
        Array<TaskId> m_tasks;
        Array<u8> m_flags;
        Array<time_interval> m_frameTimes;
        Array<time_interval> m_frameDeltaTimes;
        Array<s32> m_referenceCounts;

        Array<u32> m_generations;
        Array<u8> m_active;
        Array<u32> m_freeIndices;
        size_t m_activeCount = 0;
    };
}  // namespace workphone::scene

#endif  // TransformSystem_h__
