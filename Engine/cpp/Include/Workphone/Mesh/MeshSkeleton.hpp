#ifndef __MeshSkeleton_h__
#define __MeshSkeleton_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Mesh/ISkeleton.hpp>
#include <map>

namespace workphone
{
    /**
     * @class MeshSkeleton
     * @brief Represents a skeleton structure for mesh animations.
     *
     * The MeshSkeleton class implements the ISkeleton interface and provides functionality
     * for managing bones and animations in a mesh skeleton system. This class serves as
     * the primary container for skeletal animation data, including bone hierarchies and
     * animation sequences.
     *
     * The skeleton maintains collections of bones and animations, allowing for complex
     * skeletal animation systems with support for linked skeleton animation sources
     * for animation sharing between different skeleton instances.
     *
     * @see ISkeleton
     * @see IBone
     * @see IAnimation
     * @see LinkedSkeletonAnimationSource
     */
    class WPCore_API MeshSkeleton : public ISkeleton
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes an empty skeleton with no bones or animations.
         */
        MeshSkeleton();

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of all bones, animations, and linked sources.
         * Called automatically when the skeleton is destroyed.
         */
        ~MeshSkeleton() override;

        /**
         * @brief Creates a new animation for this skeleton.
         *
         * Creates and registers a new animation with the specified name and duration.
         * The animation will be owned by this skeleton and can be retrieved later
         * using getAnimation().
         *
         * @param name The unique name identifier for the animation. Must not be empty.
         * @param length The duration of the animation in seconds. Must be positive.
         * @return SmartPtr<IAnimation> Pointer to the newly created animation.
         * @retval nullptr If creation fails due to invalid parameters or memory allocation failure.
         *
         * @note If an animation with the same name already exists, behavior is implementation-defined.
         * @see getAnimation()
         * @see hasAnimation()
         */
        SmartPtr<IAnimation> createAnimation( const String &name, f32 length ) override;

        /**
         * @brief Retrieves an animation by name with optional linked skeleton source.
         *
         * Searches for an animation with the specified name in this skeleton and
         * optionally returns information about any linked skeleton animation source
         * that provides the animation.
         *
         * @param name The name of the animation to retrieve.
         * @param linker Output parameter that receives a pointer to the linked skeleton
         *               animation source if the animation comes from a linked skeleton.
         *               Can be nullptr if linker information is not needed.
         * @return SmartPtr<IAnimation> Pointer to the animation if found.
         * @retval nullptr If no animation with the specified name exists.
         *
         * @see getAnimation(const String&) const
         * @see LinkedSkeletonAnimationSource
         */
        SmartPtr<IAnimation> getAnimation( const String &name,
                                           const LinkedSkeletonAnimationSource **linker ) const override;

        /**
         * @brief Retrieves an animation by name.
         *
         * Searches for an animation with the specified name in this skeleton.
         * This is a convenience method that doesn't return linked skeleton information.
         *
         * @param name The name of the animation to retrieve.
         * @return SmartPtr<IAnimation> Pointer to the animation if found.
         * @retval nullptr If no animation with the specified name exists.
         *
         * @see getAnimation(const String&, const LinkedSkeletonAnimationSource**) const
         * @see hasAnimation()
         */
        SmartPtr<IAnimation> getAnimation( const String &name ) const override;

        /**
         * @brief Checks if an animation with the given name exists.
         *
         * Determines whether this skeleton contains an animation with the specified name,
         * either locally or through linked skeleton animation sources.
         *
         * @param name The name of the animation to check for.
         * @return bool True if the animation exists, false otherwise.
         *
         * @note This method is more efficient than calling getAnimation() and checking for nullptr.
         * @see getAnimation()
         */
        bool hasAnimation( const String &name ) const override;

        /**
         * @brief Creates a new bone with an automatically assigned handle.
         *
         * Creates a new bone owned by this skeleton with an automatically generated
         * unique handle identifier.
         *
         * @return SmartPtr<IBone> Pointer to the newly created bone.
         * @retval nullptr If creation fails due to memory allocation failure.
         *
         * @see createBone(u32)
         * @see createBone(const String&)
         * @see createBone(const String&, u32)
         */
        SmartPtr<IBone> createBone() override;

