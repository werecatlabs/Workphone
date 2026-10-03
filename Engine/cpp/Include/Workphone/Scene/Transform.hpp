#ifndef __WP_Transformation_h__
#define __WP_Transformation_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Scene/TransformSystem.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @class Transform
         * @brief Compatibility facade for a data-oriented 3D transform.
         *
         * @details
         * Transform keeps the established ITransform API used by actors, scripting and
         * editor code, but owns no transform values itself. Its handle addresses a slot
         * in TransformSystem, where local/world position, orientation and scale are held
         * in contiguous structure-of-arrays storage.
         *
         * Responsibilities:
         * - Store and expose local and world transforms (position, orientation, scale).
         * - Convert/update between local and world transforms when parent changes.
         * - Maintain per-frame timing information for interpolation and smooth motion.
         * - Provide common utilities (lookAt, direction vectors).
         *
         * Thread-safety is provided by TransformSystem around the shared arrays.
         *
         * Usage:
         * - Call `setLocalDirty` / `setDirty` to mark transforms that need recalculation.
         * - Call `updateTransform()` once per frame (or when needed) to propagate
         *   and recompute cached world transforms.
         *
         * @see workphone::scene::ITransform, workphone::scene::IActor, Transform3
         */
        class WPCore_API Transform : public ITransform
        {
        public:
            /**
             * @brief Hash identifier for the position property name ("TransformationPosition").
             *
             * Value is defined in the implementation (.cpp) file. Used for fast
             * property lookups, serialization and editor integration.
             */
            static const hash_type TRANSFORMATION_POSITION_HASH;

            /**
             * @brief Default constructor.
             *
             * Initializes local and world transforms to identity, resets timing and flags,
             * and sets the transform to enabled by default. No actor association is set.
             */
            Transform();

            /**
             * @brief Deleted copy constructor.
             *
             * Copying is disallowed because the instance contains runtime state such as
             * weak actor references, atomics and mutexes that must not be duplicated.
             */
            Transform( const Transform &other ) = delete;

            /**
             * @brief Virtual destructor.
             *
             * Ensures derived classes clean up correctly. Follows the engine life-cycle
             * contract and will call `unload` as necessary.
             */
            ~Transform() override;

            /**
             * @copydoc ITransform::load
             * @param data Optional data used to initialize state (format is implementation-specific).
             *
             * @note Loading may set properties and transforms from serialized data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ITransform::unload
             * @param data Optional data provided to assist unloading.
             *
             * @note Unload should release resources and detach from the actor when required.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ITransform::updateTransform
             *
             * @details Recomputes the world channels in TransformSystem when this transform
             * is marked dirty. The calculation uses the local channels and parent world
             * transform, if present.
             */
            void update() override;

            /**
             * @copydoc ITransform::parentChanged
             * @param newParent The new parent actor (may be null).
             * @param oldParent The previous parent actor (may be null).
             *
             * @details Handles bookkeeping when the owning actor's parent changes.
             * Converts between local and world transforms as required to preserve
             * spatial relationship or to adopt new parent-relative space.
             */
            void parentChanged( SmartPtr<IGameActor> newParent,
                                SmartPtr<IGameActor> oldParent ) override;

            /**
             * @copydoc ITransform::getActorPtr
             * @return Raw pointer to the associated actor or nullptr if none.
             *
             * @warning The returned raw pointer is not reference-counted by this call.
             * Use `getActor()` for ownership-safe access.
             */
            IGameActor *getActorPtr() const override;

            /**
             * @copydoc ITransform::getActor
             * @return Smart pointer to the associated actor. May be null.
             */
            SmartPtr<IGameActor> getActor() const override;

            /**
             * @copydoc ITransform::setActor
             * @param actor Actor to associate with this transform.
             *
             * @details The actor association is stored as a weak pointer to avoid
             * circular ownership between actor and its components.
             */
            void setActor( SmartPtr<IGameActor> actor ) override;

            /**
             * @copydoc ITransform::isLocalDirty
             * @return True if the local transform has been modified and requires update.
             */
            bool isLocalDirty() const override;

            /**
             * @copydoc ITransform::setLocalDirty
             * @param localDirty When true, marks the local transform dirty.
             * @param cascade When true (default), mark descendant world transforms dirty.
             *
             * @details Call this when position, orientation or scale in world space change.
             * Propagation to children ensures dependent world transforms will be recomputed.
             */
            void setLocalDirty( bool localDirty, bool cascade = true ) override;

            /**
             * @copydoc ITransform::isDirty
             * @return True if the world transform is dirty and requires updating.
             */
            bool isDirty() const override;

            /**
             * @copydoc ITransform::setDirty
             * @param dirty When true, marks the world transform dirty.
             * @param cascade When true (default), propagate the dirty state to children.
             *
             * @details Mark the world-space transform for recomputation, typically used
             * when the parent transform changed or local transform was modified.
             */
            void setDirty( bool dirty, bool cascade = true ) override;

            /**
             * @brief Enable or disable this transform.
             * @param enabled True to enable transform updates and effects; false to disable.
             *
             * @note Disabling a transform may cause some systems (rendering, physics)
             * to ignore it depending on integration.
             */
            void setEnabled( bool enabled );

            /**
             * @brief Query whether this transform is enabled.
             * @return True if enabled, false otherwise.
             */
            bool isEnabled() const;

            /**
             * @copydoc ITransform::getLocalPosition
             * @return The local-space position vector.
             */
            Vector3<real_Num> getLocalPosition() const override;

            /**
             * @copydoc ITransform::setLocalPosition
             * @param localPosition The new local-space position to set.
             *
             * @details Marks the local transform as dirty and triggers propagation if required.
             */
            void setLocalPosition( const Vector3<real_Num> &localPosition ) override;

            /**
             * @copydoc ITransform::getLocalScale
             * @return The local-space scale vector.
             */
            Vector3<real_Num> getLocalScale() const override;

            /**
             * @copydoc ITransform::setLocalScale
             * @param localScale The new local-space scale to set (non-uniform supported).
             *
             * @details Setting the scale will flag transforms dirty to ensure world-space
             * values are recomputed.
             */
            void setLocalScale( const Vector3<real_Num> &localScale ) override;

            /**
             * @copydoc ITransform::getLocalOrientation
             * @return The local-space orientation as a quaternion.
             */
            Quaternion<real_Num> getLocalOrientation() const override;

            /**
             * @copydoc ITransform::setLocalOrientation
             * @param localOrientation The new local-space orientation quaternion.
             *
             * @note Quaternions should be normalized for predictable results.
             * Setting orientation marks the local transform dirty.
             */
            void setLocalOrientation( const Quaternion<real_Num> &localOrientation ) override;

            /**
             * @copydoc ITransform::getLocalRotation
             * @return Local rotation expressed as Euler angles (degrees).
             *
             * @note Euler angles are provided for convenience; the internal storage
             * uses quaternions.
             */
            Vector3<real_Num> getLocalRotation() const override;

            /**
             * @copydoc ITransform::setLocalRotation
             * @param localRotation Euler angles in degrees for the local rotation.
             *
             * @details Convenience method that updates the underlying quaternion representation.
             */
            void setLocalRotation( const Vector3<real_Num> &localRotation ) override;

            /**
             * @copydoc ITransform::getPosition
             * @return World-space position vector.
             */
            Vector3<real_Num> getPosition() const override;

            /**
             * @copydoc ITransform::setPosition
             * @param position New world-space position.
             *
             * @details Setting the world position will compute a corresponding local
             * transform when a parent exists so that spatial relationship is preserved.
             */
            void setPosition( const Vector3<real_Num> &position ) override;

            /**
             * @copydoc ITransform::getScale
             * @return World-space scale vector.
             */
            Vector3<real_Num> getScale() const override;

            /**
             * @copydoc ITransform::setScale
             * @param scale New world-space scale vector.
             */
            void setScale( const Vector3<real_Num> &scale ) override;

            /**
             * @copydoc ITransform::getOrientation
             * @return World-space orientation quaternion.
             */
            Quaternion<real_Num> getOrientation() const override;

            /**
             * @copydoc ITransform::setOrientation
             * @param orientation New world-space orientation quaternion.
             */
            void setOrientation( const Quaternion<real_Num> &orientation ) override;

            /**
             * @copydoc ITransform::getRotation
             * @return World-space rotation expressed as Euler angles (degrees).
             */
            Vector3<real_Num> getRotation() const override;

            /**
             * @copydoc ITransform::setRotation
             * @param rotation Euler angles in degrees for the world rotation.
             */
            void setRotation( const Vector3<real_Num> &rotation ) override;

            /**
             * @copydoc ITransform::getProperties
             * @return Smart pointer to a `Properties` object containing metadata for this transform.
             *
             * @note Properties are commonly used for editor metadata and serialization.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc ITransform::setProperties
             * @param properties Properties object to assign to this transform.
             *
             * @details Replaces current properties used for serialization, editor metadata
             * or custom per-transform values.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @copydoc ITransform::getLocalTransform
             * @return Local transform (position, orientation, scale).
             */
            Transform3<real_Num> getLocalTransform() const override;

            /**
             * @copydoc ITransform::setLocalTransform
             * @param transform The complete local transform to set.
             *
             * @details Replaces the local `Transform3` and marks local/world state dirty as required.
             */
            void setLocalTransform( Transform3<real_Num> transform ) override;

            /**
             * @copydoc ITransform::getWorldTransform
             * @return World transform (position, orientation, scale).
             */
            Transform3<real_Num> getWorldTransform() const override;

            /**
             * @copydoc ITransform::setWorldTransform
             * @param transform The complete world transform to set.
             *
             * @details When a parent exists this will recompute the local transform
             * so that the world transform is preserved relative to the parent.
             */
            void setWorldTransform( Transform3<real_Num> transform ) override;

            /**
             * @copydoc ITransform::updateWorldFromLocal
             *
             * @details Compute the world transform from the stored local transform using
             * the parent's world transform (if present). Only the stored world transform
             * and related flags are modified.
             */
            void updateWorldFromLocal() override;

            /**
             * @copydoc ITransform::updateLocalFromWorld
             *
             * @details Compute the local transform from the stored world transform and
             * the parent's world transform. Useful when world transform is assigned
             * externally and a corresponding local transform is required.
             */
            void updateLocalFromWorld() override;

            /**
             * @copydoc ITransform::getFrameTime
             * @return Timestamp (seconds) when this transform was last updated.
             *
             * @note This value is stored in an atomic for safe concurrent reads.
             */
            time_interval getFrameTime() const override;

            /**
             * @copydoc ITransform::setFrameTime
             * @param frameTime Timestamp (seconds) to assign as last update time.
             */
            void setFrameTime( time_interval frameTime ) override;

            /**
             * @copydoc ITransform::getFrameDeltaTime
             * @return Time elapsed (seconds) since the last update.
             */
            time_interval getFrameDeltaTime() const override;

            /**
             * @copydoc ITransform::setFrameDeltaTime
             * @param frameDeltaTime Delta time (seconds) to set.
             *
             * @details These timing fields are commonly used to interpolate transforms
             * for smooth motion between physics and render updates.
             */
            void setFrameDeltaTime( time_interval frameDeltaTime ) override;

            /**
             * @copydoc ITransform::getSmoothMotion
             * @return True if smooth motion (interpolation) is enabled for this transform.
             */
            bool getSmoothMotion() const override;

            /**
             * @copydoc ITransform::setSmoothMotion
             * @param smoothMotion True to enable interpolation-based smoothing.
             */
            void setSmoothMotion( bool smoothMotion ) override;

            /**
             * @copydoc ITransform::getTask
             * @return Thread task identifier associated with this transform.
             */
            TaskId getTask() const override;

            /**
             * @copydoc ITransform::setTask
             * @param task Thread task identifier to associate with this transform.
             */
            void setTask( TaskId task ) override;

            /**
             * @brief Rotate the transform so its forward axis faces a world position.
             * @param position World-space target position to look at.
             *
             * @details Uses this transform's current world position as the eye point.
             * The resulting world orientation will align the forward direction with the target.
             */
            void lookAt( const Vector3<real_Num> &position ) override;

            /**
             * @brief Rotate the transform to look at a world position using a specific yaw axis.
             * @param position World-space target position.
             * @param yawAxis Axis used to constrain the yaw calculation (usually up).
             *
             * @details Constraining by `yawAxis` helps avoid unwanted roll and maintain
             * a consistent up-direction.
             */
            void lookAt( const Vector3<real_Num> &position, const Vector3<real_Num> &yawAxis ) override;

            /**
             * @copydoc ITransform::getForward
             * @return Forward direction vector in world space.
             */
            Vector3<real_Num> getForward() const override;

            /**
             * @copydoc ITransform::getUp
             * @return Up direction vector in world space.
             */
            Vector3<real_Num> getUp() const override;

            /**
             * @copydoc ITransform::getRight
             * @return Right direction vector in world space.
             */
            Vector3<real_Num> getRight() const override;

            /**
             * @copydoc ITransform::getFlags
             */
            u8 getFlags() const override;

            /**
             * @copydoc ITransform::setFlags
             */
            void setFlags( u8 flags ) override;

            void addTransformReference() override;

            void removeTransformReference() override;

            s32 getTransformReferences() const override;

            void setTransformReferences( s32 transformReferences ) override;

            /**
             * @brief Registers the class for runtime type information and reflection.
             *
             * @note Macro implemented by the engine's reflection/RTTI system.
             */
            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Update internal frame time and delta time from the global timer.
             *
             * @details Internal helper that refreshes this slot's timing channels in
             * TransformSystem. Called before per-frame transform updates.
             */
            void updateFrameTime();

            /** Stable address of this facade's data inside TransformSystem. */
            TransformSystem::Handle m_handle;

            /**
             * @brief Static extension identifier used to provide unique IDs for transforms.
             *
             * @details Implementation detail used for registration/serialization systems.
             */
            static u32 m_idExt;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // Transformation_h__
