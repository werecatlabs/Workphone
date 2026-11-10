#ifndef ImGuiAssetBrowserItems_h__
#define ImGuiAssetBrowserItems_h__

struct ImVec2;

namespace workphone::ui
{
    class IUITreeNode;

    // Each function leaves the full asset hit area as the current ImGui item.
    void drawAssetBrowserTile( const IUITreeNode &node, bool selected, const ImVec2 &size );
    void drawAssetBrowserRow( const IUITreeNode &node, bool selected );
}

#endif  // ImGuiAssetBrowserItems_h__
