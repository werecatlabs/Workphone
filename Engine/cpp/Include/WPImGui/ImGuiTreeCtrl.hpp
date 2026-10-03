#ifndef __ImGuiTreeCtrl_h__
#define __ImGuiTreeCtrl_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUITreeCtrl.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>

namespace workphone
{
    namespace ui
    {
        /**
         * @brief ImGui implementation of a UI tree control.
         *
         * This class adapts the engine's `IUITreeCtrl` interface to an ImGui-based
         * UI element. It owns and manages tree nodes, selection state (including
         * multi-selection), drag & drop source/destination elements and basic
         * traversal utilities.
         *
         * Thread-safety:
         * - The container of nodes uses `ConcurrentArray` and many members are
         *   atomic smart pointers to allow safe access from different threads.
         * - Non-const operations on the control should generally be performed on
         *   the UI thread. Atomic members protect simple concurrent reads/writes.
         *
         * Usage:
         * - Use `addRoot`/`addNode` to create nodes.
         * - Use `getSelectedTreeNode`/`getSelectedTreeNodes` to query selection.
         * - Use `setMultiSelect(true|false)` to enable/disable multi-selection.
         */
        class ImGuiTreeCtrl : public ImGuiElement<IUITreeCtrl>
        {
        public:
            /**
             * @brief Internal state listener for handling state messages.
             *
             * Subclasses may override behavior by deriving from this listener class.
             */
            class StateListener : public BaseStateListener
            {
            public:
                /**
                 * @brief Handle a state message.
                 *
                 * Override of BaseStateListener::handleStateMessage to react to
                 * messages relevant to this tree control (e.g. selection changes).
                 *
                 * @param message State message to handle.
                 * @return true if the message was handled and no further processing is required.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
            };

            /**
             * @brief Construct a new ImGuiTreeCtrl.
             *
             * Initializes internal state. Default construction leaves an empty tree.
             */
            ImGuiTreeCtrl();

            /**
             * @brief Destroy the ImGuiTreeCtrl.
             *
             * Releases resources and unregisters listeners. Override ensures proper
             * cleanup in derived classes.
             */
            ~ImGuiTreeCtrl() override;

            /**
             * @brief Load the tree control from shared object data.
             *
             * @param data Data source used to initialise the control (may be nullptr).
             *
             * @copydoc ISharedObject::load
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload the tree control and release resources.
             *
             * @param data Optional unload data passed from the owner.
             *
             * @copydoc ISharedObject::unload
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Create and add a root-level tree node.
             *
             * The returned node is owned by the control and also stored in the
             * internal node list.
             *
             * @return SmartPtr<IUITreeNode> New root node.
             */
            SmartPtr<IUITreeNode> addRoot() override;

            /**
             * @brief Create and add a child node (implementation dependent).
             *
             * Typically this creates a new node that can be appended to the currently
             * selected node or root depending on UI behaviour.
             *
             * @return SmartPtr<IUITreeNode> New node.
             */
            SmartPtr<IUITreeNode> addNode() override;

            /**
             * @brief Expand the specified node so its children are visible.
             *
             * @param node Node to expand. If `node` is null this is a no-op.
             */
            void expand( SmartPtr<IUITreeNode> node ) override;

            /**
             * @brief Remove all nodes and reset selection state.
             *
             * Clears both the internal node containers and any selection metadata.
             */
            void clear() override;

            /**
             * @brief Get a copy of the current tree nodes.
             *
             * The result is a snapshot copy of the internal `ConcurrentArray`.
             *
             * @return Array<SmartPtr<IUITreeNode>> Vector of nodes.
             */
            Array<SmartPtr<IUITreeNode>> getTreeNodes() const override;

            /**
             * @brief Replace the current root-level nodes with `treeNodes`.
             *
             * The control takes ownership of references contained in the array.
             *
             * @param treeNodes New node list.
             */
            void setTreeNodes( Array<SmartPtr<IUITreeNode>> treeNodes ) override;

            /**
             * @brief Get the currently selected nodes (multi-select).
             *
             * @return Array<SmartPtr<IUITreeNode>> Selected nodes in selection order.
             */
            Array<SmartPtr<IUITreeNode>> getSelectedTreeNodes() const override;

            /**
             * @brief Set the current selection to `treeNodes`.
             *
             * Replaces the existing selection set.
             *
             * @param treeNodes Nodes to select.
             */
            void setSelectedTreeNodes( Array<SmartPtr<IUITreeNode>> treeNodes ) override;

            /**
             * @brief Add a single node to the current selection.
             *
             * If multi-select is disabled, this will replace the existing selection.
             *
             * @param treeNode Node to add to selection.
             */
            void addSelectedTreeNode( SmartPtr<IUITreeNode> treeNode ) override;

            /**
             * @brief Clear the selection set.
             */
            void clearSelectedTreeNodes() override;

            /**
             * @brief Get a single selected node (commonly primary selection).
             *
             * When multi-select is enabled this typically returns the last or primary
             * selected node. May return nullptr if no selection exists.
             *
             * @return SmartPtr<IUITreeNode> Selected node or nullptr.
             */
            SmartPtr<IUITreeNode> getSelectedTreeNode() const override;

            /**
             * @brief Set the primary selected node (and optionally update selection set).
             *
             * @param treeNode Node to mark as selected.
             */
            void setSelectedTreeNode( SmartPtr<IUITreeNode> treeNode ) override;

            /**
             * @brief Get the root node of the tree.
             *
             * @return SmartPtr<IUITreeNode> Root node (may be null).
             */
            SmartPtr<IUITreeNode> getRoot() const override;

            /**
             * @brief Set the root node for the tree control.
             *
             * @param root Root node smart pointer.
             */
            void setRoot( SmartPtr<IUITreeNode> root ) override;

