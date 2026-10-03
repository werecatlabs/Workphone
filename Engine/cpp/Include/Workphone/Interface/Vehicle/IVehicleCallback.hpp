#ifndef IVehicleCallback_h__
#define IVehicleCallback_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Ray3.hpp>
#include <Workphone/Core/FixedArray.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * @brief Interface for a vehicle callback class.
         */
        class WPCore_API IVehicleCallback : public ISharedObject
        {
        public:
            /**
             * @brief Destructor for IVehicleCallback.
             */
            ~IVehicleCallback() override;

            /**
             * @brief Gets data from a file.
             * @param filePath The path to the file.
             * @return The data in the file.
             */
            virtual String getData( const String &filePath ) = 0;

            /**
             * @brief Adds a force to a body.
             * @param bodyId The ID of the body.
             * @param force The force to add.
             * @param pos The position to apply the force at.
             */
            virtual void addForce( s32 bodyId, const Vector3<real_Num> &force,
                                   const Vector3<real_Num> &pos ) = 0;

            /**
             * @brief Adds a torque to a body.
             * @param bodyId The ID of the body.
             * @param torque The torque to add.
             */
            virtual void addTorque( s32 bodyId, const Vector3<real_Num> &torque ) = 0;

            /**
             * @brief Adds a local force to a body.
             * @param bodyId The ID of the body.
             * @param force The force to add.
             * @param pos The position to apply the force at.
             */
            virtual void addLocalForce( s32 bodyId, const Vector3<real_Num> &force,
                                        const Vector3<real_Num> &pos ) = 0;

            /**
             * @brief Adds a local torque to a body.
             * @param bodyId The ID of the body.
             * @param torque The torque to add.
             */
            virtual void addLocalTorque( s32 bodyId, const Vector3<real_Num> &torque ) = 0;

            /**
             * @brief Gets the angular velocity of the body.
             * @return The angular velocity of the body.
             */
            virtual Vector3<real_Num> getAngularVelocity() const = 0;

            /**
             * @brief Gets the linear velocity of the body.
             * @return The linear velocity of the body.
             */
            virtual Vector3<real_Num> getLinearVelocity() const = 0;

            /**
             * @brief Gets the local angular velocity of the body.
             * @return The local angular velocity of the body.
             */
            virtual Vector3<real_Num> getLocalAngularVelocity() const = 0;

            /**
             * @brief Gets the local linear velocity of the body.
             * @return The local linear velocity of the body.
             */
            virtual Vector3<real_Num> getLocalLinearVelocity() const = 0;

            /**
             * @brief Gets the position of the body.
             * @return The position of the body.
             */
            virtual Vector3<real_Num> getPosition() const = 0;

            /**
             * @brief Gets the scale of the body.
             * @return The scale of the body.
             */
            virtual Vector3<real_Num> getScale() const = 0;

            /**
             * @brief Gets the orientation of the body.
             * @return The orientation of the body.
             */
            virtual Quaternion<real_Num> getOrientation() const = 0;

            /**
             * @brief Displays a local vector.
             * @param Bdy The ID of the body.
             * @param V The vector to display.
             * @param Org The origin of the vector.
             * @param colour The color of the vector.
             */
            virtual void displayLocalVector( s32 Bdy, const Vector3<real_Num> &V,
                                             const Vector3<real_Num> &Org, s32 colour ) const = 0;

            /**
             * @brief Displays a vector.
             * @param Bdy The ID of the body.
             * @param id The ID of the vector.
             * @param V The vector to display.
             * @param Org The origin of the vector.
             * @param colour The color of the vector.
             */
            virtual void displayVector( s32 Bdy, s32 id, const Vector3<real_Num> &V,
                                        const Vector3<real_Num> &Org, s32 colour ) const = 0;

            /**
             * @brief Displays a local vector.
             * @param Bdy The ID of the body.
             * @param id The ID of the vector.
             * @param V The vector to display.
             * @param Org The origin of the vector.
             * @param colour The color of the vector.
             */
            virtual void displayLocalVector( s32 Bdy, s32 id, const Vector3<real_Num> &V,
                                             const Vector3<real_Num> &Org, s32 colour ) const = 0;

            /**
             * @brief Gets the callback function.
             * @return The callback function.
             */
            virtual void *getCallbackFunction() const = 0;

            /**
             * @brief Sets the callback function.
             * @param callbackFunc The callback function.
             */
            virtual void setCallbackFunction( void *callbackFunc ) = 0;

            /**
             * @brief Gets the callback data function.
             * @return The callback data function.
             */
            virtual void *getCallbackDataFunction() const = 0;

            /**
             * @brief Sets the callback data function.
             * @param callbackDataFunc The callback data function.
             */
            virtual void setCallbackDataFunction( void *callbackDataFunc ) = 0;

            /**
             * @brief Gets the input data.
             * @return The input data.
             */
            virtual FixedArray<f32, 8> getInputData() const = 0;

            /**
             * @brief Gets the control angles.
             * @return The control angles.
             */
            virtual FixedArray<f32, 11> getControlAngles() const = 0;

            /**
             * @brief Gets the velocity of a point on the body.
             * @param p The point on the body.
             * @return The velocity of the point.
             */
            virtual Vector3<real_Num> getPointVelocity( const Vector3<real_Num> &p ) = 0;

            /**
             * @brief Casts a ray in the local space of the body.
             * @param ray The ray to cast.
             * @param data The hit data.
             * @return True if the ray hit something, false otherwise.
             */
            virtual bool castLocalRay( const Ray3<real_Num> &ray,
                                       SmartPtr<physics::IRaycastHit> &data ) = 0;

            /**
             * @brief Casts a ray in the world space.
             * @param ray The ray to cast.
             * @param data The hit data.
             * @return True if the ray hit something, false otherwise.
             */
            virtual bool castWorldRay( const Ray3<real_Num> &ray,
                                       SmartPtr<physics::IRaycastHit> &data ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IVehicleCallback_h__
