#ifndef IUITreeNode_h__
#define IUITreeNode_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * Interface for a tree node in a UI tree control.
         */
        class WPCore_API IUITreeNode : public IUIElement
        {
        public:
            /**
             * Enumerates the possible types of tree nodes.
             */
            enum class Type
            {
                Text,          ///< Node contains only text.
                TextAndIcons,  ///< Node contains text and icons.
                Count          ///< Total number of types.
            };

            IUITreeNode() : IUIElement( IUITreeNode::typeInfo() )
            {
            }

            IUITreeNode( u32 poolTypeId ) : IUIElement( poolTypeId )
            {
            }

            /** Virtual destructor. */
            ~IUITreeNode() override;

            /**
             * Gets the data associated with the node.
             * @return A smart pointer to the UI element representing the node data.
             */
            virtual SmartPtr<IUIElement> getNodeData() const = 0;

            /**
             * Sets the data associated with the node.
             * @param nodeData The UI element to associate with the node.
             */
            virtual void setNodeData( SmartPtr<IUIElement> nodeData ) = 0;

            /**
             * Gets the user data associated with the node.
             * @return A smart pointer to the shared object representing the user data.
             */
            virtual SmartPtr<ISharedObject> getNodeUserData() const = 0;

            /**
             * Sets the user data associated with the node.
             * @param nodeUserData The shared object to associate with the node.
             */
            virtual void setNodeUserData( SmartPtr<ISharedObject> nodeUserData ) = 0;

            /**
             * Gets the type of the node.
             * @return The type of the node.
             */
            virtual Type getNodeType() const = 0;

            /**
             * Sets the type of the node.
             * @param nodeType The type of the node.
             */
            virtual void setNodeType( Type nodeType ) = 0;

            /**
             * Gets the tree control that owns the node.
             * @return A smart pointer to the owning tree control.
             */
            virtual SmartPtr<IUITreeCtrl> getOwnerTree() const = 0;

            /**
             * Sets the tree control that owns the node.
             * @param owner The tree control that owns the node.
             */
            virtual void setOwnerTree( SmartPtr<IUITreeCtrl> owner ) = 0;

            /**
             * Gets the unique ID of the node.
             * @return The unique ID of the node.
             */
            virtual u32 getTreeNodeId() const = 0;

            /**
             * Sets the unique ID of the node.
             * @param treeNodeId The unique ID to assign to the node.
             */
            virtual void setTreeNodeId( u32 treeNodeId ) = 0;

            /**
             * Determines whether the node is expanded.
             * @return True if the node is expanded, false otherwise.
             */
            virtual bool isExpanded() const = 0;

            /**
             * Sets the expanded state of the node.
             * @param expanded True to expand the node, false to collapse it.
             */
            virtual void setExpanded( bool expanded ) = 0;

            /**
             * Determines whether the node is selected.
             * @return True if the node is selected, false otherwise.
             */
            virtual bool isSelected() const = 0;

            /**
             * Sets the selected state of the node.
             * @param selected True to select the node, false to deselect it.
             */
            virtual void setSelected( bool selected ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // IUITreeNode_h__
