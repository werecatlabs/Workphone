#ifndef IVehicleBody_h__
#define IVehicleBody_h__

#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Ray3.hpp>

namespace workphone
{
    namespace vehicle
    {

        class WPCore_API IVehicleBody : public IVehicleComponent
        {
        public:
            ~IVehicleBody() override;

            /**
             * @brief Gets the world-space velocity of the body.
             * @return The velocity.
             */
            virtual Vector3<real_Num> getVelocity() const = 0;

            /**
             * @brief Sets the world-space velocity of the body.
             * @param velocity The velocity to set.
             */
            virtual void setVelocity( const Vector3<real_Num> &velocity ) = 0;

            /**
             * @brief Gets the world-space angular velocity of the body.
             * @return The angular velocity.
             */
            virtual Vector3<real_Num> getAngularVelocity() const = 0;

            /**
             * @brief Sets the world-space angular velocity of the body.
             * @param angularVelocity The angular velocity to set.
             */
            virtual void setAngularVelocity( const Vector3<real_Num> &angularVelocity ) = 0;

            /**
             * @brief Gets the local-space velocity of the body.
             * @return The local velocity.
             */
            virtual Vector3<real_Num> getLocalVelocity() const = 0;

            /**
             * @brief Sets the local-space velocity of the body.
             * @param localVelocity The local velocity to set.
             */
            virtual void setLocalVelocity( const Vector3<real_Num> &localVelocity ) = 0;

            /**
             * @brief Gets the local-space angular velocity of the body.
             * @return The local angular velocity.
             */
            virtual Vector3<real_Num> getLocalAngularVelocity() const = 0;

            /**
             * @brief Sets the local-space angular velocity of the body.
             * @param localAngularVelocity The local angular velocity to set.
             */
            virtual void setLocalAngularVelocity( const Vector3<real_Num> &localAngularVelocity ) = 0;

            /**
             * @brief Gets the world-space center of mass of the body.
             * @return The center of mass.
             */
            virtual Vector3<real_Num> getWorldCenterOfMass() const = 0;

            /**
             * @brief Sets the world-space center of mass of the body.
             * @param worldCenterOfMass The center of mass to set.
             */
            virtual void setWorldCenterOfMass( const Vector3<real_Num> &worldCenterOfMass ) = 0;

            /**
             * @brief Gets the velocity of a point on the body.
             * @param p The point on the body.
             * @return The velocity of the point.
             */
            virtual Vector3<real_Num> getPointVelocity( const Vector3<real_Num> &p ) = 0;

            /**
             * @brief Applies a force to the body in local space at a local position.
             * @param localForce The force to apply.
             * @param localPosition The position to apply the force.
             */
            virtual void addLocalForceAtLocalPosition( const Vector3<real_Num> &localForce,
                                                       const Vector3<real_Num> &localPosition ) = 0;

            /**
             * @brief Applies a force to the body at a position in world space.
             * @param force The force to apply.
             * @param position The position to apply the force.
             */
            virtual void addForceAtPosition( const Vector3<real_Num> &force,
                                             const Vector3<real_Num> &position ) = 0;

            /**
             * @brief Applies a torque to the body in world space.
             * @param torque The torque to apply.
             */
            virtual void addTorque( const Vector3<real_Num> &torque ) = 0;

            /**
             * @brief Applies a torque to the body in local space.
             * @param localTorque The torque to apply.
             */
            virtual void addLocalTorque( const Vector3<real_Num> &localTorque ) = 0;

            /**
             * @brief Casts a ray in the local space of the body.
             * @param ray The ray to cast.
             * @param data The hit data.
             * @return True if the ray hits.
             */
            virtual bool castLocalRay( const Ray3<real_Num> &ray,
                                       SmartPtr<physics::IRaycastHit> &data ) = 0;

            /**
             * @brief Casts a ray in the world space of the body.
             * @param ray The ray to cast.
             * @param data The hit data.
             * @return True if the ray hits.
             */
            virtual bool castWorldRay( const Ray3<real_Num> &ray,
                                       SmartPtr<physics::IRaycastHit> &data ) = 0;

            /**
             * @brief Gets the mass of the body.
             * @return The mass.
             */
            virtual real_Num getMass() const = 0;

            /**
             * @brief Sets the mass of the body.
             * @param mass The mass to set.
             */
            virtual void setMass( real_Num mass ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IAircraftBody_h__
