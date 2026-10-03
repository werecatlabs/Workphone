#ifndef __IBone_h__
#define __IBone_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone
{

    /**
     * @brief Interface representing a single skeletal bone.
     *
     * An `IBone` represents a node in a skeletal hierarchy used for skinning
     * and animation. Implementations store position and orientation relative
     * to the bone's parent and maintain binding-pose information. Ownership
     * of bones is expressed using `SmartPtr<IBone>`; parent/child relationships
     * are returned as smart pointers to make lifetime management explicit.
     */
    class IBone : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IBone() override;

        /**
         * @brief Get the current local position of the bone.
         *
         * The returned position is in the bone's local space (relative to its parent).
         *
         * @return Local translation as a `Vector3<real_Num>`.
         */
        virtual Vector3<real_Num> getPosition() const = 0;

        /**
         * @brief Set the current local position of the bone.
         *
         * Setting the position updates the bone's transform relative to its parent.
         * Implementations should mark the bone as changed so that any cached world
         * transforms are updated as needed.
         *
         * @param position New local translation as a `Vector3<real_Num>`.
         */
        virtual void setPosition( const Vector3<real_Num> &position ) = 0;

        /**
         * @brief Get the current local orientation of the bone.
         *
         * The returned orientation is in the bone's local space (relative to its parent).
         *
         * @return Local orientation as a `Quaternion<real_Num>`.
         */
        virtual Quaternion<real_Num> getOrientation() const = 0;

        /**
         * @brief Set the current local orientation of the bone.
         *
         * Setting the orientation updates the bone's rotation relative to its parent.
         * Implementations should mark dependent transforms as dirty so they are recomputed.
         *
         * @param orientation New local orientation as a `Quaternion<real_Num>`.
         */
        virtual void setOrientation( const Quaternion<real_Num> &orientation ) = 0;

        /**
         * @brief Gets the unique handle/ID of the bone.
         *
         * The handle is an implementation-defined identifier (typically an index)
         * used to reference the bone within a skeleton or animation system.
         *
         * @return Bone handle as `u16`.
         */
        virtual u16 getBoneHandle() const = 0;

        /**
         * @brief Sets the unique handle/ID of the bone.
         *
         * Use this to assign or change the bone's identifier. Implementations may
         * validate uniqueness when used within a skeleton.
         *
         * @param handle New bone handle (u16).
         */
        virtual void setBoneHandle( u16 handle ) = 0;

        /**
         * @brief Get the parent of this bone.
         *
         * If this bone is a root bone the returned pointer may be null.
         * The returned `SmartPtr` does not imply duplication of ownership beyond
         * standard reference-counting semantics of `SmartPtr`.
         *
         * @return `SmartPtr<IBone>` to the parent bone, or a null smart pointer
         *         if this is a root bone.
         */
        virtual SmartPtr<IBone> getParent() const = 0;

        /**
         * @brief Get direct child bones of this bone.
         *
         * The returned array contains `SmartPtr<IBone>` entries for each direct child.
         * The order of children is implementation-defined (often insertion order).
         *
         * @return `Array<SmartPtr<IBone>>` containing direct child bones. May be empty.
         */
        virtual Array<SmartPtr<IBone>> getChildren() const = 0;

        /**
         * @brief Create and attach a new child bone to this bone.
         *
         * The new child's local transform is provided by `translate` and `rotate`.
         * The created bone is owned via a `SmartPtr` and will have this bone set
         * as its parent.
         *
         * @param handle  Unique handle/ID to assign to the new child bone.
         * @param translate Local translation for the child (default: zero vector).
         * @param rotate    Local orientation for the child (default: identity quaternion).
         * @return `SmartPtr<IBone>` referencing the newly created child bone.
         */
        virtual SmartPtr<IBone> createChild(
            u16 handle, const Vector3<real_Num> &translate = Vector3<real_Num>::zero(),
            const Quaternion<real_Num> &rotate = Quaternion<real_Num>::identity() ) = 0;

        /**
         * @brief Set the current transform as the binding pose.
         *
         * The binding pose is the reference pose in which the mesh was originally
         * bound to the skeleton. Calling this updates whatever internal storage
         * holds the bind transform (position and orientation) so subsequent
         * calls to `reset()` restore to this pose.
         */
        virtual void setBindingPose() = 0;

        /**
         * @brief Reset the bone to its stored binding pose.
         *
         * Restores the bone's position and orientation to the values previously
         * saved by `setBindingPose()`. Implementations should also propagate
         * resets to child bones if appropriate.
         */
        virtual void reset() = 0;

        /**
         * @brief Mark the bone as manually controlled or not.
         *
         * When a bone is manually controlled it is typically excluded from automatic
         * animation updates so that external systems (e.g., IK solvers, physics)
         * can drive it instead.
         *
         * @param manuallyControlled True to mark the bone as manually controlled,
         *                           false to allow automatic updates.
         */
        virtual void setManuallyControlled( bool manuallyControlled ) = 0;

        /**
         * @brief Query whether the bone is currently manually controlled.
         *
         * @return True if the bone is flagged as manually controlled, false otherwise.
         */
        virtual bool isManuallyControlled() const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IBone_h__
