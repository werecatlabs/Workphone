#ifndef ImGuiTreeNode_h__
#define ImGuiTreeNode_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUITreeNode.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiTreeNode : public ImGuiElement<IUITreeNode>
        {
        public:
            ImGuiTreeNode();
            ~ImGuiTreeNode() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            SmartPtr<IUIElement> getNodeData() const override;

            /** @copydoc ISharedObject::unload */
            void setNodeData( SmartPtr<IUIElement> nodeData ) override;

            /** @copydoc ISharedObject::unload */
            SmartPtr<ISharedObject> getNodeUserData() const override;

            /** @copydoc ISharedObject::unload */
            void setNodeUserData( SmartPtr<ISharedObject> nodeUserData ) override;

            /** @copydoc ISharedObject::unload */
            Type getNodeType() const override;

            /** @copydoc ISharedObject::unload */
            void setNodeType( Type nodeType ) override;

            /** @copydoc ISharedObject::unload */
            SmartPtr<IUITreeCtrl> getOwnerTree() const override;

            /** @copydoc ISharedObject::unload */
            void setOwnerTree( SmartPtr<IUITreeCtrl> owner ) override;

            /** @copydoc ISharedObject::unload */
            u32 getTreeNodeId() const override;

            /** @copydoc ISharedObject::unload */
            void setTreeNodeId( u32 treeNodeId ) override;

            /** @copydoc IUITreeNode::isExpanded */
            bool isExpanded() const override;

            /** @copydoc IUITreeNode::setExpanded */
            void setExpanded( bool expanded ) override;

            /**
             * @brief Consume a pending programmatic expansion-state change.
             *
             * The renderer uses this to synchronize ImGui's internal tree storage
             * when setExpanded() is called after the node has already been drawn.
             */
            bool consumeExpansionStateChange();

            /** @copydoc IUITreeNode::isSelected */
            bool isSelected() const override;

            /** @copydoc IUITreeNode::setSelected */
            void setSelected( bool selected ) override;

            static void createTreeNode( SmartPtr<IUITreeCtrl> &tree, SmartPtr<IUITreeNode> &treeNode,
                                        s32 &numNodesDisplayed );

            WP_CLASS_REGISTER_DECL;

        protected:
            WeakPtr<IUITreeCtrl> m_ownerTree;
            SmartPtr<ISharedObject> m_nodeUserData;
            SmartPtr<IUIElement> m_nodeData;
            Type m_nodeType = Type::Text;

            u32 m_treeNodeId = 0;

            bool m_expanded = false;
            bool m_expansionStateDirty = false;
            bool m_selected = false;

            static u32 m_idExt;
        };

        inline bool ImGuiTreeNode::isExpanded() const
        {
            return m_expanded;
        }

        inline bool ImGuiTreeNode::isSelected() const
        {
            return m_selected;
        }
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiTreeNode_h__
