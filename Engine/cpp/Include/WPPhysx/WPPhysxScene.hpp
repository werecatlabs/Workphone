#ifndef WPPhysxWorld_h__
#define WPPhysxWorld_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <Workphone/Physics/PhysicsScene3.hpp>
#include <Workphone/Core/Array.hpp>
#include <PxSimpleTypes.h>
#include <PxQueryReport.h>
#include <PxFiltering.h>
#include <PxSimulationEventCallback.h>
#include <PxContactModifyCallback.h>

namespace workphone
{
    namespace physics
    {
        /**
         * @brief PhysX-backed implementation of a physics scene.
         *
         * This class adapts the engine's abstract `PhysicsScene3` interface to an underlying
         * PhysX `physx::PxScene`. It owns runtime callbacks, manages simulation stepping,
         * provides scene queries (raycasts / intersection tests), and forwards events from PhysX
         * into the engine (contacts, triggers, sleep/wake, constraint breaks, etc).
         *
         * Responsibilities:
         * - Create, hold and expose a `physx::PxScene` instance.
         * - Drive simulation via `simulate()` / `fetchResults()`.
         * - Provide synchronous and asynchronous raycast helpers.
         * - Register callbacks for filtering, simulation events and contact modification.
         *
         * Threading / safety:
         * - PhysX scene write/read locking is handled by PhysX. Callers must respect PhysX
         *   threading rules when interacting with the raw `PxScene` pointer returned by `getScene()`.
         */
        class PhysxScene : public PhysicsScene3
        {
        public:
            /** @brief Default constructor. Initializes internal callback objects to nullptr. */
            PhysxScene();

            /** @brief Virtual destructor. Cleans up any PhysX callbacks and references. */
            ~PhysxScene() override;

            /**
             * @brief Load/initialise scene from shared data.
             *
             * Expected to configure or create the underlying PhysX `PxScene` using parameters
             * supplied in `data`. The exact format of `data` is implementation-defined.
             *
             * @param data Generic shared object containing initialization parameters.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload or release scene resources.
             *
             * Removes actors, unregisters callbacks and releases any owned references to the
             * PhysX scene. After unload the scene should be in a state safe to destroy.
             *
             * @param data Optional data describing unload specifics.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Work performed before the simulation step.
             *
             * Called once per frame before `simulate()`; used to prepare state, perform
             * scene locking or flush deferred changes to actors.
             */
            void preUpdate() override;

            /**
             * @brief Execute any per-frame logic while simulation is in progress.
             *
             * Called per-frame to update internal timers, animations or other pre/post-simulation
             * processing that does not directly step the PhysX simulation.
             */
            void update() override;

            /**
             * @brief Work performed after the simulation step.
             *
             * Called after `fetchResults()` to read back transforms from PhysX, dispatch events
             * and update any engine-side representations of physics objects.
             */
            void postUpdate() override;

            /** Set/query the gravity used by the native PhysX scene. */
            void setGravity( const Vector3<real_Num> &gravity ) override;
            Vector3<real_Num> getGravity() const override;

            /**
             * @brief Remove all actors and clear internal state.
             *
             * Clears the scene contents (actors, caches) without destroying the PxScene itself.
             * Use to reset simulation state between scenarios.
             */
            void clear() override;

            /**
             * @brief Perform a single ray test against the scene.
             *
             * This is a convenience wrapper that casts a ray from `start` along `direction`
             * and returns the first hit (if any).
             *
             * @param start Ray origin in world space.
             * @param direction Ray direction (normalized or scaled).
             * @param[out] hitPos If true returned, the world space hit position.
             * @param[out] hitNormal If true returned, the surface normal at the hit.
             * @param collisionType Bitmask describing the collision category of the ray.
             * @param collisionMask Bitmask describing which categories the ray can hit.
             * @return true if an object was hit; false otherwise.
             */
            bool rayTest( const Vector3F &start, const Vector3F &direction, Vector3F &hitPos,
                          Vector3F &hitNormal, u32 collisionType, u32 collisionMask ) override;

            /**
             * @brief Test if a segment intersects any object in the scene.
             *
             * Casts a line segment from `start` to `end`. If a hit is found the hit position/normal
             * and the associated engine object are returned.
             *
             * @param start Segment start in world space.
             * @param end Segment end in world space.
             * @param[out] hitPos World space impact point.
             * @param[out] hitNormal Surface normal at impact.
             * @param[out] object The engine object that was hit (if any).
             * @param collisionType Category bits for the test.
             * @param collisionMask Mask of categories to test against.
             * @return true if the segment intersects an object; false otherwise.
             */
            bool intersects( const Vector3F &start, const Vector3F &end, Vector3F &hitPos,
                             Vector3F &hitNormal, SmartPtr<ISharedObject> &object, u32 collisionType,
                             u32 collisionMask ) override;

            /**
             * @brief Get the underlying PhysX scene pointer.
             *
             * Returns the raw `physx::PxScene*` managed by this wrapper. The pointer may be null
             * if the scene has not been created or has been released.
             *
             * NOTE: callers must obey PhysX threading rules when using the returned pointer.
             *
             * @return Pointer to the PhysX scene, or nullptr.
             */
            physx::PxScene *getScene() const;

