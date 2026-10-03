#ifndef CMaterialManagerOgreNext_h__
#define CMaterialManagerOgreNext_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/MaterialManager.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render
    {

        /** Class used to manage materials. */
        class CMaterialManagerOgreNext : public MaterialManager
        {
        public:
            /** Constructor. */
            CMaterialManagerOgreNext();

            /** Destructor. */
            ~CMaterialManagerOgreNext() override;

            /** @copydoc MaterialManager::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc MaterialManager::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc MaterialManager::cloneMaterial */
            SmartPtr<IMaterial> cloneMaterial( SmartPtr<IMaterial> material,
                                               const String &clonedMaterialName );

            /** @copydoc MaterialManager::cloneMaterial */
            SmartPtr<IMaterial> cloneMaterial( const String &name,
                                               const String &clonedMaterialName ) override;

            /** @copydoc MaterialManager::create */
            SmartPtr<IResource> create( const String &name ) override;

            /** @copydoc MaterialManager::create */
            SmartPtr<IResource> create( const String &uuid, const String &name ) override;

            /** @copydoc MaterialManager::_getObject */
            void _getObject( void **ppObject ) const override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CMaterialManagerOgreNext_h__
