#ifndef __ISkyboxPlane_h_
#define __ISkyboxPlane_h_

#include <Workphone/Interface/Graphics/ISkybox.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * Represents a skybox.
         */
        class WPCore_API ISkyboxPlane : public ISkybox
        {
        public:
            /** Virtual destructor. */
            ~ISkyboxPlane() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // __ISkyboxPlane_h_
