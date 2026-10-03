/**
 * @file SceneWindow.hpp
 * @brief Header file for the SceneWindow class which provides scene management functionality in the editor.
 */

#ifndef __SceneWindow_h__
#define __SceneWindow_h__

#include "ui/EditorWindow.hpp"
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/UI/IUIDropTarget.hpp>
#include <Workphone/Interface/UI/IUIDragSource.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Memory/AtomicSharedPtr.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        /**
         * @class SceneWindow
         * @brief Represents a scene window in the editor that manages scene hierarchy and interactions.
         *
         * This class provides functionality for managing scene objects, handling drag and drop operations,
         * and maintaining the scene tree structure.
         */
        class SceneWindow : public EditorWindow
        {
        public:
            /** Coalesced primary-task job used to rebuild UI-owned hierarchy state safely. */
            class BuildTreeJob : public Job
            {
            public:
                BuildTreeJob();
                ~BuildTreeJob() override;

                void execute() override;

                SmartPtr<SceneWindow> getOwner() const;
                void setOwner( SmartPtr<SceneWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<SceneWindow> m_owner;
            };

            /**
             * @class ApplicationEventListener
             * @brief Listener class for handling application-level events in the scene window.
             */
            class ApplicationEventListener : public IEventListener
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                ApplicationEventListener();

                /**
                 * @brief Destructor.
                 */
                ~ApplicationEventListener() override;

                /**
                 * @brief Unloads the event listener.
                 * @param data Shared object data to unload.
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handles application events.
                 * @param eventType Type of the event.
                 * @param eventValue Hash value of the event.
                 * @param arguments Array of event parameters.
                 * @param sender The event sender.
                 * @param object The event object.
                 * @param event The event itself.
                 * @return Parameter containing the event handling result.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Gets the owner scene window.
                 * @return Smart pointer to the owner scene window.
                 */
                SmartPtr<SceneWindow> getOwner() const;

                /**
                 * @brief Sets the owner scene window.
                 * @param owner The scene window to set as owner.
                 */
                void setOwner( SmartPtr<SceneWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<SceneWindow> m_owner;  ///< Weak pointer to the owner scene window.
            };

            /**
             * @class TreeCtrlListener
             * @brief Listener class for handling tree control events in the scene window.
             */
            class TreeCtrlListener : public IEventListener
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                TreeCtrlListener();

                /**
                 * @brief Destructor.
                 */
                ~TreeCtrlListener() override;

                /**
                 * @brief Handles tree control events.
                 * @param eventType Type of the event.
                 * @param eventValue Hash value of the event.
                 * @param arguments Array of event parameters.
                 * @param sender The event sender.
                 * @param object The event object.
                 * @param event The event itself.
                 * @return Parameter containing the event handling result.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Gets the owner scene window.
                 * @return Smart pointer to the owner scene window.
                 */
                SmartPtr<SceneWindow> getOwner() const;

                /**
                 * @brief Sets the owner scene window.
                 * @param owner The scene window to set as owner.
                 */
                void setOwner( SmartPtr<SceneWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<SceneWindow> m_owner;  ///< Weak pointer to the owner scene window.
            };

            /**
             * @class SceneWindowListener
             * @brief Listener class for handling scene window specific events.
             */
            class SceneWindowListener : public IEventListener
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                SceneWindowListener();

                /**
                 * @brief Destructor.
                 */
                ~SceneWindowListener() override;

                /**
                 * @brief Handles scene window events.
                 * @param eventType Type of the event.
                 * @param eventValue Hash value of the event.
                 * @param arguments Array of event parameters.
                 * @param sender The event sender.
                 * @param object The event object.
                 * @param event The event itself.
                 * @return Parameter containing the event handling result.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Gets the owner scene window.
                 * @return Smart pointer to the owner scene window.
                 */
                SmartPtr<SceneWindow> getOwner() const;

                /**
                 * @brief Sets the owner scene window.
                 * @param owner The scene window to set as owner.
                 */
                void setOwner( SmartPtr<SceneWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<SceneWindow> m_owner;  ///< Weak pointer to the owner scene window.
            };

            /**
             * @class PromptListener
             * @brief Listener class for handling prompt events in the scene window.
             */
            class PromptListener : public IEventListener
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                PromptListener();

                /**
                 * @brief Destructor.
                 */
                ~PromptListener() override;

                /**
                 * @brief Handles prompt events.
                 * @param eventType Type of the event.
                 * @param eventValue Hash value of the event.
                 * @param arguments Array of event parameters.
                 * @param sender The event sender.
                 * @param object The event object.
                 * @param event The event itself.
                 * @return Parameter containing the event handling result.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Gets the owner scene window.
                 * @return Smart pointer to the owner scene window.
                 */
                SmartPtr<SceneWindow> getOwner() const;

                /**
                 * @brief Sets the owner scene window.
                 * @param owner The scene window to set as owner.
                 */
                void setOwner( SmartPtr<SceneWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<SceneWindow> m_owner;  ///< Weak pointer to the owner scene window.
            };

            /**
             * @class DragSource
             * @brief Class for handling drag operations in the scene window.
             */
            class DragSource : public ui::IUIDragSource
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                DragSource();

                /**
                 * @brief Destructor.
                 */
                ~DragSource() override;

                /**
                 * @brief Handles drag events.
                 * @param eventType Type of the event.
                 * @param eventValue Hash value of the event.
                 * @param arguments Array of event parameters.
                 * @param sender The event sender.
                 * @param object The event object.
                 * @param event The event itself.
                 * @return Parameter containing the event handling result.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Handles drag operations.
                 * @param position The drag position.
                 * @param element The UI element being dragged.
                 * @return String containing drag operation result.
                 */
                String handleDrag( const Vector2I &position, SmartPtr<ui::IUIElement> element );

                /**
                 * @brief Gets the owner scene window.
                 * @return Smart pointer to the owner scene window.
                 */
                SmartPtr<SceneWindow> getOwner() const;

                /**
                 * @brief Sets the owner scene window.
                 * @param owner The scene window to set as owner.
                 */
                void setOwner( SmartPtr<SceneWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<SceneWindow> m_owner;  ///< Weak pointer to the owner scene window.
            };

            /**
             * @class DropTarget
             * @brief Class for handling drop operations in the scene window.
             */
            class DropTarget : public ui::IUIDropTarget
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                DropTarget();

                /**
                 * @brief Destructor.
                 */
                ~DropTarget() override;

                /**
                 * @brief Handles drop events.
                 * @param eventType Type of the event.
                 * @param eventValue Hash value of the event.
                 * @param arguments Array of event parameters.
                 * @param sender The event sender.
                 * @param object The event object.
                 * @param event The event itself.
                 * @return Parameter containing the event handling result.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Gets the owner scene window.
                 * @return Smart pointer to the owner scene window.
                 */
                SmartPtr<SceneWindow> getOwner() const;

                /**
                 * @brief Sets the owner scene window.
                 * @param owner The scene window to set as owner.
                 */
                void setOwner( SmartPtr<SceneWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<SceneWindow> m_owner;  ///< Weak pointer to the owner scene window.
            };

            /**
             * @brief Enumeration for tree item states.
             */
            enum
            {
                TREE_ITEM_STATE_NOT_FOUND = 0,    ///< Item not found in tree
                TREE_ITEM_STATE_EXPANDED = 1,     ///< Item is expanded
                TREE_ITEM_STATE_NOT_EXPANDED = 2  ///< Item is not expanded
            };

            /**
             * @brief Enumeration for menu IDs used in the SceneWindow.
             */
            enum class MenuId
            {
                ADD_SCRIPT_ID,             ///< Add script menu item
                ADD_NEW_ENTITY,            ///< Add new entity menu item
                ADD_SKYBOX,                ///< Add skybox menu item
                ADD_NEW_TERRAIN,           ///< Add new terrain menu item
                ADD_CAMERA,                ///< Add camera menu item
                ADD_CAR,                   ///< Add car menu item
                ADD_HELICOPTER,            ///< Add helicopter menu item
                ADD_PLANE,                 ///< Add aircraft plane menu item
                ADD_CONSTRAINT,            ///< Add constraint menu item
                ADD_PARTICLESYSTEM,        ///< Add particle system menu item
                ADD_PARTICLESYSTEM_SMOKE,  ///< Add smoke particle system menu item
                ADD_PARTICLESYSTEM_SAND,   ///< Add sand particle system menu item
                ADD_PLANE_MESH,            ///< Add plane mesh menu item
                ADD_CUBE,                  ///< Add cube menu item
                ADD_CUBE_MESH,             ///< Add cube mesh menu item
                ADD_CUBEMAP,               ///< Add cubemap menu item
                ADD_PHYSICS_CUBE,          ///< Add physics cube menu item
                ADD_DIRECTIONAL_LIGHT,     ///< Add directional light menu item
                ADD_POINT_LIGHT,           ///< Add point light menu item

                ADD_BUTTON,              ///< Add button menu item
                ADD_SIMPLE_BUTTON,       ///< Add simple button menu item
                ADD_CANVAS,              ///< Add canvas menu item
                ADD_CHECKBOX,            ///< Add checkbox menu item
                ADD_DROPDOWN,            ///< Add dropdown menu item
                ADD_PANEL,               ///< Add panel menu item
                ADD_SCROLLBAR,           ///< Add scrollbar menu item
                ADD_SCROLLBAR_VERTICAL,  ///< Add vertical scrollbar menu item
                ADD_SCROLLVIEW,          ///< Add scrollview menu item
                ADD_SLIDER,              ///< Add slider menu item
                ADD_SLIDER_VERTICAL,     ///< Add vertical slider menu item
                ADD_TABVIEW,             ///< Add tabview menu item
                ADD_TABLELAYOUT,         ///< Add table layout menu item
                ADD_TEXT,                ///< Add text menu item
                ADD_TOGGLE_BUTTON,       ///< Add toggle button menu item
                ADD_TOGGLE_WITH_TEXT,    ///< Add toggle with text menu item

                SCENE_REMOVE_ACTOR,  ///< Remove actor menu item
                SCENE_REFRESH,       ///< Refresh scene menu item

                SEND_PROMPT,  ///< Send prompt menu item

                ADD_RENDER_TARGET,  ///< Add render-target texture menu item

                COUNT  ///< Total count of menu items
            };

            /**
             * @brief Default constructor.
             */
            SceneWindow();

            /**
             * @brief Constructor with parent window.
             * @param parent The parent window.
             */
            SceneWindow( SmartPtr<ui::IUIWindow> parent );

            /**
             * @brief Destructor.
             */
            ~SceneWindow() override;

            /**
             * @brief Loads the specified data.
             * @param data The data to load.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the specified data.
             * @param data The data to unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Builds the scene tree.
             */
            void buildTree();

            /** Applies a case-insensitive filter to the hierarchy without changing the scene. */
            void applySearchFilter( const String &filter );

            /**
             * @brief Adds an object to the tree.
             * @param object The object to add.
             * @param parentNode The parent node.
             */
            void addObjectToTree( SmartPtr<ISharedObject> object, SmartPtr<ui::IUITreeNode> parentNode );

            /**
             * @brief Adds an actor to the scene tree.
             * @param actor The actor to add.
             * @param parentNode The parent node.
             */
            void addActorToTree( SmartPtr<scene::IGameActor> actor,
                                 SmartPtr<ui::IUITreeNode> parentNode );

            /**
             * @brief Saves the state of the scene tree.
             */
            void saveTreeState();

            /**
             * @brief Restores the state of the scene tree.
             */
            void restoreTreeState();

            /**
             * @brief Gets the selected object.
             * @return The selected object.
             */
            SmartPtr<ISharedObject> getSelectedObject() const;

            /**
             * @brief Sets the selected object.
             * @param selectedObject The object to select.
             */
            void setSelectedObject( SmartPtr<ISharedObject> selectedObject );

            /**
             * @brief Deselects all objects.
             */
            void deselectAll();

            /**
             * @brief Checks if the window is valid.
             * @return true if the window is valid, false otherwise.
             */
            bool isValid() const override;

            /**
             * @brief Gets the scene tree control.
             * @return The scene tree control.
             */
            SmartPtr<ui::IUITreeCtrl> getTree() const;

            /**
             * @brief Sets the scene tree control.
             * @param tree The tree control to set.
             */
            void setTree( SmartPtr<ui::IUITreeCtrl> tree );

            /**
             * @brief Gets the state of the scene tree.
             * @return The state of the scene tree.
             */
            SharedPtr<std::map<String, bool>> getTreeState() const;

            /**
             * @brief Sets the state of the scene tree.
             * @param treeState The state of the scene tree.
             */
            void setTreeState( SharedPtr<std::map<String, bool>> treeState );

            /**
             * @brief Gets the drag and drop actor command.
             * @return The drag and drop actor command.
             */
            SmartPtr<ICommand> getDragDropActorCmd() const;

            /**
             * @brief Sets the drag and drop actor command.
             * @param dragDropActorCmd The drag and drop actor command.
             */
            void setDragDropActorCmd( SmartPtr<ICommand> dragDropActorCmd );

            /**
             * @brief Gets the drop job.
             * @return The drop job.
             */
            SmartPtr<IJob> getDropJob() const;

            /**
             * @brief Sets the drop job.
             * @param dropJob The drop job to set.
             */
            void setDropJob( SmartPtr<IJob> dropJob );

            /**
             * @brief Gets the build job.
             * @return The build job.
             */
            SmartPtr<IJob> getBuildJob() const;

            /**
             * @brief Sets the build job.
             * @param buildJob The build job to set.
             */
            void setBuildJob( SmartPtr<IJob> buildJob );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Rebuilds the hierarchy on the primary/UI task. */
            void rebuildTree();

            /** Returns true when this node or one of its descendants matches the active filter. */
            bool updateNodeFilter( SmartPtr<ui::IUITreeNode> node, const String &filter );

            /** Restores the hierarchy highlight from the editor's authoritative selection. */
            void restoreSelection();

            void restoreSelection( SmartPtr<ui::IUITreeNode> node,
                                   const Array<SmartPtr<ISharedObject>> &selection );

            /** Builds a stable expansion-state key from object UUIDs with a path fallback. */
            String getTreeItemStateKey( SmartPtr<ui::IUITreeNode> parent,
                                        SmartPtr<ui::IUITreeNode> node ) const;

            /**
             * @brief Handles window click events.
             */
            void handleWindowClicked();

            /**
             * @brief Handles tree selection change events.
             * @param node The selected node.
             */
            void handleTreeSelectionChanged( SmartPtr<ui::IUITreeNode> node );

            /**
             * @brief Gets the state of an item.
             * @param itemName The name of the item.
             * @return The state of the item.
             */
            s32 getItemState( const String &itemName ) const;

            /**
             * @brief Saves the state of an item.
             * @param parent The parent node.
             * @param node The node to save.
             */
            void saveItemState( SmartPtr<ui::IUITreeNode> parent, SmartPtr<ui::IUITreeNode> node );

            /**
             * @brief Restores the state of an item.
             * @param parent The parent node.
             * @param node The node to restore.
             * @param parentWasNew Whether the parent was new.
             */
            void restoreItemState( SmartPtr<ui::IUITreeNode> parent, SmartPtr<ui::IUITreeNode> node,
                                   bool parentWasNew );

            RecursiveMutex m_buildTreeMutex;  ///< Mutex for building tree operations
            RecursiveMutex m_treeStateMutex;  ///< Mutex for tree state operations

            SmartPtr<IJob> m_dropJob;   ///< Job for handling drops
            SmartPtr<IJob> m_buildJob;  ///< Job for building operations

            SmartPtr<ui::IUIWindow> m_window;       ///< Main window
            SmartPtr<ui::IUIWindow> m_sceneWindow;  ///< Scene window

            SmartPtr<IEventListener> m_applicationEventListener;  ///< Application event listener

            SmartPtr<ui::IUIButton> m_inputTextButton;  ///< Input text button
            SmartPtr<ui::IUITextEntry> m_inputText;     ///< Input text entry
            SmartPtr<IEventListener> m_promptListener;  ///< Prompt listener

            SmartPtr<ui::IUITextEntry> m_searchEntry;  ///< Hierarchy search entry
            String m_searchFilter;                     ///< Normalized hierarchy filter

            SmartPtr<ui::IUITreeCtrl> m_tree;         ///< Tree control
            SmartPtr<IEventListener> m_treeListener;  ///< Tree listener

            SmartPtr<ui::IUIMenu> m_applicationMenu;     ///< Application menu
            SmartPtr<ui::IUIMenu> m_applicationAddMenu;  ///< Application add menu
            SmartPtr<ui::IUIMenu> m_applicationParticleMenu;  ///< Particle preset menu

            SmartPtr<IEventListener> m_menuListener;  ///< Menu listener

            SmartPtr<ISharedObject> m_selectedObject;  ///< Selected object
            SmartPtr<ISharedObject> m_selectedEntity;  ///< Selected entity

            SmartPtr<ICommand> m_dragDropActorCmd;  ///< Drag and drop actor command

            time_interval m_nodeSelectTime = 0.0;  ///< Node selection time

            AtomicSharedPtr<std::map<String, bool>> m_treeState;  ///< Tree state
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // EntityWindow_h__
