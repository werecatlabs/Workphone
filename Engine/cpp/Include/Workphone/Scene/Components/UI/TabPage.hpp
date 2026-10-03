#ifndef TabPage_h__
#define TabPage_h__

#include <Workphone/Scene/Components/UI/UIComponent.hpp>

namespace workphone
{
    namespace scene
    {
        class TabItem;

        /**
         * @class TabPage
         * @brief Component for a tab page that manages multiple tab items and their content.
         *
         * A TabPage manages a collection of TabItem objects and provides functionality
         * to switch between tabs, display their content, and handle tab-related events.
         */
        class WPCore_API TabPage : public UIComponent
        {
        public:
            /** String constants for properties */
            static const String activeTabIndexStr;
            static const String tabsStr;
            static const String contentAreaStr;

            TabPage();
            ~TabPage() override;

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
             * @brief Adds a tab item to the tab page.
             * @param tabItem The tab item to add.
             */
            void addTabItem( SmartPtr<TabItem> tabItem );

            /**
             * @brief Removes a tab item by index.
             * @param index The index of the tab item to remove.
             */
            void removeTabItem( u32 index );

            /**
             * @brief Removes a specific tab item.
             * @param tabItem The tab item to remove.
             */
            void removeTabItem( SmartPtr<TabItem> tabItem );

            /**
             * @brief Gets a tab item by index.
             * @param index The index of the tab item.
             * @return The tab item at the specified index, or nullptr if index is out of range.
             */
            SmartPtr<TabItem> getTabItem( u32 index ) const;

            /**
             * @brief Gets all tab items.
             * @return Array of all tab items.
             */
            Array<SmartPtr<TabItem>> getTabItems() const;

            /**
             * @brief Gets the number of tab items.
             * @return The count of tab items.
             */
            u32 getTabCount() const;

            /**
             * @brief Sets the active tab by index.
             * @param index The index of the tab to make active.
             */
            void setActiveTabIndex( u32 index );

            /**
             * @brief Gets the index of the currently active tab.
             * @return The index of the active tab.
             */
            u32 getActiveTabIndex() const;

            /**
             * @brief Gets the currently active tab item.
             * @return The active tab item, or nullptr if no tabs exist.
             */
            SmartPtr<TabItem> getActiveTab() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Updates the content display for the active tab.
             */
            void updateActiveTabContent();

            /** Array of tab items managed by this tab page */
            Array<SmartPtr<TabItem>> m_tabItems;

            /** Index of the currently active tab */
            u32 m_activeTabIndex;

            /** Content area for displaying tab content */
            SmartPtr<ISharedObject> m_contentArea;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // TabPage_h__
