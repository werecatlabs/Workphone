//
// Created by Zane Desir on 11/11/2021.
//

#ifndef WP_ResourceWindow_H
#define WP_ResourceWindow_H

#include <EditorPrerequisites.hpp>
#include <ui/EditorWindow.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone
{
    namespace editor
    {
        class ResourceWindow : public EditorWindow
        {
        public:
            class TreeCtrlListener : public IEventListener
            {
            public:
                TreeCtrlListener();
                ~TreeCtrlListener() override;

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                void handleTreeSelectionChanged( SmartPtr<ui::IUITreeNode> node );

                void handleTreeSelectionActivated( SmartPtr<ui::IUITreeNode> node );

                void handleTreeDragStart( SmartPtr<ui::IUITreeNode> node );

                ResourceWindow *getOwner() const;

                void setOwner( ResourceWindow *owner );

                WP_CLASS_REGISTER_DECL;

            private:
                ResourceWindow *m_owner = nullptr;
            };

            ResourceWindow( SmartPtr<ui::IUIWindow> parent );
            ~ResourceWindow() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void updateSelection() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            void buildTree();

            void addMaterialToTree( SmartPtr<render::IMaterial> material,
                                    SmartPtr<ui::IUITreeNode> node );

            void addObjectToTree( SmartPtr<ISharedObject> object, SmartPtr<ui::IUITreeNode> node );

            // SmartPtr<ui::IUIDropdown> m_dropdown;

            SmartPtr<ui::IUITreeCtrl> m_tree;
            SmartPtr<IEventListener> m_treeListener;
            SmartPtr<render::IMaterial> m_material;

            SmartPtr<PropertiesWindow> m_propertiesWindow;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // WP_MATERIALWINDOW_H
