#ifndef IAnimationContainer_h__
#define IAnimationContainer_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @class IAnimationContainer
     * @brief Interface for an object that owns and manages a collection of animations.
     *
     * IAnimationContainer provides an abstract API to query, create and remove
     * animations associated with the implementing object. Implementations are
     * responsible for the lifetime of animations they own (for example, animations
     * returned from `createAnimation` are owned by the container and callers must
     * not delete them). Accessors return raw pointers for compatibility with the
     * existing codebase; they may return `nullptr` when a requested animation is
     * not found (or when an index is out of range).
     *
     * @note Thread-safety is implementation-defined. If multiple threads may
     * access or modify the animation container concurrently, the concrete
     * implementation should document and provide the required synchronization.
     *
     * @see IAnimation
     */
    class WPCore_API IAnimationContainer : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures derived destructors are called correctly when an object is
         * deleted through a pointer to this interface.
         */
        ~IAnimationContainer() override;

        /**
         * @brief Get the number of animations contained.
         *
         * @return The number of animations currently stored in the container.
         *
         * This count reflects animations that are considered part of the
         * container's owned set. Implementations should return a stable
         * value for the lifetime of a `const` object, but may change when
         * animations are added or removed.
         */
        virtual u16 getNumAnimations() const = 0;

        /**
         * @brief Retrieve an animation by index.
         *
         * @param index Zero-based index of the animation to retrieve.
         * @return Pointer to the `IAnimation` at the specified index, or `nullptr`
         *         if `index` is out of range.
         *
         * The returned pointer is owned by the container; callers must not
         * delete it. Prefer `hasAnimation` / name lookup for stable lookup
         * semantics when index ordering is not known.
         */
        virtual IAnimation *getAnimation( u16 index ) const = 0;

        /**
         * @brief Retrieve an animation by name.
         *
         * @param name The name of the animation to retrieve.
         * @return Pointer to the named `IAnimation`, or `nullptr` if no animation
         *         with the given name exists.
         *
         * Name comparison semantics (case sensitivity, trimming, etc.) are
         * implementation-defined and should be documented by concrete classes.
         * The returned pointer is owned by the container and must not be deleted
         * by the caller.
         */
        virtual IAnimation *getAnimation( const String &name ) const = 0;

        /**
         * @brief Create (or allocate) a new animation owned by this container.
         *
         * @param name   The name to assign to the new animation. Implementations
         *               may enforce unique names; if a name collision occurs the
         *               behaviour is implementation-defined (may replace, fail,
         *               or create with a modified name).
         * @param length The duration/length of the animation in seconds (or the
         *               coordinate system used by the engine). Use `f32` for
         *               compatibility with existing code.
         * @return Pointer to the newly created `IAnimation`, or `nullptr` on
         *         failure.
         *
         * The returned animation is owned by the container. The container must
         * manage the lifetime and cleanup of the created animation.
         */
        virtual IAnimation *createAnimation( const String &name, f32 length ) = 0;

        /**
         * @brief Check whether the container contains an animation with the given name.
         *
         * @param name The animation name to look up.
         * @return `true` if an animation with `name` exists in this container,
         *         otherwise `false`.
         *
         * This provides a fast existence check before calling `getAnimation`
         * when that behaviour is preferred.
         */
        virtual bool hasAnimation( const String &name ) const = 0;

        /**
         * @brief Remove an animation by name from this container.
         *
         * @param name The name of the animation to remove.
         *
         * If the named animation exists it will be removed and its resources
         * released by the container. If no animation with the specified name
         * exists this function should be a no-op. Behaviour for concurrent
         * callers is implementation-defined.
         */
        virtual void removeAnimation( const String &name ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IAnimationContainer_h__
