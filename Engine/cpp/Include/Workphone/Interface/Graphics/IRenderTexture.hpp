#ifndef IRenderTexture_h__
#define IRenderTexture_h__

#include <Workphone/Interface/Graphics/IRenderTarget.hpp>

namespace workphone
{
    namespace render
    {

        /** Interface to manage a render texture. */
        class WPCore_API IRenderTexture : public IRenderTarget
        {
        public:
            /** Virtual destructor. */
            ~IRenderTexture() override;

            /**
             * Returns the texture used by this object.
             * @return A pointer representing the texture used by this object
             */
            virtual SmartPtr<ITexture> getTexture() const = 0;

            /**
             * Sets the texture that will be used by this object.
             * @param texture A pointer representing the texture to be set
             */
            virtual void setTexture( SmartPtr<ITexture> texture ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // IRenderTexture_h__
