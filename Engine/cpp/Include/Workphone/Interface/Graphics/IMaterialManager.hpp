#ifndef IMaterialManager_h__
#define IMaterialManager_h__

#include <Workphone/Interface/System/IResourceManager.hpp>

namespace workphone
{
    namespace render
    {

        /** Interface for a material manager. */
        class WPCore_API IMaterialManager : public IResourceManager
        {
        public:
            /** Virtual destructor. */
            ~IMaterialManager() override;

            /** Function used to clone a material.
             * @param material The existing material.
             * @param clonedMaterialName The name of the cloned material.
             */
            virtual SmartPtr<IMaterial> cloneMaterial( SmartPtr<IMaterial> material,
                                                       const String &clonedMaterialName ) = 0;

            /** Function used to clone a material.
             * @param name The name of the existing material.
             * @param clonedMaterialName The name of the cloned material.
             */
            virtual SmartPtr<IMaterial> cloneMaterial( const String &name,
                                                       const String &clonedMaterialName ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // IMaterialManager_h__
