#ifndef _ClawUIMENUITEM_H
#define _ClawUIMENUITEM_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ClawUIMenuItem : public ClawUIElement<IUIElement>
        {
        public:
            ClawUIMenuItem();
            ~ClawUIMenuItem() override;

            bool handleEvent( const SmartPtr<IInputEvent> &event ) override;

            void draw( struct wp_context *ctx ) override;
        };
    }  // end namespace ui
}  // namespace workphone

#endif
