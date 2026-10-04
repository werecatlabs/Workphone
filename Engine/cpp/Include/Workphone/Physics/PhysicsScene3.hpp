/**
 * @file PhysicsScene3.hpp
 * @brief Declaration of the PhysicsScene3 class which manages a 3D physics simulation scene.
 * @author Zane Desir
 * @date 31/10/2021
 *
 * This header declares a concrete implementation of a 3D physics scene. The scene owns and
 * manages physics bodies, performs queries (raycasts/intersections), controls simulation
 * stepping and result fetching, and integrates with a state-management subsystem.
 */

#ifndef WP_CPHYSICSSCENE_H
#define WP_CPHYSICSSCENE_H

#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @class PhysicsScene3
         * @brief Manages a 3D physics simulation scene.
         *
         * PhysicsScene3 provides operations to manage physics actors (rigid bodies),
         * perform scene queries (ray tests, raycasts, segment intersections), configure
         * world parameters (gravity, size), and control simulation execution and result
         * retrieval. It implements the IPhysicsScene3 interface and is intended to be
         * used by higher-level systems (game logic, editors, tools) that require
         * deterministic access to a physics world.
         *
         * Thread-safety:
         * - The scene exposes explicit lock/try_lock/unlock methods to coordinate access
         *   across threads; callers must follow the scene's synchronization rules.
         *
         * State integration:
         * - The scene can be attached to an IStateContext and IStateListener to receive
         *   and handle state messages or lifecycle events (see setStateContext / setStateListener).
         *
         * @see IPhysicsScene3
         */
        class WPCore_API PhysicsScene3 : public IPhysicsScene3
        {
        public:
            /**
             * @class StateListener
             * @brief Adapter that forwards state system events to a PhysicsScene3 instance.
             *
             * The nested StateListener implements IStateListener and holds a weak reference
             * to its owning PhysicsScene3. This prevents circular ownership and allows the
             * listener to be safely used by external state systems. When state events arrive
             * the listener can locate the owner and forward handling calls.
             */
            class StateListener : public IStateListener
            {
            public:
                /**
                 * @brief Construct a StateListener with no owner.
                 */
                StateListener();

                /**
                 * @brief Virtual destructor.
                 *
                 * Ensures derived cleanup via the IStateListener contract.
                 */
                ~StateListener() override;

                /**
                 * @brief Called by the state system to unload state-related resources.
                 * @param data Optional user data passed by the state system.
                 *
                 * Implementations should perform any cleanup or resource release required
                 * when the state subsystem requests an unload.
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handle an incoming state message.
                 * @param message The state message to process.
                 * @return true if the message was handled and no further processing is required,
                 *         false to allow other listeners to process it.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handle a state change event.
                 * @param state The new state object.
                 * @return true if the state change was handled, false otherwise.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Get a strong pointer to the owner PhysicsScene3, if still alive.
                 * @return SmartPtr<PhysicsScene3> A strong reference to the owner or null if expired.
                 */
                SmartPtr<PhysicsScene3> getOwner() const;

                /**
                 * @brief Set the owner PhysicsScene3 for this listener.
                 * @param owner Strong pointer to the owning PhysicsScene3.
                 *
                 * The listener stores a weak reference internally and will not keep the owner alive.
                 */
                void setOwner( SmartPtr<PhysicsScene3> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                AtomicWeakPtr<PhysicsScene3> m_owner;  ///< Weak reference to the owning PhysicsScene3
            };

            /**
             * @brief Construct a PhysicsScene3 instance.
             *
             * The constructor prepares internal containers and default world settings.
             */
            PhysicsScene3();

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup of derived resources and physics objects.
             */
            ~PhysicsScene3() override;

            /**
             * @brief Remove all actors and reset scene state.
             *
             * Clears internal actor lists and releases any resources owned by the scene.
             * After calling clear the scene is empty and must be repopulated before simulation.
             */
            void clear() override;

            /**
             * @brief Add a physics body to the scene.
             * @param body Strong pointer to a physics body implementing IPhysicsBody3.
             *
             * The scene takes ownership of the body via the provided smart pointer. If the
             * body is already present the behavior is implementation-defined (may ignore or replace).
             */
            void addActor( SmartPtr<IPhysicsBody3> body ) override;

            /**
             * @brief Remove a physics body from the scene.
             * @param body Strong pointer to the physics body to remove.
             *
             * If the body is present it will be removed and any scene-owned resources released.
             * If not present the call is a no-op.
             */
            void removeActor( SmartPtr<IPhysicsBody3> body ) override;

            Array<SmartPtr<IPhysicsBody3>> getActors() const;

            bool hasActor( SmartPtr<IPhysicsBody3> body ) const;

            u32 numDynamicActors() const;

            u32 numStaticActors() const;

            /**
             * @brief Set the logical size of the physics scene (world extents).
             * @param size 3D vector describing the nominal size or bounds of the simulation space.
             *
             * The interpretation of size depends on the concrete physics backend (e.g., broadphase
             * extents).
             */
            void setSize( const Vector3<real_Num> &size ) override;

            /**
             * @brief Get the current logical size of the physics scene.
             * @return Vector3<real_Num> Current world extents previously set via setSize.
             */
            Vector3<real_Num> getSize() const override;

            /**
             * @brief Perform a single ray test against the scene.
             * @param start World-space origin of the test ray.
             * @param direction Direction (and length) of the test ray.
             * @param hitPos Output: world-space position of the first hit, if any.
             * @param hitNormal Output: surface normal at the hit point, if any.
             * @param collisionType Collision type filter (backend-specific).
             * @param collisionMask Collision mask filter (backend-specific).
             * @return true if the ray hit an object; false otherwise.
             *
             * This method is intended for quick visibility or line-of-sight checks. For
             * retrieving multiple hits use castRay overload that returns all hits.
             */
            bool rayTest( const Vector3<real_Num> &start, const Vector3<real_Num> &direction,
                          Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal, u32 collisionType = 0,
                          u32 collisionMask = 0 ) override;

            /**
             * @brief Test whether a segment intersects any scene object.
             * @param start Segment start position in world-space.
             * @param end Segment end position in world-space.
             * @param hitPos Output: intersection position (closest).
             * @param hitNormal Output: normal at the intersection (closest).
             * @param object Output: shared pointer to the hit object (closest).
             * @param collisionType Collision type filter (backend-specific).
             * @param collisionMask Collision mask filter (backend-specific).
             * @return true if an intersection was found; false otherwise.
             *
             * This function performs a segment test and returns the first/closest hit found.
             */
            bool intersects( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                             Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal,
                             SmartPtr<ISharedObject> &object, u32 collisionType = 0,
                             u32 collisionMask = 0 ) override;

            /**
             * @brief Cast a ray and collect all hits along its path.
             * @param origin World-space origin of the ray.
             * @param dir World-space direction vector (may encode length).
             * @param hits Output array that will be populated with IRaycastHit entries (sorted by
             * distance).
             * @return true if one or more hits were found; false otherwise.
             *
             * The caller is responsible for providing an Array to receive results. The order of hits
             * is typically from nearest to farthest but may be backend-specific; check concrete backend
             * docs.
             */
            bool castRay( const Vector3<real_Num> &origin, const Vector3<real_Num> &dir,
                          Array<SmartPtr<IRaycastHit>> &hits ) override;

            /**
             * @brief Cast a ray and return the first (closest) hit.
             * @param ray Ray structure describing origin and direction.
             * @param hit Output pointer to receive the closest hit details.
             * @return true if a hit was found; false otherwise.
             */
            bool castRay( const Ray3<real_Num> &ray, SmartPtr<IRaycastHit> hit ) override;

            /**
             * @brief Cast a ray against dynamic (moving) objects only and return the first hit.
             * @param ray Ray structure describing origin and direction.
             * @param hit Output pointer to receive the hit details.
             * @return true if a dynamic object was hit; false otherwise.
             *
             * This is useful for queries that should ignore static scene geometry.
             */
            bool castRayDynamic( const Ray3<real_Num> &ray, SmartPtr<IRaycastHit> hit ) override;

            /**
             * @brief Set the gravity vector used by the scene.
             * @param gravity World-space gravity vector (units per second squared).
             *
             * Gravity is applied to dynamic bodies during simulation steps. The default gravity
             * is engine/backend dependent.
             */
            void setGravity( const Vector3<real_Num> &gravity ) override;

            /**
             * @brief Get the current gravity vector used by the scene.
             * @return Vector3<real_Num> Current gravity vector.
             */
            Vector3<real_Num> getGravity() const override;

            /**
             * @brief Advance the physics simulation by a time step.
             * @param elapsedTime Time in seconds to simulate.
             * @param scratchMemBlock Optional pointer to backend scratch memory required for the step.
             * @param scratchMemBlockSize Size in bytes of the scratch memory block.
             * @param controlSimulation If true the caller expects to control stepping externally.
             *
             * The simulate method schedules or performs the physics step depending on the backend.
             * When using asynchronous simulation, fetchResults must be called to obtain final results.
             */
            void simulate( real_Num elapsedTime, void *scratchMemBlock, u32 scratchMemBlockSize,
                           bool controlSimulation ) override;

            /**
             * @brief Fetch results produced by a previous simulate call.
             * @param block If true, block until results are available; otherwise return immediately.
             * @param errorState Optional output parameter to receive backend-specific error/state codes.
             * @return true if results were fetched successfully and are valid; false on error or if not
             * ready.
             *
             * Call this after simulate when the backend runs steps asynchronously. When block is true,
             * this method will wait for completion.
             */
            bool fetchResults( bool block, u32 *errorState ) override;

            /**
             * @brief Get the associated state context.
             * @return SmartPtr<IStateContext> The currently assigned state context or null.
             *
             * The state context is used to integrate the scene with a larger application state system.
             */
            SmartPtr<IStateContext> getStateContext() const;

            /**
             * @brief Assign a state context to the scene.
             * @param stateContext The state context to associate with this scene.
             *
             * The scene will use the context to register and receive state messages via its listener.
             */
            void setStateContext( SmartPtr<IStateContext> stateContext );

            /**
             * @brief Get the current state listener.
             * @return SmartPtr<IStateListener> The currently set state listener or null.
             */
            SmartPtr<IStateListener> getStateListener() const;

            /**
             * @brief Set the state listener used to receive state events.
             * @param stateListener Listener implementing IStateListener.
             *
             * Typically an instance of PhysicsScene3::StateListener is used so events are forwarded
             * to the owning scene instance.
             */
            void setStateListener( SmartPtr<IStateListener> stateListener );

            /**
             * @brief Handle a state message forwarded from the state subsystem.
             * @param message Message object describing the state event.
             *
             * Default implementation forwards to the configured state listener or performs
             * basic handling. Override in derived classes to implement custom behavior.
             */
            virtual void handleStateChanged( const SmartPtr<IStateMessage> &message );

            /**
             * @brief Handle a state object change forwarded from the state subsystem.
             * @param state The new state object.
             *
             * Implementers may react to lifecycle changes or configuration updates here.
             */
            virtual void handleStateChanged( SmartPtr<IState> &state );

            void setSpatialPartitioning( SpatialPartitioningMethodEnum method );
            SpatialPartitioningMethodEnum getSpatialPartitioning() const;

            void setSpatialOptions( const SpatialPartitioningOptions &options );

            SpatialPartitioningOptions getSpatialOptions() const;
            void setContactOptions( const ContactOptions &options );
            ContactOptions getContactOptions() const;

            /**
             * @brief Get the minimum worker thread count used by the physics backend.
             * @return u32 Minimum number of worker threads for simulation tasks.
             */
            u32 getMinThreads() const override;

            /**
             * @brief Set the minimum worker thread count for the physics backend.
             * @param minThreads The minimum number of threads to reserve.
             */
            void setMinThreads( u32 minThreads ) override;

            /**
             * @brief Get the maximum worker thread count used by the physics backend.
             * @return u32 Maximum number of worker threads for simulation tasks.
             */
            u32 getMaxThreads() const override;

            /**
             * @brief Set the maximum worker thread count for the physics backend.
             * @param maxThreads The maximum number of threads allowed.
             */
            void setMaxThreads( u32 maxThreads ) override;

            /**
             * @brief Acquire an exclusive lock for the physics scene.
             *
             * Callers should use lock/try_lock/unlock to guard access to scene state from multiple
             * threads.
             */
            void lock() override;

            /**
             * @brief Attempt to acquire the scene lock without blocking.
             * @return true if the lock was successfully acquired; false otherwise.
             */
            bool try_lock() override;

            /**
             * @brief Release a previously acquired scene lock.
             */
            void unlock() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            ///< Container of rigid bodies currently in the scene
            ConcurrentArray<SmartPtr<IRigidBody3>> m_rigidBodies;

            ///< Optional state context for integration with state system
            AtomicSmartPtr<IStateContext> m_stateContext;

            ///< Optional listener receiving state events for this scene
            AtomicSmartPtr<IStateListener> m_stateListener;
        };
    }  // end namespace physics
}  // namespace workphone

#endif  // WP_CPHYSICSSCENE_H
