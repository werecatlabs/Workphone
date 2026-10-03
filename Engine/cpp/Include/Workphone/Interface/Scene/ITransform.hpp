#ifndef ITransform_h__
#define ITransform_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @brief Interface for an actor transform in the scene.
         *
         * ITransform provides a consistent API for reading and writing an actor's
         * translation, rotation and scale in both local (parent-relative) and world
         * (global) coordinate spaces.
         *
         * Implementations are expected to:
         * - Maintain both local and world transform representations (typically as
         *   decomposed components and composed matrices).
         * - Use dirty flags to delay expensive recomputation and to cascade updates
         *   to child transforms when required.
         * - Optionally support smooth motion interpolation (frame blending) when
         *   enabled.
         *
         * Coordinate space terminology:
         * - Local: transform relative to this actor's parent.
         * - World: transform in global scene space.
         *
         * Common responsibilities of implementations:
         * - Keep local and world transforms in sync when parent changes or when
         *   components are modified.
         * - Provide helper queries such as direction vectors and lookAt helpers.
         * - Provide a minimal threading/task affinity (via TaskId) so that
         *   transform updates can be scheduled/observed in multithreaded systems.
         *
         * @author Zane Desir
         * @version 1.0
         * @since 1.0
         */
        class WPCore_API ITransform : public ISharedObject
        {
        public:
            /**
             * @brief Identifiers describing which region or component of a transform changed.
             *
             * Implementations and callers can use these values to communicate the
             * specific portion of a transform that was modified (for eventing,
             * optimization or incremental updates).
             */
            enum class Type
            {
                None,          /**< No region specified. */
                LocalPosition, /**< Local translation component changed. */
                LocalRotation, /**< Local rotation (Euler) component changed. */
                LocalScale,    /**< Local scale component changed. */
                Position,      /**< World translation component changed. */
                Rotation,      /**< World rotation (Euler) component changed. */
                Scale,         /**< World scale component changed. */

                Count /**< Sentinel: number of enum values. */
            };

            /** Flag bit: local transform requires recomputation. */
            static const u8 localDirtyFlag;
            /** Flag bit: world transform requires recomputation. */
            static const u8 dirtyFlag;
            /** Flag bit: transform (or its actor) is enabled. */
            static const u8 enabledFlag;
            /** Flag bit: smooth motion / interpolation is enabled. */
            static const u8 smoothMotionFlag;

            /** Virtual destructor. */
            ~ITransform() override;

            /**
             * @brief Returns a raw pointer to the actor that owns this transform.
             *
             * This returns a non-owning pointer and may be nullptr if the transform
             * is not attached. Use SmartPtr-based accessors when ownership or
             * reference-counting is required.
             *
             * @return Non-owning pointer to owning actor, or nullptr.
             */
            virtual IGameActor *getActorPtr() const = 0;

            /**
             * @brief Returns a SmartPtr to the actor that owns this transform.
             *
             * Use this to safely retain a reference to the owning actor.
             *
             * @return SmartPtr referencing the owning actor (may be null).
             */
            virtual SmartPtr<IGameActor> getActor() const = 0;

            /**
             * @brief Assigns the actor that owns this transform.
             *
             * Implementations may reparent the transform, recompute local/world
             * state and update any scene registration when the actor is changed.
             *
             * @param actor SmartPtr to the new owning actor (may be null).
             */
            virtual void setActor( SmartPtr<IGameActor> actor ) = 0;

            /**
             * @brief Get the local translation relative to the parent.
             *
             * @return Local position vector.
             */
            virtual Vector3<real_Num> getLocalPosition() const = 0;

            /**
             * @brief Set the local translation relative to the parent.
             *
             * Implementations should mark local/world dirty state and optionally
             * cascade to children so that dependent transforms are updated.
             *
             * @param localPosition New local position vector.
             */
            virtual void setLocalPosition( const Vector3<real_Num> &localPosition ) = 0;

            /**
             * @brief Get the local scale relative to the parent.
             *
             * @return Local scale vector.
             */
            virtual Vector3<real_Num> getLocalScale() const = 0;

            /**
             * @brief Set the local scale relative to the parent.
             *
             * @param localScale New local scale vector.
             */
            virtual void setLocalScale( const Vector3<real_Num> &localScale ) = 0;

            /**
             * @brief Get the local orientation as a quaternion.
             *
             * Quaternions are preferred internally to avoid gimbal lock and enable
             * smooth interpolation (slerp).
             *
             * @return Local orientation quaternion.
             */
            virtual Quaternion<real_Num> getLocalOrientation() const = 0;

            /**
             * @brief Set the local orientation as a quaternion.
             *
             * Implementations should update internal rotation state and mark dirty.
             *
             * @param localOrientation New local orientation quaternion.
             */
            virtual void setLocalOrientation( const Quaternion<real_Num> &localOrientation ) = 0;

            /**
             * @brief Get the local rotation expressed as Euler angles (radians).
             *
             * Euler angles are provided for convenience. Converting to/from
             * quaternion may be required depending on storage.
             *
             * @return Euler rotation vector (pitch, yaw, roll) in radians.
             */
            virtual Vector3<real_Num> getLocalRotation() const = 0;

            /**
             * @brief Set the local rotation using Euler angles (radians).
             *
             * Implementations storing orientation as a quaternion should convert
             * the provided Euler angles into the quaternion representation.
             *
             * @param localRotation Euler angles (radians) representing local rotation.
             */
            virtual void setLocalRotation( const Vector3<real_Num> &localRotation ) = 0;

            /**
             * @brief Get the world-space translation.
             *
             * @return World position vector.
             */
            virtual Vector3<real_Num> getPosition() const = 0;

            /**
             * @brief Set the world-space translation.
             *
             * If this transform is parented, implementations should update local
             * components accordingly so the local/world relationship remains valid.
             *
             * @param position New world position vector.
             */
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;

            /**
             * @brief Get the world-space scale.
             *
             * @return World-space scale vector.
             */
            virtual Vector3<real_Num> getScale() const = 0;

            /**
             * @brief Set the world-space scale.
             *
             * Changing world scale can affect child transforms depending on how
             * scale propagation is implemented.
             *
             * @param scale New world-space scale vector.
             */
            virtual void setScale( const Vector3<real_Num> &scale ) = 0;

            /**
             * @brief Get the world-space orientation as a quaternion.
             *
             * @return World-space orientation quaternion.
             */
            virtual Quaternion<real_Num> getOrientation() const = 0;

            /**
             * @brief Set the world-space orientation as a quaternion.
             *
             * When parented, implementations should update local orientation to
             * maintain the same world orientation.
             *
             * @param orientation New world-space orientation quaternion.
             */
            virtual void setOrientation( const Quaternion<real_Num> &orientation ) = 0;

            /**
             * @brief Get the world-space rotation as Euler angles (radians).
             *
             * @return World-space Euler rotation vector (radians).
             */
            virtual Vector3<real_Num> getRotation() const = 0;

            /**
             * @brief Set the world-space rotation using Euler angles (radians).
             *
             * @param rotation World-space Euler angles (radians).
             */
            virtual void setRotation( const Vector3<real_Num> &rotation ) = 0;

            /**
             * @brief Get the local transform (translation, rotation, scale) as a Transform3.
             *
             * The returned Transform3 represents this transform relative to its parent.
             *
             * @return Local Transform3.
             */
            virtual Transform3<real_Num> getLocalTransform() const = 0;

            /**
             * @brief Set the local transform using a Transform3.
             *
             * Implementations should decompose the transform into translation,
             * rotation and scale and mark the appropriate dirty flags.
             *
             * @param localTransform New local Transform3.
             */
            virtual void setLocalTransform( Transform3<real_Num> localTransform ) = 0;

            /**
             * @brief Get the world transform as a Transform3.
             *
             * @return World-space Transform3.
             */
            virtual Transform3<real_Num> getWorldTransform() const = 0;

            /**
             * @brief Set the world transform using a Transform3.
             *
             * When parented, implementations should update local transform so that
             * composing local with parent's world transform yields the provided worldTransform.
             *
             * @param worldTransform New world Transform3.
             */
            virtual void setWorldTransform( Transform3<real_Num> worldTransform ) = 0;

            /**
             * @brief Recalculate the world transform from the local transform.
             *
             * Should take the parent's world transform into account if present.
             */
            virtual void updateWorldFromLocal() = 0;

            /**
             * @brief Recalculate the local transform from the world transform.
             *
             * Useful when world transform has been modified directly and local
             * components must be updated to reflect the change.
             */
            virtual void updateLocalFromWorld() = 0;

            /**
             * @brief Returns true if the local transform is marked dirty.
             *
             * Clients should call update methods before relying on transform values
             * that may be stale when this returns true.
             *
             * @return True when local transform needs recalculation.
             */
            virtual bool isLocalDirty() const = 0;

            /**
             * @brief Mark or clear the local dirty state.
             *
             * When @p localDirty and @p cascade are true, child world transforms are
             * marked dirty so they follow the parent while retaining their local poses.
             *
             * @param localDirty True to mark local transform dirty; false to clear.
             * @param cascade When true, refresh descendant world transforms.
             */
            virtual void setLocalDirty( bool localDirty, bool cascade = true ) = 0;

            /**
             * @brief Returns true if the world transform is marked dirty.
             *
             * @return True when world transform needs recalculation.
             */
            virtual bool isDirty() const = 0;

            /**
             * @brief Mark or clear the world dirty state.
             *
             * When @p cascade is true, implementations should apply the same state
             * to child transforms to ensure consistent propagation.
             *
             * @param dirty True to mark world transform dirty; false to clear.
             * @param cascade When true, apply the same state to child transforms.
             */
            virtual void setDirty( bool dirty, bool cascade = true ) = 0;

            /**
             * @brief Query whether smooth motion (interpolation) is enabled.
             *
             * When enabled, implementations may interpolate between previous and
             * current transform states to produce smooth visual motion.
             *
             * @return True if smooth motion is enabled.
             */
            virtual bool getSmoothMotion() const = 0;

            /**
             * @brief Enable or disable smooth motion interpolation.
             *
             * @param smoothMotion True to enable smoothing; false to disable.
             */
            virtual void setSmoothMotion( bool smoothMotion ) = 0;

            /**
             * @brief Notification that the parent actor has changed.
             *
             * Implementations should recompute local/world relationships, update
             * scene registration or caches as needed.
             *
             * @param newParent New parent actor (may be null).
             * @param oldParent Previous parent actor (may be null).
             */
            virtual void parentChanged( SmartPtr<IGameActor> newParent,
                                        SmartPtr<IGameActor> oldParent ) = 0;

            /**
             * @brief Get the current frame timestamp used for motion calculations.
             *
             * The scene or update loop typically sets this each frame.
             *
             * @return Current frame time (application-specific units).
             */
            virtual time_interval getFrameTime() const = 0;

            /**
             * @brief Set the current frame timestamp used for motion calculations.
             *
             * @param frameTime Frame time for the current frame.
             */
            virtual void setFrameTime( time_interval frameTime ) = 0;

            /**
             * @brief Get the delta time between frames used for motion calculations.
             *
             * @return Frame delta time.
             */
            virtual time_interval getFrameDeltaTime() const = 0;

            /**
             * @brief Set the delta time between frames used for motion calculations.
             *
             * @param frameDeltaTime Delta time for the current frame.
             */
            virtual void setFrameDeltaTime( time_interval frameDeltaTime ) = 0;

            /**
             * @brief Get the task affinity associated with this transform.
             *
             * This allows systems to know which thread/task context the transform
             * should be updated on or is associated with.
             *
             * @return Task identifier.
             */
            virtual TaskId getTask() const = 0;

            /**
             * @brief Set the task affinity associated with this transform.
             *
             * @param task Task identifier to associate with this transform.
             */
            virtual void setTask( TaskId task ) = 0;

            /**
             * @brief Rotate this transform so the actor looks at the target world position.
             *
             * The implementation should modify orientation so that the actor's
             * forward direction points at @p position. Up-vector constraints should be
             * preserved where applicable.
             *
             * @param position Target in world-space to look at.
             */
            virtual void lookAt( const Vector3<real_Num> &position ) = 0;

            /**
             * @brief Rotate this transform so the actor looks at the target using a yaw axis.
             *
             * The @p yawAxis parameter constrains how the yaw (heading) component is computed,
             * useful for keeping characters upright or using a non-standard up direction.
             *
             * @param position Target in world-space to look at.
             * @param yawAxis Axis to use when computing yaw (typically world up).
             */
            virtual void lookAt( const Vector3<real_Num> &position,
                                 const Vector3<real_Num> &yawAxis ) = 0;

            /**
             * @brief Get the forward direction vector in world-space.
             *
             * Implementations should document which convention is used (local +Z or -Z).
             *
             * @return Normalized forward direction vector.
             */
            virtual Vector3<real_Num> getForward() const = 0;

            /**
             * @brief Get the up direction vector in world-space.
             *
             * @return Normalized up direction vector.
             */
            virtual Vector3<real_Num> getUp() const = 0;

            /**
             * @brief Get the right direction vector in world-space.
             *
             * @return Normalized right direction vector.
             */
            virtual Vector3<real_Num> getRight() const = 0;

            /**
             * @brief Get the current flags bitfield for this transform.
             *
             * The flags include bits such as dirty/localDirty/enabled/smoothMotion.
             *
             * @return Bitfield of u8 flags.
             */
            virtual u8 getFlags() const = 0;

            /**
             * @brief Replace the flags bitfield for this transform.
             *
             * Implementations may choose to validate or mask incoming flags.
             *
             * @param flags New flags bitfield.
             */
            virtual void setFlags( u8 flags ) = 0;

            /**
             * @brief Increments the transform reference count.
             *
             * Used by systems that need to track how many objects are currently
             * dependent on this transform's state.
             */
            virtual void addTransformReference() = 0;

            /**
             * @brief Decrements the transform reference count.
             */
            virtual void removeTransformReference() = 0;

            /**
             * @brief Returns the current number of transform references.
             *
             * @return Number of active references.
             */
            virtual s32 getTransformReferences() const = 0;

            /**
             * @brief Directly sets the transform reference count.
             *
             * @param transformReferences New reference count.
             */
            virtual void setTransformReferences( s32 transformReferences ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // ITransform_h__
