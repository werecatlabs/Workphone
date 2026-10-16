#ifndef WPFoliagePhysics_h__
#define WPFoliagePhysics_h__
#include <Workphone/Graphics/FoliageData.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <functional>

namespace workphone::physics
{
    class IPhysicsManager;
    class IPhysicsScene3;
    class IRigidStatic3;
}  // namespace workphone::physics

namespace workphone::scene
{
    class FoliageAuthoring;

    struct FoliageCollisionStats
    {
        u32 active = 0, eligible = 0, omitted = 0, visitedCells = 0;
        u64 created = 0, retired = 0, reservedBytes = 0;
    };

    /** Owner-thread proximity solids, independent of visual LOD and shadow distance.
     * Uses conservative trunk boxes (grass and ground cover never create solids).
     * Caller updates observers on the physics owner thread before stepping the scene.
     * A bounded nearest set wins capacity pressure deterministically by stable ID.
     */
    class WPCore_API FoliagePhysics final
    {
    public:
        FoliagePhysics( SmartPtr<physics::IPhysicsManager>, SmartPtr<physics::IPhysicsScene3>,
                        u32 maximumColliders = 512 );
        ~FoliagePhysics();
        bool setSource( render::FoliageSnapshot, const Transform3<real_Num> &placement, String &error );
        bool update( const Array<Vector3F> &observers, String &error );
        void clear();
        FoliageCollisionStats getStats() const;
        SmartPtr<physics::IRigidStatic3> getBody( u64 instanceId ) const;
        FoliagePhysics( const FoliagePhysics & ) = delete;
        FoliagePhysics &operator=( const FoliagePhysics & ) = delete;

    private:
        struct State;
        std::unique_ptr<State> m_state;
    };

    /** Transactional bridge for an application's gameplay actor/prefab factory.
     * The acquire callback must return a fully staged actor; release must not throw.
     * Promoted IDs persist in authoring; callers saving a game must save their actors
     * too, or demote before saving. Removal tombstones survive regeneration.
     */
    class WPCore_API FoliageGameplay final
    {
    public:
        using Acquire =
            std::function<SmartPtr<ISharedObject>( const render::FoliageInstance &, String & )>;
        using Release = std::function<void( SmartPtr<ISharedObject> )>;
        FoliageGameplay( SmartPtr<FoliageAuthoring>, Acquire, Release );
        ~FoliageGameplay();
        bool promote( u64 instanceId, String &error );
        bool demote( u64 instanceId, String &error );
        bool remove( u64 instanceId, String &error );
        SmartPtr<ISharedObject> getActor( u64 instanceId ) const;
        FoliageGameplay( const FoliageGameplay & ) = delete;
        FoliageGameplay &operator=( const FoliageGameplay & ) = delete;

    private:
        struct State;
        std::unique_ptr<State> m_state;
    };
}  // namespace workphone::scene
#endif
