#ifndef _COverlayManager_H
#define _COverlayManager_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IOverlayManager.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>

namespace workphone
{
    namespace render
    {
        class ClawOverlay;

        /**
         * @class COverlayManager
         * @brief Base implementation of an overlay manager.
         */
        class ClawOverlayManager : public IOverlayManager
        {
        public:
            ClawOverlayManager();
            ~ClawOverlayManager() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            virtual void update( struct wp_context *ctx );

            SmartPtr<IOverlay> addOverlay( const String &instanceName ) override;
            bool removeOverlay( const String &instanceName ) override;
            bool removeOverlay( SmartPtr<IOverlay> overlay ) override;
            Array<SmartPtr<IOverlay>> getOverlays() const override;
            SmartPtr<IOverlay> findOverlay( const String &instanceName ) override;
            bool hasOverlay( const String &instanceName ) override;

            SmartPtr<IOverlayElement> addElement( const String &typeName,
                                                  const String &instanceName ) override;
            SmartPtr<IOverlayElement> findElement( const String &instanceName ) override;
            bool removeElement( SmartPtr<IOverlayElement> element ) override;

            void _getObject( void **ppObject ) const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            ConcurrentArray<SmartPtr<IOverlay>> m_overlays;
            ConcurrentArray<SmartPtr<IOverlayElement>> m_overlayElements;
            std::unique_ptr<ui::ClawUIWorkphoneContext> m_workphoneContext;
        };
    }  // end namespace render
}  // namespace workphone

#endif
