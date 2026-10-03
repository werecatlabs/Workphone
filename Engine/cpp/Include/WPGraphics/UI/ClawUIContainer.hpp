#ifndef _ClawUICONTAINER_H
#define _ClawUICONTAINER_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>
#include <Workphone/Interface/UI/IUILayoutContainer.hpp>

struct wp_context;

namespace workphone
{
    namespace ui
    {
        class ClawUIContainer : public ClawUIElement<IUILayoutContainer>
        {
        public:
            ClawUIContainer();
            ~ClawUIContainer() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void setPosition( const Vector2F &position ) override;

            /** @copydoc GuiElement::draw */
            void draw( struct wp_context *ctx ) override;

            SmartPtr<render::IOverlayElementContainer> getOverlayContainer() const;
            void setOverlayContainer( SmartPtr<render::IOverlayElementContainer> overlayContainer );

            void _getObject( void **ppObject ) const override;

        private:
        };
    }  // end namespace ui
}  // namespace workphone

#endif
