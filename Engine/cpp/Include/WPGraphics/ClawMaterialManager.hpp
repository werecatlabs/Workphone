#ifndef CMaterialManagerWPGraphics_h__
#define CMaterialManagerWPGraphics_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/MaterialManager.hpp>

namespace workphone::render
{
    /** Material manager for the WPGraphics/ClawHammer renderer. */
    class WPGraphics_API ClawMaterialManager : public MaterialManager
    {
    public:
        ClawMaterialManager();
        ~ClawMaterialManager() override;

        /** @copydoc MaterialManager::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc MaterialManager::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc MaterialManager::cloneMaterial */
        SmartPtr<IMaterial> cloneMaterial( SmartPtr<IMaterial> material,
                                           const String &clonedMaterialName ) override;

        /** @copydoc MaterialManager::cloneMaterial */
        SmartPtr<IMaterial> cloneMaterial( const String &name,
                                           const String &clonedMaterialName ) override;

        /** @copydoc MaterialManager::create */
        SmartPtr<IResource> create( const String &name ) override;

        /** @copydoc MaterialManager::create */
        SmartPtr<IResource> create( const String &uuid, const String &name ) override;

        SmartPtr<IResource> loadFromFile( const String &filePath ) override;
        u32 releaseUnusedResources( u32 maximumCandidates = 32 ) override;
        u32 releaseUnusedResourcesOnUpdate( u32 maximumCandidates = 32 ) override;

        /** @copydoc MaterialManager::_getObject */
        void _getObject( void **ppObject ) const override;

        WP_CLASS_REGISTER_DECL;

    private:
        u32 releaseUnusedResourcesImpl( u32 maximumCandidates, bool automatic );
        size_t m_evictionCursor = 0;
    };
}  // namespace workphone::render

#endif  // CMaterialManagerWPGraphics_h__
