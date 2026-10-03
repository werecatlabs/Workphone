#ifndef __CFontManagerWPGraphics_h__
#define __CFontManagerWPGraphics_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/FontManager.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render
    {
        /** @brief Font manager for the WPGraphics/ClawHammer renderer. */
        class WPGraphics_API ClawFontManager : public FontManager
        {
        public:
            /** @brief Constructor */
            ClawFontManager();

            /** @brief Destructor */
            ~ClawFontManager() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IResourceManager::create */
            SmartPtr<IResource> create( const String &name ) override;

            /** @copydoc IResourceManager::create */
            SmartPtr<IResource> create( const String &uuid, const String &name ) override;

            /** @copydoc IResourceManager::createOrRetrieve */
            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &uuid, const String &path,
                                                              const String &type ) override;

            /** @copydoc IResourceManager::createOrRetrieve */
            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &path ) override;

            /** @copydoc IResourceManager::saveToFile */
            void saveToFile( const String &filePath, SmartPtr<IResource> resource ) override;

            /** @copydoc IResourceManager::loadFromFile */
            SmartPtr<IResource> loadFromFile( const String &filePath ) override;

            /** @copydoc IResourceManager::load */
            SmartPtr<IResource> loadResource( const String &name ) override;

            /** @copydoc IResourceManager::getByName */
            SmartPtr<IResource> getByName( const String &name ) override;

            /** @copydoc IResourceManager::getById */
            SmartPtr<IResource> getById( const String &uuid ) override;

            /** @copydoc IResourceManager::cloneResource */
            SmartPtr<IResource> cloneResource( const String &name,
                                               const String &clonedResourceName ) override;

            /** @copydoc IResourceManager::_getObject */
            void _getObject( void **ppObject ) const override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // __CFontManagerWPGraphics_h__