        /**
         * @brief Creates a new bone with a specific handle.
         *
         * Creates a new bone owned by this skeleton with the specified handle identifier.
         * The handle should be unique within this skeleton.
         *
         * @param handle The unique identifier for the bone.
         * @return SmartPtr<IBone> Pointer to the newly created bone.
         * @retval nullptr If creation fails or handle is already in use.
         *
         * @warning Using duplicate handles may result in undefined behavior.
         * @see createBone()
         * @see getBone()
         */
        SmartPtr<IBone> createBone( u32 handle ) override;

        /**
         * @brief Creates a new bone with a specific name.
         *
         * Creates a new bone owned by this skeleton with the specified name.
         * The name should be unique within this skeleton for proper identification.
         *
         * @param name The unique name identifier for the bone. Should not be empty.
         * @return SmartPtr<IBone> Pointer to the newly created bone.
         * @retval nullptr If creation fails due to invalid name or memory allocation failure.
         *
         * @see createBone(const String&, u32)
         * @see getBone()
         * @see hasBone()
         */
        SmartPtr<IBone> createBone( const String &name ) override;

        /**
         * @brief Creates a new bone with both name and handle.
         *
         * Creates a new bone owned by this skeleton with both a specific name and
         * handle identifier. Both should be unique within this skeleton.
         *
         * @param name The unique name identifier for the bone. Should not be empty.
         * @param handle The unique handle identifier for the bone.
         * @return SmartPtr<IBone> Pointer to the newly created bone.
         * @retval nullptr If creation fails due to invalid parameters or conflicts.
         *
         * @warning Using duplicate names or handles may result in undefined behavior.
         * @see createBone()
         * @see createBone(u32)
         * @see createBone(const String&)
         */
        SmartPtr<IBone> createBone( const String &name, u32 handle ) override;

        /**
         * @brief Retrieves a bone by name.
         *
         * Searches for a bone with the specified name in this skeleton's bone hierarchy.
         *
         * @param name The name of the bone to retrieve.
         * @return SmartPtr<IBone> Pointer to the bone if found.
         * @retval nullptr If no bone with the specified name exists.
         *
         * @see hasBone()
         * @see createBone()
         */
        SmartPtr<IBone> getBone( const String &name ) const override;

        /**
         * @brief Checks if a bone with the given name exists.
         *
         * Determines whether this skeleton contains a bone with the specified name
         * in its bone hierarchy.
         *
         * @param name The name of the bone to check for.
         * @return bool True if the bone exists, false otherwise.
         *
         * @note This method is more efficient than calling getBone() and checking for nullptr.
         * @see getBone()
         */
        bool hasBone( const String &name ) const override;

        const Array<SmartPtr<IAnimation>> &getAnimations() const;

        const Array<SmartPtr<IBone>> &getBones() const;

        SkeletonAnimationBlendMode getBlendMode() const override;

        /** Sets the animation blending mode this skeleton will use. */
        void setBlendMode( SkeletonAnimationBlendMode state ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Collection of animations owned by this skeleton.
         *
         * Contains all animations that have been created for this skeleton.
         * Animations are stored as smart pointers to ensure proper memory management.
         */
        Array<SmartPtr<IAnimation>> m_animations;

        /**
         * @brief Collection of bones that form the skeleton hierarchy.
         *
         * Contains all bones that belong to this skeleton, forming the bone hierarchy
         * used for skeletal animation. Bones are stored as smart pointers to ensure
         * proper memory management and enable shared ownership when needed.
         */
        Array<SmartPtr<IBone>> m_bones;

        std::map<String, SmartPtr<IAnimation>> m_animationsByName;

        std::map<String, SmartPtr<IBone>> m_bonesByName;

        /**
         * @brief Linked skeleton animation source for animation sharing.
         *
         * Optional pointer to a linked skeleton animation source that allows this
         * skeleton to share animations from another skeleton. This enables animation
         * reuse across multiple skeleton instances while maintaining separate bone
         * hierarchies.
         *
         * @see LinkedSkeletonAnimationSource
         */
        SmartPtr<LinkedSkeletonAnimationSource> m_linker;

        SkeletonAnimationBlendMode m_blendMode = SkeletonAnimationBlendMode::ANIMBLEND_AVERAGE;
    };
}  // namespace workphone

#endif  // __MeshSkeleton_h__
