#ifndef __WP_CorePrerequisites_h__
#define __WP_CorePrerequisites_h__

#include <Workphone/WorkphoneTypes.hpp>

using ZZIP_DIR = struct zzip_dir;
using ZZIP_FILE = struct zzip_file;
struct zip_t;

// forward declarations
class TiXmlElement;
class TiXmlNode;
class TiXmlElement;
class TiXmlDocument;
class TiXmlHandle;
struct cJSON;

namespace Opcode
{
    class RayCollider;
    class Model;
    class MeshInterface;
}  // namespace Opcode

namespace IceMaths
{
    class Point;
}  // namespace IceMaths

namespace boost
{
    namespace json
    {
        class object;
    }  // namespace json
}  // namespace boost

namespace workphone
{
    struct BaseObjectData;
    struct SharedObjectData;

    class ActorLoadJob;

    class IObject;
    class ISharedObject;
    class ISharedObjectListener;
    class FactoryManager;
    class Property;
    class Properties;
    class Resolution;
    class IData;
    class ICoroutineData;
    class Handle;
    class ILockTrackerListener;
    class IBuildDirector;

    // Ai
    class IAiAgent;
    class IAiTrack;
    class IAiWaypoint;
    class IPathfinder2;
    class IPathfinder3;
    class IPathNode2;
    class IPathNode3;

    // mesh
    class MeshSerializerListener;
    class LodStrategy;

    // animation
    class IAnimation;
    class IAnimationContainer;
    class IAnimationInterface;
    class IAnimationKeyFrame;
    class IAnimationMorphKeyFrame;
    class IAnimationPoseKeyFrame;
    class IAnimationTimeIndex;
    class IAnimationTrack;
    class IAnimationNumericTrack;
    class IAnimationVertexTrack;
    class IAnimator;
    class IAnimableValue;
    class IActorAnimationTrack;

    // ai
    class IAi;
    class IAiCompositeGoal;
    class IAiGoal;
    class IAiGoalEvaluator;
    class IAiManager;
    class IAiSteering3;
    class IAiTargeting3;
    class IPathfinder2;
    class IPathNode2;
    class IAiScene;
    class IVehicleAiManager;
    class IAiTrackElement;
    class IAiTrack;

    // audio
    class IAudioEffect;
    class IAudioProcessor;
    class IAudioEffectVolume;
    class IAudioEffectDelay;
    class IAudioBusBuffers;
    class IAudioProcessData;

    // command
    class ICommand;
    class ICommandManager;
    class ICommandManagerListener;
    class IMouseCommand;

    // core
    class IEvent;
    class ILogManager;

    // database
    class IDatabase;
    class IDatabaseQuery;
    class IDatabaseManager;

    // finite state machine
    class IFSM;
    class IFSMListener;
    class IFSMManager;

    // input
    class InputState;
    class IInputEvent;
    class IInputManager;

    // ik
    class IIKController;
    class IIKGoal;
    class IIKJoint;
    class IIKManager;

    // math
    template <class T>
    class Vector2;

    template <class T>
    class Vector3;

    template <class T>
    class Transform3;

    // mesh
    class IBone;
    class IIndexBuffer;
    class IMesh;
    class IMeshLoader;
    class IMeshPose;
    class IMeshResource;
    class ISkeleton;
    class ISubMesh;
    class IVertexBoneAssignment;
    class IVertexBuffer;
    class IVertexDeclaration;
    class IVertexElement;

    // messages
    class StatePhysicsForce2;
    class StatePhysicsVelocity2;
    class StatePhysicsDynamicState2;
    class StateMessageLoad;
    class StateMessageStringValue;
    class StateMessageSetTexture;
    class StateMessageFragmentParam;
    class StateMessageMaterialName;
    class StateMessagePlay;
    class StateMessageStop;
    class StateMessageVisible;
    class StateMessageIntValue;
    class StateMessageUIntValue;
    class StateMessageAnimationEnable;
    class StateMessageDirty;
    class StateMessageBlendMapValue;
    class StateMessageVector3;

    // input
    class IChordDetector;
    class IGameInput;
    class IGameInputMap;
    class IGameInputState;
    class IInputAction;
    class IInputConverter;
    class IInputEvent;
    class IInputDeviceManager;
    class IJoystick;
    class IJoystickState;
    class IKeyboardState;
    class IMouseState;
    class ISequenceDetector;
    class ITapDetector;

    // io
    class IArchive;
    class IStream;
    class IFileList;
    class IFileListener;
    class IFileSystem;
    class IFolderExplorer;
    class IFolderExplorerW;
    class INativeFileDialog;

    // mesh
    class CollisionMesh;
    class CollisionSubMesh;
    class EdgeData;
    class IndexBuffer;
    class Mesh;
    class MeshLodUsage;
    class GraphicsSkeleton;
    class SubMesh;
    class VertexBoneAssignment;
    class VertexBuffer;
    class VertexDeclaration;

