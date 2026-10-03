#ifndef ToolTip_h__
#define ToolTip_h__

#include <Workphone/Scene/Components/UI/UIComponent.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class ToolTip
         * @brief Component that displays contextual tooltip text for UI elements.
         *
         * The ToolTip component stores the text and runtime state required to show a tooltip when a
         * user hovers the mouse over a UI element. It tracks the tooltip position, whether the mouse
         * is currently over the target, and how long the mouse has been hovering. This class is a
         * lightweight data/component holder and does not itself perform rendering; rendering is
         * performed by the UI system which consumes this component's data.
         */
        class WPCore_API ToolTip : public Component
        {
        public:
            /// Property name for tooltip position in serialized/property maps.
            static const String positionStr;

            /// Property name for accumulated mouse-over time (seconds) in serialized/property maps.
            static const String mouseOverTimeStr;

            /// Property name for the mouse-over flag in serialized/property maps.
            static const String isMouseOverStr;

            /// Property name for the tooltip text in serialized/property maps.
            static const String tooltipStr;

            /**
             * @brief Default constructor.
             *
             * Initializes members to sensible defaults: zero position, zero hover time and empty text.
             */
            ToolTip();

            /**
             * @brief Virtual destructor.
             */
            ~ToolTip() override;

            /**
             * @brief Load state from a serialized/shared object.
             *
             * This will populate the component's properties (position, tooltip text, etc.) from
             * the provided shared object. The exact keys are defined by the static property
             * name constants (positionStr, tooltipStr, ...).
             *
             * @param data Shared object containing initialization data.
             * @see Component::load
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload or reset component state.
             *
             * Called when the component is being removed or the owning object is being unloaded.
             * Implementations should clear or release any resources allocated during load().
             *
             * @param data Shared object containing unload data.
             * @see Component::unload
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Returns any child objects owned by this component.
             *
             * ToolTip does not normally own child objects but the method is provided to satisfy
             * the UIComponent interface. Returns an empty array by default.
             *
             * @return Array of shared pointers to child objects.
             * @see UIComponent::getChildObjects
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Get a Properties object representing this component for editors/serializers.
             *
             * The returned Properties object contains keys/values for position, tooltip text,
             * mouse-over time and the mouse-over flag.
             *
             * @return Shared pointer to the Properties object.
             * @see UIComponent::getProperties
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply properties from a Properties object to this component.
             *
             * Only recognized keys (positionStr, tooltipStr, etc.) are applied; unknown keys are
             * ignored. This is used by editors and deserialization code.
             *
             * @param properties Shared pointer to the Properties object to set.
             * @see UIComponent::setProperties
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get the tooltip position in world or UI space.
             * @return Position where the tooltip should be shown.
             */
            Vector3F getPosition() const;

            /**
             * @brief Set the tooltip position.
             * @param position New tooltip position in world or UI space.
             */
            void setPosition( const Vector3F &position );

            /**
             * @brief Get how long the mouse has been hovering (seconds).
             * @return Accumulated hover time in seconds.
             */
            f32 getMouseOverTime() const;
            /**
             * @brief Set the mouse-over accumulation time (seconds).
             * @param mouseOverTime Hover time in seconds.
             */
            void setMouseOverTime( f32 mouseOverTime );

            /**
             * @brief Query whether the mouse is currently over the target element.
             * @return True if the mouse is over the element; false otherwise.
             */
            bool isMouseOver() const;
            /**
             * @brief Set the mouse-over flag.
             * @param isMouseOver True to indicate the mouse is over the element.
             */
            void setMouseOver( bool isMouseOver );

            /**
             * @brief Get the tooltip text content.
             * @return Tooltip string displayed to the user.
             */
            String getTooltip() const;
            /**
             * @brief Set the tooltip text content.
             * @param tooltip Text to display inside the tooltip.
             */
            void setTooltip( const String &tooltip );

            WP_CLASS_REGISTER_DECL;

        protected:
            /// Tooltip position in world or UI coordinate space. Default is zero vector.
            Vector3F m_position = Vector3F::zero();

            /// Accumulated mouse hover time in seconds. Used to control tooltip delay.
            f32 m_mouseOverTime = 0.0f;

            /// True when the mouse is currently over the associated UI element.
            bool m_isMouseOver = false;

            /// The text displayed by the tooltip. Empty when no tooltip is set.
            String m_tooltip;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // ToolTip_h__
