#ifndef TabView_h__
#define TabView_h__

#include <Workphone/Scene/Components/UI/UIComponent.hpp>

namespace workphone
{
    namespace scene
    {

        /** Component for a tabview that manages multiple tab pages. */
        class WPCore_API TabView : public UIComponent
        {
        public:
            static const String tabCountStr;

            /** Enumeration for tab orientation */
            enum class Orientation
            {
                Horizontal = 0,
                Vertical = 1
            };

            /** Enumeration for tab position */
            enum class TabPosition
            {
                Top = 0,
                Bottom = 1,
                Left = 2,
                Right = 3
            };

            /** String constants for properties */
            static const String orientationStr;
            static const String tabPositionStr;
            static const String tabSizeStr;
            static const String showTabHeadersStr;

            TabView();
            ~TabView() override;

            /** @copydoc UIComponent::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc UIComponent::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc UIComponent::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /** @copydoc UIComponent::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc UIComponent::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc UIComponent::isValid */
            bool isValid() const override;

            /** @copydoc UIComponent::createUI */
            void createUI() override;

            /** @copydoc UIComponent::updateElementState */
            void updateElementState() override;

            /** @copydoc UIComponent::handleEvent */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            /**
             * @brief Adds a tab page to the tab view.
             * @param tabPage The tab page to add.
             */
            void addTabPage( SmartPtr<TabPage> tabPage );

            /**
             * @brief Removes a tab page by index.
             * @param index The index of the tab page to remove.
             */
            void removeTabPage( u32 index );

            /**
             * @brief Removes a specific tab page.
             * @param tabPage The tab page to remove.
             */
            void removeTabPage( SmartPtr<TabPage> tabPage );

            /**
             * @brief Gets a tab page by index.
             * @param index The index of the tab page.
             * @return The tab page at the specified index, or nullptr if index is out of range.
             */
            SmartPtr<TabPage> getTabPage( u32 index ) const;

            /**
             * @brief Gets all tab pages.
             * @return Array of all tab pages.
             */
            Array<SmartPtr<TabPage>> getTabPages() const;

            /**
             * @brief Gets the number of tab pages.
             * @return The count of tab pages.
             */
            u32 getTabPageCount() const;

            /**
             * @brief Sets the orientation of the tab view.
             * @param orientation The orientation (horizontal or vertical).
             */
            void setOrientation( Orientation orientation );

            /**
             * @brief Gets the orientation of the tab view.
             * @return The current orientation.
             */
            Orientation getOrientation() const;

            /**
             * @brief Sets the position of tab headers.
             * @param position The tab position (top, bottom, left, right).
             */
            void setTabPosition( TabPosition position );

            /**
             * @brief Gets the position of tab headers.
             * @return The current tab position.
             */
            TabPosition getTabPosition() const;

            /**
             * @brief Sets the size of individual tabs.
             * @param size The tab size.
             */
            void setTabSize( const Vector2F &size );

            /**
             * @brief Gets the size of individual tabs.
             * @return The current tab size.
             */
            Vector2F getTabSize() const;

            /**
             * @brief Sets whether to show tab headers.
             * @param showHeaders True to show headers, false to hide them.
             */
            void setShowTabHeaders( bool showHeaders );

            /**
             * @brief Gets whether tab headers are shown.
             * @return True if headers are shown, false otherwise.
             */
            bool getShowTabHeaders() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Updates the tab layout based on current settings.
             */
            void updateTabLayout();

            /** Array of tab pages managed by this tab view */
            Array<SmartPtr<TabPage>> m_tabPages;

            /** Orientation of the tab view */
            Orientation m_orientation;

            /** Position of tab headers */
            TabPosition m_tabPosition;

            /** Size of individual tabs */
            Vector2F m_tabSize;

            /** Whether to show tab headers */
            bool m_showTabHeaders;

            /** Header area component for tab buttons */
            SmartPtr<UIComponent> m_headerArea;

            /** Content area component for tab content */
            SmartPtr<UIComponent> m_contentArea;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // TabView_h__
