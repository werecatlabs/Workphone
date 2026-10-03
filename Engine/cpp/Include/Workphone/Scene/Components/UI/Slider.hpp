#ifndef SliderComponent_h__
#define SliderComponent_h__

#include <Workphone/Scene/Components/UI/UIComponent.hpp>

namespace workphone
{
    namespace scene
    {

        /** Slider component for UI. */
        class WPCore_API Slider : public UIComponent
        {
        public:
            static const String handleStr;
            static const String backgroundStr;
            static const String fillStr;
            static const String valueStr;
            static const String minValueStr;
            static const String maxValueStr;
            static const String directionStr;
            static const String isDraggingStr;

            /** Constructor. */
            Slider();

            /** Destructor. */
            ~Slider() override;

            /** @copydoc UIComponent::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc UIComponent::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc UIComponent::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc UIComponent::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc UIComponent::handleEvent */
            bool handleEvent( const SmartPtr<IInputEvent> &event ) override;

            /** Get the handle actor. */
            SmartPtr<IGameActor> getHandleActor() const;

            /** Set the handle actor. */
            void setHandleActor( SmartPtr<IGameActor> handle );

            /** Get the background actor. */
            SmartPtr<IGameActor> getBackground() const;

            /** Set the background actor. */
            void setBackground( SmartPtr<IGameActor> background );

            /** Get the fill actor. */
            SmartPtr<IGameActor> getFill() const;

            /** Set the fill actor. */
            void setFill( SmartPtr<IGameActor> fill );

            /** Set the callback function. */
            void setCallbackFunction( std::function<void( hash_type )> callbackFunction );

            /** Get the slider value. */
            f32 getValue() const;

            /** Set the slider value. */
            void setValue( f32 value );

            /** Get the minimum value. */
            f32 getMinValue() const;

            /** Set the minimum value. */
            void setMinValue( f32 minValue );

            /** Get the maximum value. */
            f32 getMaxValue() const;

            /** Set the maximum value. */
            void setMaxValue( f32 maxValue );

            /** Get the slider direction. */
            Direction getDirection() const;

            /** Set the slider direction. */
            void setDirection( Direction direction );

            /** Check if the slider is being dragged. */
            bool isDragging() const;

            /** Set the dragging state. */
            void setDragging( bool dragging );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @brief Updates the slider's handle position based on the current value and direction. */
            void updateSliderPosition();

            /** @brief Handles component-specific events for the slider. */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /**
             * @brief Creates the UI element.
             */
            void createUI() override;

            /**
             * @brief Updates the visual state of the element.
             */
            void updateElementState() override;

            // Handle actor
            SmartPtr<IGameActor> m_handleActor;

            // Background actor
            SmartPtr<IGameActor> m_background;

            // Fill actor
            SmartPtr<IGameActor> m_fill;

            // Current value
            f32 m_sliderValue = 0.0f;

            // Min/max values
            f32 m_minValue = 0.0f;
            f32 m_maxValue = 1.0f;

            // Slider direction
            Direction m_direction = Direction::Horizontal;

            // Dragging state
            bool m_isDragging = false;

            // Event for slider value changes
            std::function<void( hash_type )> m_callbackFunction;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // SliderComponent_h__
