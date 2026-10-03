#ifndef IMaterialNodeAnimatedTexture_h__
#define IMaterialNodeAnimatedTexture_h__

#include <Workphone/Interface/Graphics/IMaterialNode.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Interface for a material node that cycles through a sequence of textures over time.
         *
         * Derived classes implement an animated texture effect by advancing through an ordered
         * list of textures driven by a time value and an optional @c IAnimator.  The node
         * also exposes the shader used to render the animation and a per-node scale transform.
         *
         * @see IMaterialNode
         * @see IAnimator
         * @see ITexture
         */
        class WPCore_API IMaterialNodeAnimatedTexture : public IMaterialNode
        {
        public:
            /** Virtual destructor. */
            ~IMaterialNodeAnimatedTexture() override;

            /**
             * @brief Gets the name of the texture at the specified index.
             * @param index Zero-based index of the texture in the animation sequence.
             * @return The name of the texture at @p index.
             */
            virtual String getTextureName( u32 index ) const = 0;

            /**
             * @brief Sets the name of the texture at the specified index.
             * @param index Zero-based index of the texture in the animation sequence.
             * @param name  The new name to assign to the texture slot.
             */
            virtual void setTextureName( u32 index, const String &name ) = 0;

            /**
             * @brief Gets the name of the shader used to render this animated texture.
             * @return The shader name.
             */
            virtual String getShaderName() const = 0;

            /**
             * @brief Sets the name of the shader used to render this animated texture.
             * @param shaderName The shader name to assign.
             */
            virtual void setShaderName( const String &shaderName ) = 0;

            /**
             * @brief Gets a mutable reference to the texture at the specified index.
             * @param index Zero-based index of the texture in the animation sequence.
             * @return A mutable smart pointer reference to the texture at @p index.
             */
            virtual SmartPtr<ITexture> &getTexture( u32 index ) = 0;

            /**
             * @brief Gets a read-only reference to the texture at the specified index.
             * @param index Zero-based index of the texture in the animation sequence.
             * @return A const smart pointer reference to the texture at @p index.
             */
            virtual const SmartPtr<ITexture> &getTexture( u32 index ) const = 0;

            /**
             * @brief Replaces the texture at the specified index.
             * @param index   Zero-based index of the slot to replace.
             * @param texture The new texture to assign to the slot.
             */
            virtual void setTexture( u32 index, SmartPtr<ITexture> texture ) = 0;

            /**
             * @brief Appends a texture to the end of the animation sequence.
             * @param texture The texture to add.
             */
            virtual void addTexture( SmartPtr<ITexture> texture ) = 0;

            /**
             * @brief Sets the scale transform applied to this animated texture node.
             * @param scale A 3D vector representing the scale along each axis.
             */
            virtual void setScale( const Vector3<real_Num> &scale ) = 0;

            /**
             * @brief Gets a mutable reference to the animator controlling the texture sequence.
             * @return A mutable smart pointer reference to the associated @c IAnimator.
             */
            virtual SmartPtr<IAnimator> &getAnimator() = 0;

            /**
             * @brief Gets a read-only reference to the animator controlling the texture sequence.
             * @return A const smart pointer reference to the associated @c IAnimator.
             */
            virtual const SmartPtr<IAnimator> &getAnimator() const = 0;

            /**
             * @brief Sets the animator that drives the texture animation.
             * @param animator The animator to assign.
             */
            virtual void setAnimator( SmartPtr<IAnimator> animator ) = 0;

            /**
             * @brief Gets the current playback time of the animation.
             * @return The current time position within the animation sequence.
             */
            virtual time_interval getTime() const = 0;

            /**
             * @brief Sets the current playback time of the animation.
             * @param time The time position to seek to within the animation sequence.
             */
            virtual void setTime( time_interval time ) = 0;

            /**
             * @brief Gets the total duration of the animation sequence.
             * @return The total length of the animation in time units.
             */
            virtual time_interval getTotalTime() const = 0;

            /**
             * @brief Sets the total duration of the animation sequence.
             * @param totalTime The total length to assign to the animation sequence.
             */
            virtual void setTotalTime( time_interval totalTime ) = 0;

            /**
             * @brief Retrieves a pointer to the underlying implementation object.
             * @param ppObject Address of a pointer that receives the internal object.
             * @note This method is intended for internal or interop use only.
             */
            virtual void _getObject( void **ppObject ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // IMaterialNodeAnimatedTexture_h__
