#ifndef IShader_h__
#define IShader_h__

#include <Workphone/Interface/System/IResource.hpp>

namespace workphone
{
    namespace render
    {

        /** An interface for a material shader. */
        class WPCore_API IMaterialShader : public IResource
        {
        public:
            /** Virtual destructor. */
            ~IMaterialShader() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // IShader_h__
