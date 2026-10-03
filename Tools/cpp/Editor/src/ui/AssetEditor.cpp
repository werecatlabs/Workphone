#include <EditorPCH.hpp>
#include "ui/AssetEditor.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{

    AssetEditor::AssetEditor()
    {
    }

    AssetEditor::~AssetEditor()
    {
    }

    void AssetEditor::load( SmartPtr<ISharedObject> data )
    {
    }

    void AssetEditor::unload( SmartPtr<ISharedObject> data )
    {
        EditorWindow::unload( data );
    }

}  // namespace workphone::editor
