#ifndef IUITreeCtrl_h__
#define IUITreeCtrl_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @brief Interface for a tree control UI element.
         *
         * IUITreeCtrl describes the contract for a tree-style UI control used
         * to display hierarchical data made of `IUITreeNode` objects. Implementers
         * are responsible for managing node lifetime through `SmartPtr` and
         * for emitting any UI-change notifications required by the framework.
         *
         * The interface provides methods for creating and manipulating nodes,
         * managing selection (single and multi-select), and supporting drag &
         * drop source/destination elements.
         */
        class WPCore_API IUITreeCtrl : public IUIElement
        {
        public:
            /**
             * @brief Special hash value used to indicate a full-tree clear.
             *
             * Implementations and listeners may use this value when broadcasting
             * change notifications to signal that the tree content was cleared
             * and that consumers should reset any cached node state.
             */
            static const hash_type clearHash;

            IUITreeCtrl();

            IUITreeCtrl( u32 poolTypeId );

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup in derived implementations.
             */
            ~IUITreeCtrl() override;

            /**
             * @brief Remove all nodes from the tree and reset internal state.
             *
             * Implementations should remove every node and update any UI
             * representation. When notifying observers of the change, the
             * `clearHash` value can be used to indicate a complete clear.
             */
            virtual void clear() = 0;

            /**
             * @brief Create and add a new root node to the tree.
             *
             * @return A smart pointer to the newly created root node. Ownership
             * is held via `SmartPtr`.
             */
            virtual SmartPtr<IUITreeNode> addRoot() = 0;

            /**
             * @brief Create and add a new node to the tree.
             *
             * The exact placement (for example, as a child of a selected
             * node versus a top-level node) is implementation-defined and
             * should be documented by the concrete control.
             *
             * @return A smart pointer to the newly created node.
             */
            virtual SmartPtr<IUITreeNode> addNode() = 0;

            /**
             * @brief Expand the specified tree node so its children are visible.
             *
             * If the provided `node` is null, implementations should perform no
             * action. Expansion state changes should update the UI and may
             * trigger layout or rendering updates.
             *
             * @param node The node to expand.
             */
            virtual void expand( SmartPtr<IUITreeNode> node ) = 0;

            /**
             * @brief Retrieve the top-level tree nodes.
             *
             * Returns an array of the tree's root nodes (top-level entries).
             * Modifying the returned array does not affect the control unless
             * passed back to `setTreeNodes`.
             *
             * @return An array of smart pointers to the top-level nodes.
             */
            virtual Array<SmartPtr<IUITreeNode>> getTreeNodes() const = 0;

            /**
             * @brief Replace the control's top-level nodes with `treeNodes`.
             *
             * The control takes the provided nodes as the new root set and
             * should update its visual representation accordingly.
             *
             * @param treeNodes Array of nodes that become the new top-level
             * tree nodes.
             */
            virtual void setTreeNodes( Array<SmartPtr<IUITreeNode>> treeNodes ) = 0;

            /**
             * @brief Get currently selected nodes (for multi-select controls).
             *
             * When the control does not support multi-selection this may return
             * an array with zero or one element depending on the selection
             * state.
             *
             * @return Array of selected tree nodes.
             */
            virtual Array<SmartPtr<IUITreeNode>> getSelectedTreeNodes() const = 0;

            /**
             * @brief Set the selection to the nodes in `treeNodes`.
             *
             * For controls that do not support multiple selection, the
             * behavior is implementation-defined (typically the first node in
             * the array is selected).
             *
             * @param treeNodes The nodes to select.
             */
            virtual void setSelectedTreeNodes( Array<SmartPtr<IUITreeNode>> treeNodes ) = 0;

            /**
             * @brief Add `treeNode` to the current selection.
             *
             * Has effect only when multi-selection is enabled; otherwise it may
             * replace the current selection. Implementations should ensure
             * selection state is updated and observers notified.
             *
             * @param treeNode The tree node to add to the selection.
             */
            virtual void addSelectedTreeNode( SmartPtr<IUITreeNode> treeNode ) = 0;

            /**
             * @brief Clear the current selection.
             *
             * After this call there should be no selected nodes in the control.
             */
            virtual void clearSelectedTreeNodes() = 0;

            /**
             * @brief Get the primary selected tree node.
             *
             * For multi-select controls this typically returns the focused or
             * last-selected node. If there is no selection an empty `SmartPtr`
             * is returned.
             *
             * @return The primary selected tree node or null pointer.
             */
            virtual SmartPtr<IUITreeNode> getSelectedTreeNode() const = 0;

            /**
             * @brief Set the primary selection to `treeNode`.
             *
             * This will typically clear other selections unless the control
             * supports multi-selection and the caller explicitly preserves
             * other selected nodes.
             *
             * @param treeNode The tree node to select.
             */
            virtual void setSelectedTreeNode( SmartPtr<IUITreeNode> treeNode ) = 0;

            /**
             * @brief Get the control's primary root node.
             *
             * Some tree controls maintain a single root node that anchors the
             * hierarchy; this method returns that root when present.
             *
             * @return The root node or an empty `SmartPtr` if none is set.
             */
            virtual SmartPtr<IUITreeNode> getRoot() const = 0;

            /**
             * @brief Set or replace the control's primary root node.
             *
             * @param root The node to use as the primary root for the tree.
             */
            virtual void setRoot( SmartPtr<IUITreeNode> root ) = 0;

            /**
             * @brief Get the element that is the current drag source.
             *
             * This is used during drag & drop operations to indicate which UI
             * element initiated the drag.
             *
             * @return The drag source element or null if none is set.
             */
            virtual SmartPtr<IUIElement> getDragSourceElement() const = 0;

            /**
             * @brief Set the element that will be treated as the drag source.
             *
             * @param dragSource The UI element initiating a drag operation.
             */
            virtual void setDragSourceElement( SmartPtr<IUIElement> dragSource ) = 0;

            /**
             * @brief Get the element that is the current drop destination.
             *
             * Used during drag & drop to indicate the potential receiver of a
             * dragged item.
             *
             * @return The drop destination element or null if none is set.
             */
            virtual SmartPtr<IUIElement> getDropDestinationElement() const = 0;

            /**
             * @brief Set the element that will be treated as the drop destination.
             *
             * @param dropDestination The UI element that can accept a drop.
             */
            virtual void setDropDestinationElement( SmartPtr<IUIElement> dropDestination ) = 0;

            /**
             * @brief Query whether the control supports multiple selection.
             *
             * @return True if multiple nodes can be selected; false otherwise.
             */
            virtual bool isMultiSelect() const = 0;

            /**
             * @brief Enable or disable multi-selection behavior.
             *
             * @param multiSelect True to allow multiple node selection; false
             * to restrict selection to a single node.
             */
            virtual void setMultiSelect( bool multiSelect ) = 0;

            /**
             * @brief Get the index of the selected sibling relative to its parent.
             *
             * This value describes the position of the selected node among its
             * siblings. For example, 0 indicates the first sibling.
             *
             * @return The selected sibling index.
             */
            virtual s32 getSelectedSiblingIndex() const = 0;

            /**
             * @brief Set the selection by sibling index relative to the current
             * parent.
             *
             * The control should select the sibling at `siblingIndex` under
             * the relevant parent node if valid.
             *
             * @param siblingIndex The index of the sibling to select.
             */
            virtual void setSelectedSiblingIndex( s32 siblingIndex ) = 0;

            /**
             * @brief Select a contiguous range of nodes between `node1` and
             * `node2` under the given `root`.
             *
             * The selection semantics (inclusive/exclusive) are implementation
             * defined. The boolean reference `in_range` should be updated by
             * implementations to indicate whether the requested range was
             * valid (for example both nodes were found and share the same
             * traversal range).
             *
             * @param root The root node under which to perform the range
             * selection.
             * @param node1 One end of the range to select.
             * @param node2 The other end of the range to select.
             * @param[in,out] in_range Set to true if the range operation was
             * successful and the nodes lay in a common selectable range; set
             * to false otherwise.
             */
            virtual void selectRange( SmartPtr<IUITreeNode> root, SmartPtr<IUITreeNode> node1,
                                      SmartPtr<IUITreeNode> node2, bool &in_range ) = 0;

            /**
             * @brief Find a node by its identifier hash.
             *
             * @param id The hash identifier of the node to find.
             * @return Smart pointer to the node if found; otherwise an empty
             * pointer.
             */
            virtual SmartPtr<IUITreeNode> getNodeById( const hash_type &id ) const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IUITreeCtrl_h__
