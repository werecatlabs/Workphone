#ifndef ResourceDatabaseDialog_h__
#define ResourceDatabaseDialog_h__
#include "ui/EditorWindow.hpp"
#include <Workphone/Database/AssetCatalogBrowser.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone::editor
{
    /** Bounded, read-only asset picker with an undoable assignment boundary. */
    class ResourceDatabaseDialog : public EditorWindow
    {
    public:
        enum WidgetId { Tree, AddComponent, Select, Previous, Next, Source, Count };
        class UIElementListener : public IEventListener
        {
        public:
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;
            SmartPtr<ResourceDatabaseDialog> getOwner() const;
            void setOwner( SmartPtr<ResourceDatabaseDialog> owner );
            void unload( SmartPtr<ISharedObject> data ) override;
            WP_CLASS_REGISTER_DECL;
        private:
            ObjectObserver<ResourceDatabaseDialog> m_owner;
        };
        class DragSource : public ui::IUIDragSource
        {
        public:
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;
            String handleDrag( const Vector2I &position, SmartPtr<ui::IUIElement> element );
            SmartPtr<ResourceDatabaseDialog> getOwner() const;
            void setOwner( SmartPtr<ResourceDatabaseDialog> owner );
            void unload( SmartPtr<ISharedObject> data ) override;
            WP_CLASS_REGISTER_DECL;
        private:
            ObjectObserver<ResourceDatabaseDialog> m_owner;
        };

        ResourceDatabaseDialog() = default;
        ~ResourceDatabaseDialog() override;
        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void populate();
        void select();
        void selectObject( SmartPtr<ISharedObject> selected, const String &uuid );
        String getSelectedObject() const;
        void setSelectedObject( const String &value );
        SmartPtr<ui::IUITreeCtrl> getTree() const;
        void setTree( SmartPtr<ui::IUITreeCtrl> tree );
        SmartPtr<ISharedObject> getCurrentObject() const;
        void setCurrentObject( SmartPtr<ISharedObject> object );
        String getPropertyName() const;
        void setPropertyName( const String &name );
        void updateSelection() override;
        void setWindowVisible( bool visible ) override;
        hash_type getResourceType() const { return m_resourceType; }
        void setResourceType( hash_type type ) { m_resourceType = type; m_browser.clear(); }
        WP_CLASS_REGISTER_DECL;
    private:
        SmartPtr<AssetDatabaseManager> catalog() const;
        SmartPtr<AssetDatabaseManager> projectCatalog() const;
        bool entryForNode( SmartPtr<ui::IUITreeNode> node, AssetDatabaseManager::EntrySnapshot &entry );
        void renderPage();
        void captureTargets();
        void assign( const Array<SmartPtr<ISharedObject>> &targets,
                     const AssetDatabaseManager::EntrySnapshot &entry );
        hash_type m_resourceType = 0;
        ObjectObserver<ISharedObject> m_currentObject;
        Array<ObjectObserver<ISharedObject>> m_targets;
        String m_propertyName;
        String m_selectedObject;
        AssetCatalogBrowser m_browser;
        AssetCatalogSession m_targetSession;
        SmartPtr<ui::IUITreeCtrl> m_tree;
        SmartPtr<ui::IUITextEntry> m_search;
        SmartPtr<ui::IUIText> m_status;
        SmartPtr<ui::IUIButton> m_refresh;
        SmartPtr<ui::IUIButton> m_select;
        SmartPtr<ui::IUIButton> m_previous;
        SmartPtr<ui::IUIButton> m_next;
        SmartPtr<ui::IUIButton> m_source;
        bool m_showMedia = false;
        SmartPtr<UIElementListener> m_uiListener;
        SmartPtr<DragSource> m_dragSource;
    };
}
#endif
