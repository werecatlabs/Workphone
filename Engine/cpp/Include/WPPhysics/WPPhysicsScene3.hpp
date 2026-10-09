#ifndef WPPHYSICSSCENE3_HPP
#define WPPHYSICSSCENE3_HPP

#include <WPPhysics/WPPhysicsUtil.hpp>
#include <Workphone/Physics/PhysicsScene3.hpp>
#include <unordered_map>

namespace workphone::physics
{
    /**
     * @class WPPhysicsScene3
     * @brief Concrete implementation of the PhysicsScene3 interface.
     *
     * This class manages the physics simulation environment, including actors,
     * gravity, and raycasting tests.
     */
    class WPPhysicsScene3 : public PhysicsScene3
    {
    public:
        WPPhysicsScene3();
        ~WPPhysicsScene3() override;

        /** @brief Updates the physics scene state. */
        void update() override;

        /** @brief Clears all actors and state from the physics scene. */
        void clear() override;

        /** @brief Adds a physics body to the scene. */
        void addActor(SmartPtr<IPhysicsBody3> body) override;

        /** @brief Removes a physics body from the scene. */
        void removeActor(SmartPtr<IPhysicsBody3> body) override;

        /** @brief Retrieves all actors currently in the scene. */
        Array<SmartPtr<IPhysicsBody3>> getActors() const override;

        /** @brief Checks if a specific actor exists in the scene. */
        bool hasActor(SmartPtr<IPhysicsBody3> body) const override;

        /** @brief Returns the number of dynamic actors. */
        u32 numDynamicActors() const override;

        /** @brief Returns the number of static actors. */
        u32 numStaticActors() const override;

        /** @brief Sets the dimensions of the physics scene. */
        void setSize(const Vector3<real_Num> &size) override;

        /** @brief Gets the dimensions of the physics scene. */
        Vector3<real_Num> getSize() const override;
        /**
         * @brief Performs a basic ray test for collisions.
         * @param start Starting position of the ray.
         * @param direction Direction of the ray.
         * @param hitPos Output: Position where the ray hit.
         * @param hitNormal Output: Normal at the hit position.
         * @param collisionType Type of collision to detect.
         * @param collisionMask Mask to filter collision types.
         * @return True if a hit occurred, false otherwise.
         */
        bool rayTest(const Vector3<real_Num> &start, const Vector3<real_Num> &direction,
                     Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal, u32 collisionType = 0,
                     u32 collisionMask = 0) override;
        /**
         * @brief Tests if a segment intersects any object in the scene.
         * @param start Start of the segment.
         * @param end End of the segment.
         * @param hitPos Output: Hit position.
         * @param hitNormal Output: Hit normal.
         * @param object Output: The object that was intersected.
         * @param collisionType Type of collision to detect.
         * @param collisionMask Mask to filter collision types.
         * @return True if intersection occurred.
         */
        bool intersects(const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                        Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal,
                        SmartPtr<ISharedObject> &object, u32 collisionType = 0,
                        u32 collisionMask = 0) override;

        /** @brief Casts a ray and collects all hits. */
        bool castRay(const Vector3<real_Num> &origin, const Vector3<real_Num> &dir,
                     Array<SmartPtr<IRaycastHit>> &hits) override;

        /** @brief Casts a ray and returns the first hit. */
        bool castRay(const Ray3<real_Num> &ray, SmartPtr<IRaycastHit> hit) override;

        /** @brief Casts a ray specifically against dynamic actors. */
        bool castRayDynamic(const Ray3<real_Num> &ray, SmartPtr<IRaycastHit> hit) override;

        /** @brief Sets the gravity vector for the scene. */
        void setGravity(const Vector3<real_Num> &vec) override;

        /** @brief Gets the current gravity vector. */
        Vector3<real_Num> getGravity() const override;

        /** @brief Steps the physics simulation. */
        void simulate(real_Num elapsedTime, void *scratchMemBlock, u32 scratchMemBlockSize,
                      bool controlSimulation) override;

        /** @brief Fetches results from the physics engine. */
        bool fetchResults(bool block, u32 *errorState) override;

        /** @brief Gets the minimum number of threads used for simulation. */
        u32 getMinThreads() const override;

        /** @brief Sets the minimum number of threads. */
        void setMinThreads(u32 minThreads) override;

        /** @brief Gets the maximum number of threads used for simulation. */
        u32 getMaxThreads() const override;

        /** @brief Sets the maximum number of threads. */
        void setMaxThreads(u32 maxThreads) override;

        /** @brief Returns the underlying low-level physics scene handle. */
        wp_physics_scene *getScene() const;

        /** @brief Sets the spatial partitioning method. */
        void setSpatialPartitioning(SpatialPartitioningMethodEnum method) override;

        /** @brief Gets the spatial partitioning method. */
        SpatialPartitioningMethodEnum getSpatialPartitioning() const override;

        /** @brief Sets the spatial partitioning options. */
        void setSpatialOptions(const SpatialPartitioningOptions &options) override;

        /** @brief Gets the spatial partitioning options. */
        SpatialPartitioningOptions getSpatialOptions() const override;

        /** @brief Sets the contact options. */
        void setContactOptions(const ContactOptions &options) override;

        /** @brief Gets the contact options. */
        ContactOptions getContactOptions() const override;

    private:
        /** @brief Publishes updated transforms of active actors back to the engine. */
        void publishActiveTransforms();

        wp_physics_scene *m_scene = nullptr; ///< Handle to the internal physics scene.

        ConcurrentArray<SmartPtr<IPhysicsBody3>> m_actors; ///< List of actors managed by this scene.

        std::unordered_map<const IPhysicsBody3 *, Transform3<real_Num>>
        m_lastPublishedTransforms; ///< Cache of transforms to avoid redundant updates.
    };
} // namespace workphone::physics

#endif
