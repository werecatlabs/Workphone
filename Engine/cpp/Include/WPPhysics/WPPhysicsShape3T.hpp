#ifndef WPPHYSICSSHAPE3T_HPP
#define WPPHYSICSSHAPE3T_HPP

#include <WPPhysics/WPPhysicsUtil.hpp>
#include <WPPhysics/WPPhysicsMaterial3.hpp>
#include <stdexcept>
#include <Workphone/Physics/BoxShape3.hpp>
#include <Workphone/Physics/SphereShape.hpp>
#include <Workphone/Physics/PlaneShape.hpp>
#include <Workphone/Physics/MeshShape.hpp>
#include <Workphone/Physics/TerrainShape.hpp>

namespace workphone::physics
{
    class WPPhysicsShape3Backend
    {
    public:
        virtual ~WPPhysicsShape3Backend() = default;
        virtual AABB3<real_Num> getAABB() const = 0;
    };

    /** Shares native collision-shape behavior across the concrete shape bases. */
    template <class T>
    class WPPhysicsShape3T : public T, public WPPhysicsShape3Backend
    {
    public:
        WPPhysicsShape3T();

        WPPhysicsShape3T( u32 type );

        ~WPPhysicsShape3T() override;

        void load( SmartPtr<ISharedObject> data ) override;

        void unload( SmartPtr<ISharedObject> data ) override;

        SmartPtr<IPhysicsMaterial3> getMaterial() const;

        void setMaterial( SmartPtr<IPhysicsMaterial3> material );

        void setLocalPose( const Transform3<real_Num> &pose );

        Transform3<real_Num> getLocalPose() const;

        void setSimulationFilterData( const FilterData &data );

        FilterData getSimulationFilterData() const;

        void setActor( SmartPtr<IPhysicsBody3> body );

        SmartPtr<IPhysicsBody3> getActor() const;

        void _getObject( void **ppObject ) const;

        bool hasShapeData() const;

        SmartPtr<IPhysicsShape3> clone() override;

        Vector3<real_Num> getExtents() const;

        void setExtents( const Vector3<real_Num> &extents );

        AABB3<real_Num> getAABB() const;

        void setAABB( const AABB3<real_Num> &box );

        void setRadius( real_Num radius );

        real_Num getRadius() const;

        wp_collision_shape *getShape() const;

        bool isAttached() const;

        void setEnabled( bool enabled );

        bool isEnabled() const;

        void setTrigger( bool trigger );

        bool isTrigger() const;

        void setStateContext( SmartPtr<IStateContext> stateContext );

        SmartPtr<IStateContext> getStateContext() const;

        void setStateListener( SmartPtr<IStateListener> stateListener );

        SmartPtr<IStateListener> getStateListener() const;

        void setCollisionType( u32 mask );

        u32 getCollisionType() const;

        void setCollisionMask( u32 mask );

        u32 getCollisionMask() const;

        void lock() override;

        bool try_lock() override;

        void unlock() override;

        u32 getType() const;

        void setType( u32 type );

    protected:
        u32 m_type = 0;  ///< The type of the shape, as defined by the physics engine.
        wp_collision_shape *m_shape =
            nullptr;  ///< Internal pointer to the physics engine's collision shape.
        SmartPtr<IPhysicsMaterial3> m_material;  ///< The material assigned to this shape.
        WeakPtr<IPhysicsBody3> m_actor;  ///< Weak reference to the physics body owning this shape.
        SmartPtr<IStateContext> m_stateContext;    ///< Context for state management.
        SmartPtr<IStateListener> m_stateListener;  ///< Listener for state change notifications.
        AABB3<real_Num> m_aabb;                    ///< Cached axis-aligned bounding box.
        Vector3<real_Num> m_boxExtents = Vector3<real_Num>::unit();
        Vector3<real_Num> m_boxScale = Vector3<real_Num>::unit();
    };
    extern template class WPPhysicsShape3T<BoxShape3>;
    extern template class WPPhysicsShape3T<SphereShape>;
    extern template class WPPhysicsShape3T<PlaneShape>;
    extern template class WPPhysicsShape3T<MeshShape>;
    extern template class WPPhysicsShape3T<TerrainShape>;
}  // namespace workphone::physics

#endif
