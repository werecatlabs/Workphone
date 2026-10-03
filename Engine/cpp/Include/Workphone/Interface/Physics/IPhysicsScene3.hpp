#ifndef IPhysicsScene3_h__
#define IPhysicsScene3_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Physics/PhysicsTypes.hpp>
#include <Workphone/Math/Ray3.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a 3D physics scene.
         *
         * This class represents a physics simulation environment in 3D space.
         * It manages all physics objects, handles collision detection, and simulates
         * physical interactions between objects.
         *
         * The scene provides functionality for:
         * - Managing physics actors (bodies, shapes, etc.)
         * - Performing collision detection and raycasting
         * - Simulating physics with configurable parameters
         * - Managing scene properties (gravity, size, etc.)
         * - Thread management for physics simulation
         *
         * @see IPhysicsManager
         * @see IPhysicsBody3
         * @see IRaycastHit
         */
        class WPCore_API IPhysicsScene3 : public ISharedObject
        {
        public:
            /** Destructor */
            ~IPhysicsScene3() override;

            /**
             * @brief Removes all actors and clears the scene.
             *
             * This method removes all physics actors from the scene and resets
             * the scene to its initial state.
             */
            virtual void clear() = 0;

            /**
             * @brief Adds a physics body to the scene.
             *
             * @param body A shared pointer to the physics body to be added.
             *             The body will participate in physics simulation once added.
             */
            virtual void addActor( SmartPtr<IPhysicsBody3> body ) = 0;

            /**
             * @brief Removes a physics body from the scene.
             *
             * @param body A shared pointer to the physics body to be removed.
             *             The body will no longer participate in physics simulation.
             */
            virtual void removeActor( SmartPtr<IPhysicsBody3> body ) = 0;

            virtual Array<SmartPtr<IPhysicsBody3>> getActors() const = 0;

            virtual bool hasActor( SmartPtr<IPhysicsBody3> body ) const = 0;

            virtual u32 numDynamicActors() const = 0;

            virtual u32 numStaticActors() const = 0;

            /**
             * @brief Sets the size of the scene.
             *
             * @param size A vector representing the dimensions of the scene.
             *             This can be used to define the bounds of the physics world.
             */
            virtual void setSize( const Vector3<real_Num> &size ) = 0;

            /**
             * @brief Gets the size of the scene.
             *
             * @return A vector representing the dimensions of the scene.
             */
            virtual Vector3<real_Num> getSize() const = 0;

            /**
             * @brief Performs a ray test in the scene.
             *
             * @param start The starting point of the ray.
             * @param direction The direction of the ray.
             * @param hitPos The position where the ray hit an object.
             * @param hitNormal The normal of the surface that was hit.
             * @param collisionType The type of collision to test for.
             * @param collisionMask The collision mask to use for filtering.
             * @return True if the ray hit an object, false otherwise.
             */
            virtual bool rayTest( const Vector3<real_Num> &start, const Vector3<real_Num> &direction,
                                  Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal,
                                  u32 collisionType = 0, u32 collisionMask = 0 ) = 0;

            /**
             * @brief Performs a line intersection test in the scene.
             *
             * @param start The starting point of the line.
             * @param end The end point of the line.
             * @param hitPos The position where the line hit an object.
             * @param hitNormal The normal of the surface that was hit.
             * @param object The object that was hit.
             * @param collisionType The type of collision to test for.
             * @param collisionMask The collision mask to use for filtering.
             * @return True if the line hit an object, false otherwise.
             */
            virtual bool intersects( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                                     Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal,
                                     SmartPtr<ISharedObject> &object, u32 collisionType = 0,
                                     u32 collisionMask = 0 ) = 0;

            /**
             * @brief Casts a ray and returns all hits.
             *
             * @param origin The origin point of the ray.
             * @param dir The direction of the ray.
             * @param hits An array where all the hits will be stored.
             * @return True if there was at least one hit, false otherwise.
             */
            virtual bool castRay( const Vector3<real_Num> &origin, const Vector3<real_Num> &dir,
                                  Array<SmartPtr<IRaycastHit>> &hits ) = 0;

            /**
             * @brief Casts a ray and returns the first hit.
             *
             * @param ray The ray to cast.
             * @param hit The hit information to be stored.
             * @return True if there was a hit, false otherwise.
             */
            virtual bool castRay( const Ray3<real_Num> &ray, SmartPtr<IRaycastHit> hit ) = 0;

            /**
             * @brief Casts a dynamic ray and returns the first hit.
             *
             * This method is similar to castRay but is optimized for dynamic objects.
             *
             * @param ray The ray to cast.
             * @param hit The hit information to be stored.
             * @return True if there was a hit, false otherwise.
             */
            virtual bool castRayDynamic( const Ray3<real_Num> &ray, SmartPtr<IRaycastHit> hit ) = 0;

            /**
             * @brief Sets the gravity vector for the scene.
             *
             * @param vec The gravity vector to set.
             *             This affects all dynamic objects in the scene.
             */
            virtual void setGravity( const Vector3<real_Num> &vec ) = 0;

            /**
             * @brief Gets the gravity vector of the scene.
             *
             * @return The current gravity vector.
             */
            virtual Vector3<real_Num> getGravity() const = 0;

            /**
             * @brief Simulates physics for a given time step.
             *
             * @param elapsedTime The time step to simulate.
             * @param scratchMemBlock A block of memory for temporary calculations.
             * @param scratchMemBlockSize The size of the scratch memory block.
             * @param controlSimulation Whether to control the simulation timing.
             */
            virtual void simulate( real_Num elapsedTime, void *scratchMemBlock, u32 scratchMemBlockSize,
                                   bool controlSimulation ) = 0;

            /**
             * @brief Fetches the results of the physics simulation.
             *
             * @param block Whether to block until results are available.
             * @param errorState A pointer to store any error state.
             * @return True if results were successfully fetched, false otherwise.
             */
            virtual bool fetchResults( bool block, u32 *errorState ) = 0;

            /**
             * @brief Gets the minimum number of threads for physics simulation.
             *
             * @return The minimum number of threads.
             */
            virtual u32 getMinThreads() const = 0;

            /**
             * @brief Sets the minimum number of threads for physics simulation.
             *
             * @param minThreads The minimum number of threads to use.
             */
            virtual void setMinThreads( u32 minThreads ) = 0;

            /**
             * @brief Gets the maximum number of threads for physics simulation.
             *
             * @return The maximum number of threads.
             */
            virtual u32 getMaxThreads() const = 0;

            /**
             * @brief Sets the maximum number of threads for physics simulation.
             *
             * @param maxThreads The maximum number of threads to use.
             */
            virtual void setMaxThreads( u32 maxThreads ) = 0;

            /**
             * @brief Sets the spatial partitioning method for the scene.
             * @param method The partitioning method to use.
             */
            virtual void setSpatialPartitioning( SpatialPartitioningMethodEnum method ) = 0;

            /**
             * @brief Gets the current spatial partitioning method.
             * @return The current partitioning method.
             */
            virtual SpatialPartitioningMethodEnum getSpatialPartitioning() const = 0;

            /**
             * @brief Configures the spatial partitioning options.
             * @param options The options to apply.
             */
            virtual void setSpatialOptions( const SpatialPartitioningOptions &options ) = 0;

            /**
             * @brief Gets the current spatial partitioning options.
             * @return The current options.
             */
            virtual SpatialPartitioningOptions getSpatialOptions() const = 0;

            /**
             * @brief Configures the contact manifold update options.
             * @param options The options to apply.
             */
            virtual void setContactOptions( const ContactOptions &options ) = 0;

            /**
             * @brief Gets the current contact manifold update options.
             * @return The current options.
             */
            virtual ContactOptions getContactOptions() const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace physics
}  // namespace workphone

#endif  // IPhysicsScene3_h__
