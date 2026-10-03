#ifndef __ClawUIToggleGroup_H
#define __ClawUIToggleGroup_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ClawUIToggleGroup : public ClawUIElement<IUIElement>
        {
        public:
            ClawUIToggleGroup( const String &id );
            ~ClawUIToggleGroup() override;

            void addToggleButton( ClawUIToggleButton *button );
            bool removeToggleButton( ClawUIToggleButton *button );

            void OnSetButtonToggled( ClawUIToggleButton *button );

            ClawUIToggleButton *getCurToggledButton() const;

            void draw( struct wp_context *ctx ) override;

        private:
            Array<ClawUIToggleButton *> m_toggleButtons;
            ClawUIToggleButton *m_curToggledBtn;
        };
    }  // end namespace ui
}  // namespace workphone

#endif
