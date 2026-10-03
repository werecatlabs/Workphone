#ifndef _COverlayManager_H
#define _COverlayManager_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IOverlayManager.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render
    {

        /** Implements IOverlayManager interface for Ogre. */
        class COverlayManagerOgre : public IOverlayManager
        {
        public:
            /** Constructor. */
            COverlayManagerOgre();

            /** Destructor. */
            ~COverlayManagerOgre() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IOverlayManager::addOverlay */
            SmartPtr<IOverlay> addOverlay( const String &instanceName ) override;

            /** @copydoc IOverlayManager::removeOverlay */
            bool removeOverlay( const String &instanceName ) override;

            /** @copydoc IOverlayManager::removeOverlay */
            bool removeOverlay( SmartPtr<IOverlay> overlay ) override;

            /** @copydoc IOverlayManager::findOverlay */
            SmartPtr<IOverlay> findOverlay( const String &instanceName ) override;

            /** @copydoc IOverlayManager::hasOverlay */
            bool hasOverlay( const String &instanceName ) override;

            /** @copydoc IOverlayManager::addElement */
            SmartPtr<IOverlayElement> addElement( const String &typeName,
                                                  const String &instanceName ) override;

            /** @copydoc IOverlayManager::findElement */
            SmartPtr<IOverlayElement> findElement( const String &instanceName ) override;

            /** @copydoc IOverlayManager::removeElement */
            bool removeElement( SmartPtr<IOverlayElement> element ) override;

            /** @copydoc IOverlayManager::_getObject */
            void _getObject( void **ppObject ) const override;

            /** @copydoc IOverlayManager::getOverlays */
            Array<SmartPtr<IOverlay>> getOverlays() const override;

            void lock() override;
            void unlock() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** The array of overlay elements. */
            ConcurrentArray<SmartPtr<IOverlayElement>> m_overlayElements;

            /** The array of overlays. */
            ConcurrentArray<SmartPtr<IOverlay>> m_overlays;

            /** The name extension used to create unique names for overlays. */
            static u32 m_nameExt;
        };
    }  // end namespace render
}  // namespace workphone

#endif
