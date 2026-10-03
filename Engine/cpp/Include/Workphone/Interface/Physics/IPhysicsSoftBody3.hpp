#ifndef __IPhysicsSoftBody3__H
#define __IPhysicsSoftBody3__H

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @file IPhysicsSoftBody3.hpp
         * @brief Interface for a 3D soft-body physics object.
         *
         * This interface exposes minimal functionality required by the engine to
         * manipulate and query the world-space position of a soft-body object.
         *
         * Implementations are expected to integrate with the engine's physics
         * backend (e.g., PhysX, custom solver) and to manage any internal data
         * required to represent a deformable body.
         */
        class WPCore_API IPhysicsSoftBody3 : public ISharedObject
        {
        public:
            /**
             * @brief Virtual destructor.
             *
             * Ensures derived soft-body implementations are properly cleaned up
             * when deleted through a pointer to this interface.
             */
            ~IPhysicsSoftBody3() override;

            /**
             * @brief Set the soft-body's world-space position.
             *
             * This sets the primary position for the soft-body. Depending on the
             * implementation this may set the object's centroid, reference frame,
             * or transform origin used by the physics solver. Calling this may
             * implicitly wake the object in the physics simulation or update
             * internal solver state.
             *
             * @param position The new world-space position as a Vector3 of type real_Num.
             *
             * @note Units are engine units (commonly meters). Implementations should
             *       document any deviations if a different convention is used.
             */
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;

            /**
             * @brief Get the soft-body's current world-space position.
             *
             * Returns the primary position for the soft-body (for example the
             * centroid or transform origin). The returned reference refers to an
             * internal value owned by the implementation. Do not store the
             * reference beyond the lifetime of the soft-body object.
             *
             * @return const Vector3<real_Num>& Current world-space position.
             */
            virtual const Vector3<real_Num> &getPosition() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace physics
}  // namespace workphone

#endif
