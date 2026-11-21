#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUITerrainEditor.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUITerrainEditor, IUIElement );

    IUITerrainEditor::IUITerrainEditor( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUITerrainEditor::IUITerrainEditor() : IUIElement( IUITerrainEditor::typeInfo() )
    {
    }

    const hash_type IUITerrainEditor::selectTerrainTextureHash =
        StringUtil::getHash( "selectTerrainTexture" );

    IUITerrainEditor::~IUITerrainEditor() = default;
}  // namespace workphone::ui
