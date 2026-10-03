#ifndef __AnimationInterface_h__
#define __AnimationInterface_h__

#include <Workphone/Interface/Animation/IAnimationInterface.hpp>
#include <Workphone/Interface/Animation/IAnimation.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Map.hpp>

namespace workphone
{
    /**
     * @brief The AnimationInterface class.
     *
     * This class manages a collection of animations, providing functionality to create,
     * retrieve, and remove animations by name or index. It serves as the concrete
     * implementation of the IAnimationInterface.
     */
    class WPCore_API AnimationInterface : public IAnimationInterface
    {
    public:
        /**
         * @brief Default constructor for AnimationInterface.
         */
        AnimationInterface();

        /**
         * @brief Destructor for AnimationInterface.
         */
        ~AnimationInterface() override;

        /**
         * @brief Creates a new animation and adds it to the interface management.
         * @param name The unique name to assign to the animation.
         * @param length The duration of the animation in seconds.
         * @return A smart pointer to the newly created IAnimation.
         */
        SmartPtr<IAnimation> createAnimation( const String &name, f32 length ) override;

        /**
         * @brief Retrieves an animation by its name.
         * @param name The name of the animation to find.
         * @return A smart pointer to the IAnimation if found, otherwise nullptr.
         */
        SmartPtr<IAnimation> getAnimation( const String &name ) const override;

        /**
         * @brief Checks if an animation with the given name exists.
         * @param name The name of the animation to check.
         * @return True if the animation exists, false otherwise.
         */
        bool hasAnimation( const String &name ) const override;

        /**
         * @brief Removes an animation from the interface by its name.
         * @param name The name of the animation to remove.
         */
        void removeAnimation( const String &name ) override;

        /**
         * @brief Gets the total number of animations currently managed.
         * @return The count of animations.
         */
        u16 getNumAnimations() const override;

        /**
         * @brief Retrieves an animation by its numerical index.
         * @param index The index of the animation in the collection.
         * @return A smart pointer to the IAnimation at the specified index.
         */
        SmartPtr<IAnimation> getAnimation( u16 index ) const override;

        /**
         * @brief Retrieves raw vertex data associated with a specific track handle.
         * @param handle The handle of the animation track.
         * @return A void pointer to the vertex data.
         */
        void *getVertexDataByTrackHandle( u16 handle ) override;

        /**
         * @brief Creates a deep copy of the current AnimationInterface.
         * @return A smart pointer to the cloned IAnimationInterface.
         */
        SmartPtr<IAnimationInterface> clone() override;

        WP_CLASS_REGISTER_DECL;

    private:
        /**
         * @brief Map storing animations associated with their unique names.
         */
        Map<String, SmartPtr<IAnimation>> m_animations;

        /**
         * @brief Array maintaining the ordered list of animation names for index-based access.
         */
        Array<String> m_animationNames;
    };
}  // namespace workphone

#endif  // __AnimationInterface_h__
