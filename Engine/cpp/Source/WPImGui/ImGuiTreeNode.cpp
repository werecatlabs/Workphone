#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiTreeCtrl.hpp>
#include <WPImGui/ImGuiTreeNode.hpp>
#include <WPImGui/ImGuiText.hpp>
#include <WPImGui/ImGuiUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiTreeNode, ImGuiElement<IUITreeNode> );

    u32 ImGuiTreeNode::m_idExt = 0;

    ImGuiTreeNode::ImGuiTreeNode() = default;

    ImGuiTreeNode::~ImGuiTreeNode()
    {
        unload( nullptr );
    }

    void ImGuiTreeNode::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loaded );
    }

    void ImGuiTreeNode::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                if( auto parent = getParent() )
                {
                    parent->removeChild( this );
                }

                m_ownerTree = nullptr;
                m_nodeUserData = nullptr;
                m_nodeData = nullptr;

                ImGuiElement<IUITreeNode>::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<IUIElement> ImGuiTreeNode::getNodeData() const
    {
        return m_nodeData;
    }

    void ImGuiTreeNode::setNodeData( SmartPtr<IUIElement> nodeData )
    {
        m_nodeData = nodeData;
    }

    SmartPtr<ISharedObject> ImGuiTreeNode::getNodeUserData() const
    {
        return m_nodeUserData;
    }

    void ImGuiTreeNode::setNodeUserData( SmartPtr<ISharedObject> nodeUserData )
    {
        m_nodeUserData = nodeUserData;
    }

    IUITreeNode::Type ImGuiTreeNode::getNodeType() const
    {
        return m_nodeType;
    }

    void ImGuiTreeNode::setNodeType( Type nodeType )
    {
        m_nodeType = nodeType;
    }

    SmartPtr<IUITreeCtrl> ImGuiTreeNode::getOwnerTree() const
    {
        return m_ownerTree.lock();
    }

    void ImGuiTreeNode::setOwnerTree( SmartPtr<IUITreeCtrl> owner )
    {
        m_ownerTree = owner;
    }

    u32 ImGuiTreeNode::getTreeNodeId() const
    {
        return m_treeNodeId;
    }

    void ImGuiTreeNode::setTreeNodeId( u32 treeNodeId )
    {
        m_treeNodeId = treeNodeId;
    }

    void ImGuiTreeNode::setExpanded( bool expanded )
    {
        if( m_expanded != expanded )
        {
            m_expanded = expanded;
            m_expansionStateDirty = true;
        }
    }

    bool ImGuiTreeNode::consumeExpansionStateChange()
    {
        auto const changed = m_expansionStateDirty;
        m_expansionStateDirty = false;
        return changed;
    }

    void ImGuiTreeNode::setSelected( bool selected )
    {
        m_selected = selected;
    }

    void ImGuiTreeNode::createTreeNode( SmartPtr<IUITreeCtrl> &tree, SmartPtr<IUITreeNode> &treeNode,
                                        s32 &numNodesDisplayed )
    {
        try
        {
            if( !treeNode || !treeNode->isVisible() )
            {
                return;
            }

            c8 *label = nullptr;

            auto pTreeNode = (ImGuiTreeNode *)treeNode.get();
            auto nodeChildren = pTreeNode->getChildren();

            for( auto &child : nodeChildren )
            {
                if( child->isDerived<ui::IUIText>() )
                {
                    auto text = (ImGuiText *)child.get();
                    label = (c8 *)text->getTextPtr();
                }
            }

            if( StringUtil::isNullOrEmpty( label ) )
            {
                label = "Untitled";
            }

            auto node_flags = static_cast<s32>( ImGuiTreeNodeFlags_None );
            if( treeNode->isExpanded() )
            {
                node_flags |= ImGuiTreeNodeFlags_DefaultOpen;
            }

            if( treeNode->isSelected() )
            {
                node_flags |= ImGuiTreeNodeFlags_Selected;
            }

            auto numChildTreeNodes = 0;
            for( auto &child : nodeChildren )
            {
                if( child->isDerived<ui::IUITreeNode>() )
                {
                    numChildTreeNodes++;
                }
            }

            if( numChildTreeNodes == 0 )
            {
                node_flags |= ImGuiTreeNodeFlags_Leaf;
            }

            node_flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
            node_flags |= ImGuiTreeNodeFlags_OpenOnArrow;
            node_flags |= ImGuiTreeNodeFlags_FramePadding;
            node_flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
            if( numChildTreeNodes > 0 )
            {
                node_flags |= ImGuiTreeNodeFlags_NavLeftJumpsBackHere;
            }

            auto handle = treeNode->getHandle();
            auto id = handle ? handle->getInstanceId() : treeNode->getTreeNodeId();

            if( pTreeNode->consumeExpansionStateChange() )
            {
                ImGui::SetNextItemOpen( pTreeNode->isExpanded(), ImGuiCond_Always );
            }

            auto opened = ImGui::TreeNodeEx( (void *)static_cast<intptr_t>( id ), node_flags, "%s",
                                             label );

            if( ImGui::IsMouseDragging( 0 ) )
            {
                auto siblingIndex = treeNode->getSiblingIndexByType<IUITreeNode>();
                auto siblingCount = treeNode->getSiblingCountByType<IUITreeNode>();

                auto rectMin = ImGui::GetItemRectMin();
                auto rectMax = ImGui::GetItemRectMax();

                auto lineRectMin = ImGui::GetItemRectMin();
                auto lineRectMax = ImGui::GetItemRectMax();
                lineRectMin.y -= ( lineRectMax.y - lineRectMin.y ) * 0.1f;

                auto lastLineRectMin = ImGui::GetItemRectMin();
                auto lastLineRectMax = ImGui::GetItemRectMax();
                lastLineRectMax.y += ( lastLineRectMax.y - lastLineRectMin.y ) * 0.1f;

                if( ImGui::IsMouseHoveringRect( rectMin, rectMax, true ) )
                {
                    auto highlightColor = ImGui::GetColorU32( ImGuiCol_DragDropTarget );
                    ImGui::GetWindowDrawList()->AddRect( rectMin, rectMax, highlightColor, 2.0f,
                                                         0, 2.0f );

                    tree->setSelectedSiblingIndex( -1 );

                    tree->setDropDestinationElement( treeNode );
                }
                else if( ImGui::IsMouseHoveringRect( lineRectMin, lineRectMax, true ) )
                {
                    // Get the position of the hovered node
                    auto startPos = ImGui::GetCursorScreenPos();
                    startPos.y = lineRectMin.y;

                    auto endPos = ImVec2( ImGui::GetWindowPos().x +
                                              ImGui::GetWindowContentRegionMax().x,
                                          startPos.y );

                    // Draw a line
                    ImGui::GetWindowDrawList()->AddLine(
                        startPos, endPos, ImGui::GetColorU32( ImGuiCol_DragDropTarget ), 2.0f );

                    tree->setSelectedSiblingIndex( siblingIndex );

                    tree->setDropDestinationElement( treeNode->getParent() );
                }
                else if( siblingIndex >= siblingCount - 1 &&
                         ImGui::IsMouseHoveringRect( lastLineRectMin, lastLineRectMax, true ) )
                {
                    // Get the position of the hovered node
                    auto startPos = ImGui::GetCursorScreenPos();
                    auto endPos = ImVec2( ImGui::GetWindowPos().x +
                                              ImGui::GetWindowContentRegionMax().x,
                                          startPos.y );

                    // Draw a line
                    ImGui::GetWindowDrawList()->AddLine(
                        startPos, endPos, ImGui::GetColorU32( ImGuiCol_DragDropTarget ), 2.0f );

                    tree->setSelectedSiblingIndex( siblingIndex );

                    tree->setDropDestinationElement( treeNode->getParent() );
                }
            }

            // Store UI-driven changes without scheduling a redundant state override.
            pTreeNode->m_expanded = opened;
            numNodesDisplayed++;

            auto isNodeHovered = ImGui::IsItemHovered();
            treeNode->setHovered( isNodeHovered );

            if( isNodeHovered && ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
            {
                // Preserve an existing multi-selection when opening a context menu.
                if( !treeNode->isSelected() )
                {
                    tree->setSelectedTreeNode( treeNode );
                }
            }

            if( auto dragSource = tree->getDragSource() )
            {
                if( ImGui::BeginDragDropSource( ImGuiDragDropFlags_None ) )
                {
                    tree->setDragSourceElement( treeNode );

                    auto args = Array<Parameter>();
                    args.resize( 3 );

                    args[0].object = treeNode;
                    args[1].object = nullptr;
                    args[2].object = nullptr;

                    auto retValue = dragSource->handleEvent( EventType::UI, IEvent::handleDrag, args,
                                                             treeNode, treeNode, nullptr );

                    auto &dragData = retValue.str;

                    if( !StringUtil::isNullOrEmpty( dragData ) )
                    {
                        ImGui::SetDragDropPayload( "_TREENODE", dragData.c_str(), dragData.size() );
                        ImGui::Text( "%s", label );
                    }

                    ImGui::EndDragDropSource();
                }
            }

            if( auto dropTarget = tree->getDropTarget() )
            {
                auto isNodeTarget = ImGui::BeginDragDropTarget();
                if( isNodeTarget )
                {
                    auto payload = ImGui::AcceptDragDropPayload( "_TREENODE" );
                    if( payload )
                    {
                        tree->setDropDestinationElement( treeNode );

                        auto data =
                            String( static_cast<const char *>( payload->Data ), payload->DataSize );
                        if( !StringUtil::isNullOrEmpty( data ) )
                        {
                            auto src = tree->getDragSourceElement();

                            auto args = Array<Parameter>();
                            args.reserve( 2 );

                            args.emplace_back( data );
                            args.emplace_back( -1 );

                            dropTarget->handleEvent( EventType::UI, IEvent::handleDrop, args, src,
                                                     treeNode, nullptr );
                        }
                    }

                    ImGui::EndDragDropTarget();
                }
                else if( ImGui::IsMouseReleased( 0 ) &&
                         tree->getSelectedSiblingIndex() != -1 )
                {
                    if( auto payload = ImGui::GetDragDropPayload() )
                    {
                        auto data =
                            String( static_cast<const char *>( payload->Data ), payload->DataSize );
                        if( !StringUtil::isNullOrEmpty( data ) )
                        {
                            auto src = tree->getDragSourceElement();
                            auto dst = tree->getDropDestinationElement();

                            auto args = Array<Parameter>();
                            args.reserve( 2 );

                            args.emplace_back( data );
                            args.emplace_back( tree->getSelectedSiblingIndex() );

                            dropTarget->handleEvent( EventType::UI, IEvent::handleDrop, args, src,
                                                     dst, nullptr );
                        }
                    }
                }
            }

            if( ImGui::IsMouseReleased( 0 ) && ImGui::IsItemHovered( ImGuiHoveredFlags_None ) )
            {
                auto const &io = ImGui::GetIO();
                if( tree->isMultiSelect() && io.KeyCtrl )
                {
                    auto selectedNodes = tree->getSelectedTreeNodes();
                    auto selectedNode =
                        std::find( selectedNodes.begin(), selectedNodes.end(), treeNode );
                    if( selectedNode == selectedNodes.end() )
                    {
                        selectedNodes.push_back( treeNode );
                    }
                    else
                    {
                        selectedNodes.erase( selectedNode );
                    }

                    tree->setSelectedTreeNodes( selectedNodes );
                    static_cast<ImGuiTreeCtrl *>( tree.get() )->setLastSelectedNode( treeNode );
                }
                else if( tree->isMultiSelect() && io.KeyShift )
                {
                    auto imguiTree = static_cast<ImGuiTreeCtrl *>( tree.get() );
                    auto anchor = imguiTree->getLastSelectedNode();
                    bool rangeSelected = false;
                    tree->selectRange( tree->getRoot(), anchor, treeNode, rangeSelected );
                    if( !rangeSelected )
                    {
                        tree->setSelectedTreeNode( treeNode );
                    }
                }
                else
                {
                    tree->setSelectedTreeNode( treeNode );
                }
            }

            // The full-row hit target is intentionally larger than the label. Show
            // the complete value when the text itself is clipped by the row width.
            if( isNodeHovered && ImGui::CalcTextSize( label ).x > ImGui::GetContentRegionAvail().x )
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted( label );
                ImGui::EndTooltip();
            }

            if( ImGui::IsMouseClicked( 0 ) && ImGui::IsItemHovered( ImGuiHoveredFlags_None ) )
            {
                auto ownerTree = treeNode->getOwnerTree();
                if( ownerTree )
                {
                    auto listeners = ownerTree->getObjectListeners();
                    for( auto &listener : listeners )
                    {
                        auto args = Array<Parameter>();
                        args.resize( 2 );

                        args[0].object = treeNode;

                        listener->handleEvent( EventType::Object,
                                               IEvent::handleTreeSelectionActivated, args, tree,
                                               treeNode, nullptr );
                    }
                }

                if( ImGui::IsMouseDoubleClicked( 0 ) )
                {
                    if( ownerTree )
                    {
                        auto listeners = ownerTree->getObjectListeners();
                        for( auto &listener : listeners )
                        {
                            auto args = Array<Parameter>();
                            args.resize( 2 );

                            args[0].object = treeNode;

                            listener->handleEvent( EventType::Object,
                                                   IEvent::handleTreeNodeDoubleClicked, args, tree,
                                                   treeNode, nullptr );
                        }
                    }
                }
            }

            if( ImGui::IsMouseReleased( 0 ) && ImGui::IsItemHovered( ImGuiHoveredFlags_None ) )
            {
                auto ownerTree = treeNode->getOwnerTree();
                if( ownerTree )
                {
                    auto listeners = ownerTree->getObjectListeners();
                    for( auto &listener : listeners )
                    {
                        auto args = Array<Parameter>();
                        args.resize( 2 );

                        args[0].object = treeNode;

                        listener->handleEvent( EventType::Object, IEvent::handleTreeSelectionRelease,
                                               args, tree, treeNode, nullptr );
                    }
                }
            }

            //ImGui::SameLine();  // Move to the same row
            auto cursorPosX = ImGui::GetCursorPosX();
            //auto cursorScreenPos = ImGui::GetCursorScreenPos();

            for( auto &childElement : nodeChildren )
            {
                if( childElement->isDerived<IUIToggle>() )
                {
                    auto childToggle = workphone::static_pointer_cast<IUIToggle>( childElement );

                    //ImGui::SameLine();  // Move to the same row
                    //float treeNodeWidth = 1000;
                    //ImGui::GetTreeNodeToLabelSpacing();
                    //ImGui::SetCursorPosX( treeNodeWidth );

                    // Align the checkbox to the right
                    float currentPosX = ImGui::GetCursorPosX();  // Save current X position
                    float rightAlignPosX = ImGui::GetWindowContentRegionMax().x -
                                           ImGui::GetStyle().FramePadding.x - ImGui::GetCursorPos().x;
                    ImGui::SameLine();  // Place the checkbox on the same row
                    //auto xpos = 1000;
                    ImGui::SetCursorPosX( currentPosX + ( rightAlignPosX - 15.0f ) );
                    //ImGui::SetCursorScreenPos(
                    //    ImVec2( cursorScreenPos.x + rightAlignPosX, cursorScreenPos.y ) );
                    //ImGui::SetCursorScreenPos( ImVec2( xpos, cursorScreenPos.y ) );

                    //auto name = treeNode->getName();
                    //if( ImGui::Button( name.c_str() ) )
                    //{
                    //    //m_isToggled = !m_isToggled;
                    //}

                    auto name = treeNode->getName();
                    bool isToggled = false;
                    //ImGui::Checkbox( name.c_str(), &isToggled );
                    //uiApplication->createElement( childElement );

                    auto scale = childElement->getScale();
                    auto toggled = childToggle->isToggled();
                    if( ImGuiUtil::ToggleButton( name.c_str(), &toggled, scale ) )
                    {
                        childToggle->setToggled( toggled );

                        auto ownerTree = treeNode->getOwnerTree();
                        if( ownerTree )
                        {
                            auto listeners = ownerTree->getObjectListeners();
                            for( auto listener : listeners )
                            {
                                auto args = Array<Parameter>();
                                args.resize( 2 );

                                args[0].object = treeNode;
                                args[1].object = childToggle;

                                listener->handleEvent( EventType::Object, IEvent::handleToggle, args,
                                                       tree, treeNode, nullptr );
                            }
                        }
                    }
                }
            }

            ImGui::SetCursorPosX( cursorPosX );
            //ImGui::SetCursorScreenPos( cursorScreenPos );

            if( opened )
            {
                try
                {
                    for( auto &child : nodeChildren )
                    {
                        if( child->isDerived<ui::IUITreeNode>() )
                        {
                            auto childTreeNode =
                                workphone::static_pointer_cast<ui::IUITreeNode>( child );
                            createTreeNode( tree, childTreeNode, numNodesDisplayed );
                        }
                    }
                }
                catch( std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                }

                // if(tree->getRoot() == treeNode)
                //{
                //     if(( node_flags & ImGuiTreeNodeFlags_Leaf ) != 0)
                //     {
                //         ImGui::TreePop();
                //     }
                // }
                // else

                ImGui::TreePop();
            }

            // Handle selection input
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }
}  // namespace workphone::ui