    // net
    class INetworkListener;
    class INetworkManager;
    class INetworkView;
    class INetworkPlayer;
    class INetworkStream;
    class IPacket;
    class ISystemAddress;

    // resource
    class IResource;
    class IResourceManager;
    class IResourceDatabase;
    class IResourceGroupManager;

    // script
    class IScript;
    class IScriptClass;
    class IScriptBreakpoint;
    class IScriptData;
    class IScriptEvent;
    class IScriptFunction;
    class IScriptInvoker;
    class IScriptManager;
    class IScriptReceiver;
    class IScriptObject;
    class IScriptVariable;

    // sound
    class ISound;
    class ISoundListener3;
    class ISoundManager;
    class ISoundPlayer;
    class ISoundEventGroup;
    class ISoundEvent;
    class ISoundEventParam;
    class ISoundProject;

    // system
    class IAsyncOperation;
    class IConsole;
    class IEditorManager;
    class IEvent;
    class IEventListener;
    class IFrameStatistics;
    class IFrameGrabber;
    class IFactory;
    class IFactoryManager;
    class IJob;
    class IJobGroup;
    class IJobQueue;
    class ILibrary;
    class IOutputManager;
    class IPackageManager;
    class IPlugin;
    class IPluginManager;
    class IProfile;
    class IProfiler;
    class IProject;
    class IProcessManager;
    class ISelectionManager;
    class IStateContext;
    class IState;
    class IStateListener;
    class IStateContext;
    class IStateQueue;
    class IStateManager;
    class IStateMessage;
    class IStateManager;
    class IScheduler;
    class ISystemSettings;
    class ISystemManagerListener;
    class ITask;
    class ITaskLock;
    class ITaskManager;
    class IThreadPool;
    class ITimer;
    class IWorkerThread;

    // test
    class ITest;
    class ITestManager;

    class AssetDatabaseManager;
    class DatabaseManager;

    class PoolDataTBB;
    class PoolDataStandard;
    class Plot;
    class StateQueue;
    class StateContextTBB;
    class StateQueueTBB;
    class TaskDataStandard;
    class Task;
    class TaskManager;

    class FileList;
    class ObfuscatedZipArchive;
    class ZipArchive;

    class MLP;

    class ProjectManager;

    class RotateManipulator;
    class TranslateManipulator;
    class ScaleManipulator;

    class StateMessageBuffer;
    class StateMessageJobStatus;
    class StateMessageProperties;
    class StateMessageObject;
    class StateMessageObjectsArray;
    class StateMessageContact2;
    class StateMessageType;
    class StateMessageUIntValue;
    class StateMessageFloatValue;
    class StateMessageVector3;
    class StateMessageVector4;

    class State;
    class BoxShapeStateData;
    class GraphicsMeshState;
    class GraphicsObjectData;
    class LightStateData;
    class MaterialPassStateData;
    class MeshShapeStateData;
    class OverlayState;
    class OverlayTextState;
    class RenderTextureState;
    class WindowStateData;
    class RigidbodyState;
    class GraphicsSceneState;
    class SceneNodeStateData;
    class SkyStateData;
    class TerrainStateData;
    class TextureStateData;
    class TransformStateData;
    class UIAnchorStateData;
    class UIElementStateData;
    class UILayoutStateData;
    class UITransformStateData;
    class ViewportStateData;
    class TransformStateData;

    class Test;
    class TestManager;

    class Director;

    namespace core
    {
        class IApplication;
        class IApplicationManager;
        class ApplicationManager;

    }  // end namespace core

    namespace reflection
    {
        struct Property;
        struct Method;
        struct Type;
    }  // namespace reflection

    namespace scene
    {
        class IGameActor;
        class IActorProxy;
        class IGraphicsCameraController;
        class ICameraManager;
        class IComponent;
        class IComponentEvent;
        class IComponentEventListener;
        class IComponentSystem;
        class IGamePrefab;
        class IGamePrefabManager;
        class IGameScene;
        class IGameManager;
        class ISubComponent;
        class ITransform;

    }  // namespace scene

    // physics
    namespace physics
    {
        class IPhysicsShape;
        class IPhysicsShape2;
        class IPhysicsShape3;
        class IBoxShape2;
        class IBoxShape3;
        class ICharacterController2;
        class ICharacterController3;
        class IPhysicsBody2D;
        class IPhysicsBody3;
        class IRigidBody2;
        class IRigidBody3;
        class IRigidDynamic3;
        class IRigidStatic3;
        class IPhysicsManager2D;
        class IPhysicsManager;
        class IPhysicsMaterial2;
        class IPhysicsMaterial3;
        class IPhysicsParticle2;
        class IPhysicsParticle3;
        class IPlaneShape3;
        class ISphereShape2;
        class ISphereShape3;
        class ITerrainShape;
        class IPhysicsVehicle3;
        class IPhysicsVehicleInput3;
        class IPhysicsVehicleWheel3;
        class IPhysicsScene2;
        class IPhysicsScene3;
        class IPhysicsEffect2;
        class IPhysicsBodyEffectSnap2;
        class IRaycastHit;
        class IPhysicsConstraint2;
        class IPhysicsConstraint3;
        class IConstraintD6;
        class IConstraintDrive;
        class IConstraintFixed3;
        class IPhysicsCooking;
        class IPhysicsSpring;
        class IConstraintLimit;
        class IConstraintLinearLimit;
        class IPhysicsSoftBody2;
        class IPhysicsSoftBody3;
        class IMeshShape;
    }  // namespace physics

    // procedural
    namespace procedural
    {

        class IBlockGenerator;
        class ICityMap;
        class ICityBlock;
        class ILot;
        class ILSystem;
        class ILSystemRule;
        class IProceduralCollision;
        class ICityGenerator;
        class ITerrainGenerator;
        class IRoadNetwork;
        class IProceduralCity;
        class IProceduralCityCenter;
        class IProceduralManager;
        class IProceduralNode;
        class IProceduralObject;
        class IProceduralTerrain;
        class IProceduralScene;
        class IProceduralWorld;
        class IMeshGenerator;
        class IRoadGenerator;
        class IRoad;
        class IRoadConnection;
        class IRoadConnectionData;
        class IRoadElement;
        class IRoadSection;
        class IRoadMeshElement;
        class IRoadNode;
        class ISidewalk;
        class ITerrainGenerator;

    }  // namespace procedural

    // ui
    namespace ui
    {
        class IUIAbout;
        class IUIApplication;
        class IUIButton;
        class IUICheckbox;
        class IUICollapsingHeader;
        class IUIColourPicker;
        class IUILayoutContainer;
        class IUICursor;
        class IUIDataGrid;
        class IUIDragSource;
        class IUIDropdown;
        class IUIDropTarget;
        class IUIElement;
        class IUIEventWindow;
        class IUIFileBrowser;
        class IUIFrame;
        class IUIGrid;
        class IUIImage;
        class IUIImageArray;
        class IUIInputManager;
        class IUILabelTogglePair;
        class IUILabelDropdownPair;
        class IUILabelSliderPair;
        class IUILabelTextInputPair;
        class IUILayoutWindow;
        class IUIManager;
        class IUIMenu;
        class IUIMenubar;
        class IUIMenuItem;
        class IUIPropertyGrid;
        class IUIProfilerWindow;
        class IUIProfileWindow;
        class IUIRenderWindow;
        class IUIScrollingText;
        class IUISlider;
        class IUISpinner;
        class IUITabBar;
        class IUITabItem;
        class IUIText;
        class IUITextEntry;
        class IUITerrainEditor;
        class IUIToggle;
        class IUIToggleGroup;
        class IUIToolbar;
        class IUITreeCtrl;
        class IUITreeNode;
        class IUIVector2;
        class IUIVector3;
        class IUIVector4;
        class IUIWindow;
    }  // namespace ui

    // graphics
    namespace render
    {
        class IAnimationState;
        class IAnimationController;
        class IAnimationControllerListener;
        class IAnimationStateController;
        class IAnimationTextureControl;
        class IBillboard;
        class IBillboardSet;
        class IGraphicsBone;
        class IGraphicsCamera;
        class IGraphicsCubemap;
        class IGraphicsWindowEvent;
        class IGraphicsWindowListener;
        class IDebug;
        class IDebugCircle;
        class IDebugLine;
        class IDecalCursor;
        class IGraphicsDeferredShading;
        class IDepthBuffer;
        class IDynamicLines;
        class IDynamicMesh;
        class IFont;
        class IFontManager;
        class IFrustum;
        class IGraphicsSystem;
        class IGraphicsMesh;
        class IGraphicsSkeleton;
        class IGraphicsSubMesh;
        class IGraphicsObject;
        class IGraphicsState;
        class IGraphicsSettings;
        class IHardwareBuffer;
        class IHardwareIndexBuffer;
        class IHardwareVertexBuffer;
        class IInstancedObject;
        class IInstanceManager;
        class IGraphicsLight;
        class ILightmap;
        class ILightmapper;
        class IMaterial;
        class IMaterialNode;
        class IMaterialManager;
        class IMaterialNodePasses;
        class IMeshConverter;
        class IOverlay;
        class IOverlayElement;
        class IOverlayElementContainer;
        class IOverlayElementText;
        class IOverlayElementVector;
        class IOverlayManager;
        class IMaterialPass;
        class IMaterialTechnique;
        class IMaterialShader;
        class IMaterialEvent;
        class IMaterialNodeAnimatedTexture;
        class IRenderer;
        class IRenderer2;
        class IRenderer3;
        class IRenderTarget;
        class IRenderTask;
        class IGraphicsSceneNode;
        class ISky;
        class ISkybox;
        class ISkyboxCube;
        class ISkyboxPlane;
        class ISkySphere;
        class IGraphicsScene;
        class IScreenSpaceEffect;
        class IScreenSpaceEffectRenderer;
        class ISkybox;
        class ISprite;
        class ISpriteRenderer;

        class IGraphicsTerrain;
        class ITerrainBlendMap;
        class ITerrainRayResult;
        class ITexture;
        class ITextureManager;
        class IMaterialTexture;

        class IVideoManager;
        class IVideo;
        class IVideoStream;
        class IVideoTexture;

        class IViewport;
        class IGraphicsWater;
        class IGraphicsWindow;
        class IGraphicsWindowEvent;
        class IGraphicsWindowListener;

        // particle
        class IParticleAffector;
        class IParticleEmitter;
        class IParticle;
        class IParticleManager;
        class IParticleNode;
        class IParticleRenderer;
        class IParticleSystem;
        class IParticleTechnique;

        class CameraVisibilitySet;
    }  // namespace render

    // vehicle
    namespace vehicle
    {
        class IVehicle;
        class IVehicleManager;
        class IVehicleCallback;

        class IVehicleComponent;
        class IDriveTrain;
        class IDifferential;
        class IVehiclePowerUnit;
        class IGearBox;
        class IWheelComponent;
        class IBatteryPack;
        class IESController;
        class IVehicleBody;
        class ITruckController;

        // aerodynamics
        class IAerodymanicsWind;
        class IAircraftCallback;
        class IAircraft;
        class IAerofoil;
        class IAircraftPowerUnit;
        class IAircraftPropeller;
        class IAircraftPropellerUnit;
        class IAircraftWing;
        class IAircraftControlSurface;
        class IAircraftPowerUnit;
        class IAircraftBody;
        class IAircraftPropWash;
    }  // namespace vehicle

    namespace scene
    {
        class AudioEmitter;
        class AudioSource;

        class Camera;
        class CameraFollow;
        class CameraTarget;
        class CameraController;
        class FPSCameraController;
        class EditorCameraController;
        class SphericalCameraController;
        class ThirdPersonCameraController;
        class VehicleCameraController;

        class FiniteStateMachine;

        class Constraint;
        class Collision;
        class CollisionMesh;
        class CollisionBox;
        class Material;
        class Mesh;
        class MeshRenderer;
        class NetworkView;
        class ParticleSystem;
        class Rigidbody;
        class RigidbodyListener;
        class Skybox;
        class Shader;
        class Tooltip;
        class Transform;
        class WheelController;

        class ButtonDirector;
        class GraphicsSettingsDirector;
        class MeshResourceDirector;
        class LightingDirector;
        class UiDialogDirector;
        class UiDirector;
        class UiElementDirector;

        // terrain
        class TerrainBlendMap;
        class TerrainSystem;
        class TerrainData;
        class TerrainLayer;
        class TerrainTreeLayer;
        class TerrainGrassLayer;

        // ui
        class Button;
        class Dropdown;
        class GridLayout;
        class HorizontalLayout;
        class Image;
        class InputField;
        class Layout;
        class LayoutTransform;
        class Image;
        class ScrollBar;
        class ScrollView;
        class Slider;
        class TabItem;
        class TableCell;
        class TableLayout;
        class TabPage;
        class TabView;
        class Text;
        class Thumbnail;
        class Toggle;
        class ToggleGroup;
        class ToolTip;
        class UIComponent;
        class VerticalLayout;

    }  // namespace scene

    // Object flags
    static const u8 OBJECT_FLAG_RESERVED = ( 1 << 0 );
    static const u8 OBJECT_FLAG_GARBAGE_COLLECTED = ( 1 << 1 );
    static const u8 OBJECT_FLAG_ALIVE = ( 1 << 2 );
    static const u8 OBJECT_FLAG_POOL_ELEMENT = ( 1 << 3 );

    // For object to trigger events
    static const u8 OBJECT_FLAG_TRIGGER_EVENTS = ( 1 << 4 );

    // Flags for object to trigger global events
    static const u8 OBJECT_FLAG_GLOBAL_EVENTS = ( 1 << 5 );

    // Flag for object to receive events
    static const u8 OBJECT_FLAG_RECEIVE_EVENTS = ( 1 << 6 );

    static const u8 OBJECT_FLAG_TRACK_REFERENCES = ( 1 << 7 );

}  // namespace workphone

#endif  // __WP_CorePrerequisites_h__
