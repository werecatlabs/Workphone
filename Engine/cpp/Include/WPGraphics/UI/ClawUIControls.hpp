#ifndef WORKPHONE_CLAW_UI_CONTROLS_HPP
#define WORKPHONE_CLAW_UI_CONTROLS_HPP

#include <WPGraphics/UI/ClawUIElement.hpp>
#include <Workphone/Interface/UI/IUIDropdown.hpp>
#include <Workphone/Interface/UI/IUISlider.hpp>

namespace workphone::ui
{
    /** Backend dropdown using the WorkphoneCore/Nuklear combo widget. */
    class ClawUIDropdown : public ClawUIElement<IUIDropdown>
    {
    public:
        Array<String> getOptions() const override;
        void setOptions( const Array<String> &options ) override;
        void addOption( const String &option ) override;
        u32 getSelectedOption() const override;
        void setSelectedOption( u32 selected ) override;
        void draw( wp_context *ctx ) override;
        WP_CLASS_REGISTER_DECL;

    private:
        Array<String> m_options;
        u32 m_selected = 0;
    };

    /** Backend slider; vertical orientation uses the same context, style and input state. */
    class ClawUISlider : public ClawUIElement<IUISlider>
    {
    public:
        f32 getValue() const override;
        void setValue( f32 value ) override;
        f32 getMinValue() const override;
        void setMinValue( f32 value ) override;
        f32 getMaxValue() const override;
        void setMaxValue( f32 value ) override;
        Direction getDirection() const override;
        void setDirection( Direction direction ) override;
        bool isDragging() const override;
        void setDragging( bool dragging ) override;
        void draw( wp_context *ctx ) override;
        WP_CLASS_REGISTER_DECL;

    private:
        f32 m_value = 0;
        f32 m_minValue = 0;
        f32 m_maxValue = 1;
        Direction m_direction = Direction::Horizontal;
        bool m_dragging = false;
    };
}  // namespace workphone::ui
#endif
