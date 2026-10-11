#ifndef AssetDatabaseWindow_h__
#define AssetDatabaseWindow_h__
#include <EditorPrerequisites.hpp>
#include "EditorWindow.hpp"
#include <Workphone/Database/AssetCatalogBrowser.hpp>

namespace workphone::editor
{
    /** Read-only browser of the active project's authored asset catalog. */
    class AssetDatabaseWindow : public EditorWindow
    {
    public:
        class EventListener : public IEventListener
        {
        public:
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;
            void unload( SmartPtr<ISharedObject> data ) override;
            void setOwner( SmartPtr<AssetDatabaseWindow> owner );
            WP_CLASS_REGISTER_DECL;
        private:
            AtomicWeakPtr<AssetDatabaseWindow> m_owner;
        };
        ~AssetDatabaseWindow() override;
        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        SmartPtr<AssetDatabaseManager> getAssetDatabaseManager() const;
        void setAssetDatabaseManager( SmartPtr<AssetDatabaseManager> manager );
        void refresh();
        void setWindowVisible( bool visible ) override;
        Parameter handleEvent( EventType eventType, hash_type eventValue,
            const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
            SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;
        WP_CLASS_REGISTER_DECL;
    private:
        void populate();
        SmartPtr<AssetDatabaseManager> m_assetDatabaseManager;
        SmartPtr<ui::IUITreeCtrl> m_tree;
        SmartPtr<ui::IUITextEntry> m_search;
        SmartPtr<ui::IUITextEntry> m_type;
        SmartPtr<ui::IUIText> m_status;
        SmartPtr<ui::IUIText> m_details;
        SmartPtr<ui::IUIButton> m_refresh;
        SmartPtr<ui::IUIButton> m_previous;
        SmartPtr<ui::IUIButton> m_next;
        AssetCatalogBrowser m_browser;
    };
}
#endif
