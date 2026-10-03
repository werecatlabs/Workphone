#ifndef _COverlay_H
#define _COverlay_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IOverlay.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class COverlay
         * @brief Base implementation of an overlay.
         */
        class ClawOverlay : public IOverlay
        {
        public:
            ClawOverlay();
            ~ClawOverlay() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;
            void update( struct wp_context *ctx );

            void addElement( SmartPtr<IOverlayElement> element ) override;
            bool removeElement( SmartPtr<IOverlayElement> element ) override;
            Array<SmartPtr<IOverlayElement>> getElements() const override;

            void addSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode ) override;
            bool removeSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode ) override;

            void setVisible( bool visible ) override;
            bool isVisible() const override;
            void setZOrder( u32 zorder ) override;
            u32 getZOrder() const override;
            void updateZOrder() override;

            Vector2I getAbsoluteResolution() const override;
            void setAbsoluteResolution( const Vector2I &absoluteResolution ) override;

            void _getObject( void **ppObject ) const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            ConcurrentArray<SmartPtr<IOverlayElement>> m_elements;
            bool m_visible = true;
            u32 m_zOrder = 0;
            Vector2I m_absoluteResolution = Vector2I( 1920, 1080 );
        };
    }  // end namespace render
}  // namespace workphone

#endif
