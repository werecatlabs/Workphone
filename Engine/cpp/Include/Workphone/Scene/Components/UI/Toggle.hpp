#ifndef ToggleComponent_h__
#define ToggleComponent_h__

#include <Workphone/Interface/UI/IUIToggle.hpp>
#include <Workphone/Scene/Components/UI/UIComponent.hpp>

namespace workphone
{
    namespace scene
    {

        /** Toggle component class. */
        class WPCore_API Toggle : public UIComponent
        {
        public:
            // Property key constants
            static const String toggleStr;
            static const String toggleBgTransformStr;
            static const String toggleTransformStr;
            static const String toggledColourStr;
            static const String untoggledColourStr;
            static const String labelStr;
            static const String textSizeStr;
            static const String showLabelStr;
            static const String toggleTypeStr;
            static const String toggleStateStr;
            static const String toggledPositionFactorStr;
            static const String untoggledPositionFactorStr;

            using ToggleType = ui::IUIToggle::ToggleType;
            using ToggleState = ui::IUIToggle::ToggleState;

            /** Constructor. */
            Toggle();

            /** Destructor. */
            ~Toggle() override;

            /** @copydoc UIComponent::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc UIComponent::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc UIComponent::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc UIComponent::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc UIComponent::updateTransform */
            void updateTransform() override;

            /** Get the toggled state.
             * @return True if toggled, false otherwise.
             */
            bool isToggled() const;

            /** Set the toggled state.
             * @param toggled True to toggle, false to untoggle.
             */
            void setToggled( bool toggled );

            /** Update the colour based on the toggled state. */
            void updateColour() override;

            /** Get the toggle background transform.
             * @return The toggle background transform.
             */
            SmartPtr<LayoutTransform> getToggleBgTransform() const;

            /** Set the toggle background transform.
             * @param toggleBgTransform The toggle background transform.
             */
            void setToggleBgTransform( SmartPtr<LayoutTransform> toggleBgTransform );

            /** Gets the toggle transform.
             * @return The toggle transform.
             */
            SmartPtr<LayoutTransform> getToggleTransform() const;

            /** Sets the toggle transform.
             * @param toggleTransform The toggle transform.
             */
            void setToggleTransform( SmartPtr<LayoutTransform> toggleTransform );

            /** Get the toggled colour.
             * @return The toggled colour.
             */
            ColourF getToggledColour() const;

            /** Set the toggled colour.
             * @param toggledColour The toggled colour.
             */
            void setToggledColour( const ColourF &toggledColour );

            /** Get the untoggled colour.
             * @return The untoggled colour.
             */
            ColourF getUntoggledColour() const;

            /** Set the untoggled colour.
             * @param untoggledColour The untoggled colour.
             */
            void setUntoggledColour( const ColourF &untoggledColour );

            String getLabel() const;
            void setLabel( const String &label );

            f32 getTextSize() const;
            void setTextSize( f32 textSize );

            bool getShowLabel() const;
            void setShowLabel( bool showLabel );

            ToggleType getToggleType() const;
            void setToggleType( ToggleType toggleType );

            ToggleState getToggleState() const;
            void setToggleState( ToggleState toggleState );

            Vector2<real_Num> getToggledPositionFactor() const;
            void setToggledPositionFactor( const Vector2<real_Num> &factor );

            Vector2<real_Num> getUntoggledPositionFactor() const;
            void setUntoggledPositionFactor( const Vector2<real_Num> &factor );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @copydoc UIComponent::handleEvent */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            /** @copydoc UIComponent::handleComponentEvent */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /** @copydoc UIComponent::createUI */
            void createUI() override;

            /** @copydoc UIComponent::updateElementState */
            void updateElementState() override;

            /** The toggle background transform. */
            SmartPtr<LayoutTransform> m_toggleBgTransform;

            /** The toggle transform. */
            SmartPtr<LayoutTransform> m_toggleTransform;

            /** The toggled colour. */
            ColourF m_toggledColour = ColourF::Green;

            /** The untoggled colour. */
            ColourF m_untoggledColour = ColourF::White;

            FixedString<128> m_label = "Toggle";
            f32 m_textSize = 1.0f;
            bool m_showLabel = true;
            ToggleType m_toggleType = ToggleType::ToggleButton;
            ToggleState m_toggleState = ToggleState::On;
            Vector2<real_Num> m_toggledPositionFactor =
                Vector2<real_Num>( static_cast<real_Num>( 0.5 ), static_cast<real_Num>( 0.0 ) );
            Vector2<real_Num> m_untoggledPositionFactor =
                Vector2<real_Num>( static_cast<real_Num>( -0.5 ), static_cast<real_Num>( 0.0 ) );

            /** The toggled state. */
            bool m_isToggled = true;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // ToggleComponent_h__
