#ifndef _ClawUIScrollingTxtElement_H
#define _ClawUIScrollingTxtElement_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ClawUIScrollingTextElement : public ClawUIElement<IUIElement>
        {
        public:
            ClawUIScrollingTextElement();
            ~ClawUIScrollingTextElement() override;

            bool handleEvent( const SmartPtr<IInputEvent> &event ) override;

            void draw( struct wp_context *ctx ) override;
        };
    }  // end namespace ui
}  // namespace workphone

#endif
