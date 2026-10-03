#ifndef Collision_h__
#define Collision_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{
    namespace scene
    {
        /** Base class for a collision component. */
        class WPCore_API Collision : public Component
        {
        public:
            /** Property key for isTrigger flag */
            static const String isTriggerStr;

            /** Property key for extents */
            static const String extentsStr;

            /** Property key for position offset */
            static const String positionStr;

            /** Property key for radius */
            static const String radiusStr;

            /** Property key for static friction */
            static const String staticFrictionStr;

            /** Property key for dynamic friction */
            static const String dynamicFrictionStr;

            /** Property key for restitution */
            static const String restitutionStr;

            /** Default constructor. */
            Collision();

            /** Default destructor. */
            ~Collision() override;

            /** @copydoc Component::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /** @copydoc Component::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc Component::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** Get the extents of the collision. */
            Vector3<real_Num> getExtents() const;

            /** Set the extents of the collision. */
            virtual void setExtents( const Vector3<real_Num> &extents );

            /** Get the position of the collision. */
            Vector3<real_Num> getPosition() const;

            /** Set the position offset of the collision and update the shape local pose. */
            void setPosition( const Vector3<real_Num> &position );

            /** Get the radius of the collision. */
            f32 getRadius() const;

            /** Set the radius of the collision. */
            void setRadius( f32 radius );

            /** Get the static friction of the physics material (direction 0). */
            f32 getStaticFriction() const;

            /** Set the static friction on the physics material (direction 0). */
            void setStaticFriction( f32 friction );

            /** Get the dynamic friction of the physics material (direction 0). */
            f32 getDynamicFriction() const;

            /** Set the dynamic friction on the physics material (direction 0). */
            void setDynamicFriction( f32 friction );

            /** Get the restitution of the physics material. */
            f32 getRestitution() const;

            /** Set the restitution on the physics material. */
            void setRestitution( f32 restitution );

            /** Gets the rigid body. */
            SmartPtr<Rigidbody> getRigidBody() const;

            /** Sets the rigid body. */
            void setRigidBody( SmartPtr<Rigidbody> rigidBody );

            /** Get the bounding box of the collision. */
            AABB3<real_Num> getBoundingBox() const override;

            /** Get physics material. */
            SmartPtr<physics::IPhysicsMaterial3> getMaterial() const;

            /** Set physics material. */
            void setMaterial( SmartPtr<physics::IPhysicsMaterial3> material );

            /** Get physics shape. */
            physics::IPhysicsShape3 *getShapePtr() const;

            /** Get physics shape. */
            SmartPtr<physics::IPhysicsShape3> getShape() const;

            /** Set physics shape. */
            void setShape( SmartPtr<physics::IPhysicsShape3> shape );

            /** @copydoc IComponent::setEnabled */
            void setEnabled( bool enabled ) override;

            /** Get trigger flag. */
            bool isTrigger() const;

            /** Set trigger flag. */
            void setTrigger( bool trigger );

            /** @copydoc Component::isValid */
            bool isValid() const override;

            /** @copydoc Component::updateTransform */
            void updateTransform() override;

            /** @copydoc Component::handleComponentEvent */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Creates the physics shape. */
            virtual void createPhysicsShape();

            /** Updates the rigid body. */
            virtual void updateRigidBody();

            // The extents of the collision.
            Vector3<real_Num> m_extents = Vector3<real_Num>::unit();

            // The position of the collision.
            Vector3<real_Num> m_position = Vector3<real_Num>::zero();

            // The radius of the collision.
            f32 m_radius = 1.0f;

            // The rigid body.
            SmartPtr<Rigidbody> m_rigidBody;

            // The physics material.
            SmartPtr<physics::IPhysicsMaterial3> m_material;

            // The physics shape.
            SmartPtr<physics::IPhysicsShape3> m_shape;

            // The trigger flag.
            bool m_isTrigger = false;

            // Cached static friction (direction 0).
            f32 m_staticFriction = 0.5f;

            // Cached dynamic friction (direction 0).
            f32 m_dynamicFriction = 0.5f;

            // Cached restitution.
            f32 m_restitution = 0.0f;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // Collision_h__