            /**
             * @brief Query whether multiple nodes can be selected.
             *
             * @return true if multi-select is enabled.
             */
            bool isMultiSelect() const override;

            /**
             * @brief Enable or disable multi-selection.
             *
             * @param multiSelect true to allow multiple node selection.
             */
            void setMultiSelect( bool multiSelect ) override;

            /**
             * @brief Get the element currently acting as a drag source.
             *
             * Used by drag & drop operations.
             *
             * @return SmartPtr<IUIElement> Drag source element or null.
             */
            SmartPtr<IUIElement> getDragSourceElement() const override;

            /**
             * @brief Set the element that is the drag source.
             *
             * @param dragSource Element initiating a drag operation.
             */
            void setDragSourceElement( SmartPtr<IUIElement> dragSource ) override;

            /**
             * @brief Get the element currently acting as a drop destination.
             *
             * @return SmartPtr<IUIElement> Drop destination element or null.
             */
            SmartPtr<IUIElement> getDropDestinationElement() const override;

            /**
             * @brief Set the element that is the drop destination.
             *
             * @param dropDestination Target element for a drop operation.
             */
            void setDropDestinationElement( SmartPtr<IUIElement> dropDestination ) override;

            /**
             * @brief Get the sibling index of the currently selected node.
             *
             * Returns -1 if no valid sibling index is selected.
             *
             * @return s32 Sibling index or -1.
             */
            s32 getSelectedSiblingIndex() const override;

            /**
             * @brief Set the sibling index for the current selection.
             *
             * @param siblingIndex Index to set.
             */
            void setSelectedSiblingIndex( s32 siblingIndex ) override;

            /**
             * @brief Get the last selected node (weak reference).
             *
             * May return `nullptr` if the last selection has been destroyed.
             *
             * @return SmartPtr<IUITreeNode> Last selected node or null.
             */
            SmartPtr<IUITreeNode> getLastSelectedNode() const;

            /**
             * @brief Set the last selected node.
             *
             * This stores a weak reference used for range selection operations.
             *
             * @param lastSelectedNode Node to store as last selected.
             */
            void setLastSelectedNode( SmartPtr<IUITreeNode> lastSelectedNode );

            /**
             * @brief Select a continuous range of nodes between node1 and node2.
             *
             * The algorithm will walk children under `root` and select nodes in the
             * inclusive range. The `in_range` flag is used during recursive traversal
             * and is mutated by the function.
             *
             * @param root Root node to start traversal from.
             * @param node1 Range start node.
             * @param node2 Range end node.
             * @param[in,out] in_range Internal flag used while recursing (pass false).
             */
            void selectRange( SmartPtr<IUITreeNode> root, SmartPtr<IUITreeNode> node1,
                              SmartPtr<IUITreeNode> node2, bool &in_range ) override;

            /**
             * @brief Find a node by its id.
             *
             * @param id Hash id to search for.
             * @return SmartPtr<IUITreeNode> Node with matching id or null.
             */
            SmartPtr<IUITreeNode> getNodeById( const hash_type &id ) const override;

            /**
             * @brief Factory helper used to create and register an ImGui tree element.
             *
             * Typically called when creating UI via reflection/factories.
             *
             * @param element Element instance to initialize as ImGuiTreeCtrl.
             */
            static void createElement( SmartPtr<IUIElement> element );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Add a node to the internal node container.
             *
             * This does not update UI selection; it simply registers the node.
             *
             * @param node Node to add.
             */
            void addTreeNode( SmartPtr<IUITreeNode> node );

            /**
             * @brief Remove a node from the internal node container.
             *
             * Also clears selection references to the node where applicable.
             *
             * @param node Node to remove.
             */
            void removeTreeNode( SmartPtr<IUITreeNode> node );

            /**
             * @brief Get a mutable reference to the internal concurrent node container.
             *
             * @return ConcurrentArray<SmartPtr<IUITreeNode>>& Reference to internal container.
             */
            ConcurrentArray<SmartPtr<IUITreeNode>> &getTreeNodesRef();

            /**
             * @brief Get a const reference to the internal concurrent node container.
             *
             * @return const ConcurrentArray<SmartPtr<IUITreeNode>>& Const reference to internal container.
             */
            const ConcurrentArray<SmartPtr<IUITreeNode>> &getTreeNodesRef() const;

            /// Element currently being dragged (atomic smart pointer).
            AtomicSmartPtr<IUIElement> m_dragSourceElement;

            /// Element currently targeted for a drop (atomic smart pointer).
            AtomicSmartPtr<IUIElement> m_dropDestinationElement;

            /// Root node of the tree (atomic smart pointer).
            AtomicSmartPtr<IUITreeNode> m_root;

            /// Primary selected node (atomic smart pointer).
            AtomicSmartPtr<IUITreeNode> m_selectedTreeNode;

            /// Weak reference to the last selected node (used for range selection).
            AtomicWeakPtr<IUITreeNode> m_lastSelectedNode;

            /// Currently selected nodes (explicit array for multi-selection).
            ConcurrentArray<SmartPtr<IUITreeNode>> m_treeSelectedNodes;

            /// Internal container of all nodes displayed in the control.
            ConcurrentArray<SmartPtr<IUITreeNode>> m_treeNodes;

            /// Whether multiple nodes may be selected concurrently.
            atomic_bool m_multiSelect = true;

            /// Number of nodes currently displayed in the UI (used for optimisation).
            atomic_s32 numNodesDisplayed = 0;

            /// Sibling index for selection context (-1 if none).
            atomic_s32 m_siblingIndex = -1;

            /// Static id extension used for generating unique node ids.
            static u32 m_nodeIdExt;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // __ImGuiTreeCtrl_h__
