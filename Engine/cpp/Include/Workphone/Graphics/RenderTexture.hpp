#ifndef __WP_RenderTexture_h__
#define __WP_RenderTexture_h__

#include <Workphone/Graphics/RenderTarget.hpp>
#include <Workphone/Interface/Graphics/IRenderTexture.hpp>

namespace workphone
{
    namespace render
    {

        class WPCore_API RenderTexture : public RenderTarget<IRenderTexture>
        {
        public:
            RenderTexture();
            ~RenderTexture() override;

            SmartPtr<ITexture> getTexture() const override;
            void setTexture( SmartPtr<ITexture> texture ) override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // RenderTexture_h__
