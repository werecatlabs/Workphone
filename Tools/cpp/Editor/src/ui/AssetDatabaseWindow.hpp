#ifndef AssetDatabaseWindow_h__
#define AssetDatabaseWindow_h__
#include <EditorPrerequisites.hpp>
#include "EditorWindow.hpp"
#include <Workphone/Database/AssetCatalogBrowser.hpp>
#include <Workphone/Database/AssetOperationBrowser.hpp>
#include <Workphone/Database/AssetDocumentInventory.hpp>

namespace workphone::editor
{
    /** Bounded catalog inspection and guarded project asset controls. */
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
            ObjectObserver<AssetDatabaseWindow> m_owner;
        };

        ~AssetDatabaseWindow() override;
        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        SmartPtr<AssetDatabaseManager> getAssetDatabaseManager() const;
        void setAssetDatabaseManager( SmartPtr<AssetDatabaseManager> manager );
        void setMediaCatalog( SmartPtr<AssetDatabaseManager> catalog );
        void refresh();
        void setWindowVisible( bool visible ) override;
        Parameter handleEvent( EventType eventType, hash_type eventValue,
                               const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                               SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;
        WP_CLASS_REGISTER_DECL;

    private:
        void populate();
        void populateOperations();
        void populateDocuments();
        SmartPtr<AssetDatabaseManager> m_assetDatabaseManager;
        SmartPtr<AssetDatabaseManager> m_mediaCatalog;
        bool m_showMedia = false;
        SmartPtr<ui::IUITreeCtrl> m_tree;
        SmartPtr<ui::IUITextEntry> m_search;
        SmartPtr<ui::IUITextEntry> m_type;
        SmartPtr<ui::IUIText> m_status;
        SmartPtr<ui::IUIText> m_details;
        SmartPtr<ui::IUIButton> m_refresh;
        SmartPtr<ui::IUIButton> m_previous;
        SmartPtr<ui::IUIButton> m_next;
        SmartPtr<ui::IUIButton> m_source;
        SmartPtr<ui::IUIButton> m_history;
        SmartPtr<ui::IUIButton> m_restore;
        SmartPtr<ui::IUIButton> m_reindex;
        SmartPtr<ui::IUIButton> m_scanDocuments, m_reindexDocuments;
        SmartPtr<ui::IUIButton> m_shipRuntime, m_shipEditorOnly, m_shipExcluded;
        AssetDocumentInventory m_documentInventory;
        bool m_showDocuments = false, m_documentCanApply = false;
        size_t m_documentPage = 0;
        bool m_showOperations = false;
        size_t m_selectedOperation = static_cast<size_t>( -1 );
        AssetOperationPage m_operations;
        Array<String> m_operationCursors;
        String m_operationAfter;
        AssetCatalogBrowser m_browser;
    };
}  // namespace workphone::editor
#endif
