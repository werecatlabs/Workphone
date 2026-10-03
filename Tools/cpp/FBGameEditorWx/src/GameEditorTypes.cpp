#include <GameEditorPCH.hpp>
#include "GameEditorTypes.hpp"
#include <FBCore/Base/StringUtil.hpp>



namespace fb
{	
	namespace editor
	{



		const u32 GameEditorTypes::SCENE_VIEW_ID_TERRAIN = StringUtil::getHash("SceneViewIdTerrain");
		const u32 GameEditorTypes::SCENE_VIEW_ID_MESH = StringUtil::getHash("SceneViewIdMesh");
		const u32 GameEditorTypes::SCENE_VIEW_ID_PARTICLE = StringUtil::getHash("SceneViewIdParticle");

		const u32 GameEditorTypes::CAMERA_TYPE_VIEWPORT_0 = StringUtil::getHash("CameraTypeViewport0");
		const u32 GameEditorTypes::CAMERA_TYPE_VIEWPORT_1 = StringUtil::getHash("CameraTypeViewport1");
		const u32 GameEditorTypes::CAMERA_TYPE_VIEWPORT_2 = StringUtil::getHash("CameraTypeViewport2");
		const u32 GameEditorTypes::CAMERA_TYPE_VIEWPORT_3 = StringUtil::getHash("CameraTypeViewport3");



	} // end namespace editor
} // end namespace fb


