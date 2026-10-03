#ifndef GameEditorPrerequisites_h__
#define GameEditorPrerequisites_h__

#define _WINSOCKAPI_

#include <Workphone/WorkphonePrerequisites.hpp>

namespace workphone
{
    namespace editor
    {
        class SceneDropJob;
        class SceneWindow;

        class PromptEvaluator;
        class RemovePromptEvaluator;

        class ActorWindow;
        class AnimationWindow;
        class AnimationGraphWindow;
        class ApplicationFrame;
        class EditorWindow;
        class BuildingRecord;
        class CityLayer;
        class CityCenter;
        class CollisionMaskDialog;
        class CollisionMaskManager;
        class ComponentTemplateMgr;
        class DecalSceneNode;
        class DecalCursor;
        class EventListenerWindow;
        class EventsWindow;
        class EventWindow;
        class EntityTemplateMgr;
        class EditorGrid;
        class EditorEntity;
        class EditorMap;
        class EditorManager;
        class EditorSceneManager;
        class EntityIntersectionData;
        class FoliageEnt;
        class FoliageWindow;
        class FileViewWindow;
        class FileWindow;
        class FileSelection;
        class FoliageTool;
        class Foliage;
        class FoliageLayer;
        class FoliageManager;
        class HoudiniWindow;
        class InputManagerWindow;
        class ITerrainTool;
        class ITerrainManagerListener;
        class IDebugOutput;
        class IRiverManagerListener;
        class IFoliageManagerListener;
        class LayerDialog;
        class LayerManager;
        class JunctionConnection;
        class LuaEdit;
        class LuaEditConfig;
        class MeshImportWindow;
        class MessageScriptError;
        class MaterialWindow;
        class OutputWindow;
        class ObjectWindow;
        class ObjectBrowserDialog;
        class ProjectWindow;
        class PropertiesWindow;
        class ProjectWindow;
        class ProjectAssetsWindow;
        class PlacementManager;
        class Project;
        class ProjectWindow;
        class ProfilerWindow;
        class PavementEnt;
        class ResourceDatabaseDialog;
        class ResourceWindow;
        class RenderWindow;
        class RoadFrame;
        class RiverManager;
        class RoadManager;
        class RoadEnt;
        class RoadNodeEnt;
        class RoadSegment;
        class RiverEnt;
        class RigidBodyMesh;
        class IRoadManagerListener;
        class SceneWindow;
        class ScriptWindow;
        class TagDialog;
        class TagManager;
        class TransformWindow;
        class TerrainManager;
        class TerrainWindow;
        class TextureWindow;
        class UIManager;

        // A1/A2: Procedural bindings
        class ProceduralBindings;
        class ProceduralTextureBindings;
        class SceneBuilderBindings;
    }  // end namespace editor
}  // namespace workphone

#endif  // GameEditorPrerequisites_h__

