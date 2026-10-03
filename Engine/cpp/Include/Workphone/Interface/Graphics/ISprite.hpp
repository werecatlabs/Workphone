#ifndef ISprite_h__
#define ISprite_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Matrix4.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * Represents a 2D sprite object.
         */
        class WPCore_API ISprite : public ISharedObject
        {
        public:
            /** Virtual destructor. */
            ~ISprite() override;

            /** Gets the texture used by the sprite. */
            virtual SmartPtr<ITexture> getTexture() const = 0;

            /** Sets the texture used by the sprite. */
            virtual void setTexture( SmartPtr<ITexture> texture ) = 0;

            /** Gets the transformation matrix for the sprite. */
            virtual Matrix4F getTransform() const = 0;

            /** Sets the transformation matrix for the sprite. */
            virtual void setTransform( const Matrix4F &transform ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // ISprite_h__
