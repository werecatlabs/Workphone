#ifndef AssetDatabaseWindow_h__
#define AssetDatabaseWindow_h__

#include <EditorPrerequisites.hpp>
#include "EditorWindow.hpp"

namespace workphone
{
    namespace editor
    {
        /**
         * @brief A window interface for managing and editing the asset database.
         *
         * The AssetDatabaseWindow provides a graphical interface for viewing and manipulating
         * assets, resources, and components in the editor. It consists of multiple tabbed views
         * with tree controls for hierarchical data display and data grids for attribute editing.
         *
         * The window is divided into three main sections:
         * - Resources: For managing game assets and resources
         * - Static Components: For handling static component definitions
         * - Components: For managing dynamic component instances
         */
        class AssetDatabaseWindow : public EditorWindow
        {
        public:
            /**
             * @brief Event listener class for handling AssetDatabaseWindow events.
             *
             * This nested class manages event handling for the AssetDatabaseWindow,
             * processing various events related to asset management and UI interactions.
             */
            class EventListener : public IEventListener
            {
            public:
                /**
                 * @brief Constructs a new EventListener instance.
                 */
                EventListener();

                /**
                 * @brief Destructor for the EventListener.
                 */
                ~EventListener() override;

                /**
                 * @brief Unloads the event listener and cleans up resources.
                 * @param data Shared object containing unload data
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handles events for the AssetDatabaseWindow.
                 * @param eventType The type of event
                 * @param eventValue The hash value of the event
                 * @param arguments Array of event parameters
                 * @param sender The object that sent the event
                 * @param object The target object of the event
                 * @param event The event object itself
                 * @return Parameter containing the event handling result
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Gets the owner window of this event listener.
                 * @return Smart pointer to the owner AssetDatabaseWindow
                 */
                SmartPtr<AssetDatabaseWindow> getOwner() const;

                /**
                 * @brief Sets the owner window for this event listener.
                 * @param owner Smart pointer to the AssetDatabaseWindow to set as owner
                 */
                void setOwner( SmartPtr<AssetDatabaseWindow> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                AtomicWeakPtr<AssetDatabaseWindow> m_owner;  ///< Weak reference to the owner window
            };

            /**
             * @brief Constructs a new AssetDatabaseWindow instance.
             */
            AssetDatabaseWindow();

            /**
             * @brief Destructor for the AssetDatabaseWindow.
             */
            ~AssetDatabaseWindow() override;

            /**
             * @brief Loads the window and initializes its components.
             * @param data Shared object containing load data
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the window and cleans up resources.
             * @param data Shared object containing unload data
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Gets the asset database manager instance.
             * @return Smart pointer to the AssetDatabaseManager
             */
            SmartPtr<AssetDatabaseManager> getAssetDatabaseManager() const;

            /**
             * @brief Sets the asset database manager for this window.
             * @param assetDatabaseManager Smart pointer to the AssetDatabaseManager to set
             */
            void setAssetDatabaseManager( SmartPtr<AssetDatabaseManager> assetDatabaseManager );

            /**
             * @brief Gets the tab bar control.
             * @return Smart pointer to the tab bar UI control
             */
            SmartPtr<ui::IUITabBar> getTabBar() const;

            /**
             * @brief Sets the tab bar control.
             * @param tabBar Smart pointer to the tab bar UI control to set
             */
            void setTabBar( SmartPtr<ui::IUITabBar> tabBar );

            /**
             * @brief Gets the resource tree control.
             * @return Smart pointer to the resource tree UI control
             */
            SmartPtr<ui::IUITreeCtrl> getResourceTreeCtrl() const;

            /**
             * @brief Sets the resource tree control.
             * @param resourceTreeCtrl Smart pointer to the resource tree UI control to set
             */
            void setResourceTreeCtrl( SmartPtr<ui::IUITreeCtrl> resourceTreeCtrl );

            /**
             * @brief Gets the resource attributes grid.
             * @return Smart pointer to the resource attributes data grid
             */
            SmartPtr<ui::IUIDataGrid> getResourceAttribsGrid() const;

            /**
             * @brief Sets the resource attributes grid.
             * @param resourceAttribsGrid Smart pointer to the resource attributes data grid to set
             */
            void setResourceAttribsGrid( SmartPtr<ui::IUIDataGrid> resourceAttribsGrid );

