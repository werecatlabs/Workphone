#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/WorkphonePlugin.hpp>
#include <Workphone/Scene/Components/Script.hpp>
#include <Workphone/Jobs/RunCommandJob.hpp>
#include <Workphone/IO/FolderListing.hpp>
#include <Workphone/IO/FileSystem.hpp>
#include <Workphone/IO/FileSystemArchive.hpp>
#include <Workphone/IO/FileDataStream.hpp>
#include <Workphone/IO/ObfuscatedZipArchive.hpp>
#include <Workphone/IO/ObfuscatedZipFile.hpp>
#include <Workphone/IO/ZipArchive.hpp>
#include <Workphone/IO/ZipFile.hpp>
#include <Workphone/Database/ResourceDatabase.hpp>
#include <Workphone/Memory/FactoryUtil.hpp>
#include <Workphone/Scene/GamePrefab.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/ConfigFile.hpp>
#include <Workphone/System/JobFunction.hpp>
#include <Workphone/System/Rttr.hpp>
#include <Workphone/System/FSM.hpp>
#include <Workphone/Vehicle/Vehicle.hpp>
#include <Workphone/Vehicle/WheelController.hpp>
#include <Workphone/Application.hpp>
#include <Workphone/WorkphoneHeaders.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, WorkphonePlugin, ISharedObject );

    WorkphonePlugin::WorkphonePlugin() = default;

    WorkphonePlugin::~WorkphonePlugin() = default;

    void WorkphonePlugin::load( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto typeManager = TypeManager::instance();

        FactoryUtil::addFactory<ConfigFile>();
        FactoryUtil::addFactory<FSM>();
        FactoryUtil::addFactory<FSMListener>();
        FactoryUtil::addFactory<FSMManager>();

        FactoryUtil::addFactory<CameraManagerReset>();
        FactoryUtil::addFactory<EventJob>();
        FactoryUtil::addFactory<JobFunction>();
        FactoryUtil::addFactory<LoadPluginJob>();
        FactoryUtil::addFactory<RunCommandJob>();
        FactoryUtil::addFactory<UnloadPluginJob>();

        FactoryUtil::addFactory<GameInput>();
        FactoryUtil::addFactory<GameInputState>();
        FactoryUtil::addFactory<InputDeviceManager>();
        FactoryUtil::addFactory<InputEvent>();
        FactoryUtil::addFactory<Joystick>();
        FactoryUtil::addFactory<JoystickState>();
        FactoryUtil::addFactory<KeyboardState>();
        FactoryUtil::addFactory<MouseState>();

        FactoryUtil::addFactory<FrameStatistics>();
        FactoryUtil::addFactory<Profiler>();
        FactoryUtil::addFactory<Properties>();
        FactoryUtil::addFactory<ProcessManager>();
        FactoryUtil::addFactory<StateQueue>();
        FactoryUtil::addFactory<StateContext>();
        FactoryUtil::addFactory<StateContext::SharedObjectListener>();
        FactoryUtil::addFactory<StateManager>();
        FactoryUtil::addFactory<WorkerThread>();

        FactoryUtil::addFactory<AssetDatabaseManager>();
        FactoryUtil::addFactory<DatabaseManager>();
        FactoryUtil::addFactory<ResourceDatabase>();
        FactoryUtil::addFactory<ResourceDatabase::ImportFileJob>();

        FactoryUtil::addFactory<FolderListing>();

        FactoryUtil::addFactory<FileList>();
        FactoryUtil::addFactory<FileSystem>();
        FactoryUtil::addFactory<FileDataStream>();
        FactoryUtil::addFactory<FileSystemArchive>();
        FactoryUtil::addFactory<ObfuscatedZipArchive>();
        FactoryUtil::addFactory<ObfuscatedZipFile>();
        FactoryUtil::addFactory<ZipArchive>();
        FactoryUtil::addFactory<ZipFile>();

        FactoryUtil::addFactory<vehicle::Vehicle>();
        FactoryUtil::addFactory<vehicle::VehicleManager>();
        FactoryUtil::addFactory<vehicle::WheelComponent>();

        FactoryUtil::addFactory<scene::GamePrefab>();
        FactoryUtil::addFactory<scene::GamePrefabManager>();

        FactoryUtil::addFactory<Mesh>();
        FactoryUtil::addFactory<SubMesh>();
        FactoryUtil::addFactory<IndexBuffer>();
        FactoryUtil::addFactory<VertexBuffer>();
        FactoryUtil::addFactory<VertexDeclaration>();

        FactoryUtil::addFactory<scene::GameActor>();
        FactoryUtil::addFactory<scene::GameActor::FsmListener>();
        FactoryUtil::addFactory<Director>();
        FactoryUtil::addFactory<scene::GameScene>();
        FactoryUtil::addFactory<scene::GameManager>();
        FactoryUtil::addFactory<scene::Transform>();

        FactoryUtil::addFactory<scene::CameraController>();
        FactoryUtil::addFactory<scene::CameraController::EventListener>();
        FactoryUtil::addFactory<scene::CameraFollow>();
        FactoryUtil::addFactory<scene::CameraTarget>();
        FactoryUtil::addFactory<scene::EditorCameraController>();
        FactoryUtil::addFactory<scene::FpsCameraController>();
        FactoryUtil::addFactory<scene::SphericalCameraController>();
        FactoryUtil::addFactory<scene::ThirdPersonCameraController>();
        FactoryUtil::addFactory<scene::VehicleCameraController>();

        FactoryUtil::addFactory<scene::AnimatedMaterial>();
        FactoryUtil::addFactory<scene::Animation>();
        FactoryUtil::addFactory<scene::Animator>();
        FactoryUtil::addFactory<scene::AudioEmitter>();
        FactoryUtil::addFactory<scene::Billboard>();
        FactoryUtil::addFactory<scene::Billboards>();
        FactoryUtil::addFactory<scene::Camera>();
        FactoryUtil::addFactory<scene::CarController>();
        FactoryUtil::addFactory<scene::CharacterController>();
        FactoryUtil::addFactory<scene::Collision>();
        FactoryUtil::addFactory<scene::CollisionBox>();
        FactoryUtil::addFactory<scene::CollisionMesh>();
        FactoryUtil::addFactory<scene::CollisionPlane>();
        FactoryUtil::addFactory<scene::CollisionSphere>();
        FactoryUtil::addFactory<scene::CollisionTerrain>();
        FactoryUtil::addFactory<scene::Component>();
        FactoryUtil::addFactory<scene::Component::ComponentFSMListener>();
        FactoryUtil::addFactory<scene::ComponentEvent>();
        FactoryUtil::addFactory<scene::ComponentEventListener>();
        FactoryUtil::addFactory<scene::Constraint>();
        FactoryUtil::addFactory<scene::Cubemap>();
        FactoryUtil::addFactory<scene::FiniteStateMachine>();
        FactoryUtil::addFactory<scene::Light>();
        FactoryUtil::addFactory<scene::LODGroup>();
        FactoryUtil::addFactory<scene::Material>();
        FactoryUtil::addFactory<scene::Material::MaterialStateObjectListener>();
        FactoryUtil::addFactory<scene::Material::MaterialStateListener>();
        FactoryUtil::addFactory<scene::Mesh>();
        FactoryUtil::addFactory<scene::MeshRenderer>();
        FactoryUtil::addFactory<scene::NetworkListener>();
        FactoryUtil::addFactory<scene::NetworkView>();
        FactoryUtil::addFactory<scene::ParticleSystem>();
        FactoryUtil::addFactory<scene::Renderer>();
        FactoryUtil::addFactory<scene::RenderTexture>();
        FactoryUtil::addFactory<scene::Rigidbody>();
        FactoryUtil::addFactory<scene::RigidbodyListener>();
        FactoryUtil::addFactory<scene::Skybox>();
        FactoryUtil::addFactory<scene::Skybox::MaterialSharedListener>();
        FactoryUtil::addFactory<scene::SkyboxPanorama>();
        FactoryUtil::addFactory<scene::ProceduralVehicle>();
        FactoryUtil::addFactory<scene::ProceduralRaceScene>();
        FactoryUtil::addFactory<scene::ProceduralRoad>();
        FactoryUtil::addFactory<scene::ProceduralSky>();
        FactoryUtil::addFactory<scene::ProceduralSurfaceTexture>();

        FactoryUtil::addFactory<scene::SubComponent>();
        FactoryUtil::addFactory<scene::TerrainBlendMap>();
        FactoryUtil::addFactory<scene::TerrainGrassLayer>();
        FactoryUtil::addFactory<scene::TerrainLayer>();
        FactoryUtil::addFactory<scene::TerrainSystem>();
        FactoryUtil::addFactory<scene::TerrainTreeLayer>();
        FactoryUtil::addFactory<scene::Script>();
        FactoryUtil::addFactory<scene::VehicleController>();
        FactoryUtil::addFactory<scene::VideoPlayer>();
        FactoryUtil::addFactory<scene::WheelController>();
        FactoryUtil::addFactory<scene::NetworkPlayer>();
        FactoryUtil::addFactory<scene::NetworkStream>();

        FactoryUtil::addFactory<scene::Button>();
        FactoryUtil::addFactory<scene::Dropdown>();
        FactoryUtil::addFactory<scene::Dropdown::Option>();
        FactoryUtil::addFactory<scene::UIComponent>();
        FactoryUtil::addFactory<scene::UIComponent::UIElementListener>();
        FactoryUtil::addFactory<scene::Layout>();
        FactoryUtil::addFactory<scene::LayoutContainer>();
        FactoryUtil::addFactory<scene::LayoutTransform>();
        FactoryUtil::addFactory<scene::Image>();
        FactoryUtil::addFactory<scene::InputField>();
        FactoryUtil::addFactory<scene::ScrollBar>();
        FactoryUtil::addFactory<scene::ScrollView>();
        FactoryUtil::addFactory<scene::Slider>();
        FactoryUtil::addFactory<scene::TabItem>();
        FactoryUtil::addFactory<scene::TableCell>();
        FactoryUtil::addFactory<scene::TableLayout>();
        FactoryUtil::addFactory<scene::TabPage>();
        FactoryUtil::addFactory<scene::TabView>();
        FactoryUtil::addFactory<scene::Toggle>();
        FactoryUtil::addFactory<scene::Text>();
        FactoryUtil::addFactory<scene::Thumbnail>();
        FactoryUtil::addFactory<scene::ToggleGroup>();
        FactoryUtil::addFactory<scene::ToolTip>();
        FactoryUtil::addFactory<scene::GridLayout>();
        FactoryUtil::addFactory<scene::HorizontalLayout>();
        FactoryUtil::addFactory<scene::VerticalLayout>();

        FactoryUtil::addFactory<scene::ButtonDirector>();
        FactoryUtil::addFactory<scene::GraphicsSettingsDirector>();
        FactoryUtil::addFactory<scene::MaterialResourceDirector>();
        FactoryUtil::addFactory<scene::MeshResourceDirector>();
        FactoryUtil::addFactory<scene::LightingDirector>();
        FactoryUtil::addFactory<scene::SoundResourceDirector>();
        FactoryUtil::addFactory<scene::TextDirector>();
        FactoryUtil::addFactory<scene::TextureResourceDirector>();
        FactoryUtil::addFactory<scene::UiDialogDirector>();
        FactoryUtil::addFactory<scene::UiDirector>();
        FactoryUtil::addFactory<scene::UiElementDirector>();

        // script factories
        FactoryUtil::addFactory<ScriptInvoker>();

        FactoryUtil::addFactory<StateFrameData>();
        FactoryUtil::addFactory<StateMessage>();
        FactoryUtil::addFactory<StateMessageAnimationEnable>();
        FactoryUtil::addFactory<StateMessageBlendMapValue>();
        FactoryUtil::addFactory<StateMessageBuffer>();
        FactoryUtil::addFactory<StateMessageContact2>();
        FactoryUtil::addFactory<StateMessageDefault>();
        FactoryUtil::addFactory<StateMessageDirty>();
        FactoryUtil::addFactory<StateMessageDrawLine>();
        FactoryUtil::addFactory<StateMessageFloatValue>();
        FactoryUtil::addFactory<StateMessageFragmentParam>();
        FactoryUtil::addFactory<StateMessageJobStatus>();
        FactoryUtil::addFactory<StateMessageLoad>();
        FactoryUtil::addFactory<StateMessageMaterial>();
        FactoryUtil::addFactory<StateMessageMaterialName>();
        FactoryUtil::addFactory<StateMessageObject>();
        FactoryUtil::addFactory<StateMessageObjectsArray>();
        FactoryUtil::addFactory<StateMessageOrientation>();
        FactoryUtil::addFactory<StateMessageParamVector4>();
        FactoryUtil::addFactory<StateMessagePlay>();
        FactoryUtil::addFactory<StateMessageProperties>();
        FactoryUtil::addFactory<StateMessageSetTexture>();
        FactoryUtil::addFactory<StateMessageSkyBox>();
        FactoryUtil::addFactory<StateMessageStop>();
        FactoryUtil::addFactory<StateMessageStringValue>();
        FactoryUtil::addFactory<StateMessageText>();
        FactoryUtil::addFactory<StateMessageTransform3>();
        FactoryUtil::addFactory<StateMessageType>();
        FactoryUtil::addFactory<StateMessageVector2I>();
        FactoryUtil::addFactory<StateMessageVector2F>();
        FactoryUtil::addFactory<StateMessageVector3>();
        FactoryUtil::addFactory<StateMessageVector4>();
        FactoryUtil::addFactory<StateMessageUIntValue>();
        FactoryUtil::addFactory<StateMessageIntValue>();
        FactoryUtil::addFactory<StateMessageVisible>();
        FactoryUtil::addFactory<StatePhysicsDynamicState2>();
        FactoryUtil::addFactory<StatePhysicsForce2>();
        FactoryUtil::addFactory<StatePhysicsPosition2>();
        FactoryUtil::addFactory<StatePhysicsVelocity2>();

        FactoryUtil::addFactory<AmbientLightStateData>();
        FactoryUtil::addFactory<ApplicationStateData>();
        FactoryUtil::addFactory<BoundingBoxStateData>();
        FactoryUtil::addFactory<BoxShapeStateData>();
        FactoryUtil::addFactory<CameraStateData>();
        FactoryUtil::addFactory<CompositorStateData>();
        FactoryUtil::addFactory<ConstraintD6StateData>();
        FactoryUtil::addFactory<ConstraintFixedStateData>();
        FactoryUtil::addFactory<ConstraintStateData>();
        FactoryUtil::addFactory<FlagsStateData>();
        FactoryUtil::addFactory<FrustumStateData>();
        FactoryUtil::addFactory<GraphicsMeshState>();
        FactoryUtil::addFactory<GraphicsObjectData>();
        FactoryUtil::addFactory<GraphicsSceneState>();
        FactoryUtil::addFactory<InputState>();
        FactoryUtil::addFactory<LightAttenuationStateData>();
        FactoryUtil::addFactory<LightStateData>();
        FactoryUtil::addFactory<MaterialStateData>();
        FactoryUtil::addFactory<MaterialPassStateData>();
        FactoryUtil::addFactory<MaterialTechniqueStateData>();
        FactoryUtil::addFactory<MaterialTextureStateData>();
        FactoryUtil::addFactory<MeshShapeStateData>();
        FactoryUtil::addFactory<OverlayContainerState>();
        FactoryUtil::addFactory<OverlayElementState>();
        FactoryUtil::addFactory<OverlayState>();
        FactoryUtil::addFactory<OverlayTextState>();
        FactoryUtil::addFactory<PhysicsBodyMassState>();
        FactoryUtil::addFactory<PhysicsBodyMotionState>();
        FactoryUtil::addFactory<PhysicsBodyState>();
        FactoryUtil::addFactory<PhysicsMaterialStateData>();
        FactoryUtil::addFactory<PhysicsSceneState>();
        FactoryUtil::addFactory<PlaneShapeState>();
        FactoryUtil::addFactory<RenderTargetStateData>();
        FactoryUtil::addFactory<RenderTextureState>();
        FactoryUtil::addFactory<RigidbodyState>();
        FactoryUtil::addFactory<SceneNodeStateData>();
        FactoryUtil::addFactory<ShapeStateData>();
        FactoryUtil::addFactory<SkyStateData>();
        FactoryUtil::addFactory<SoundManagerStateData>();
        FactoryUtil::addFactory<SoundStateData>();
        FactoryUtil::addFactory<SphereShapeStateData>();
        FactoryUtil::addFactory<State>();
        FactoryUtil::addFactory<StateData>();
        FactoryUtil::addFactory<TerrainStateData>();
        FactoryUtil::addFactory<TextureSamplerState>();
        FactoryUtil::addFactory<TextureStateData>();
        FactoryUtil::addFactory<TransformStateData>();

        FactoryUtil::addFactory<UIAnchorStateData>();
        FactoryUtil::addFactory<UIDragStateData>();
        FactoryUtil::addFactory<UIDropdownStateData>();
        FactoryUtil::addFactory<UIElementStateData>();
        FactoryUtil::addFactory<UIImageStateData>();
        FactoryUtil::addFactory<UILayoutStateData>();
        FactoryUtil::addFactory<UIProgressBarStateData>();
        FactoryUtil::addFactory<UISliderStateData>();
        FactoryUtil::addFactory<UITextStateData>();
        FactoryUtil::addFactory<UIToggleStateData>();
        FactoryUtil::addFactory<UITransformStateData>();
        FactoryUtil::addFactory<UIWindowStateData>();

        FactoryUtil::addFactory<VehicleStateData>();
        FactoryUtil::addFactory<WindowMessageData>();
        FactoryUtil::addFactory<ViewportStateData>();
        FactoryUtil::addFactory<WindowStateData>();

        //factoryManager->setPoolSizeByType<Data<FileInfo>>( 32 );

        //factoryManager->setPoolSizeByType<DirectoryListing>( 4096 );

        const auto size = 128;
        const auto jobPoolSize = 8;
        const auto uiPoolSize = 8;
        const auto messagePoolSize = 12;
        const auto inputPoolSize = 12;

        factoryManager->setPoolSizeByType<FileList>( 4096 );
        factoryManager->setPoolSizeByType<FileDataStream>( 4096 );
        factoryManager->setPoolSizeByType<FileSystemArchive>( 32 );
        factoryManager->setPoolSizeByType<ObfuscatedZipFile>( 32 );
        factoryManager->setPoolSizeByType<ZipFile>( 32 );

        factoryManager->setPoolSizeByType<JobFunction>( jobPoolSize );

        factoryManager->setPoolSizeByType<CameraManagerReset>( 4 );
        factoryManager->setPoolSizeByType<EventJob>( 32 );
        factoryManager->setPoolSizeByType<LoadPluginJob>( 4 );
        factoryManager->setPoolSizeByType<RunCommandJob>( 4 );
        factoryManager->setPoolSizeByType<UnloadPluginJob>( 1 );

        factoryManager->setPoolSizeByType<GameInput>( inputPoolSize );
        factoryManager->setPoolSizeByType<GameInputState>( inputPoolSize );
        factoryManager->setPoolSizeByType<InputDeviceManager>( 1 );
        factoryManager->setPoolSizeByType<InputEvent>( inputPoolSize );
        factoryManager->setPoolSizeByType<Joystick>( inputPoolSize );
        factoryManager->setPoolSizeByType<JoystickState>( inputPoolSize );
        factoryManager->setPoolSizeByType<KeyboardState>( inputPoolSize );
        factoryManager->setPoolSizeByType<MouseState>( inputPoolSize );

        factoryManager->setPoolSizeByType<Properties>( 32 );
        factoryManager->setPoolSizeByType<StateQueue>( 32 );

        factoryManager->setPoolSizeByType<StateContext>( 32 );
        factoryManager->setPoolSizeByType<StateContext::SharedObjectListener>( 32 );

        factoryManager->setPoolSizeByType<WorkerThread>( 8 );

        factoryManager->setPoolSizeByType<SceneNodeStateData>( 32 );
        factoryManager->setPoolSizeByType<TextureStateData>( 4096 );
        factoryManager->setPoolSizeByType<TransformStateData>( 32 );
        factoryManager->setPoolSizeByType<RigidbodyState>( 4096 );

        factoryManager->setPoolSizeByType<WindowMessageData>( 4 );

        factoryManager->setPoolSizeByType<Properties>( size );

        auto meshPoolSize = 128;
        factoryManager->setPoolSizeByType<Mesh>( meshPoolSize );
        factoryManager->setPoolSizeByType<SubMesh>( meshPoolSize );
        factoryManager->setPoolSizeByType<IndexBuffer>( meshPoolSize );
        factoryManager->setPoolSizeByType<VertexBuffer>( meshPoolSize );
        factoryManager->setPoolSizeByType<VertexDeclaration>( meshPoolSize );

        auto actorPoolSize = 8;
        factoryManager->setPoolSizeByType<scene::GameActor>( actorPoolSize );
        factoryManager->setPoolSizeByType<scene::GameActor::FsmListener>( actorPoolSize );

        factoryManager->setPoolSizeByType<scene::AnimatedMaterial>( size );
        factoryManager->setPoolSizeByType<scene::Animation>( size );
        factoryManager->setPoolSizeByType<scene::Animator>( size );
        factoryManager->setPoolSizeByType<scene::AudioEmitter>( 4 );
        factoryManager->setPoolSizeByType<scene::Billboard>( size );
        factoryManager->setPoolSizeByType<scene::Billboards>( size );
        factoryManager->setPoolSizeByType<scene::Camera>( size );
        factoryManager->setPoolSizeByType<scene::CameraController>( size );
        factoryManager->setPoolSizeByType<scene::CameraController::EventListener>( size );
        factoryManager->setPoolSizeByType<scene::CameraFollow>( 2 );
        factoryManager->setPoolSizeByType<scene::CameraTarget>( 2 );
        factoryManager->setPoolSizeByType<scene::EditorCameraController>( size );
        factoryManager->setPoolSizeByType<scene::FpsCameraController>( size );
        factoryManager->setPoolSizeByType<scene::SphericalCameraController>( size );
        factoryManager->setPoolSizeByType<scene::ThirdPersonCameraController>( size );
        factoryManager->setPoolSizeByType<scene::VehicleCameraController>( size );
        factoryManager->setPoolSizeByType<scene::CarController>( size );
        factoryManager->setPoolSizeByType<scene::CharacterController>( size );
        factoryManager->setPoolSizeByType<scene::Collision>( size );
        factoryManager->setPoolSizeByType<scene::CollisionBox>( size );
        factoryManager->setPoolSizeByType<scene::CollisionMesh>( size );
        factoryManager->setPoolSizeByType<scene::CollisionPlane>( size );
        factoryManager->setPoolSizeByType<scene::CollisionSphere>( size );
        factoryManager->setPoolSizeByType<scene::CollisionTerrain>( size );
        factoryManager->setPoolSizeByType<scene::Component>( size );
        factoryManager->setPoolSizeByType<scene::Component::ComponentFSMListener>( size );
        factoryManager->setPoolSizeByType<scene::ComponentEvent>( size );
        factoryManager->setPoolSizeByType<scene::ComponentEventListener>( size );
        factoryManager->setPoolSizeByType<scene::Constraint>( size );
        factoryManager->setPoolSizeByType<scene::Cubemap>( size );
        factoryManager->setPoolSizeByType<scene::FiniteStateMachine>( size );
        factoryManager->setPoolSizeByType<scene::Light>( size );
        factoryManager->setPoolSizeByType<scene::Material>( size );
        factoryManager->setPoolSizeByType<scene::Material::MaterialStateObjectListener>( size );
        factoryManager->setPoolSizeByType<scene::Material::MaterialStateListener>( size );
        factoryManager->setPoolSizeByType<scene::Mesh>( size );
        factoryManager->setPoolSizeByType<scene::MeshRenderer>( size );
        factoryManager->setPoolSizeByType<scene::NetworkListener>( size );
        factoryManager->setPoolSizeByType<scene::NetworkPlayer>( size );
        factoryManager->setPoolSizeByType<scene::NetworkStream>( size );
        factoryManager->setPoolSizeByType<scene::NetworkView>( size );
        factoryManager->setPoolSizeByType<scene::ParticleSystem>( size );
        factoryManager->setPoolSizeByType<scene::Renderer>( size );
        factoryManager->setPoolSizeByType<scene::RenderTexture>( size );
        factoryManager->setPoolSizeByType<scene::Rigidbody>( size );
        factoryManager->setPoolSizeByType<scene::RigidbodyListener>( size );
        factoryManager->setPoolSizeByType<scene::Skybox>( size );
        factoryManager->setPoolSizeByType<scene::Skybox::MaterialSharedListener>( size );
        factoryManager->setPoolSizeByType<scene::SubComponent>( size );
        factoryManager->setPoolSizeByType<scene::TerrainBlendMap>( size );
        factoryManager->setPoolSizeByType<scene::TerrainGrassLayer>( size );
        factoryManager->setPoolSizeByType<scene::TerrainLayer>( size );
        factoryManager->setPoolSizeByType<scene::TerrainSystem>( size );
        factoryManager->setPoolSizeByType<scene::TerrainTreeLayer>( size );
        factoryManager->setPoolSizeByType<scene::Transform>( size );
        factoryManager->setPoolSizeByType<scene::Script>( size );
        factoryManager->setPoolSizeByType<scene::VehicleController>( size );
        factoryManager->setPoolSizeByType<scene::VideoPlayer>( size );
        factoryManager->setPoolSizeByType<scene::WheelController>( size );

        factoryManager->setPoolSizeByType<scene::Button>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::Dropdown>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::Dropdown::Option>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::UIComponent>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::UIComponent::UIElementListener>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::Layout>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::LayoutContainer>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::LayoutTransform>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::Image>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::InputField>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::ScrollBar>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::ScrollView>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::Slider>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::TabItem>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::TableCell>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::TableLayout>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::TabPage>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::TabView>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::Toggle>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::ToggleGroup>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::Text>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::Thumbnail>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::ToolTip>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::GridLayout>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::HorizontalLayout>( uiPoolSize );
        factoryManager->setPoolSizeByType<scene::VerticalLayout>( uiPoolSize );

        // script factory pool sizes
        factoryManager->setPoolSizeByType<ScriptInvoker>( 32 );

        factoryManager->setPoolSizeByType<StateFrameData>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessage>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageAnimationEnable>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageBlendMapValue>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageBuffer>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageContact2>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageDefault>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageDirty>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageDrawLine>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageFloatValue>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageFragmentParam>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageJobStatus>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageLoad>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageMaterial>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageMaterialName>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageObject>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageObjectsArray>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageOrientation>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageParamVector4>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessagePlay>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageProperties>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageSetTexture>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageSkyBox>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageStop>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageStringValue>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageText>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageTransform3>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageType>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageVector2I>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageVector2F>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageVector3>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageVector4>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageUIntValue>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageIntValue>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageVisible>( messagePoolSize );
        factoryManager->setPoolSizeByType<StatePhysicsDynamicState2>( messagePoolSize );
        factoryManager->setPoolSizeByType<StatePhysicsForce2>( messagePoolSize );
        factoryManager->setPoolSizeByType<StatePhysicsPosition2>( messagePoolSize );
        factoryManager->setPoolSizeByType<StatePhysicsVelocity2>( messagePoolSize );

        auto numStates = 4;
        factoryManager->setPoolSizeByType<AmbientLightStateData>( numStates );
        factoryManager->setPoolSizeByType<ApplicationStateData>( numStates );
        factoryManager->setPoolSizeByType<BoundingBoxStateData>( numStates );
        factoryManager->setPoolSizeByType<BoxShapeStateData>( numStates );
        factoryManager->setPoolSizeByType<CameraStateData>( numStates );
        factoryManager->setPoolSizeByType<CompositorStateData>( numStates );
        factoryManager->setPoolSizeByType<ConstraintD6StateData>( numStates );
        factoryManager->setPoolSizeByType<ConstraintFixedStateData>( numStates );
        factoryManager->setPoolSizeByType<ConstraintStateData>( numStates );
        factoryManager->setPoolSizeByType<FlagsStateData>( numStates );
        factoryManager->setPoolSizeByType<FrustumStateData>( numStates );
        factoryManager->setPoolSizeByType<GraphicsMeshState>( numStates );
        factoryManager->setPoolSizeByType<GraphicsObjectData>( numStates );
        factoryManager->setPoolSizeByType<GraphicsSceneState>( numStates );

        factoryManager->setPoolSizeByType<InputEvent>( 8 );
        factoryManager->setPoolSizeByType<MouseState>( 8 );

        factoryManager->setPoolSizeByType<LightAttenuationStateData>( numStates );
        factoryManager->setPoolSizeByType<LightStateData>( numStates );
        factoryManager->setPoolSizeByType<MaterialStateData>( numStates );
        factoryManager->setPoolSizeByType<MaterialPassStateData>( numStates );
        factoryManager->setPoolSizeByType<MaterialTechniqueStateData>( numStates );
        factoryManager->setPoolSizeByType<MaterialTextureStateData>( numStates );
        factoryManager->setPoolSizeByType<MeshShapeStateData>( numStates );
        factoryManager->setPoolSizeByType<OverlayContainerState>( numStates );
        factoryManager->setPoolSizeByType<OverlayElementState>( numStates );
        factoryManager->setPoolSizeByType<OverlayState>( numStates );
        factoryManager->setPoolSizeByType<OverlayTextState>( numStates );
        factoryManager->setPoolSizeByType<PhysicsBodyMassState>( numStates );
        factoryManager->setPoolSizeByType<PhysicsBodyMotionState>( numStates );
        factoryManager->setPoolSizeByType<PhysicsBodyState>( numStates );
        factoryManager->setPoolSizeByType<PhysicsMaterialStateData>( numStates );
        factoryManager->setPoolSizeByType<PhysicsSceneState>( numStates );
        factoryManager->setPoolSizeByType<PlaneShapeState>( numStates );
        factoryManager->setPoolSizeByType<RenderTargetStateData>( numStates );
        factoryManager->setPoolSizeByType<RenderTextureState>( numStates );
        factoryManager->setPoolSizeByType<PhysicsSceneState>( numStates );
        factoryManager->setPoolSizeByType<RigidbodyState>( numStates );
        factoryManager->setPoolSizeByType<SceneNodeStateData>( numStates );
        factoryManager->setPoolSizeByType<ShapeStateData>( numStates );
        factoryManager->setPoolSizeByType<SkyStateData>( numStates );
        factoryManager->setPoolSizeByType<SoundManagerStateData>( numStates );
        factoryManager->setPoolSizeByType<SoundStateData>( numStates );
        factoryManager->setPoolSizeByType<SphereShapeStateData>( numStates );
        factoryManager->setPoolSizeByType<State>( numStates );
        factoryManager->setPoolSizeByType<StateData>( numStates );
        factoryManager->setPoolSizeByType<TerrainStateData>( numStates );
        factoryManager->setPoolSizeByType<TextureSamplerState>( numStates );
        factoryManager->setPoolSizeByType<TextureStateData>( numStates );
        factoryManager->setPoolSizeByType<TransformStateData>( numStates );

        factoryManager->setPoolSizeByType<UIAnchorStateData>( numStates );
        factoryManager->setPoolSizeByType<UIDragStateData>( numStates );
        factoryManager->setPoolSizeByType<UIDropdownStateData>( numStates );
        factoryManager->setPoolSizeByType<UIElementStateData>( numStates );
        factoryManager->setPoolSizeByType<UIImageStateData>( numStates );
        factoryManager->setPoolSizeByType<UILayoutStateData>( numStates );
        factoryManager->setPoolSizeByType<UIProgressBarStateData>( numStates );
        factoryManager->setPoolSizeByType<UISliderStateData>( numStates );
        factoryManager->setPoolSizeByType<UITextStateData>( numStates );
        factoryManager->setPoolSizeByType<UIToggleStateData>( numStates );
        factoryManager->setPoolSizeByType<UITransformStateData>( numStates );
        factoryManager->setPoolSizeByType<UIWindowStateData>( numStates );

        factoryManager->setPoolSizeByType<VehicleStateData>( numStates );
        factoryManager->setPoolSizeByType<WindowMessageData>( numStates );
        factoryManager->setPoolSizeByType<ViewportStateData>( numStates );
        factoryManager->setPoolSizeByType<WindowStateData>( numStates );

        factoryManager->setPoolSizeByType<FileList>( 32 );
        factoryManager->setPoolSizeByType<FileSystemArchive>( 32 );
        factoryManager->setPoolSizeByType<FileDataStream>( 4 );
        factoryManager->setPoolSizeByType<ZipArchive>( 8 );
        factoryManager->setPoolSizeByType<ZipFile>( 4 );

        factoryManager->setPoolSizeByType<ResourceDatabase::ImportFileJob>( 1024 );

        factoryManager->setPoolSizeByType<vehicle::Vehicle>( 4 );
        factoryManager->setPoolSizeByType<vehicle::WheelComponent>( 32 );
    }

    void WorkphonePlugin::unload( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();

        FactoryUtil::removeFactory<ConfigFile>();
        FactoryUtil::removeFactory<FSM>();
        FactoryUtil::removeFactory<FSMListener>();
        FactoryUtil::removeFactory<FSMManager>();

        FactoryUtil::removeFactory<CameraManagerReset>();
        FactoryUtil::removeFactory<EventJob>();
        FactoryUtil::removeFactory<JobFunction>();
        FactoryUtil::removeFactory<LoadPluginJob>();
        FactoryUtil::removeFactory<RunCommandJob>();
        FactoryUtil::removeFactory<UnloadPluginJob>();

        FactoryUtil::removeFactory<GameInput>();
        FactoryUtil::removeFactory<GameInputState>();
        FactoryUtil::removeFactory<InputDeviceManager>();
        FactoryUtil::removeFactory<InputEvent>();
        FactoryUtil::removeFactory<Joystick>();
        FactoryUtil::removeFactory<JoystickState>();
        FactoryUtil::removeFactory<KeyboardState>();
        FactoryUtil::removeFactory<MouseState>();

        FactoryUtil::removeFactory<FrameStatistics>();
        FactoryUtil::removeFactory<Profiler>();
        FactoryUtil::removeFactory<Properties>();
        FactoryUtil::removeFactory<ProcessManager>();
        FactoryUtil::removeFactory<StateQueue>();
        FactoryUtil::removeFactory<StateContext>();
        FactoryUtil::removeFactory<StateManager>();
        FactoryUtil::removeFactory<WorkerThread>();

        FactoryUtil::removeFactory<AssetDatabaseManager>();
        FactoryUtil::removeFactory<DatabaseManager>();

        FactoryUtil::removeFactory<ResourceDatabase>();
        FactoryUtil::removeFactory<ResourceDatabase::ImportFileJob>();

        FactoryUtil::removeFactory<FolderListing>();

        FactoryUtil::removeFactory<FileSystem>();
        FactoryUtil::removeFactory<FileDataStream>();
        FactoryUtil::removeFactory<FileSystemArchive>();
        FactoryUtil::removeFactory<ObfuscatedZipArchive>();
        FactoryUtil::removeFactory<ObfuscatedZipFile>();
        FactoryUtil::removeFactory<ZipArchive>();
        FactoryUtil::removeFactory<ZipFile>();
        FactoryUtil::removeFactory<FileList>();

        FactoryUtil::removeFactory<Mesh>();
        FactoryUtil::removeFactory<SubMesh>();
        FactoryUtil::removeFactory<IndexBuffer>();
        FactoryUtil::removeFactory<VertexBuffer>();
        FactoryUtil::removeFactory<VertexDeclaration>();

        FactoryUtil::removeFactory<vehicle::Vehicle>();
        FactoryUtil::removeFactory<vehicle::WheelComponent>();
        FactoryUtil::removeFactory<vehicle::WheelComponent>();

        FactoryUtil::removeFactory<scene::GameActor>();
        FactoryUtil::removeFactory<Director>();
        FactoryUtil::removeFactory<scene::GameScene>();
        FactoryUtil::removeFactory<scene::GameManager>();
        FactoryUtil::removeFactory<scene::Transform>();

        FactoryUtil::removeFactory<scene::CameraFollow>();
        FactoryUtil::removeFactory<scene::CameraTarget>();
        FactoryUtil::removeFactory<scene::EditorCameraController>();
        FactoryUtil::removeFactory<scene::FpsCameraController>();
        FactoryUtil::removeFactory<scene::SphericalCameraController>();
        FactoryUtil::removeFactory<scene::ThirdPersonCameraController>();
        FactoryUtil::removeFactory<scene::VehicleCameraController>();

        FactoryUtil::removeFactory<scene::Animator>();
        FactoryUtil::removeFactory<scene::AudioEmitter>();
        FactoryUtil::removeFactory<scene::Camera>();
        FactoryUtil::removeFactory<scene::CarController>();
        FactoryUtil::removeFactory<scene::Constraint>();
        FactoryUtil::removeFactory<scene::CollisionBox>();
        FactoryUtil::removeFactory<scene::CollisionMesh>();
        FactoryUtil::removeFactory<scene::CollisionPlane>();
        FactoryUtil::removeFactory<scene::CollisionSphere>();
        FactoryUtil::removeFactory<scene::CollisionTerrain>();
        FactoryUtil::removeFactory<scene::FiniteStateMachine>();
        FactoryUtil::removeFactory<scene::Light>();
        FactoryUtil::removeFactory<scene::LODGroup>();
        FactoryUtil::removeFactory<scene::Material>();
        FactoryUtil::removeFactory<scene::Mesh>();
        FactoryUtil::removeFactory<scene::MeshRenderer>();
        FactoryUtil::removeFactory<scene::ParticleSystem>();
        FactoryUtil::removeFactory<scene::Rigidbody>();
        FactoryUtil::removeFactory<scene::Skybox>();
        FactoryUtil::removeFactory<scene::Skybox::MaterialSharedListener>();
        FactoryUtil::removeFactory<scene::SkyboxPanorama>();
        FactoryUtil::removeFactory<scene::ProceduralVehicle>();
        FactoryUtil::removeFactory<scene::ProceduralRaceScene>();
        FactoryUtil::removeFactory<scene::ProceduralRoad>();
        FactoryUtil::removeFactory<scene::ProceduralSky>();
        FactoryUtil::removeFactory<scene::ProceduralSurfaceTexture>();

        FactoryUtil::removeFactory<scene::TerrainBlendMap>();
        FactoryUtil::removeFactory<scene::TerrainLayer>();
        FactoryUtil::removeFactory<scene::TerrainSystem>();
        FactoryUtil::removeFactory<scene::Script>();
        FactoryUtil::removeFactory<scene::WheelController>();

        FactoryUtil::removeFactory<scene::Button>();
        FactoryUtil::removeFactory<scene::Dropdown>();
        FactoryUtil::removeFactory<scene::Dropdown::Option>();
        FactoryUtil::removeFactory<scene::Layout>();
        FactoryUtil::removeFactory<scene::LayoutTransform>();
        FactoryUtil::removeFactory<scene::Image>();
        FactoryUtil::removeFactory<scene::InputField>();
        FactoryUtil::removeFactory<scene::ScrollBar>();
        FactoryUtil::removeFactory<scene::ScrollView>();
        FactoryUtil::removeFactory<scene::Slider>();
        FactoryUtil::removeFactory<scene::TableLayout>();
        FactoryUtil::removeFactory<scene::Toggle>();
        FactoryUtil::removeFactory<scene::ToggleGroup>();
        FactoryUtil::removeFactory<scene::ToolTip>();
        FactoryUtil::removeFactory<scene::Text>();
        FactoryUtil::removeFactory<scene::Thumbnail>();
        FactoryUtil::removeFactory<scene::GridLayout>();
        FactoryUtil::removeFactory<scene::HorizontalLayout>();
        FactoryUtil::removeFactory<scene::VerticalLayout>();

        FactoryUtil::removeFactory<scene::ButtonDirector>();
        FactoryUtil::removeFactory<scene::GraphicsSettingsDirector>();
        FactoryUtil::removeFactory<scene::MaterialResourceDirector>();
        FactoryUtil::removeFactory<scene::MeshResourceDirector>();
        FactoryUtil::removeFactory<scene::LightingDirector>();
        FactoryUtil::removeFactory<scene::SoundResourceDirector>();
        FactoryUtil::removeFactory<scene::TextDirector>();
        FactoryUtil::removeFactory<scene::TextureResourceDirector>();
        FactoryUtil::removeFactory<scene::UiDialogDirector>();
        FactoryUtil::removeFactory<scene::UiDirector>();
        FactoryUtil::removeFactory<scene::UiElementDirector>();

        // script factories
        FactoryUtil::removeFactory<ScriptInvoker>();

        FactoryUtil::removeFactory<StateMessageObject>();
        FactoryUtil::removeFactory<StateMessageVector2I>();
        FactoryUtil::removeFactory<StateMessageVector2F>();
        FactoryUtil::removeFactory<StateMessageVector3>();
        FactoryUtil::removeFactory<StateMessageVector4>();
        FactoryUtil::removeFactory<StateMessageUIntValue>();
        FactoryUtil::removeFactory<StateMessageIntValue>();
        FactoryUtil::removeFactory<StateMessageVisible>();

        FactoryUtil::removeFactory<AmbientLightStateData>();
        FactoryUtil::removeFactory<ApplicationStateData>();
        FactoryUtil::removeFactory<BoundingBoxStateData>();
        FactoryUtil::removeFactory<BoxShapeStateData>();
        FactoryUtil::removeFactory<CameraStateData>();
        FactoryUtil::removeFactory<CompositorStateData>();
        FactoryUtil::removeFactory<ConstraintD6StateData>();
        FactoryUtil::removeFactory<ConstraintFixedStateData>();
        FactoryUtil::removeFactory<ConstraintStateData>();
        FactoryUtil::removeFactory<FlagsStateData>();
        FactoryUtil::removeFactory<GraphicsMeshState>();
        FactoryUtil::removeFactory<GraphicsObjectData>();
        FactoryUtil::removeFactory<GraphicsSceneState>();
        FactoryUtil::removeFactory<InputState>();
        FactoryUtil::removeFactory<LightAttenuationStateData>();
        FactoryUtil::removeFactory<LightStateData>();
        FactoryUtil::removeFactory<MaterialStateData>();
        FactoryUtil::removeFactory<MaterialPassStateData>();
        FactoryUtil::removeFactory<MeshShapeStateData>();
        FactoryUtil::removeFactory<OverlayContainerState>();
        FactoryUtil::removeFactory<OverlayElementState>();
        FactoryUtil::removeFactory<OverlayState>();
        FactoryUtil::removeFactory<PhysicsBodyState>();
        FactoryUtil::removeFactory<PhysicsSceneState>();
        FactoryUtil::removeFactory<PlaneShapeState>();
        FactoryUtil::removeFactory<RigidbodyState>();
        FactoryUtil::removeFactory<SceneNodeStateData>();
        FactoryUtil::removeFactory<ShapeStateData>();
        FactoryUtil::removeFactory<SoundManagerStateData>();
        FactoryUtil::removeFactory<SoundStateData>();
        FactoryUtil::removeFactory<SphereShapeStateData>();
        FactoryUtil::removeFactory<TerrainStateData>();
        FactoryUtil::removeFactory<TextureStateData>();
        FactoryUtil::removeFactory<TransformStateData>();
        FactoryUtil::removeFactory<UITransformStateData>();
        FactoryUtil::removeFactory<WindowMessageData>();
        FactoryUtil::removeFactory<ViewportStateData>();

        factoryManager->unload( data );
    }
}  // namespace workphone
