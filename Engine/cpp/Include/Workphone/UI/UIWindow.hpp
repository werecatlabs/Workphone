#ifndef UIWindow_h__
#define UIWindow_h__

#include <Workphone/UI/UIElement.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>

namespace workphone
{
    namespace ui
    {

        //! UIWindow class representation for WorkPhone application's Windows interface.
        //! This class inherits from the UIElement base and provides implementing functionality
        //! for the IUIWindow interface.
        /*!
            \tparam IUIWindow An interface which represents this UI window, inheriting the IUElement
           interface.
        */
        class WPCore_API UIWindow : public UIElement<IUIWindow>
        {
        public:
            // Constructor
            UIWindow();

            // Destructor
            ~UIWindow() override;

            // Set the label of the window, represented as a string.
            /*!
                \param[in] label The new Label for this Window.
            */
            void setLabel( const String &label ) override;

            // Return the current label of the window as a string.
            /*!
                \return The current Label for this Window.
            */
            String getLabel() const override;

            // Associate a context menu to this window.
            /*!
                \param[in] menu A SmartPtr instance pointing to IUIMenu object representing the menu.
            */
            void setContextMenu( SmartPtr<IUIMenu> menu ) override;

            // Get the associated context menu from this window in the form of a SmartPtr instance.
            /*!
                \return A SmartPtr pointing to IUIMenu object representing the menu for this Window.
            */
            SmartPtr<IUIMenu> getContextMenu() const override;

            // Check if this window has a border to see if it is a window or dialog box.
            /*!
                \return A boolean value: true if the window has a border, false otherwise.
            */
            bool hasBorder() const override;

            // Set whether this window has a border, making it a window or dialog box.
            /*!
                \param[in] border If the Window should have a border, pass in true. False indicates a
               dialog box type.
            */
            void setHasBorder( bool border ) override;

            // Determine if this window is currently docked (or detached).
            /*!
                \return A boolean value: true if the window is docked, false otherwise (indicating it is
               a floating window).
            */
            bool isDocked() const override;

            // Set the docking state for this window.
            /*!
                \param[in] docked If the Window should be docked or detached, pass in true or false
               accordingly.
            */
            void setDocked( bool docked ) override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // UIWindow_h__
