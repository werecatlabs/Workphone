#ifndef IShaderManager_h__
#define IShaderManager_h__

#include <Workphone/Interface/System/IResourceManager.hpp>

namespace workphone
{
    namespace render
    {

        class WPCore_API IShaderManager : public IResourceManager
        {
        public:
            /** Virtual destructor. */
            ~IShaderManager() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // IShaderManager_h__
