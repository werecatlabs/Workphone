#ifndef Bone_h__
#define Bone_h__

#include <Workphone/Interface/Mesh/IGraphicsBone.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Memory/WeakPtr.hpp>

namespace workphone
{
    /**
     * @class Bone
     * @brief Represents a bone in a skeletal structure.
     *
     * This class implements the IBone interface and provides functionality
     * for managing bone properties and transformations in a hierarchical
     * bone structure. Each bone can have children and maintains transform
     * information for skeletal animation.
     */
    class Bone : public IBone
    {
    public:
        /**
         * @brief Default constructor.
         * Initializes the bone with default values.
         */
        Bone();

        /**
         * @brief Destructor.
         * Cleans up resources and removes references to parent and children.
         */
        ~Bone() override;

        /**
         * @brief Creates a child bone with specified transform.
         * @param handle Unique identifier for the child bone
         * @param translate Initial position offset from parent
         * @param rotate Initial rotation from parent
         * @return Smart pointer to the created child bone
         */
        SmartPtr<IBone> createChild(
            u16 handle, const Vector3<real_Num> &translate = Vector3<real_Num>::zero(),
            const Quaternion<real_Num> &rotate = Quaternion<real_Num>::identity() ) override;

        /**
         * @brief Sets the current position/orientation as the binding pose.
         * The binding pose is the reference pose used for mesh binding.
         */
        void setBindingPose() override;

        /**
         * @brief Resets the bone to its binding pose.
         * Restores position and orientation to the stored binding values.
         */
        void reset() override;

        /**
         * @brief Sets whether this bone is manually controlled.
         * @param manuallyControlled True if bone should be manually controlled
         */
        void setManuallyControlled( bool manuallyControlled ) override;

        /**
         * @brief Gets the manual control state of the bone.
         * @return True if bone is manually controlled, false if driven by animation
         */
        bool isManuallyControlled() const override;

        /**
         * @brief Gets the current position of the bone.
         * @return Current position vector
         */
        Vector3<real_Num> getPosition() const override;

        /**
         * @brief Sets the current position of the bone.
         * @param position New position vector
         */
        void setPosition( const Vector3<real_Num> &position ) override;

        /**
         * @brief Gets the current orientation of the bone.
         * @return Current orientation quaternion
         */
        Quaternion<real_Num> getOrientation() const override;

        /**
         * @brief Sets the current orientation of the bone.
         * @param orientation New orientation quaternion
         */
        void setOrientation( const Quaternion<real_Num> &orientation ) override;

        /**
         * @brief Gets the unique handle/ID of the bone.
         * @return Bone handle
         */
        u16 getBoneHandle() const override;

        /**
         * @brief Sets the unique handle/ID of the bone.
         * @param handle New bone handle
         */
        void setBoneHandle( u16 handle ) override;

        /**
         * @brief Gets the parent bone.
         * @return Smart pointer to parent bone, or null if this is a root bone
         */
        SmartPtr<IBone> getParent() const override;

        /**
         * @brief Gets all child bones.
         * @return Array of smart pointers to child bones
         */
        Array<SmartPtr<IBone>> getChildren() const override;

        WP_CLASS_REGISTER_DECL;

    protected:
        //! Current position of the bone
        Vector3<real_Num> m_position;

        //! Current orientation of the bone
        Quaternion<real_Num> m_orientation;

        //! Initial position when bone was created
        Vector3<real_Num> m_initialPosition;

        //! Initial orientation when bone was created
        Quaternion<real_Num> m_initialOrientation;

        //! Binding pose position (reference pose for mesh binding)
        Vector3<real_Num> m_bindingPosition;

        //! Binding pose orientation (reference pose for mesh binding)
        Quaternion<real_Num> m_bindingOrientation;

        //! Weak reference to parent bone to avoid circular references
        WeakPtr<Bone> m_parent;

        //! Unique identifier for this bone
        u16 m_iHandle;

        //! Flag indicating if this bone is manually controlled
        bool m_manuallyControlled;

        //! Collection of child bones
        Array<SmartPtr<IBone>> m_children;
    };
}  // namespace workphone

#endif  // Bone_h__