            /**
             * @brief Set or replace the internal PhysX scene pointer.
             *
             * Ownership semantics: this function stores the raw pointer only; lifetime management
             * responsibility must be agreed by caller and callee (no automatic release is performed
             * by this setter).
             *
             * @param scene Raw PhysX scene pointer to use.
             */
            void setScene( physx::PxScene *scene );

            /**
             * @brief Advance the PhysX simulation by a time step.
             *
             * This will enqueue a simulation step on the underlying `PxScene`. If `controlSimulation`
             * is true, the caller expects to manually call `fetchResults()`; otherwise the scene may
             * be stepped with default behaviour.
             *
             * @param elapsedTime Time step (seconds).
             * @param scratchMemBlock Optional scratch memory block for PhysX to use.
             * @param scratchMemBlockSize Size of the scratch memory block in bytes.
             * @param controlSimulation If true, the caller will control `fetchResults()`.
             */
            void simulate( physics_Num elapsedTime, void *scratchMemBlock, u32 scratchMemBlockSize,
                           bool controlSimulation ) override;

            /**
             * @brief Retrieve results from the previous simulation step.
             *
             * This will block if `block` is true until the simulation step completes.
             * `errorState` may be written if PhysX reports an error condition.
             *
             * @param block If true, block until results are available.
             * @param[out] errorState Optional pointer to receive an error code.
             * @return true if fetch was successful; false if an error occurred.
             */
            bool fetchResults( bool block, u32 *errorState ) override;

            /**
             * @brief Add an actor (body) to the scene.
             *
             * Actor is added to both PhysX and the engine bookkeeping structures.
             *
             * @param body Engine physics body to add.
             */
            void addActor( SmartPtr<IPhysicsBody3> body ) override;

            /**
             * @brief Remove an actor (body) from the scene.
             *
             * Actor will be removed from PhysX and engine bookkeeping.
             *
             * @param body Engine physics body to remove.
             */
            void removeActor( SmartPtr<IPhysicsBody3> body ) override;

            /**
             * @brief Cast a ray and collect all hits.
             *
             * Performs a raycast using `origin` and `dir` and fills `hits` with any intersections
             * found. The results may be sorted by distance depending on implementation.
             *
             * @param origin Ray origin / direction typed as `Vector3<physics_Num>`.
             * @param dir Ray direction vector (length determines max distance).
             * @param[out] hits Array to be filled with `IRaycastHit` results.
             * @return true if at least one hit was found; false otherwise.
             */
            bool castRay( const Vector3<physics_Num> &origin, const Vector3<physics_Num> &dir,
                          Array<SmartPtr<IRaycastHit>> &hits ) override;

            /**
             * @brief Cast a single ray and return the first hit in `hit`.
             *
             * @param ray Ray to cast.
             * @param[out] hit Single hit object to populate.
             * @return true if a hit occurred.
             */
            bool castRay( const Ray3<physics_Num> &ray, SmartPtr<IRaycastHit> hit ) override;

            /**
             * @brief Cast a dynamic ray (includes dynamic bodies) and return the first hit.
             *
             * Dynamic tests may include velocity / CCD aware queries.
             *
             * @param ray Ray to cast.
             * @param[out] hit Hit info to populate.
             * @return true if a hit occurred.
             */
            bool castRayDynamic( const Ray3<physics_Num> &ray, SmartPtr<IRaycastHit> hit ) override;

            /**
             * @brief Handle incoming state change message.
             *
             * Receives a message describing a state change and applies it to the scene.
             *
             * @param message The state message to handle.
             */
            void handleStateChanged( const SmartPtr<IStateMessage> &message ) override;

            /**
             * @brief Handle direct state object change.
             *
             * @param state The new state to apply to the scene.
             */
            void handleStateChanged( SmartPtr<IState> &state ) override;

            /**
             * @brief Maximum delta time the simulation step will accept.
             *
             * The scene may clamp large frame deltas to avoid instability; this value
             * controls the maximum allowed step.
             *
             * @return Maximum delta time in seconds.
             */
            time_interval getMaxDeltaTime() const;

            /**
             * @brief Set the maximum delta time allowed for simulation steps.
             *
             * @param maxDeltaTime Maximum time step in seconds.
             */
            void setMaxDeltaTime( time_interval maxDeltaTime );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Default simulation filter shader used by the PhysX scene.
             *
             * This function is used by PhysX to decide whether two objects should collide,
             * and which contact notifications / flags should be set for the pair.
             *
             * See PhysX documentation for `PxSimulationFilterShader`.
             */
            static physx::PxFilterFlags simulationFilterShader(
                physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
                physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
                physx::PxPairFlags &pairFlags, const void *constantBlock,
                physx::PxU32 constantBlockSize );

            /**
             * @brief Callback handling collision filtering status changes.
             *
             * This callback can be used to implement custom pair re-evaluation or dynamic
             * enabling/disabling of contacts at runtime.
             */
            class CollisionCallback : public physx::PxSimulationFilterCallback
            {
            public:
                CollisionCallback();
                ~CollisionCallback() override;