            /**
             * @brief Gets the static components tree control.
             * @return Smart pointer to the static components tree UI control
             */
            SmartPtr<ui::IUITreeCtrl> getStaticComponentsTreeCtrl() const;

            /**
             * @brief Sets the static components tree control.
             * @param staticComponentsTreeCtrl Smart pointer to the static components tree UI control to set
             */
            void setStaticComponentsTreeCtrl( SmartPtr<ui::IUITreeCtrl> staticComponentsTreeCtrl );

            /**
             * @brief Gets the static components attributes grid.
             * @return Smart pointer to the static components attributes data grid
             */
            SmartPtr<ui::IUIDataGrid> getStaticComponentsAttribsGrid() const;

            /**
             * @brief Sets the static components attributes grid.
             * @param staticComponentsAttribsGrid Smart pointer to the static components attributes data grid to set
             */
            void setStaticComponentsAttribsGrid( SmartPtr<ui::IUIDataGrid> staticComponentsAttribsGrid );

            /**
             * @brief Gets the components tree control.
             * @return Smart pointer to the components tree UI control
             */
            SmartPtr<ui::IUITreeCtrl> getComponentsTreeCtrl() const;

            /**
             * @brief Sets the components tree control.
             * @param componentsTreeCtrl Smart pointer to the components tree UI control to set
             */
            void setComponentsTreeCtrl( SmartPtr<ui::IUITreeCtrl> componentsTreeCtrl );

            /**
             * @brief Gets the components attributes grid.
             * @return Smart pointer to the components attributes data grid
             */
            SmartPtr<ui::IUIDataGrid> getComponentsAttribsGrid() const;

            /**
             * @brief Sets the components attributes grid.
             * @param componentsAttribsGrid Smart pointer to the components attributes data grid to set
             */
            void setComponentsAttribsGrid( SmartPtr<ui::IUIDataGrid> componentsAttribsGrid );

            /**
             * @brief Handles events for the window.
             * @param eventType The type of event
             * @param eventValue The hash value of the event
             * @param arguments Array of event parameters
             * @param sender The object that sent the event
             * @param object The target object of the event
             * @param event The event object itself
             * @return Parameter containing the event handling result
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Builds the resource tree structure.
             * @param treeCtrl The tree control to populate with resources
             */
            void buildResourceTree( SmartPtr<ui::IUITreeCtrl> treeCtrl );

            void buildStaticComponentsTree( SmartPtr<ui::IUITreeCtrl> treeCtrl );

            /**
             * @brief Builds the resource attributes display.
             * @param grid The data grid to populate with attributes
             * @param id The ID of the resource to display attributes for
             */
            void buildResourceAttribs( SmartPtr<ui::IUIDataGrid> grid, s32 id );

            void buildStaticComponentAttribs( SmartPtr<ui::IUIDataGrid> grid, s32 id );

            void buildComponentAttribs( SmartPtr<ui::IUIDataGrid> grid, s32 id );

            /**
             * @brief Builds the components tree structure.
             * @param treeCtrl The tree control to populate with components
             */
            void buildComponentsTree( SmartPtr<ui::IUITreeCtrl> treeCtrl );

            mutable SmartPtr<AssetDatabaseManager>
                m_assetDatabaseManager;  ///< Manager for asset database operations

            SmartPtr<ui::IUITabBar> m_tabBar;  ///< Tab bar for switching between different views

            SmartPtr<ui::IUITreeCtrl> m_resourceTreeCtrl;  ///< Tree control for displaying resources
            SmartPtr<ui::IUIDataGrid>
                m_resourceAttribsGrid;  ///< Grid for displaying resource attributes

            SmartPtr<ui::IUITreeCtrl>
                m_staticComponentsTreeCtrl;  ///< Tree control for displaying static components
            SmartPtr<ui::IUIDataGrid>
                m_staticComponentsAttribsGrid;  ///< Grid for displaying static component attributes

            SmartPtr<ui::IUITreeCtrl>
                m_componentsTreeCtrl;  ///< Tree control for displaying dynamic components
            SmartPtr<ui::IUIDataGrid>
                m_componentsAttribsGrid;  ///< Grid for displaying component attributes

            String m_staticAttribsTableName = "object_attributes";
        };
    }  // namespace editor
}  // namespace workphone

#endif  // AssetDatabaseWindow_h__
