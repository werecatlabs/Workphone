#ifndef ClawUICheckBox_h__
#define ClawUICheckBox_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/UI/IUICheckbox.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

struct wp_context;

namespace workphone
{
    namespace ui
    {
        class ClawUICheckBox : public ClawUIElement<IUICheckbox>
        {
        public:
            ClawUICheckBox();
            ~ClawUICheckBox() override;

            void setValue( bool value ) override;

            bool getValue() const override;

            void draw( struct wp_context *ctx ) override;

        private:
            bool m_value = false;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // ClawUICheckBox_h__