                /**
                 * @brief Called when a potential pair is found between two shapes.
                 *
                 * Allows modification of `pairFlags` to request contact reports, notifications,
                 * CCD, etc.
                 */
                physx::PxFilterFlags pairFound( u32 pairID, physx::PxFilterObjectAttributes attributes0,
                                                physx::PxFilterData   filterData0,
                                                const physx::PxActor *a0, const physx::PxShape *s0,
                                                physx::PxFilterObjectAttributes attributes1,
                                                physx::PxFilterData             filterData1,
                                                const physx::PxActor *a1, const physx::PxShape *s1,
                                                physx::PxPairFlags &pairFlags ) override;

                /**
                 * @brief Called when a filter status for an existing pair changes.
                 *
                 * Return true to indicate the caller changed the pair flags and that PhysX should
                 * re-evaluate the pair.
                 */
                bool statusChange( u32 &pairID, physx::PxPairFlags &pairFlags,
                                   physx::PxFilterFlags &filterFlags ) override;

                /**
                 * @brief Called when a pair is removed from simulation or an object is deleted.
                 *
                 * Use to clean up any engine-side bookkeeping associated with the pair.
                 */
                void pairLost( u32 pairID, physx::PxFilterObjectAttributes attributes0,
                               physx::PxFilterData             filterData0,
                               physx::PxFilterObjectAttributes attributes1,
                               physx::PxFilterData filterData1, bool objectDeleted ) override;
            };

            /**
             * @brief Handles simulation events forwarded by PhysX (contacts, triggers, wake/sleep).
             *
             * This callback receives high-level simulation events. Implementations typically
             * translate these into engine events or update game object state.
             */
            class SimulationEventCallback : public physx::PxSimulationEventCallback
            {
            public:
                SimulationEventCallback();
                ~SimulationEventCallback() override;

                /** @brief Called when constraints break. */
                void onConstraintBreak( physx::PxConstraintInfo *constraints, u32 count ) override;

                /** @brief Called when actors go to sleep. */
                void onSleep( physx::PxActor **actors, u32 count ) override;
                /** @brief Called when actors wake up. */
                void onWake( physx::PxActor **actors, u32 count ) override;

                /**
                 * @brief Called when contact events occur.
                 *
                 * `pairHeader` and `pairs` contain contact details for the reported pairs.
                 */
                void onContact( const physx::PxContactPairHeader &pairHeader,
                                const physx::PxContactPair *pairs, u32 nbPairs ) override;

                /** @brief Helper to check for breakage on a specific rigid body/shape pair. */
                void checkBreakage( physx::PxRigidDynamic *rb0, physx::PxShape *shape );

                /** @brief Called for trigger enter/leave events. */
                void onTrigger( physx::PxTriggerPair *pairs, u32 count ) override;

                // Note: onAdvance can be implemented if advanced update callbacks are required.
            };

            /**
             * @brief Contact modification callback.
             *
             * Allows the application to inspect and modify contact points before the solver runs.
             * Useful for custom contact response, friction changes or to disable specific contacts.
             */
            class ContactModificationCallback : public physx::PxContactModifyCallback
            {
            public:
                void onContactModify( physx::PxContactModifyPair *pairs, u32 count ) override;
            };

            /**
             * @brief Raycast callback that processes multiple hits.
             *
             * Collects hits into an internal buffer and optionally filters static/dynamic results.
             */
            class RaycastCallback : public physx::PxRaycastCallback
            {
            public:
                RaycastCallback();

                /**
                 * @brief Called by PhysX to process a batch of touch hits.
                 *
                 * Return `PxAgain::eYES` (or `PxAgain(true)`) to continue receiving touches.
                 */
                physx::PxAgain processTouches( const physx::PxRaycastHit *buffer, u32 nbHits ) override;

                physx::PxRaycastHit buffer[10]; /**< Temporary buffer for batched hits. */
                physx::PxRaycastHit m_hit;      /**< Closest hit tracked by this callback. */
                f32                 m_closestHit = (f32)1e10; /**< Distance of the closest hit. */
                bool                m_checkStatic = true; /**< Whether static objects are considered. */
                bool m_checkDynamic = true;               /**< Whether dynamic objects are considered. */
            };

            RawPtr<physx::PxDefaultCpuDispatcher>
                m_cpuDispatcher; /**< PhysX CPU dispatcher used by the scene. */

            AtomicRawPtr<physx::PxScene> m_scene; /**< Atomically held pointer to the PhysX scene. */

            RawPtr<CollisionCallback> m_collisionCallback; /**< Simulation filter callback instance. */
            RawPtr<SimulationEventCallback>
                m_simulationEventCallback; /**< Simulation event callback instance. */
            RawPtr<ContactModificationCallback>
                m_contactModificationCallback; /**< Contact modify callback. */

            /** @brief Maximum time step (seconds) that will be accepted for a single simulation step. */
            time_interval m_maxDeltaTime = 1.0 / 50.0;
        };
    } // end namespace physics
} // namespace workphone

#endif // WPPhysxWorld_h__
