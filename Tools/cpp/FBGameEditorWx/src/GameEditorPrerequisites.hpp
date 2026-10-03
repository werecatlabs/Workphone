#ifndef GameEditorPrerequisites_h__
#define GameEditorPrerequisites_h__



#define _WINSOCKAPI_ 

#include <FBCore/Memory/SmartPtr.hpp>
#include <FBCore/Thread/Threading.hpp>
#include <FBCore/FBCoreHeaders.hpp>
#include <FBCore/Base/Singleton.hpp>
#include <FBCore/FBCorePrerequisites.hpp>
#include <FBApplication/FBApplicationPrerequisites.hpp>
#include <FBState/FBStatePrerequisites.hpp>
#include <FBObjectTemplates/FBObjectTemplatesPrerequisites.hpp>
#include <FBProcedural/FBProceduralPrerequisites.hpp>
#include <FBWxWidgets/FBWxWidgetsPrerequisites.hpp>



//forward decs
class wxButton;
class wxListCtrl;
class wxCheckBox;
class wxNotebook;
class wxAuiManager;
class wxAuiNotebook;
class wxTextCtrl;
class wxToggleButton;
class wxTreeCtrl;
class wxToggleButton;
class wxPropertyGrid;
class wxMenu;
class wxTimeline;
class wxTimer;
//forward decs
class wxToggleButton;
class wxButton;
class wxFrameManager;
class wxBitmapComboBox;
class wxComboBox;
class wxSlider;
class wxWindow;
class wxBoxSizer;



namespace fb
{	
	namespace editor 
	{


		// forward decs
		class ApplicationFrame;
		class ProjectWindow;
		class TerrainWindow;
		class PropertiesWindow;
		class ActorWindow;

		class EditorGrid;

		//forward declarations
		class LuaEdit;
		class OutputWindow;
		
		// forward decs
		class ComponentTemplateMgr;
		class EntityTemplateMgr;
		class UIManager;
		class FoliageTool;
		class Foliage;
		class FoliageLayer;
		class FoliageManager;
		class IDebugOutput;
		class LuaEditConfig;
		class SceneViewManager;
		class MessageManager;
		class MessageScriptError;
		class PlacementManager;
		class Project;
		class ProjectManager;
		class EditorScriptManager;
		class IMessageListener;
		class ITerrainTool;
		class ITerrainManagerListener;
		class RenderWindow;
		class FileWindow;
		class ProjectWindow;
		class SceneWindow;
		class RiverManager;
		class RoadManager;
		class IRoadManagerListener;
		class TerrainManager;
		class RigidBodyMesh;

		class EditorEntity;
		class CityLayer;
		class RoadEnt;
		class RoadNodeEnt;
		class RoadSegment;
		class PavementEnt;
		class JunctionConnection;
		class RiverEnt;
		class FoliageEnt;
		class EntityIntersectionData;
		class CityCenter;

		class BuildingRecord;

		class IRiverManagerListener;
		class IFoliageManagerListener;

		class EditorMap;

		class ActorWindow;
		class SceneWindow;

		//forward declarations
		class EditorManager;
		class ApplicationFrame;

		//forward decs
		class EditorSceneManager;
		class DecalSceneNode;
		class DecalCursor;

		class MeshImportWindow;
		class FoliageWindow;
		class RoadFrame;
		class HoudiniWindow;
		class FileSelection;
		class TextureWindow;
		class MaterialWindow;

		class ActorWindow;
		class ProjectWindow;
		class ObjectWindow;

		
		// smart pointers
		typedef SmartPtr<ComponentTemplateMgr> ComponentTemplateMgrPtr;
		typedef SmartPtr<SceneViewManager> SceneViewManagerPtr;
		typedef SmartPtr<MessageManager> MessageManagerPtr;
		typedef SmartPtr<Project> ProjectPtr;
		typedef SmartPtr<ProjectManager> ProjectManagerPtr;
		typedef SmartPtr<EditorScriptManager> EditorScriptManagerPtr;
		


	} // end namespace editor
} // end namespace fb



#endif // GameEditorPrerequisites_h__


