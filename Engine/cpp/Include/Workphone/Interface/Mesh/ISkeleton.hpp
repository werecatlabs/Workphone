#ifndef ISkeleton_h__
#define ISkeleton_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Mesh/LinkedSkeletonAnimationSource.hpp>

namespace workphone
{

    /**
     * @class ISkeleton
     * @brief Interface for managing a skeleton structure, which includes bones and animations.
     *
     * This class provides methods to create, retrieve, and manage bones and animations
     * associated with a skeleton. It serves as a base interface for skeleton-related operations
     * in a 3D engine.
     */
    class ISkeleton : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of derived classes.
         */
        ~ISkeleton() override;

        /**
         * @brief Creates a new animation for the skeleton.
         *
         * @param name The name of the animation.
         * @param length The duration of the animation in seconds.
         * @return A smart pointer to the created animation.
         */
        virtual SmartPtr<IAnimation> createAnimation( const String &name, f32 length ) = 0;

        /**
         * @brief Retrieves an animation by name, optionally providing a linker to a linked skeleton.
         *
         * @param name The name of the animation to retrieve.
         * @param linker A pointer to a linked skeleton animation source, if applicable.
         * @return A smart pointer to the requested animation, or nullptr if not found.
         */
        virtual SmartPtr<IAnimation> getAnimation(
            const String &name, const LinkedSkeletonAnimationSource **linker ) const = 0;

        /**
         * @brief Retrieves an animation by name.
         *
         * @param name The name of the animation to retrieve.
         * @return A smart pointer to the requested animation, or nullptr if not found.
         */
        virtual SmartPtr<IAnimation> getAnimation( const String &name ) const = 0;

        /**
         * @brief Checks if an animation with the given name exists.
         *
         * @param name The name of the animation to check.
         * @return True if the animation exists, false otherwise.
         */
        virtual bool hasAnimation( const String &name ) const = 0;

        /**
         * @brief Creates a new bone owned by this skeleton.
         *
         * @return A smart pointer to the created bone.
         */
        virtual SmartPtr<IBone> createBone() = 0;

        /**
         * @brief Creates a new bone with a specific handle owned by this skeleton.
         *
         * @param handle The unique handle for the bone.
         * @return A smart pointer to the created bone.
         */
        virtual SmartPtr<IBone> createBone( u32 handle ) = 0;

        /**
         * @brief Creates a new bone with a specific name owned by this skeleton.
         *
         * @param name The name of the bone.
         * @return A smart pointer to the created bone.
         */
        virtual SmartPtr<IBone> createBone( const String &name ) = 0;

        /**
         * @brief Creates a new bone with a specific name and handle owned by this skeleton.
         *
         * @param name The name of the bone.
         * @param handle The unique handle for the bone.
         * @return A smart pointer to the created bone.
         */
        virtual SmartPtr<IBone> createBone( const String &name, u32 handle ) = 0;

        /**
         * @brief Retrieves a bone by name.
         *
         * @param name The name of the bone to retrieve.
         * @return A smart pointer to the requested bone, or nullptr if not found.
         */
        virtual SmartPtr<IBone> getBone( const String &name ) const = 0;

        /**
         * @brief Checks if a bone with the given name exists.
         *
         * @param name The name of the bone to check.
         * @return True if the bone exists, false otherwise.
         */
        virtual bool hasBone( const String &name ) const = 0;

        /** Gets the animation blending mode which this skeleton will use. */
        virtual SkeletonAnimationBlendMode getBlendMode() const = 0;

        /** Sets the animation blending mode this skeleton will use. */
        virtual void setBlendMode( SkeletonAnimationBlendMode state ) = 0;

        /**
         * @brief Registers the class for runtime type information.
         */
        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // ISkeleton_h__
