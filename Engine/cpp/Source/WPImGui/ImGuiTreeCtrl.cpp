#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiTreeCtrl.hpp>
#include <WPImGui/ImGuiTreeNode.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <functional>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiTreeCtrl, ImGuiElement<IUITreeCtrl> );

    u32 ImGuiTreeCtrl::m_nodeIdExt = 0;

    void DrawRowsBackground( int row_count, float line_height, float x1, float x2, float y_offset,
                             ImU32 col_even, ImU32 col_odd )
    {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        float y0 = ImGui::GetCursorScreenPos().y + static_cast<float>( static_cast<int>( y_offset ) );

        int row_display_start;
        int row_display_end;
        ImGui::CalcListClipping( row_count, line_height, &row_display_start, &row_display_end );
        for( int row_n = row_display_start; row_n < row_display_end; row_n++ )
        {
            ImU32 col = ( row_n & 1 ) ? col_odd : col_even;
            if( ( col & IM_COL32_A_MASK ) == 0 )
            {
                continue;
            }
            float y1 = y0 + ( line_height * row_n );
            float y2 = y1 + line_height;
            draw_list->AddRectFilled( ImVec2( x1, y1 ), ImVec2( x2, y2 ), col );
        }
    }

    ImGuiTreeCtrl::ImGuiTreeCtrl()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManager();
        WP_ASSERT( stateManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto stateContext = stateManager->addStateContext();

        auto stateListener = factoryManager->make_ptr<StateListener>();
        stateListener->setOwner( this );
        stateContext->addStateListener( stateListener );

        setStateContext( stateContext );
        setStateListener( stateListener );
    }

    ImGuiTreeCtrl::~ImGuiTreeCtrl()
    {
        unload( nullptr );
    }

    void ImGuiTreeCtrl::load( SmartPtr<ISharedObject> data )
    {
        m_children.reserve(1024)
        setLoadingState( LoadingState::Loaded );
    }

    void ImGuiTreeCtrl::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded )
            {
                setLoadingState( LoadingState::Unloading );

                m_treeSelectedNodes.clear();

                auto treeNodes = getTreeNodes();
                for( auto &treeNode : treeNodes )
                {
                    if( treeNode )
                    {
                        treeNode->unload( data );
                    }
                }

                m_treeNodes.clear();

                m_dragSourceElement = nullptr;
                m_dropDestinationElement = nullptr;
                m_root = nullptr;
                m_selectedTreeNode = nullptr;

                ImGuiElement<IUITreeCtrl>::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<IUITreeNode> ImGuiTreeCtrl::addRoot()
    {
        const auto &loadingState = getLoadingState();
        if( loadingState == LoadingState::Loaded )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            auto node = ui->addElementByType<IUITreeNode>();
            m_root = node;

            auto root = getRoot();
            auto handle = root->getHandle();
            if( handle )
            {
                handle->setInstanceId( m_nodeIdExt );
            }

            m_nodeIdExt++;
            return m_root;
        }

        return nullptr;
    }

    SmartPtr<IUITreeNode> ImGuiTreeCtrl::addNode()
    {
        const auto &loadingState = getLoadingState();
        if( loadingState == LoadingState::Loaded )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            if( auto ui = applicationManager->getUI() )
            {
                if( auto node = ui->addElementByType<IUITreeNode>() )
                {
                    node->setOwnerTree( this );
                    addTreeNode( node );

                    if( auto handle = node->getHandle() )
                    {
                        handle->setInstanceId( m_nodeIdExt );
                    }

                    m_nodeIdExt++;
                    node->setTreeNodeId( m_nodeIdExt );

                    return node;
                }

                return nullptr;
            }
        }

        return nullptr;
    }

    void ImGuiTreeCtrl::expand( SmartPtr<IUITreeNode> node )
    {
        // Match the tree-list behavior: revealing an item also reveals the entire
        // branch that contains it. This makes programmatic focus/selection useful.
        auto current = node;
        while( current )
        {
            current->setExpanded( true );

            auto parent = current->getParent();
            if( parent && parent->isDerived<IUITreeNode>() )
            {
                current = workphone::static_pointer_cast<IUITreeNode>( parent );
            }
            else
            {
                current = nullptr;
            }
        }
    }

    void ImGuiTreeCtrl::clear()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto jobQueue = applicationManager->getJobQueuePtr();
        auto ui = applicationManager->getUIPtr();

        clearSelectedTreeNodes();

        if( auto root = getRoot() )
        {
            root->removeAllChildren();
            setRoot( nullptr );
        }

        auto treeNodes = getTreeNodes();
        for( auto &treeNode : treeNodes )
        {
            ui->removeElement( treeNode );
        }

        m_treeNodes.clear();
    }

    Array<SmartPtr<IUITreeNode>> ImGuiTreeCtrl::getTreeNodes() const
    {
        return m_treeNodes.snapshot();
    }

    void ImGuiTreeCtrl::setTreeNodes( Array<SmartPtr<IUITreeNode>> treeNodes )
    {
        m_treeNodes = { treeNodes.begin(), treeNodes.end() };
    }

    Array<SmartPtr<IUITreeNode>> ImGuiTreeCtrl::getSelectedTreeNodes() const
    {
        return m_treeSelectedNodes.snapshot();
    }

    void ImGuiTreeCtrl::setSelectedTreeNodes( Array<SmartPtr<IUITreeNode>> treeNodes )
    {
        clearSelectedTreeNodes();

        if( !isMultiSelect() && treeNodes.size() > 1 )
        {
            treeNodes.resize( 1 );
        }

        for( auto &treeNode : treeNodes )
        {
            addSelectedTreeNode( treeNode );
        }

        if( !treeNodes.empty() )
        {
            m_selectedTreeNode = treeNodes.back();
            setLastSelectedNode( treeNodes.back() );
        }
    }

    void ImGuiTreeCtrl::addSelectedTreeNode( SmartPtr<IUITreeNode> treeNode )
    {
        if( treeNode )
        {
            if( !isMultiSelect() )
            {
                clearSelectedTreeNodes();
            }

            auto selectedNodes = getSelectedTreeNodes();
            if( std::find( selectedNodes.begin(), selectedNodes.end(), treeNode ) !=
                selectedNodes.end() )
            {
                m_selectedTreeNode = treeNode;
                return;
            }

            m_treeSelectedNodes.push_back( treeNode );
            treeNode->setSelected( true );
            m_selectedTreeNode = treeNode;
        }
    }

    void ImGuiTreeCtrl::clearSelectedTreeNodes()
    {
        auto selectedNodes = getSelectedTreeNodes();
        for( auto &treeSelectedNode : selectedNodes )
        {
            if( treeSelectedNode )
            {
                treeSelectedNode->setSelected( false );
            }
        }

        m_treeSelectedNodes.clear();

        auto selectedTreeNode = getSelectedTreeNode();
        if( selectedTreeNode )
        {
            selectedTreeNode->setSelected( false );
        }

        m_selectedTreeNode = nullptr;
    }

    SmartPtr<IUITreeNode> ImGuiTreeCtrl::getSelectedTreeNode() const
    {
        return m_selectedTreeNode;
    }

    void ImGuiTreeCtrl::setSelectedTreeNode( SmartPtr<IUITreeNode> treeNode )
    {
        clearSelectedTreeNodes();
        if( treeNode )
        {
            addSelectedTreeNode( treeNode );
            setLastSelectedNode( treeNode );
        }
    }

    SmartPtr<IUITreeNode> ImGuiTreeCtrl::getRoot() const
    {
        return m_root;
    }

    void ImGuiTreeCtrl::setRoot( SmartPtr<IUITreeNode> root )
    {
        m_root = root;
    }

    bool ImGuiTreeCtrl::isMultiSelect() const
    {
        return m_multiSelect;
    }

    void ImGuiTreeCtrl::setMultiSelect( bool multiSelect )
    {
        if( !multiSelect && isMultiSelect() )
        {
            auto selectedNode = getSelectedTreeNode();
            clearSelectedTreeNodes();
            if( selectedNode )
            {
                addSelectedTreeNode( selectedNode );
            }
        }

        m_multiSelect = multiSelect;
    }

    SmartPtr<IUIElement> ImGuiTreeCtrl::getDragSourceElement() const
    {
        return m_dragSourceElement;
    }

    void ImGuiTreeCtrl::setDragSourceElement( SmartPtr<IUIElement> dragSource )
    {
        m_dragSourceElement = dragSource;
    }

    SmartPtr<IUIElement> ImGuiTreeCtrl::getDropDestinationElement() const
    {
        return m_dropDestinationElement;
    }

    void ImGuiTreeCtrl::setDropDestinationElement( SmartPtr<IUIElement> dropDestination )
    {
        m_dropDestinationElement = dropDestination;
    }

    s32 ImGuiTreeCtrl::getSelectedSiblingIndex() const
    {
        return m_siblingIndex;
    }

    void ImGuiTreeCtrl::setSelectedSiblingIndex( s32 siblingIndex )
    {
        m_siblingIndex = siblingIndex;
    }

    void ImGuiTreeCtrl::createElement( SmartPtr<IUIElement> element )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto treeCtrl = workphone::static_pointer_cast<ImGuiTreeCtrl>( element );
        //treeCtrl->setSiblingIndex( -1 );

        if( auto root = treeCtrl->getRoot() )
        {
            auto label = Util::getText( root );
            if( StringUtil::isNullOrEmpty( label ) )
            {
                label = "Untitled";
            }

            auto nodeFlags = static_cast<s32>( ImGuiTreeNodeFlags_None );
            if( root->isExpanded() )
            {
                nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
            }

            nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow;
            nodeFlags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
            nodeFlags |= ImGuiTreeNodeFlags_FramePadding;
            nodeFlags |= ImGuiTreeNodeFlags_SpanAvailWidth;
            nodeFlags |= ImGuiTreeNodeFlags_NavLeftJumpsBackHere;

            auto handle = root->getHandle();
            auto id = handle ? handle->getInstanceId() : root->getTreeNodeId();

            auto imguiRoot = workphone::static_pointer_cast<ImGuiTreeNode>( root );
            if( imguiRoot->consumeExpansionStateChange() )
            {
                ImGui::SetNextItemOpen( imguiRoot->isExpanded(), ImGuiCond_Always );
            }

            auto rootOpened = ImGui::TreeNodeEx( (void *)static_cast<intptr_t>( id ), nodeFlags,
                                                 "%s", label.c_str() );
            root->setExpanded( rootOpened );
            if( rootOpened )
            {
                if( auto dragSource = treeCtrl->getDragSource() )
                {
                    if( ImGui::BeginDragDropSource( ImGuiDragDropFlags_None ) )
                    {
                        auto args = Array<Parameter>();
                        args.resize( 1 );

                        args[0].object = treeCtrl;

                        auto retValue = dragSource->handleEvent( EventType::UI, IEvent::handleDrag,
                                                                 args, treeCtrl, treeCtrl, nullptr );

                        auto &dataStr = retValue.str;

                        ImGui::SetDragDropPayload( "_TREENODE", dataStr.c_str(), dataStr.size() );
                        ImGui::Text( "%s", label.c_str() );
                        ImGui::EndDragDropSource();
                    }
                }

                if( auto dropTarget = treeCtrl->getDropTarget() )
                {
                    if( ImGui::BeginDragDropTarget() )
                    {
                        auto payload = ImGui::AcceptDragDropPayload( "_TREENODE" );
                        if( payload )
                        {
                            auto data =
                                String( static_cast<const char *>( payload->Data ), payload->DataSize );

                            auto args = Array<Parameter>();
                            args.resize( 2 );

                            args[0].object = treeCtrl;
                            args[1].str = data;

                            applicationManager->triggerEvent( EventType::UI, IEvent::handleDrop, args,
                                                              dropTarget, treeCtrl, nullptr, false,
                                                              Thread::Application_Flag );
                        }

                        ImGui::EndDragDropTarget();
                    }
                }

                if( ImGui::IsMouseDragging( 0 ) )
                {
                    // Render a preview at the mouse cursor
                }

                const auto is_hovered = ImGui::IsItemHovered();  // Hovered
                const auto is_active = ImGui::IsItemActive();    // Held

                if( is_hovered && ImGui::IsMouseClicked( 0 ) )
                {
                    auto args = Array<Parameter>();
                    args.resize( 1 );

                    args[0].object = root;

                    applicationManager->triggerEvent(
                        EventType::UI, IEvent::handleTreeSelectionActivated, args, treeCtrl, treeCtrl,
                        nullptr, false, Thread::Application_Flag );
                }

                if( is_hovered && ImGui::IsMouseReleased( 0 ) )
                {
                    auto args = Array<Parameter>();
                    args.resize( 1 );

                    args[0].object = root;

                    applicationManager->triggerEvent(
                        EventType::UI, IEvent::handleTreeSelectionRelease, args, treeCtrl, treeCtrl,
                        nullptr, false, Thread::Application_Flag );
                }

                auto x1 = ImGui::GetCurrentWindow()->WorkRect.Min.x;
                auto x2 = ImGui::GetCurrentWindow()->WorkRect.Max.x;
                auto item_spacing_y = ImGui::GetStyle().ItemSpacing.y;
                auto item_offset_y = -item_spacing_y * 0.5f;
                auto line_height = ImGui::GetFrameHeight() + item_spacing_y;

                auto evenColor = 0;
                auto oddRowColor = ImGui::GetStyleColorVec4( ImGuiCol_Header );
                oddRowColor.w *= 0.16f;
                auto oddColor = ImGui::GetColorU32( oddRowColor );
                DrawRowsBackground( treeCtrl->numNodesDisplayed, line_height, x1, x2, item_offset_y,
                                    evenColor, oddColor );

                treeCtrl->numNodesDisplayed = 0;

                auto tree = workphone::static_pointer_cast<IUITreeCtrl>( treeCtrl );

                auto children = root->getChildren();

                for( auto &child : children )
                {
                    if( child->isDerived<IUITreeNode>() )
                    {
                        auto treeNode = workphone::dynamic_pointer_cast<IUITreeNode>( child );
                        if( treeNode )
                        {
                            auto numNodesDisplayed = treeCtrl->numNodesDisplayed.load();
                            ImGuiTreeNode::createTreeNode( tree, treeNode, numNodesDisplayed );
                            treeCtrl->numNodesDisplayed = numNodesDisplayed;
                        }
                    }
                }

                ImGui::TreePop();
            }
        }
    }

    void ImGuiTreeCtrl::addTreeNode( SmartPtr<IUITreeNode> node )
    {
        auto &nodes = getTreeNodesRef();
        nodes.push_back( node );
    }

    void ImGuiTreeCtrl::removeTreeNode( SmartPtr<IUITreeNode> node )
    {
        auto &treeNodes = getTreeNodesRef();
        treeNodes.erase( std::remove( treeNodes.begin(), treeNodes.end(), node ), treeNodes.end() );
    }

    ConcurrentArray<SmartPtr<IUITreeNode>> &ImGuiTreeCtrl::getTreeNodesRef()
    {
        return m_treeNodes;
    }

    const ConcurrentArray<SmartPtr<IUITreeNode>> &ImGuiTreeCtrl::getTreeNodesRef() const
    {
        return m_treeNodes;
    }

    bool ImGuiTreeCtrl::StateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( message->getType() == clearHash )
        {
            if( auto owner = workphone::static_pointer_cast<ImGuiTreeCtrl>( getOwner() ) )
            {
                owner->clear();
            }
        }

        return false;
    }

    // Callback function for when a drag operation starts
    bool dragStartCallback( ImGuiPayload *payload )
    {
        // Payload contains data about the drag operation, such as the node index being dragged
        int nodeIndex = *static_cast<int *>( payload->Data );

        // Here we could do some additional checks to see if we allow the drag operation
        // For example, we could prevent the root node from being moved

        // Store the index of the node being dragged in ImGui's data context
        ImGui::SetDragDropPayload( "Node", &nodeIndex, sizeof( nodeIndex ) );

        return true;
    }

    // Callback function for when a drag operation is released
    bool dragEndCallback( ImGuiPayload *payload )
    {
        // Payload contains data about the drag operation, such as the node index being dragged
        int nodeIndex = *static_cast<int *>( payload->Data );

        // Here we could do some additional checks to see if we allow the drop operation
        // For example, we could prevent a node from being dropped onto its own children

        // Get the index of the node being dropped onto, which is stored in ImGui's data context
        int targetIndex = *static_cast<int *>( ImGui::GetDragDropPayload()->Data );

        //// Move the dragged node to the new position by updating its parent and sibling indices
        //Node &node = nodes[nodeIndex];
        //Node &target = nodes[targetIndex];

        //if( node.parent != target.parent )
        //{
        //    // Move the node to a new parent
        //    node.parent = target.parent;
        //    target.parent = nodeIndex;

        //    // Add the node to the target's children
        //    target.children.push_back( nodeIndex );

        //    // Remove the node from its old parent's children
        //    auto it = std::find( nodes[node.parent].children.begin(),
        //                         nodes[node.parent].children.end(), nodeIndex );
        //    if( it != nodes[node.parent].children.end() )
        //    {
        //        nodes[node.parent].children.erase( it );
        //    }
        //}
        //else
        //{
        //    // Move the node within its current parent's children
        //    auto it = std::find( target.children.begin(), target.children.end(), nodeIndex );
        //    IM_ASSERT( it != target.children.end() );
        //    target.children.erase( it );
        //    target.children.insert( it, nodeIndex );
        //}

        return true;
    }

    // Helper function to select nodes in range using DFS
    void ImGuiTreeCtrl::selectRange( SmartPtr<IUITreeNode> root, SmartPtr<IUITreeNode> node1,
                                     SmartPtr<IUITreeNode> node2, bool &in_range )
    {
        in_range = false;
        if( !root || !node1 || !node2 )
        {
            return;
        }

        Array<SmartPtr<IUITreeNode>> visibleNodes;
        std::function<void( SmartPtr<IUITreeNode> const & )> appendVisibleNodes;
        appendVisibleNodes = [&]( SmartPtr<IUITreeNode> const &node )
        {
            if( !node )
            {
                return;
            }

            visibleNodes.push_back( node );
            if( !node->isExpanded() )
            {
                return;
            }

            auto children = node->getChildren();
            for( auto &child : children )
            {
                if( child && child->isDerived<IUITreeNode>() )
                {
                    appendVisibleNodes( workphone::static_pointer_cast<IUITreeNode>( child ) );
                }
            }
        };
        appendVisibleNodes( root );

        auto first = std::find( visibleNodes.begin(), visibleNodes.end(), node1 );
        auto last = std::find( visibleNodes.begin(), visibleNodes.end(), node2 );
        if( first == visibleNodes.end() || last == visibleNodes.end() )
        {
            return;
        }

        if( first > last )
        {
            std::swap( first, last );
        }

        Array<SmartPtr<IUITreeNode>> range( first, last + 1 );
        setSelectedTreeNodes( range );
        m_selectedTreeNode = node2;
        in_range = true;
    }

    void ImGuiTreeCtrl::setLastSelectedNode( SmartPtr<IUITreeNode> lastSelectedNode )
    {
        m_lastSelectedNode = lastSelectedNode;
    }

    SmartPtr<IUITreeNode> ImGuiTreeCtrl::getLastSelectedNode() const
    {
        auto p = m_lastSelectedNode.load();
        return p.lock();
    }

    SmartPtr<IUITreeNode> ImGuiTreeCtrl::getNodeById( const hash_type &id ) const
    {
        auto nodes = getTreeNodes();
        for( auto &node : nodes )
        {
            if( node && node->getId() == id )
            {
                return node;
            }
        }

        return nullptr;
    }
}  // namespace workphone::ui
