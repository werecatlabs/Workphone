#ifndef UICustomShape_h__
#define UICustomShape_h__

/**
 * @file UICustomShape.hpp
 * @brief Custom shape widget wrapper for the Colibri GUI library.
 *
 * This header declares `UICustomShape`, a specialized wrapper around
 * Colibri::CustomShape that provides extended state management functionality
 * for custom shape rendering within the engine's UI system. It is used to
 * render arbitrary geometric shapes with custom visual states.
 */

#include "ColibriGui/ColibriCustomShape.h"

namespace workphone
{
    namespace ui
    {
        /**
         * @class UICustomShape
         * @brief Extended custom shape widget with enhanced state management.
         *
         * `UICustomShape` extends Colibri::CustomShape to provide additional
         * state handling capabilities integrated with the engine's UI system.
         * This class serves as a bridge between the engine's state system and
         * Colibri's custom shape rendering, allowing for dynamic visual state
         * changes (such as idle, highlighted, pressed states).
         *
         * The class is lightweight and delegates most rendering functionality
         * to the underlying Colibri::CustomShape base class, while providing
         * hooks for state transitions that may affect the visual appearance
         * of the custom shape.
         */
        class UICustomShapeColibri : public Colibri::CustomShape
        {
        public:
            /**
             * @brief Construct a new UICustomShape.
             *
             * Initializes the custom shape widget and registers it with the
             * provided Colibri manager. The manager is responsible for the
             * lifetime and rendering of the widget.
             *
             * @param manager Pointer to the Colibri manager that will own and
             *                manage this custom shape. Must not be null.
             */
            UICustomShapeColibri( Colibri::ColibriManager *manager );

            /**
             * @brief Destroy the UICustomShape.
             *
             * Cleans up resources and unregisters the widget from the Colibri
             * manager. The destructor ensures proper cleanup of any internal
             * state or resources allocated during the widget's lifetime.
             */
            ~UICustomShapeColibri();

            /**
             * @brief Set the visual state of the custom shape.
             *
             * Updates the custom shape's visual state, triggering appropriate
             * visual changes such as color, border, or other appearance modifications.
             * This method overrides the base class implementation to provide
             * engine-specific state handling behavior.
             *
             * @param state The new visual state to apply (e.g., Idle, Highlighted,
             *              Pressed, Disabled). The state determines the visual
             *              appearance of the custom shape.
             * @param smartHighlight If true, enables intelligent highlighting that
             *                       may consider additional context such as parent
             *                       state or input focus. If false, applies the
             *                       state directly without additional logic.
             *                       Default is true.
             */
            void setState( Colibri::States::States state, bool smartHighlight = true ) override;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // UICustomShape_h__
