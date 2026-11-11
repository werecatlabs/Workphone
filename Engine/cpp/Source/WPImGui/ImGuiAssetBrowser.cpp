#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiApplication.hpp>
#include <WPImGui/ImGuiAssetBrowserItems.hpp>
#include <WPImGui/ImGuiDropJob.hpp>
#include <WPImGui/ImGuiTreeCtrl.hpp>
#include <WPImGui/ImGuiTreeNode.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui_internal.h>
#include <algorithm>

namespace workphone::ui
{
    namespace
    {
        constexpr hash_type folderKind = 1;
        constexpr hash_type assetKind = 2;
        constexpr int listModeSize = 24;

        bool isFolder( const IUITreeNode &node )
        {
            return node.getElementId() == folderKind;
        }
    }

    void ImGuiTreeCtrl::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
            return;
        if( !properties->hasProperty( "assetBrowser" ) )
        {
            ImGuiElement<IUITreeCtrl>::setProperties( properties );
            return;
        }

        if( !properties->getPropertyAsBool( "assetBrowser" ) )
        {
            m_assetBrowser = nullptr;
            return;
        }

        auto state = workphone::make_shared<AssetBrowserState>();
        state->search = properties->getProperty( "assetSearch" );
        u32 folderId = 0;
        properties->getPropertyValue( "assetFolder", folderId );
        auto root = getRoot();
        if( root )
        {
            const auto nodes = getTreeNodes();
            for( const auto &node : nodes )
            {
                if( !node )
                    continue;

                if( isFolder( *node ) )
                {
                    if( auto parent = dynamic_pointer_cast<IUITreeNode>( node->getParent() ) )
                        state->folders[parent->getTreeNodeId()].push_back( node );
                    if( node->getTreeNodeId() == folderId )
                        state->folder = node;
                }

                if( !state->search.empty() &&
                    ( isFolder( *node ) || node->getElementId() == assetKind ) &&
                    StringUtil::make_lower( node->getName() ).find( state->search ) != String::npos )
                {
                    state->items.push_back( node );
                }
            }

            const auto roots = state->folders.find( root->getTreeNodeId() );
            if( !state->folder && roots != state->folders.end() && !roots->second.empty() )
                state->folder = roots->second.front();

            if( state->search.empty() && state->folder )
            {
                for( auto child : state->folder->getChildren() )
                    if( auto node = dynamic_pointer_cast<IUITreeNode>( child ) )
                        state->items.push_back( node );
            }
            std::stable_partition( state->items.begin(), state->items.end(),
                                   []( const auto &node ) { return isFolder( *node ); } );
        }
        m_assetBrowser = state;
    }

    void ImGuiTreeCtrl::drawAssetBrowser( const AssetBrowserState &state )
    {
        if( ImGui::IsMouseReleased( ImGuiMouseButton_Right ) &&
            ImGui::IsWindowHovered( ImGuiHoveredFlags_ChildWindows |
                                    ImGuiHoveredFlags_NoPopupHierarchy |
                                    ImGuiHoveredFlags_AllowWhenBlockedByPopup ) )
        {
            m_assetContextTarget = nullptr;
        }

        const auto available = ImGui::GetContentRegionAvail();
        const auto &style = ImGui::GetStyle();
        const float paneHeight = std::max( 1.0f, available.y - ImGui::GetFrameHeightWithSpacing() );
        ImGui::PushID( this );
        if( available.x >= 360.0f )
        {
            constexpr auto flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV |
                                   ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_SizingStretchProp;
            ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 5.0f, 0.0f ) );
            if( ImGui::BeginTable( "##AssetBrowserPanes", 2, flags ) )
            {
                ImGui::TableSetupColumn( "Folders", ImGuiTableColumnFlags_WidthFixed,
                                         std::clamp( available.x * 0.23f, 125.0f, 230.0f ) );
                ImGui::TableSetupColumn( "Assets", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableNextColumn();
                if( ImGui::BeginChild( "##AssetFolders", ImVec2( 0, paneHeight ) ) )
                {
                    if( auto root = getRoot() )
                        drawAssetFolders( state, root->getTreeNodeId() );
                    drawAssetContextMenu();
                }
                ImGui::EndChild();
                ImGui::TableNextColumn();
                drawAssetContents( state, paneHeight );
                ImGui::EndTable();
            }
            ImGui::PopStyleVar();
        }
        else
        {
            drawAssetContents( state, paneHeight );
        }

        const float sliderWidth = std::clamp( available.x, 1.0f, 118.0f );
        const float statusWidth = available.x - sliderWidth - style.ItemSpacing.x;
        if( statusWidth > 20.0f )
        {
            auto selected = getSelectedTreeNode();
            auto status = StringUtil::toString( state.items.size() ) + " items";
            if( selected && selected != state.folder )
                status = selected->getName();
            const auto position = ImGui::GetCursorScreenPos();
            const auto textY = position.y + style.FramePadding.y;
            ImGui::Dummy( ImVec2( statusWidth, ImGui::GetFrameHeight() ) );
            ImGui::RenderTextEllipsis( ImGui::GetWindowDrawList(), ImVec2( position.x, textY ),
                                      ImVec2( position.x + statusWidth, textY + ImGui::GetTextLineHeight() ),
                                      position.x + statusWidth, position.x + statusWidth,
                                      status.c_str(), nullptr, nullptr );
            if( ImGui::IsItemHovered() )
                ImGui::SetTooltip( "%s", status.c_str() );
            ImGui::SameLine();
        }
        ImGui::SetNextItemWidth( sliderWidth );
        ImGui::SliderInt( "##AssetIconSize", &m_assetIconSize, listModeSize, 96,
                          m_assetIconSize == listModeSize ? "List" : "Icon size",
                          ImGuiSliderFlags_NoInput );
        if( ImGui::IsItemHovered() )
            ImGui::SetTooltip( "Resize asset icons. Drag left for list view." );
        ImGui::PopID();
    }

    void ImGuiTreeCtrl::drawAssetFolders( const AssetBrowserState &state, u32 parentId )
    {
        const auto found = state.folders.find( parentId );
        if( found == state.folders.end() )
            return;

        for( auto node : found->second )
        {
            if( node->getLoadingState() != LoadingState::Loaded )
                continue;

            const auto id = node->getTreeNodeId();
            const auto label = Path::getFileName( node->getName() );
            const bool hasFolders = state.folders.find( id ) != state.folders.end();
            auto flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
                         ImGuiTreeNodeFlags_SpanAvailWidth;
            if( state.folder == node )
                flags |= ImGuiTreeNodeFlags_Selected;
            if( !hasFolders )
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if( static_cast<ImGuiTreeNode *>( node.get() )->consumeExpansionStateChange() )
                ImGui::SetNextItemOpen( node->isExpanded(), ImGuiCond_Always );
            const auto icon = node->getLabel();
            const bool open = ImGui::TreeNodeEx( node.get(), flags, "%s  %s", icon.c_str(), label.c_str() );
            const bool activate = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();
            handleAssetItem( node, nullptr, activate );
            if( hasFolders )
            {
                node->setExpanded( open );
                if( open )
                {
                    drawAssetFolders( state, id );
                    ImGui::TreePop();
                }
            }
        }
    }

    void ImGuiTreeCtrl::drawAssetContents( const AssetBrowserState &state, float height )
    {
        const auto folderId = state.folder ? state.folder->getTreeNodeId() : 0;
        if( folderId != m_assetViewFolder || state.search != m_assetViewSearch )
        {
            ImGui::SetNextWindowScroll( ImVec2( 0, 0 ) );
            m_assetViewFolder = folderId;
            m_assetViewSearch = state.search;
        }

        if( ImGui::BeginChild( "##AssetContents", ImVec2( 0, height ), false,
                               ImGuiWindowFlags_AlwaysVerticalScrollbar ) )
        {
            if( state.items.empty() )
            {
                ImGui::TextDisabled( "%s", !state.search.empty() ? "No assets match your search."
                    : state.folder ? "This folder is empty." : "Open a project to browse assets." );
            }

            const bool listMode = m_assetIconSize == listModeSize;
            const float width = std::max( 1.0f, ImGui::GetContentRegionAvail().x );
            const auto spacing = ImGui::GetStyle().ItemSpacing.x;
            const auto tileWidth = std::min( width, std::max( 72.0f, m_assetIconSize + 22.0f ) );
            const auto tileHeight = m_assetIconSize + ImGui::GetTextLineHeight() + 12.0f;
            const int columns = listMode ? 1 : std::max( 1, static_cast<int>(
                ( width + spacing ) / ( tileWidth + spacing ) ) );
            const int count = static_cast<int>( state.items.size() );
            ImGuiListClipper clipper;
            clipper.Begin( ( count + columns - 1 ) / columns );
            while( clipper.Step() )
            {
                for( int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row )
                {
                    for( int column = 0; column < columns && row * columns + column < count; ++column )
                    {
                        const auto &node = state.items[row * columns + column];
                        if( column != 0 )
                            ImGui::SameLine();
                        if( node->getLoadingState() != LoadingState::Loaded )
                        {
                            ImGui::Dummy( ImVec2( tileWidth, listMode ? ImGui::GetTextLineHeight() : tileHeight ) );
                            continue;
                        }
                        ImGui::PushID( node.get() );
                        if( listMode )
                            drawAssetBrowserRow( *node, node->isSelected() );
                        else
                            drawAssetBrowserTile( *node, node->isSelected(), ImVec2( tileWidth, tileHeight ) );
                        handleAssetItem( node, &state, ImGui::IsItemEdited() );
                        ImGui::PopID();
                    }
                }
            }
            if( state.folder )
            {
                const auto *window = ImGui::GetCurrentWindow();
                if( ImGui::BeginDragDropTargetCustom( window->InnerRect, window->ID ) )
                    acceptAssetDrop( state.folder );
                if( ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() &&
                    ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
                {
                    setSelectedTreeNode( state.folder );
                    notifyAssetItem( state.folder, IEvent::handleTreeSelectionRelease, true );
                }
            }
            drawAssetContextMenu();
        }
        ImGui::EndChild();
    }

    void ImGuiTreeCtrl::drawAssetContextMenu()
    {
        auto parent = dynamic_pointer_cast<IUIWindow>( getParent() );
        auto menu = parent ? parent->getContextMenu() : nullptr;
        if( menu && ImGui::BeginPopupContextWindow( "##AssetContext", ImGuiPopupFlags_MouseButtonRight ) )
        {
            auto application = core::IApplicationManager::instancePtr();
            auto manager = application ? application->getUI() : nullptr;
            auto target = m_assetContextTarget.lock();
            if( auto ui = manager ? dynamic_pointer_cast<ImGuiApplication>( manager->getApplication() ) : nullptr )
                for( auto item : menu->getMenuItems() )
                    ui->createMenuItem( menu, item, target );
            ImGui::EndPopup();
        }
    }

    void ImGuiTreeCtrl::handleAssetItem( SmartPtr<IUITreeNode> node, const AssetBrowserState *contents,
                                         bool activate )
    {
        const bool hovered = ImGui::IsItemHovered();
        const bool open = contents && hovered && ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left );
        const bool context = hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Right );
        if( ImGui::IsMouseReleased( ImGuiMouseButton_Right ) &&
            ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenBlockedByPopup ) &&
            node->getParent() != getRoot() )
        {
            m_assetContextTarget = node;
        }
        if( open || ( context && !node->isSelected() ) )
            setSelectedTreeNode( node );
        else if( activate )
            selectAssetItem( node, contents );
        if( activate || open || context )
            notifyAssetItem( node, IEvent::handleTreeSelectionRelease, contents != nullptr || context );
        if( open )
            notifyAssetItem( node, IEvent::handleTreeNodeDoubleClicked, true );

        if( auto source = getDragSource(); source && ImGui::IsItemActive() && ImGui::IsMouseDragging( 0 ) )
        {
            const auto payload = getDragPayload( node, source );
            if( !payload.empty() && ImGui::BeginDragDropSource() )
            {
                ImGui::SetDragDropPayload( "_TREENODE", payload.data(), payload.size() );
                const auto label = Path::getFileName( node->getName() );
                ImGui::TextUnformatted( label.c_str() );
                ImGui::EndDragDropSource();
            }
        }
        if( ImGui::BeginDragDropTarget() )
            acceptAssetDrop( node );
        if( hovered && !ImGui::IsMouseDragging( 0 ) )
        {
            const auto path = node->getName();
            ImGui::SetTooltip( "%s", path.c_str() );
        }
    }

    void ImGuiTreeCtrl::selectAssetItem( SmartPtr<IUITreeNode> node, const AssetBrowserState *contents )
    {
        const auto &io = ImGui::GetIO();
        if( contents && isMultiSelect() && io.KeyCtrl )
        {
            auto nodes = getSelectedTreeNodes();
            const auto selected = std::find( nodes.begin(), nodes.end(), node );
            if( selected == nodes.end() )
                nodes.push_back( node );
            else
                nodes.erase( selected );
            setSelectedTreeNodes( nodes );
            return;
        }
        if( contents && isMultiSelect() && io.KeyShift )
        {
            const auto &items = contents->items;
            const auto anchor = getLastSelectedNode();
            const auto first = std::find( items.begin(), items.end(), anchor );
            const auto last = std::find( items.begin(), items.end(), node );
            if( first != items.end() && last != items.end() )
            {
                auto range = Array<SmartPtr<IUITreeNode>>( std::min( first, last ), std::max( first, last ) + 1 );
                if( first > last )
                    std::reverse( range.begin(), range.end() );
                setSelectedTreeNodes( range );
                setLastSelectedNode( anchor );
                return;
            }
        }
        setSelectedTreeNode( node );
    }

    void ImGuiTreeCtrl::notifyAssetItem( SmartPtr<IUITreeNode> node, hash_type event, bool contents )
    {
        if( auto application = core::IApplicationManager::instancePtr() )
        {
            Parameters args;
            args.emplace_back( static_pointer_cast<ISharedObject>( node ) );
            args.emplace_back( contents );
            application->triggerEvent( EventType::UI, event, args, this, node, nullptr, false,
                                       Thread::Application_Flag );
        }
    }

    void ImGuiTreeCtrl::acceptAssetDrop( SmartPtr<IUITreeNode> destination )
    {
        if( auto target = getDropTarget() )
        {
            if( auto payload = ImGui::AcceptDragDropPayload( "_TREENODE" ) )
            {
                Parameters args;
                args.emplace_back( String( static_cast<const char *>( payload->Data ), payload->DataSize ) );
                args.emplace_back( -1 );
                ImGuiDropJob::queue( target, std::move( args ), getDragSourceElement(), destination );
            }
        }
        ImGui::EndDragDropTarget();
    }
}  // namespace workphone::ui
