#ifndef _ClawUISPINNER_H
#define _ClawUISPINNER_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/UI/IUISpinner.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ClawUISpinner : public ClawUIElement<IUISpinner>
        {
        public:
            ClawUISpinner();
            ~ClawUISpinner() override;

            bool handleEvent( const SmartPtr<IInputEvent> &event ) override;

            void setText( const String &text ) override;
            String getText() const override;

            void incrementValue() override;
            void decrementValue() override;

            void setValue( const String &value ) override;
            String getValue() const override;

            void setMinValue( const String &value ) override;
            String getMinValue() const override;

            void setMaxValue( const String &value ) override;
            String getMaxValue() const override;

            void draw( struct wp_context *ctx ) override;

        private:
            String m_valueType;
            f64 m_increament = 1.0;
            f64 m_value = 0.0;
            f64 m_minValue = 0.0;
            f64 m_maxValue = 100.0;
            String m_text;
            Array<String> m_values;
        };
    }  // end namespace ui
}  // namespace workphone

#endif
