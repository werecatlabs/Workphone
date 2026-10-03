#ifndef Skeleton_h__
#define Skeleton_h__

#include <Workphone/Interface/Graphics/IGraphicsSkeleton.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class GraphicsSkeleton
         * @brief Concrete implementation of a skeletal hierarchy for character animation.
         *
         * The `GraphicsSkeleton` class manages a hierarchical collection of bones used for
         * skeletal animation and mesh skinning. It provides factory methods for creating
         * bones with various configurations (default, named, or with specific handles).
         *
         * Skeletons form the foundation of character animation systems by defining a
         * hierarchical structure of bones that can be animated and to which mesh vertices
         * are bound. Each bone in the skeleton can be transformed independently, and the
         * skeleton maintains the relationships between parent and child bones.
         *
         * @par Thread Safety
         * Instance methods are not thread-safe and should be called from a single thread
         * or with external synchronization.
         *
         * @see IGraphicsSkeleton
         * @see IBone
         * @see SharedGraphicsObject
         */
        class WPCore_API GraphicsSkeleton : public SharedGraphicsObject<IGraphicsSkeleton>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Constructs an empty skeleton with no bones. Bones must be created
             * and added using the `createBone` methods.
             */
            GraphicsSkeleton();

            /**
             * @brief Destructor.
             *
             * Cleans up all bones and associated resources. Any outstanding smart
             * pointers to bones will remain valid due to reference counting, but
             * the bones will be detached from this skeleton.
             */
            ~GraphicsSkeleton() override;

            /**
             * @brief Creates a new bone with an automatically assigned handle.
             *
             * This method creates a new bone instance and automatically assigns it
             * a unique handle. The bone is created with default position (zero) and
             * orientation (identity).
             *
             * @return SmartPtr<IBone> to the newly created bone. Never returns null.
             *
             * @par Exception Safety
             * Strong guarantee. If allocation fails, no state is modified.
             *
             * @see IBone
             */
            SmartPtr<IGraphicsBone> createBone() override;

            /**
             * @brief Creates a new bone with a specified handle.
             *
             * This method creates a bone and assigns it the provided handle/ID.
             * The handle is typically used for indexing into animation data or
             * for fast bone lookup operations.
             *
             * @param handle The unique identifier to assign to the new bone.
             *               Callers should ensure uniqueness within the skeleton.
             *
             * @return SmartPtr<IBone> to the newly created bone. Never returns null.
             *
             * @par Exception Safety
             * Strong guarantee. If allocation fails, no state is modified.
             *
             * @note If a bone with the given handle already exists, behavior is
             *       implementation-defined. Consider checking for duplicates before calling.
             *
             * @see IBone::getBoneHandle
             * @see IBone::setBoneHandle
             */
            SmartPtr<IGraphicsBone> createBone( u32 handle ) override;

            /**
             * @brief Creates a new named bone with an automatically assigned handle.
             *
             * This method creates a bone with a human-readable name, which is useful
             * for debugging, serialization, and runtime bone lookup by name. The handle
             * is assigned automatically.
             *
             * @param name The name to assign to the bone (e.g., "LeftArm", "Spine01").
             *             Names do not need to be unique but unique names are recommended
             *             for clarity.
             *
             * @return SmartPtr<IBone> to the newly created bone. Never returns null.
             *
             * @par Exception Safety
             * Strong guarantee. If allocation fails, no state is modified.
             *
             * @see IBone
             */
            SmartPtr<IGraphicsBone> createBone( const String &name ) override;

            /**
             * @brief Creates a new named bone with a specified handle.
             *
             * This is the most explicit bone creation method, allowing the caller to
             * specify both a human-readable name and a numeric handle. This is useful
             * when loading skeletons from files where both pieces of information are
             * stored explicitly.
             *
             * @param name   The name to assign to the bone (e.g., "RightFoot").
             * @param handle The unique identifier to assign to the bone.
             *
             * @return SmartPtr<IBone> to the newly created bone. Never returns null.
             *
             * @par Exception Safety
             * Strong guarantee. If allocation fails, no state is modified.
             *
             * @note Callers should ensure that the handle is unique within the skeleton
             *       to avoid ambiguous bone references.
             *
             * @see IBone::getBoneHandle
             * @see IBone::setBoneHandle
             */
            SmartPtr<IGraphicsBone> createBone( const String &name, u32 handle ) override;

            /**
             * @brief Registers the class with the runtime type system.
             *
             * This macro expands to declarations needed for runtime type identification
             * and reflection. It enables features like dynamic casting, serialization,
             * and factory-based instantiation.
             */
            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace render
}  // namespace workphone

#endif  // Skeleton_h__
