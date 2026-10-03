/**
 * @file UiUtil.hpp
 * @brief Utility class for creating and managing UI elements and alignment types in the WorkPhone
 * engine.
 *
 * Provides static helper functions and constants for UI element creation, alignment string conversion,
 * and common UI string identifiers.
 */
#ifndef UIComponentUtil_h__
#define UIComponentUtil_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/WorkphoneEnums.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @class UiUtil
         * @brief Utility class for UI element creation and alignment handling.
         *
         * This class provides static helper functions for creating common UI elements (such as text,
         * button, panel, slider, etc.), as well as utilities for converting between alignment enums and
         * their string representations. It also exposes common string constants and arrays used
         * throughout the UI system.
         */
        class WPCore_API UiUtil
        {
        public:
            /**
             * @brief Names for UI directions (e.g., "Horizontal", "Vertical").
             */
            static const Array<String> directionNames;

            /**
             * @brief List of supported vertical alignment type names (e.g., "Top", "Center", "Bottom",
             * "Custom").
             */
            static const Array<String> verticalAlignmentTypes;
            /**
             * @brief List of supported horizontal alignment type names (e.g., "Left", "Center", "Right",
             * "Custom").
             */
            static const Array<String> horizontalAlignmentTypes;

            /**
             * @brief String identifier for direction property.
             */
            static const String directionStr;

            /**
             * @brief String identifier for handle property.
             */
            static const String handleStr;

            /**
             * @brief String identifier for background property.
             */
            static const String backgroundStr;

            /**
             * @brief String identifier for fill property.
             */
            static const String fillStr;

            /**
             * @brief String identifier for scroll value property.
             */
            static const String scrollValueStr;

            /**
             * @brief String identifier for slider value property.
             */
            static const String sliderValueStr;

            static const String topStr;
            static const String centerStr;
            static const String bottomStr;

            static const String leftStr;
            static const String rightStr;

            static const String dropDownNameStr;
            static const String optionStr;
            static const String panelStr;

            /**
             * @brief Converts a HorizontalAlignment enum value to its string representation.
             * @param horizontalAlignment The horizontal alignment enum value.
             * @return The corresponding string (e.g., "Left", "Center", "Right", "Custom").
             */
            static String getHorizontalAlignmentString( HorizontalAlignment horizontalAlignment );

            /**
             * @brief Converts a string to its corresponding HorizontalAlignment enum value.
             * @param str The string representation of the alignment.
             * @return The corresponding HorizontalAlignment enum value.
             */
            static HorizontalAlignment getHorizontalAlignment( const String &str );

            /**
             * @brief Gets a comma-separated string of all supported horizontal alignment types.
             * @return A string listing all horizontal alignment types.
             */
            static String getHorizontalAlignmentTypesString();

            /**
             * @brief Converts a VerticalAlignment enum value to its string representation.
             * @param verticalAlignment The vertical alignment enum value.
             * @return The corresponding string (e.g., "Top", "Center", "Bottom", "Custom").
             */
            static String getVerticalAlignmentString( VerticalAlignment verticalAlignment );

            /**
             * @brief Converts a string to its corresponding VerticalAlignment enum value.
             * @param str The string representation of the alignment.
             * @return The corresponding VerticalAlignment enum value.
             */
            static VerticalAlignment getVerticalAlignment( const String &str );

            /**
             * @brief Gets a comma-separated string of all supported vertical alignment types.
             * @return A string listing all vertical alignment types.
             */
            static String getVerticalAlignmentTypesString();

            /**
             * @brief Creates a text UI actor with the specified label.
             * @param label The text label to display.
             * @param addToScene If true, adds the actor to the scene.
             * @return Smart pointer to the created text actor.
             */
            static SmartPtr<IGameActor> createText( const String &label, bool addToScene );

            /**
             * @brief Creates a button UI actor with the specified label.
             * @param label The button label to display.
             * @param addToScene If true, adds the actor to the scene.
             * @return Smart pointer to the created button actor.
             */
            static SmartPtr<IGameActor> createButton( const String &label, bool addToScene );

            /**
             * @brief Creates a dropdown UI actor with the specified label.
             * @param label The dropdown label to display.
             * @param addToScene If true, adds the actor to the scene.
             * @return Smart pointer to the created dropdown actor.
             */
            static SmartPtr<IGameActor> createDropdown( const String &label, bool addToScene );

            /**
             * @brief Creates a panel UI actor.
             * @param director Smart pointer to the director managing the panel.
             * @param hint Optional hint string for the panel.
             * @param addToScene If true, adds the actor to the scene.
             * @return Smart pointer to the created panel actor.
             */
            static SmartPtr<IGameActor> createPanel( SmartPtr<IBuildDirector> director,
                                                     const String &hint, bool addToScene );

            /**
             * @brief Creates a horizontal slider UI actor.
             * @param label The slider label to display.
             * @param director Smart pointer to the director managing the slider.
             * @param hint Optional hint string for the slider.
             * @param addToScene If true, adds the actor to the scene.
             * @return Smart pointer to the created slider actor.
             */
            static SmartPtr<IGameActor> createSlider( const String &label,
                                                      SmartPtr<IBuildDirector> director,
                                                      const String &hint, bool addToScene );

            /**
             * @brief Creates a vertical slider UI actor.
             * @param label The slider label to display.
             * @param director Smart pointer to the director managing the slider.
             * @param hint Optional hint string for the slider.
             * @param addToScene If true, adds the actor to the scene.
             * @return Smart pointer to the created vertical slider actor.
             */
            static SmartPtr<IGameActor> createSliderVertical( const String &label,
                                                              SmartPtr<IBuildDirector> director,
                                                              const String &hint, bool addToScene );

            /**
             * @brief Creates a horizontal scrollbar UI actor.
             * @param label The scrollbar label to display.
             * @param director Smart pointer to the director managing the scrollbar.
             * @param hint Optional hint string for the scrollbar.
             * @param addToScene If true, adds the actor to the scene.
             * @return Smart pointer to the created scrollbar actor.
             */
            static SmartPtr<IGameActor> createScrollbar( const String &label,
                                                         SmartPtr<IBuildDirector> director,
                                                         const String &hint, bool addToScene );

            /**
             * @brief Creates a vertical scrollbar UI actor.
             * @param label The scrollbar label to display.
             * @param director Smart pointer to the director managing the scrollbar.
             * @param hint Optional hint string for the scrollbar.
             * @param addToScene If true, adds the actor to the scene.
             * @return Smart pointer to the created vertical scrollbar actor.
             */
            static SmartPtr<IGameActor> createScrollbarVertical( const String &label,
                                                                 SmartPtr<IBuildDirector> director,
                                                                 const String &hint, bool addToScene );

            /**
             * @brief Creates a scrollview UI actor.
             * @param label The scrollview label to display.
             * @param director Smart pointer to the director managing the scrollview.
             * @param hint Optional hint string for the scrollview.
             * @param addToScene If true, adds the actor to the scene.
             * @return Smart pointer to the created scrollview actor.
             */
            static SmartPtr<IGameActor> createScrollview( const String &label,
                                                          SmartPtr<IBuildDirector> director,
                                                          const String &hint, bool addToScene );
        };
    }  // namespace scene
}  // namespace workphone

#endif  // UIComponentUtil_h__
