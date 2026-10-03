#include <EditorPCH.hpp>
#include <EditorTypes.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::editor
{
    const hash_type GameEditorTypes::SCENE_VIEW_ID_TERRAIN = StringUtil::getHash( "SceneViewIdTerrain" );
    const hash_type GameEditorTypes::SCENE_VIEW_ID_MESH = StringUtil::getHash( "SceneViewIdMesh" );
    const hash_type GameEditorTypes::SCENE_VIEW_ID_PARTICLE =
        StringUtil::getHash( "SceneViewIdParticle" );

    const hash_type GameEditorTypes::CAMERA_TYPE_VIEWPORT_0 =
        StringUtil::getHash( "CameraTypeViewport0" );
    const hash_type GameEditorTypes::CAMERA_TYPE_VIEWPORT_1 =
        StringUtil::getHash( "CameraTypeViewport1" );
    const hash_type GameEditorTypes::CAMERA_TYPE_VIEWPORT_2 =
        StringUtil::getHash( "CameraTypeViewport2" );
    const hash_type GameEditorTypes::CAMERA_TYPE_VIEWPORT_3 =
        StringUtil::getHash( "CameraTypeViewport3" );
}  // namespace workphone::editor
