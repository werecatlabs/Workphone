#ifndef TabItem_h__
#define TabItem_h__

#include <Workphone/Scene/Components/UI/UIComponent.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @class TabItem
         * @brief Represents a single tab item component in a tabbed UI interface.
         *
         * A TabItem encapsulates a label and associated content, and is typically used as a child of a
         * TabView or TabPage. It provides methods for managing its label, content, and properties, as
         * well as for loading and unloading its state.
         */
        class WPCore_API TabItem : public UIComponent
        {
        public:
            static const String labelStr;

            /**
             * @brief Constructs a new TabItem instance.
             */
            TabItem();

            /**
             * @brief Destroys the TabItem instance.
             */
            ~TabItem() override;

            /**
             * @brief Loads the tab item with the specified data.
             * @param data Shared pointer to the data used for loading.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the tab item and releases associated resources.
             * @param data Shared pointer to the data used for unloading.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Gets the child objects associated with this tab item.
             * @return An array of shared pointers to child objects.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Gets the properties of the tab item.
             * @return Shared pointer to the properties object.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Sets the properties of the tab item.
             * @param properties Shared pointer to the properties object.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Checks if the tab item is in a valid state.
             * @return True if valid, false otherwise.
             */
            bool isValid() const override;

            /**
             * @brief Sets the label text for the tab item.
             * @param label The label string to set.
             */
            void setLabel( const String &label );

            /**
             * @brief Gets the label text of the tab item.
             * @return Reference to the label string.
             */
            const String &getLabel() const;

            /**
             * @brief Sets the content object associated with this tab item.
             * @param content Shared pointer to the content object.
             */
            void setContent( SmartPtr<ISharedObject> content );

            /**
             * @brief Gets the content object associated with this tab item.
             * @return Shared pointer to the content object.
             */
            SmartPtr<ISharedObject> getContent() const;

            /** @copydoc UIComponent::updateElementState */
            void updateElementState() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @brief Synchronises the label string to the child Text component. */
            void syncLabelToText();

            void createUI() override;

            String m_label;                     ///< The label text displayed on the tab.
            SmartPtr<ISharedObject> m_content;  ///< The content object associated with the tab.
            SmartPtr<Text> m_textComponent;     ///< Cached child Text component for label display.
        };
    }  // namespace scene
}  // namespace workphone

#endif  // TabItem_h__
