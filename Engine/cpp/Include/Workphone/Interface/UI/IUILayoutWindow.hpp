#ifndef _IGUILayout_H
#define _IGUILayout_H

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class IUILayoutWindow
         * @brief Interface for a UI layout element, responsible for managing layout elements in the user
         * interface
         */
        class WPCore_API IUILayoutWindow : public IUIElement
        {
        public:
            /**
             * @enum LayoutStates
             * @brief Enumerates the different layout states
             */
            enum class LayoutStates
            {
                None,     ///< No state
                Active,   ///< Active state
                FadeIn,   ///< Fade-in state
                FadeOut,  ///< Fade-out state

                Count  ///< Number of layout states
            };

            /**
             * @enum WindowFlags
             * @brief Backend-agnostic bitmask controlling window chrome and behaviour.
             *
             * The values mirror `wp_panel_flags` / `wp_window_flags` from the Workphone
             * C API so that a simple static_cast<unsigned int> converts without remapping.
             */
            enum class WindowFlags : unsigned int
            {
                None = 0,
                Border = ( 1u << 0 ),          ///< Draw a border around the window
                Movable = ( 1u << 1 ),         ///< Allow the user to move the window
                Scalable = ( 1u << 2 ),        ///< Allow the user to resize the window
                Closable = ( 1u << 3 ),        ///< Show a close button in the title bar
                Minimizable = ( 1u << 4 ),     ///< Show a minimise button in the title bar
                NoScrollbar = ( 1u << 5 ),     ///< Suppress the automatic scrollbar
                Title = ( 1u << 6 ),           ///< Show the window title bar
                ScrollAutoHide = ( 1u << 7 ),  ///< Hide scrollbar when content fits
                Background = ( 1u << 8 ),      ///< Draw a filled background
                ScaleLeft = ( 1u << 9 ),       ///< Scale handle on the left edge
                NoInput = ( 1u << 10 ),        ///< Suppress all input to the window

                Default = Border | Movable | Scalable | Minimizable | Title
            };

            /** Bitwise OR for WindowFlags */
            friend WindowFlags operator|( WindowFlags a, WindowFlags b )
            {
                return static_cast<WindowFlags>( static_cast<unsigned int>( a ) |
                                                 static_cast<unsigned int>( b ) );
            }

            /** Bitwise AND for WindowFlags */
            friend WindowFlags operator&( WindowFlags a, WindowFlags b )
            {
                return static_cast<WindowFlags>( static_cast<unsigned int>( a ) &
                                                 static_cast<unsigned int>( b ) );
            }

            /** Bitwise XOR for WindowFlags */
            friend WindowFlags operator^( WindowFlags a, WindowFlags b )
            {
                return static_cast<WindowFlags>( static_cast<unsigned int>( a ) ^
                                                 static_cast<unsigned int>( b ) );
            }

            /** Bitwise NOT for WindowFlags */
            friend WindowFlags operator~( WindowFlags a )
            {
                return static_cast<WindowFlags>( ~static_cast<unsigned int>( a ) );
            }

            /** Compound OR assignment */
            friend WindowFlags &operator|=( WindowFlags &a, WindowFlags b )
            {
                return a = a | b;
            }

            /** Compound AND assignment */
            friend WindowFlags &operator&=( WindowFlags &a, WindowFlags b )
            {
                return a = a & b;
            }

            IUILayoutWindow();

            IUILayoutWindow( u32 poolTypeId );

            /**
             * @brief Destructor
             */
            ~IUILayoutWindow() override;

            /**
             */
            virtual LayoutStates getState() = 0;

            /**
             */
            virtual void setState( LayoutStates state ) = 0;

            /**
             * @brief Gets the parent UI window associated with the layout
             * @return A SmartPtr to an IUIWindow object representing the UI window
             */
            virtual SmartPtr<IUIWindow> getParentWindow() const = 0;

            /**
             * @brief Sets the parent UI window associated with the layout
             * @param uiWindow A SmartPtr to an IUIWindow object representing the UI window
             */
            virtual void setParentWindow( SmartPtr<IUIWindow> uiWindow ) = 0;

            /**
             * @brief Get the current window decoration / behaviour flags.
             * @return The active WindowFlags bitmask.
             */
            virtual WindowFlags getWindowFlags() const = 0;

            /**
             * @brief Replace the window decoration / behaviour flags.
             *
             * Changes take effect on the next call to update().
             *
             * @param flags New flags bitmask (combine values from WindowFlags).
             */
            virtual void setWindowFlags( WindowFlags flags ) = 0;

            /**
             * @brief Test whether a specific flag (or set of flags) is currently set.
             *
             * @param flag Flag(s) to test.
             * @return `true` if every bit in @p flag is set in the current flags.
             */
            virtual bool hasWindowFlag( WindowFlags flag ) const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif
