#ifndef ResourceDatabaseDialog_h__
#define ResourceDatabaseDialog_h__

#include "ui/EditorWindow.hpp"
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/System/IResource.hpp>

namespace workphone
{
    namespace editor
    {
        /**
         * @brief The ResourceDatabaseDialog class represents a dialog window that displays a resource tree for a particular resource type.
         */
        class ResourceDatabaseDialog : public EditorWindow
        {
        public:
            /**
             * @brief The WidgetId enumeration defines identifiers for various widgets used in the dialog.
             */
            enum WidgetId
            {
                Tree,
                ///< Identifier for the tree widget.
                AddComponent,
                ///< Identifier for the "add component" button widget.
                Select,
                ///< Identifier for the "select" button widget.

                Count  ///< Total count of widget identifiers.
            };

            class UIElementListener : public IEventListener
            {
            public:
                UIElementListener();
                ~UIElementListener() override;

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                ResourceDatabaseDialog *getOwner() const;

                void setOwner( ResourceDatabaseDialog *owner );

                WP_CLASS_REGISTER_DECL;

            private:
                ResourceDatabaseDialog *m_owner = nullptr;
            };

            class DragSource : public ui::IUIDragSource
            {
            public:
                DragSource();
                ~DragSource() override;

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                String handleDrag( const Vector2I &position, SmartPtr<ui::IUIElement> element );

                ResourceDatabaseDialog *getOwner() const;

                void setOwner( ResourceDatabaseDialog *owner );

                WP_CLASS_REGISTER_DECL;

            private:
                ResourceDatabaseDialog *m_owner = nullptr;
            };

            /**
             * @brief Constructs a ResourceDatabaseDialog instance.
             */
            ResourceDatabaseDialog();

            /**
             * @brief Destructs the ResourceDatabaseDialog instance.
             */
            ~ResourceDatabaseDialog() override;

            /**
             * @brief Loads the dialog's data.
             *
             * @param data A SmartPtr to the shared data object to load.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the dialog's data.
             *
             * @param data A SmartPtr to the shared data object to unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Adds a resource to the resource tree.
             *
             * @param parent A SmartPtr to the parent tree node to which the resource should be added.
             * @param resource A SmartPtr to the resource to add.
             */
            void addResourceToTree( SmartPtr<ui::IUITreeNode> parent,
                                    SmartPtr<IBuildDirector> resource );

            /**
             * @brief Populates the dialog's tree with resources.
             */
            void populate();

            /**
             * @brief Selects the specified resource in the tree.
             */
            void select();

            /**
             * @brief Selects the specified object by UUID in the resource tree.
             *
             * @param selected A SmartPtr to the selected object.
             * @param uuid The UUID of the selected object.
             */
            void selectObject( SmartPtr<ISharedObject> selected, const String &uuid );

            /**
             * @brief Gets the UUID of the currently selected object.
             *
             * @return The UUID of the currently selected object.
             */
            String getSelectedObject() const;

            /**
             * @brief Sets the UUID of the currently selected object.
             *
             * @param selectedObject The UUID of the object to select.
             */
            void setSelectedObject( const String &selectedObject );

            /**
             * @brief Gets a SmartPtr to the dialog's tree.
             *
             * @return A SmartPtr to the dialog's tree.
             */
            SmartPtr<ui::IUITreeCtrl> getTree() const;

            /**
             * @brief Sets the dialog's tree.
             *
             * @param tree A SmartPtr to the tree to set.
             */
            void setTree( SmartPtr<ui::IUITreeCtrl> tree );

            /**
             * @brief Gets the currently selected object.
             * @return A SmartPtr to the currently selected object.
             */
            SmartPtr<ISharedObject> getCurrentObject() const;

            /**
             * @brief Sets the currently selected object.
             * @param currentObject A SmartPtr to the object to set as currently selected.
             */
            void setCurrentObject( SmartPtr<ISharedObject> currentObject );

            /**
             * @brief Gets the name of the currently selected object's property.
             *
             */
            String getPropertyName() const;
            void setPropertyName( const String &propertyName );

            void updateSelection() override;

            hash_type getResourceType() const
            {
                return m_resourceType;
            }

            void setResourceType( hash_type resourceType )
            {
                m_resourceType = resourceType;
            }

            WP_CLASS_REGISTER_DECL;

        protected:
            void handleTreeSelectionChanged();
            void handleTreeSelectionActivated();

            hash_type m_resourceType = 0;

            std::map<String, SmartPtr<ui::IUITreeNode>> m_treeNodes;

            SmartPtr<ui::IUIWindow> m_buttonWindow;
            SmartPtr<ui::IUIWindow> m_treeWindow;

            SmartPtr<ui::IUITreeCtrl> m_tree;
            SmartPtr<IEventListener> m_uiListener;

            SmartPtr<ui::IUIButton> m_addComponentButton;
            SmartPtr<ui::IUIButton> m_selectButton;

            String m_selectedObject;

            SmartPtr<ISharedObject> m_currentObject;
            String m_propertyName;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // AddBodyDialog_h__
