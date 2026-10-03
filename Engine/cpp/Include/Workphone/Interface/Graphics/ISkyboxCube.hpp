#ifndef __ISkyboxCube_h__
#define __ISkyboxCube_h__

#include <Workphone/Interface/Graphics/ISkybox.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * Represents a skybox cube.
         */
        class WPCore_API ISkyboxCube : public ISkybox
        {
        public:
            /** Virtual destructor. */
            ~ISkyboxCube() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // __ISkyboxCube_h__
