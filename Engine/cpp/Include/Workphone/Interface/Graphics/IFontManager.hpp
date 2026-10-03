#ifndef IFontManager_h__
#define IFontManager_h__

#include <Workphone/Interface/System/IResourceManager.hpp>

namespace workphone
{
    namespace render
    {

        /** Interface for a font manager. */
        class WPCore_API IFontManager : public IResourceManager
        {
        public:
            IFontManager();

            IFontManager( u32 poolTypeId );

            /** Virtual destructor. */
            ~IFontManager() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // IFontManager_h__
