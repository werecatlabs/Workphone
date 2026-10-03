#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Memory/BaseObjectData.hpp>
#include <Workphone/Memory/SharedObjectData.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/IApplication.hpp>
#include <Workphone/Interface/IBuildDirector.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IMaterialNode.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IFolderExplorer.hpp>
#include <Workphone/Interface/IO/IArchive.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/Interface/Memory/ISharedObjectListener.hpp>
#include <Workphone/Interface/System/IJob.hpp>
#include <Workphone/Interface/System/ILogManager.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateQueue.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/IThreadPool.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IComponent.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/System/IFSM.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Interface/System/IFSMListener.hpp>
#include <Workphone/Interface/Graphics/IGraphicsMesh.hpp>
#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/Interface/UI/IUITreeNode.hpp>
#include <Workphone/Interface/UI/IUIRenderWindow.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/Script/IScriptManager.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/IApplication.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Application.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/AI/AiScene.hpp>
#include <Workphone/AI/AiTrack.hpp>
#include <Workphone/AI/AiTrackElement.hpp>
#include <Workphone/AI/VehicleAiManager.hpp>
#include <Workphone/Animation/AnimationMorphKeyFrame.hpp>
#include <Workphone/Animation/AnimationNumericTrack.hpp>
#include <Workphone/Animation/AnimationPoseKeyFrame.hpp>
#include <Workphone/Animation/AnimationTimeIndex.hpp>
#include <Workphone/Animation/AnimationVertexTrack.hpp>
#include <Workphone/Animation/Animator.hpp>
#include <Workphone/Animation/KeyFrameAnimator2.hpp>
#include <Workphone/Animation/KeyFrameAnimator3.hpp>
#include <Workphone/Animation/KeyFrameTransform2.hpp>
#include <Workphone/Animation/KeyFrameTransform3.hpp>
#include <Workphone/Database/ResourceReference.hpp>
#include <Workphone/Graphics/Billboard.hpp>
#include <Workphone/Graphics/BillboardSet.hpp>
#include <Workphone/Graphics/ComputeShader.hpp>
#include <Workphone/Graphics/DebugLine.hpp>
#include <Workphone/Graphics/DebugText.hpp>
#include <Workphone/Graphics/DecalCursor.hpp>
#include <Workphone/Graphics/DisableScreenSave.hpp>
#include <Workphone/Graphics/DynamicLines.hpp>
#include <Workphone/Graphics/DynamicMesh.hpp>
#include <Workphone/Graphics/Font.hpp>
#include <Workphone/Graphics/FontManager.hpp>
#include <Workphone/Graphics/GraphicsCamera.hpp>
#include <Workphone/Graphics/GraphicsCubemap.hpp>
#include <Workphone/Graphics/GraphicsMesh.hpp>
#include <Workphone/Graphics/GraphicsScene.hpp>
#include <Workphone/Graphics/GraphicsSceneNode.hpp>
#include <Workphone/Graphics/GraphicsSkeleton.hpp>
#include <Workphone/Graphics/GraphicsSystem.hpp>
#include <Workphone/Graphics/GraphicsLight.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Graphics/MaterialManager.hpp>
#include <Workphone/Graphics/MaterialPass.hpp>
#include <Workphone/Graphics/MaterialTechnique.hpp>
#include <Workphone/Graphics/MaterialTexture.hpp>
#include <Workphone/Graphics/Overlay.hpp>
#include <Workphone/Graphics/ParticleAffector.hpp>
#include <Workphone/Graphics/ParticleEmitter.hpp>
#include <Workphone/Graphics/ParticleManager.hpp>
#include <Workphone/Graphics/ParticleSystem.hpp>
#include <Workphone/Graphics/ParticleTechnique.hpp>
#include <Workphone/Graphics/Shader.hpp>
#include <Workphone/Graphics/Skybox.hpp>
#include <Workphone/Graphics/SkyboxCube.hpp>
#include <Workphone/Graphics/SkyboxPlane.hpp>
#include <Workphone/Graphics/SkySphere.hpp>
#include <Workphone/Graphics/Terrain.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <Workphone/Graphics/TextureManager.hpp>
#include <Workphone/Graphics/Viewport.hpp>
#include <Workphone/Input/AxisConfigurationData.hpp>
#include <Workphone/Input/ExternalKeymap.hpp>
#include <Workphone/Input/InputConfiguration.hpp>
#include <Workphone/Input/InputFunction.hpp>
#include <Workphone/Input/InputManagerExt.hpp>
#include <Workphone/Input/InputRecorder.hpp>
#include <Workphone/Interface/Ai/IAiAgent.hpp>
#include <Workphone/Interface/Ai/IAiSensoryMemory.hpp>
#include <Workphone/Interface/Ai/IAiTargetingSystem.hpp>
#include <Workphone/Interface/Ai/IAiTrack.hpp>
#include <Workphone/Interface/Ai/IAiTrackElement.hpp>
#include <Workphone/Interface/Ai/IAiWaypoint.hpp>
#include <Workphone/Interface/Ai/ILearning.hpp>
#include <Workphone/Interface/Ai/INeuralNetwork.hpp>
#include <Workphone/Interface/Ai/IPathfinder3.hpp>
#include <Workphone/Interface/Ai/IPathNode3.hpp>
#include <Workphone/Interface/Ai/IVehicleAi.hpp>
#include <Workphone/Interface/Ai/IVehicleAiManager.hpp>
#include <Workphone/Interface/Graphics/IAnimationState.hpp>
#include <Workphone/Interface/Graphics/IComputeShader.hpp>
#include <Workphone/Interface/Graphics/IParticleNode.hpp>
#include <Workphone/Interface/Graphics/IRenderTarget.hpp>
#include <Workphone/Interface/IPrototype.hpp>
#include <Workphone/Interface/Memory/IObject.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Physics/IPhysicsBody3.hpp>
#include <Workphone/Interface/Procedural/IProceduralGenerator.hpp>
#include <Workphone/Interface/Procedural/IProceduralMesh.hpp>
#include <Workphone/Interface/Procedural/IProceduralModelRule.hpp>
#include <Workphone/Interface/Procedural/IProceduralModelSystem.hpp>
#include <Workphone/Interface/System/IFactory.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/UI/IUIOutputConsole.hpp>
#include <Workphone/IO/DataStream.hpp>
#include <Workphone/IO/FileList.hpp>
#include <Workphone/IO/FileSystemArchive.hpp>
#include <Workphone/IO/MemoryFile.hpp>
#include <Workphone/IO/NativeFileDialog.hpp>
#include <Workphone/IO/ObfuscatedZipArchive.hpp>
#include <Workphone/IO/ObfuscatedZipFile.hpp>
#include <Workphone/IO/ZipArchive.hpp>
#include <Workphone/IO/ZipFile.hpp>
#include <Workphone/Jobs/ActorLoadJob.hpp>
#include <Workphone/Jobs/JobCreatePackage.hpp>
#include <Workphone/Jobs/RunCommandJob.hpp>
#include <Workphone/Jobs/SceneClearJob.hpp>
#include <Workphone/Jobs/SceneLoadJob.hpp>
#include <Workphone/Mesh/Bone.hpp>
#include <Workphone/Mesh/EdgeData.hpp>
#include <Workphone/Mesh/MeshPose.hpp>
#include <Workphone/Physics/BoxShape2.hpp>
#include <Workphone/Physics/BoxShape3.hpp>
#include <Workphone/Physics/CapsuleController.hpp>
#include <Workphone/Physics/ConstraintD6.hpp>
#include <Workphone/Physics/ConstraintDrive.hpp>
#include <Workphone/Physics/ConstraintFixed3.hpp>
#include <Workphone/Physics/ConstraintLimit.hpp>
#include <Workphone/Physics/MeshShape.hpp>
#include <Workphone/Physics/PhysicsManager.hpp>
#include <Workphone/Physics/PhysicsMaterial3.hpp>
#include <Workphone/Physics/PhysicsScene3.hpp>
#include <Workphone/Physics/PlaneShape.hpp>
#include <Workphone/Physics/RigidDynamic3.hpp>
#include <Workphone/Physics/RigidStatic3.hpp>
#include <Workphone/Physics/SphereShape.hpp>
#include <Workphone/Physics/TerrainShape.hpp>
#include <Workphone/Scene/Components/CutscenePlayer.hpp>
#include <Workphone/Scene/Cutscene.hpp>
#include <Workphone/Scene/Directors/ResourceDirector.hpp>
#include <Workphone/Scene/GameEditor.hpp>
#include <Workphone/Scene/GamePrefab.hpp>
#include <Workphone/Scene/Systems/ComponentSystem.hpp>
#include <Workphone/Script/ScriptFile.hpp>
#include <Workphone/Sound/Sound.hpp>
#include <Workphone/Sound/SoundListener3.hpp>
#include <Workphone/Sound/SoundManager.hpp>
#include <Workphone/Sound/SoundProject.hpp>
#include <Workphone/State/States/RigidbodyState.hpp>
#include <Workphone/State/States/TransformStateData.hpp>
#include <Workphone/System/AsyncOperation.hpp>
#include <Workphone/System/ConfigFile.hpp>
#include <Workphone/System/FSM.hpp>
#include <Workphone/System/JobFunction.hpp>
#include <Workphone/System/Manipulator.hpp>
#include <Workphone/System/Timer.hpp>
#include <Workphone/UI/UIButton.hpp>
#include <Workphone/UI/UIDropdown.hpp>
#include <Workphone/UI/UIHorizontalLayout.hpp>
#include <Workphone/UI/UIImage.hpp>
#include <Workphone/UI/UILayout.hpp>
#include <Workphone/UI/UIManager.hpp>
#include <Workphone/UI/UIProgressBar.hpp>
#include <Workphone/UI/UISlider.hpp>
#include <Workphone/UI/UIText.hpp>
#include <Workphone/UI/UITextEntry.hpp>
#include <Workphone/UI/UIToggle.hpp>
#include <Workphone/UI/UIVerticalLayout.hpp>
#include <Workphone/UI/UIWindow.hpp>
#include <Workphone/WorkphonePlugin.hpp>
#include <Workphone/WorkphoneHeaders.hpp>

namespace workphone
{

    TypeManager *TypeManager::instance_ = nullptr;

    TypeManager::TypeManager()
    {
        instance_ = this;
    }

    TypeManager::~TypeManager()
    {
        instance_ = nullptr;
    }

    void TypeManager::load()
    {
        auto size = WP_TYPE_MANAGER_NUM_TYPES;
        reserve( size );
        resizeObjectData( size );

        baseObjectDataPool.setNextSize( 128 );
        sharedObjectDataPool.setNextSize( 128 );

        baseObjectDataPool.allocateData();
        sharedObjectDataPool.allocateData();

        IObject::setupTypeInfo();
        ISharedObject::setupTypeInfo();

        WP_ASSERT( IObject::typeInfo() == 1 );
        WP_ASSERT( ISharedObject::typeInfo() == 2 );

        // Built-in Workphone types.
        ActorAnimationTrack::setupTypeInfo();
        ActorEnableJob::setupTypeInfo();
        ActorLoadJob::setupTypeInfo();
        AiManager::setupTypeInfo();
        AiScene::setupTypeInfo();
        AiTrack::setupTypeInfo();
        AiTrackElement::setupTypeInfo();
        AmbientLightStateData::setupTypeInfo();
        Animation::setupTypeInfo();
        AnimationInterface::setupTypeInfo();
        AnimationKeyFrame::setupTypeInfo();
        AnimationMorphKeyFrame::setupTypeInfo();
        AnimationNumericTrack::setupTypeInfo();
        AnimationPoseKeyFrame::setupTypeInfo();
        AnimationTimeIndex::setupTypeInfo();
        AnimationVertexTrack::setupTypeInfo();
        Animator::setupTypeInfo();
        ApplicationStateData::setupTypeInfo();
        AssetDatabaseManager::setupTypeInfo();
        AsyncOperation::setupTypeInfo();
        AxisConfigurationData::setupTypeInfo();
        Bone::setupTypeInfo();
        BoundingBoxStateData::setupTypeInfo();
        BoxShapeStateData::setupTypeInfo();
        CameraManagerReset::setupTypeInfo();
        CameraStateData::setupTypeInfo();
        CollisionMesh::setupTypeInfo();
        CollisionSubMesh::setupTypeInfo();
        CompositorStateData::setupTypeInfo();
        ConfigFile::setupTypeInfo();
        ConstraintD6StateData::setupTypeInfo();
        ConstraintFixedStateData::setupTypeInfo();
        ConstraintStateData::setupTypeInfo();
        CreateRigidBodies::setupTypeInfo();
        DatabaseManager::setupTypeInfo();
        DatabaseManager::DMLQueryJob::setupTypeInfo();
        DatabaseManager::QueryJob::setupTypeInfo();
        DataStream::setupTypeInfo();
        DisableScreenSave::setupTypeInfo();
        EdgeData::setupTypeInfo();
        EventJob::setupTypeInfo();
        ExternalKeymap::setupTypeInfo();
        Factory::setupTypeInfo();
        Factory::Listener::setupTypeInfo();
        FactoryManager::setupTypeInfo();
        FileDataStream::setupTypeInfo();
        FileList::setupTypeInfo();
        FileSystem::setupTypeInfo();
        FileSystemArchive::setupTypeInfo();
        FlagSetJob::setupTypeInfo();
        FlagsStateData::setupTypeInfo();
        FrameStatistics::setupTypeInfo();
        FrustumStateData::setupTypeInfo();
        FSM::setupTypeInfo();
        FSMListener::setupTypeInfo();
        FSMManager::setupTypeInfo();
        GameInput::setupTypeInfo();
        GameInputState::setupTypeInfo();
        GraphicsMeshState::setupTypeInfo();
        GraphicsObjectData::setupTypeInfo();
        GraphicsSceneState::setupTypeInfo();
        IActorAnimationTrack::setupTypeInfo();
        IAi::setupTypeInfo();
        IAiAgent::setupTypeInfo();
        IAiCompositeGoal::setupTypeInfo();
        IAiGoal::setupTypeInfo();
        IAiGoalEvaluator::setupTypeInfo();
        IAiManager::setupTypeInfo();
        IAiScene::setupTypeInfo();
        IAiSensoryMemory::setupTypeInfo();
        IAiSteering3::setupTypeInfo();
        IAiTargeting3::setupTypeInfo();
        IAiTargetingSystem::setupTypeInfo();
        IAiTrack::setupTypeInfo();
        IAiTrackElement::setupTypeInfo();
        IAiWaypoint::setupTypeInfo();
        IAnimation::setupTypeInfo();
        IAnimationContainer::setupTypeInfo();
        IAnimationInterface::setupTypeInfo();
        IAnimationKeyFrame::setupTypeInfo();
        IAnimationMorphKeyFrame::setupTypeInfo();
        IAnimationNumericTrack::setupTypeInfo();
        IAnimationPoseKeyFrame::setupTypeInfo();
        IAnimationState::setupTypeInfo();
        IAnimationTimeIndex::setupTypeInfo();
        IAnimationTrack::setupTypeInfo();
        IAnimationVertexTrack::setupTypeInfo();
        IAnimator::setupTypeInfo();
        IArchive::setupTypeInfo();
        IAudioBusBuffers::setupTypeInfo();
        IAudioEffect::setupTypeInfo();
        IAudioEffectDelay::setupTypeInfo();
        IAudioEffectVolume::setupTypeInfo();
        IAudioProcessData::setupTypeInfo();
        IBone::setupTypeInfo();
        IBuildDirector::setupTypeInfo();
        IChordDetector::setupTypeInfo();
        ICommand::setupTypeInfo();
        ICommandManager::setupTypeInfo();
        IConfigFile::setupTypeInfo();
        ICoroutineData::setupTypeInfo();
        IData::setupTypeInfo();
        IDatabase::setupTypeInfo();
        IDatabaseManager::setupTypeInfo();
        IDatabaseQuery::setupTypeInfo();
        IEditorManager::setupTypeInfo();
        IEvent::setupTypeInfo();
        IEventListener::setupTypeInfo();
        IFactory::setupTypeInfo();
        IFactoryManager::setupTypeInfo();
        IFileList::setupTypeInfo();
        IFileListener::setupTypeInfo();
        IFileSystem::setupTypeInfo();
        IFolderExplorer::setupTypeInfo();
        IFrameStatistics::setupTypeInfo();
        IFSM::setupTypeInfo();
        IFSMListener::setupTypeInfo();
        IFSMManager::setupTypeInfo();
        IGameInput::setupTypeInfo();
        IGameInputMap::setupTypeInfo();
        IGameInputState::setupTypeInfo();
        IIndexBuffer::setupTypeInfo();
        IInputAction::setupTypeInfo();
        IInputConverter::setupTypeInfo();
        IInputDeviceManager::setupTypeInfo();
        IInputEvent::setupTypeInfo();
        IInputManager::setupTypeInfo();
        IJob::setupTypeInfo();
        IJobGroup::setupTypeInfo();
        IJobQueue::setupTypeInfo();
        IJoystick::setupTypeInfo();
        IJoystickState::setupTypeInfo();
        IKeyboardState::setupTypeInfo();
        IKeymap::setupTypeInfo();
        ILearning::setupTypeInfo();
        ILogManager::setupTypeInfo();
        IMesh::setupTypeInfo();
        IMeshLoader::setupTypeInfo();
        IMeshPose::setupTypeInfo();
        IMeshResource::setupTypeInfo();
        IMouseState::setupTypeInfo();
        ImportResourceJob::setupTypeInfo();
        INativeFileDialog::setupTypeInfo();
        INetworkListener::setupTypeInfo();
        INetworkManager::setupTypeInfo();
        INetworkPlayer::setupTypeInfo();
        INetworkStream::setupTypeInfo();
        INetworkView::setupTypeInfo();
        INeuralNetwork::setupTypeInfo();
        InputActionData::setupTypeInfo();
        InputConfiguration::setupTypeInfo();
        InputDeviceManager::setupTypeInfo();
        InputEvent::setupTypeInfo();
        InputFunction::setupTypeInfo();
        InputManager::setupTypeInfo();
        InputManagerExt::setupTypeInfo();
        InputRecorder::setupTypeInfo();
        InputState::setupTypeInfo();
        IObjectBuilder::setupTypeInfo();
        IPackageManager::setupTypeInfo();
        IPacket::setupTypeInfo();
        IPathfinder2::setupTypeInfo();
        IPathfinder3::setupTypeInfo();
        IPathNode2::setupTypeInfo();
        IPathNode3::setupTypeInfo();
        IPluginManager::setupTypeInfo();
        IProcessManager::setupTypeInfo();
        IProfile::setupTypeInfo();
        IProfiler::setupTypeInfo();
        IProject::setupTypeInfo();
        IProjectManager::setupTypeInfo();
        IResource::setupTypeInfo();
        IResourceDatabase::setupTypeInfo();
        IResourceGroupManager::setupTypeInfo();
        IResourceManager::setupTypeInfo();
        IResourceReference::setupTypeInfo();
        IScript::setupTypeInfo();
        IScriptBreakpoint::setupTypeInfo();
        IScriptClass::setupTypeInfo();
        IScriptData::setupTypeInfo();
        IScriptEvent::setupTypeInfo();
        IScriptFunction::setupTypeInfo();
        IScriptGenerator::setupTypeInfo();
        IScriptInvoker::setupTypeInfo();
        IScriptManager::setupTypeInfo();
        IScriptObject::setupTypeInfo();
        IScriptReceiver::setupTypeInfo();
        IScriptUserData::setupTypeInfo();
        IScriptVariable::setupTypeInfo();
        ISelectionManager::setupTypeInfo();
        ISequenceDetector::setupTypeInfo();
        ISkeleton::setupTypeInfo();
        ISound::setupTypeInfo();
        ISoundEvent::setupTypeInfo();
        ISoundEventGroup::setupTypeInfo();
        ISoundEventParam::setupTypeInfo();
        ISoundListener3::setupTypeInfo();
        ISoundManager::setupTypeInfo();
        ISoundPlayer::setupTypeInfo();
        ISoundProject::setupTypeInfo();
        IState::setupTypeInfo();
        IStateContext::setupTypeInfo();
        IStateListener::setupTypeInfo();
        IStateManager::setupTypeInfo();
        IStateMessage::setupTypeInfo();
        IStateQueue::setupTypeInfo();
        IStream::setupTypeInfo();
        ISubMesh::setupTypeInfo();
        ISystemAddress::setupTypeInfo();
        ITapDetector::setupTypeInfo();
        ITask::setupTypeInfo();
        ITaskLock::setupTypeInfo();
        ITaskManager::setupTypeInfo();
        ITest::setupTypeInfo();
        ITestManager::setupTypeInfo();
        IThreadPool::setupTypeInfo();
        ITimer::setupTypeInfo();
        IVehicleAi::setupTypeInfo();
        IVehicleAiManager::setupTypeInfo();
        IVertexBoneAssignment::setupTypeInfo();
        IVertexBuffer::setupTypeInfo();
        IVertexDeclaration::setupTypeInfo();
        IVertexElement::setupTypeInfo();
        IWorkerThread::setupTypeInfo();
        Job::setupTypeInfo();
        JobCoroutine::setupTypeInfo();
        JobCreatePackage::setupTypeInfo();
        JobFunction::setupTypeInfo();
        Joystick::setupTypeInfo();
        JoystickState::setupTypeInfo();
        KeyboardState::setupTypeInfo();
        KeyFrameAnimator2::setupTypeInfo();
        KeyFrameAnimator3::setupTypeInfo();
        KeyFrameTransform2::setupTypeInfo();
        KeyFrameTransform3::setupTypeInfo();
        LightAttenuationStateData::setupTypeInfo();
        LightStateData::setupTypeInfo();
        LoadPluginJob::setupTypeInfo();
        LogManagerDefault::setupTypeInfo();
        Manipulator::setupTypeInfo();
        Manipulator::SelectionManagerListener::setupTypeInfo();
        MaterialPassStateData::setupTypeInfo();
        MaterialStateData::setupTypeInfo();
        MaterialTechniqueStateData::setupTypeInfo();
        MaterialTextureStateData::setupTypeInfo();
        MemoryFile::setupTypeInfo();
        Mesh::setupTypeInfo();
        MeshManager::setupTypeInfo();
        MeshPose::setupTypeInfo();
        MeshResource::setupTypeInfo();
        MeshShapeStateData::setupTypeInfo();
        MeshSkeleton::setupTypeInfo();
        MouseState::setupTypeInfo();
        NativeFileDialog::setupTypeInfo();
        ObfuscatedZipArchive::setupTypeInfo();
        ObfuscatedZipFile::setupTypeInfo();
        ObjectUpdateJob::setupTypeInfo();
        OverlayContainerState::setupTypeInfo();
        OverlayElementState::setupTypeInfo();
        OverlayState::setupTypeInfo();
        OverlayTextState::setupTypeInfo();
        PackageManager::setupTypeInfo();
        PhysicsBodyMassState::setupTypeInfo();
        PhysicsBodyMotionState::setupTypeInfo();
        PhysicsBodyState::setupTypeInfo();
        PhysicsMaterialStateData::setupTypeInfo();
        PhysicsSceneState::setupTypeInfo();
        PlaneShapeState::setupTypeInfo();
        ProcessManager::setupTypeInfo();
        Properties::setupTypeInfo();
        RenderTargetStateData::setupTypeInfo();
        RenderTextureState::setupTypeInfo();
        Resolution::setupTypeInfo();
        ResourceDatabase::setupTypeInfo();
        ResourceDatabase::ImportFileJob::setupTypeInfo();
        ResourceReference::setupTypeInfo();
        RigidbodyState::setupTypeInfo();
        RotateManipulator::setupTypeInfo();
        RunCommandJob::setupTypeInfo();
        ScaleManipulator::setupTypeInfo();
        SceneClearJob::setupTypeInfo();
        SceneLoadJob::setupTypeInfo();
        SceneNodeStateData::setupTypeInfo();
        ScriptClass::setupTypeInfo();
        ScriptEvent::setupTypeInfo();
        ScriptFile::setupTypeInfo();
        ScriptFunction::setupTypeInfo();
        ScriptGenerator::setupTypeInfo();
        ScriptInvoker::setupTypeInfo();
        ScriptVariable::setupTypeInfo();
        SelectionManager::setupTypeInfo();
        ShapeStateData::setupTypeInfo();
        SkyStateData::setupTypeInfo();
        Sound::setupTypeInfo();
        SoundListener3::setupTypeInfo();
        SoundManager::setupTypeInfo();
        SoundManagerStateData::setupTypeInfo();
        SoundProject::setupTypeInfo();
        SoundStateData::setupTypeInfo();
        SphereShapeStateData::setupTypeInfo();
        State::setupTypeInfo();
        StateContext::setupTypeInfo();
        StateContext::SharedObjectListener::setupTypeInfo();
        StateData::setupTypeInfo();
        StateFrameData::setupTypeInfo();
        StateManager::setupTypeInfo();
        StateMessage::setupTypeInfo();
        StateMessageAnimationEnable::setupTypeInfo();
        StateMessageBlendMapValue::setupTypeInfo();
        StateMessageBuffer::setupTypeInfo();
        StateMessageContact2::setupTypeInfo();
        StateMessageDefault::setupTypeInfo();
        StateMessageDirty::setupTypeInfo();
        StateMessageDrawLine::setupTypeInfo();
        StateMessageFloatValue::setupTypeInfo();
        StateMessageFragmentParam::setupTypeInfo();
        StateMessageIntValue::setupTypeInfo();
        StateMessageJobStatus::setupTypeInfo();
        StateMessageLoad::setupTypeInfo();
        StateMessageMaterial::setupTypeInfo();
        StateMessageMaterialName::setupTypeInfo();
        StateMessageObject::setupTypeInfo();
        StateMessageObjectsArray::setupTypeInfo();
        StateMessageOrientation::setupTypeInfo();
        StateMessageParamVector4::setupTypeInfo();
        StateMessagePlay::setupTypeInfo();
        StateMessageProperties::setupTypeInfo();
        StateMessageSetTexture::setupTypeInfo();
        StateMessageSkyBox::setupTypeInfo();
        StateMessageStop::setupTypeInfo();
        StateMessageStringValue::setupTypeInfo();
        StateMessageText::setupTypeInfo();
        StateMessageTransform3::setupTypeInfo();
        StateMessageType::setupTypeInfo();
        StateMessageUIntValue::setupTypeInfo();
        StateMessageVector3::setupTypeInfo();
        StateMessageVector4::setupTypeInfo();
        StateMessageVisible::setupTypeInfo();
        StatePhysicsDynamicState2::setupTypeInfo();
        StatePhysicsForce2::setupTypeInfo();
        StatePhysicsPosition2::setupTypeInfo();
        StatePhysicsVelocity2::setupTypeInfo();
        StateQueue::setupTypeInfo();
        SubMesh::setupTypeInfo();
        Task::setupTypeInfo();
        TaskManager::setupTypeInfo();
        TerrainStateData::setupTypeInfo();
        TextureSamplerState::setupTypeInfo();
        TextureStateData::setupTypeInfo();
        ThreadPool::setupTypeInfo();
        Timer::setupTypeInfo();
        TimerBoost::setupTypeInfo();
        TimerChrono::setupTypeInfo();

#if defined WP_USE_BOOST
        TimerCPU::setupTypeInfo();
#endif

        TimerMT::setupTypeInfo();

#if defined WP_PLATFORM_WIN32
        TimerWin32::setupTypeInfo();
#endif

        TransformStateData::setupTypeInfo();
        TranslateManipulator::setupTypeInfo();
        UIAnchorStateData::setupTypeInfo();
        UIDragStateData::setupTypeInfo();
        UIDropdownStateData::setupTypeInfo();
        UIElementStateData::setupTypeInfo();
        UIImageStateData::setupTypeInfo();
        UILayoutStateData::setupTypeInfo();
        UIProgressBarStateData::setupTypeInfo();
        UISliderStateData::setupTypeInfo();
        UITextStateData::setupTypeInfo();
        UIToggleStateData::setupTypeInfo();
        UITransformStateData::setupTypeInfo();
        UIWindowStateData::setupTypeInfo();
        UnloadPluginJob::setupTypeInfo();
        VehicleAiManager::setupTypeInfo();
        VehicleStateData::setupTypeInfo();
        VertexBoneAssignment::setupTypeInfo();
        VertexBuffer::setupTypeInfo();
        ViewportStateData::setupTypeInfo();
        WaitForSeconds::setupTypeInfo();
        WindowMessageData::setupTypeInfo();
        WindowStateData::setupTypeInfo();
        WorkerThread::setupTypeInfo();
        WorkphonePlugin::setupTypeInfo();
        ZipArchive::setupTypeInfo();
        ZipFile::setupTypeInfo();

        // Core types.
        core::Application::setupTypeInfo();
        core::Application::ApplicationEventListener::setupTypeInfo();
        core::ApplicationManager::setupTypeInfo();
        core::IApplication::setupTypeInfo();
        core::IApplicationManager::setupTypeInfo();
        core::IPrototype::setupTypeInfo();
        core::Plugin::setupTypeInfo();
        core::PluginManager::setupTypeInfo();

        // Editor types.
        editor::FileSelection::setupTypeInfo();

        // Physics types.
        physics::BoxShape2::setupTypeInfo();
        physics::BoxShape3::setupTypeInfo();
        physics::CapsuleController::setupTypeInfo();
        physics::ConstraintD6::setupTypeInfo();
        physics::ConstraintDrive::setupTypeInfo();
        physics::ConstraintFixed3::setupTypeInfo();
        physics::ConstraintLimit::setupTypeInfo();
        physics::IBoxShape2::setupTypeInfo();
        physics::IBoxShape3::setupTypeInfo();
        physics::ICharacterController2::setupTypeInfo();
        physics::ICharacterController3::setupTypeInfo();
        physics::IConstraintD6::setupTypeInfo();
        physics::IConstraintDrive::setupTypeInfo();
        physics::IConstraintFixed2::setupTypeInfo();
        physics::IConstraintFixed3::setupTypeInfo();
        physics::IConstraintLimit::setupTypeInfo();
        physics::IMassData2::setupTypeInfo();
        physics::IMassData3::setupTypeInfo();
        physics::IMeshShape::setupTypeInfo();
        physics::IPhysicsBody2D::setupTypeInfo();
        physics::IPhysicsBody3::setupTypeInfo();
        physics::IPhysicsBodyEffect2::setupTypeInfo();
        physics::IPhysicsBodyEffectSnap2::setupTypeInfo();
        physics::IPhysicsCompositeShape3::setupTypeInfo();
        physics::IPhysicsConstraint::setupTypeInfo();
        physics::IPhysicsConstraint2::setupTypeInfo();
        physics::IPhysicsConstraint3::setupTypeInfo();
        physics::IPhysicsDebug::setupTypeInfo();
        physics::IPhysicsEffect2::setupTypeInfo();
        physics::IPhysicsManager::setupTypeInfo();
        physics::IPhysicsManager2D::setupTypeInfo();
        physics::IPhysicsMaterial2::setupTypeInfo();
        physics::IPhysicsMaterial3::setupTypeInfo();
        physics::IPhysicsParticle2::setupTypeInfo();
        physics::IPhysicsParticle3::setupTypeInfo();
        physics::IPhysicsScene2::setupTypeInfo();
        physics::IPhysicsScene3::setupTypeInfo();
        physics::IPhysicsShape::setupTypeInfo();
        physics::IPhysicsShape2::setupTypeInfo();
        physics::IPhysicsShape3::setupTypeInfo();
        physics::IPhysicsSoftBody2::setupTypeInfo();
        physics::IPhysicsSoftBody3::setupTypeInfo();
        physics::IPhysicsSpring::setupTypeInfo();
        physics::IPhysicsVehicle3::setupTypeInfo();
        physics::IPhysicsVehicleInput3::setupTypeInfo();
        physics::IPhysicsVehicleWheel3::setupTypeInfo();
        physics::IPlaneShape3::setupTypeInfo();
        physics::IRaycastHit::setupTypeInfo();
        physics::IRigidBody2::setupTypeInfo();
        physics::IRigidBody3::setupTypeInfo();
        physics::IRigidDynamic3::setupTypeInfo();
        physics::IRigidStatic3::setupTypeInfo();
        physics::ISphereShape2::setupTypeInfo();
        physics::ISphereShape3::setupTypeInfo();
        physics::ITerrainShape::setupTypeInfo();
        physics::MeshShape::setupTypeInfo();
        physics::PhysicsManager::setupTypeInfo();
        physics::PhysicsMaterial3::setupTypeInfo();
        physics::PhysicsScene3::setupTypeInfo();
        physics::PhysicsScene3::StateListener::setupTypeInfo();
        physics::PlaneShape::setupTypeInfo();
        physics::RaycastHit::setupTypeInfo();
        physics::RigidDynamic3::setupTypeInfo();
        physics::RigidStatic3::setupTypeInfo();
        physics::SphereShape::setupTypeInfo();
        physics::TerrainShape::setupTypeInfo();

        // Procedural types.
        procedural::ICityBlock::setupTypeInfo();
        procedural::ICityGenerator::setupTypeInfo();
        procedural::ICityMap::setupTypeInfo();
        procedural::ILot::setupTypeInfo();
        procedural::ILSystem::setupTypeInfo();
        procedural::ILSystemRule::setupTypeInfo();
        procedural::IMeshGenerator::setupTypeInfo();
        procedural::IProceduralCity::setupTypeInfo();
        procedural::IProceduralCityCenter::setupTypeInfo();
        procedural::IProceduralGenerator::setupTypeInfo();
        procedural::IProceduralManager::setupTypeInfo();
        procedural::IProceduralMesh::setupTypeInfo();
        procedural::IProceduralModelRule::setupTypeInfo();
        procedural::IProceduralModelSystem::setupTypeInfo();
        procedural::IProceduralNode::setupTypeInfo();
        procedural::IProceduralObject::setupTypeInfo();
        procedural::IProceduralScene::setupTypeInfo();
        procedural::IProceduralTexture::setupTypeInfo();
        procedural::IRoad::setupTypeInfo();
        procedural::IRoadConnection::setupTypeInfo();
        procedural::IRoadConnectionData::setupTypeInfo();
        procedural::IRoadElement::setupTypeInfo();
        procedural::IRoadGenerator::setupTypeInfo();
        procedural::IRoadHitPoint::setupTypeInfo();
        procedural::IRoadMeshElement::setupTypeInfo();
        procedural::IRoadNetwork::setupTypeInfo();
        procedural::IRoadNode::setupTypeInfo();
        procedural::ITerrainGenerator::setupTypeInfo();
        procedural::IWorldGenerator::setupTypeInfo();

        // Rendering types.
        render::Billboard::setupTypeInfo();
        render::BillboardSet::setupTypeInfo();
        render::ComputeShader::setupTypeInfo();
        render::DebugLine::setupTypeInfo();
        render::DebugText::setupTypeInfo();
        render::DecalCursor::setupTypeInfo();
        render::DynamicLines::setupTypeInfo();
        render::DynamicMesh::setupTypeInfo();
        render::Font::setupTypeInfo();
        render::FontManager::setupTypeInfo();
        render::GraphicsCamera::setupTypeInfo();
        render::GraphicsCubemap::setupTypeInfo();
        render::GraphicsMesh::setupTypeInfo();
        render::GraphicsScene::setupTypeInfo();
        render::GraphicsScene::StateListener::setupTypeInfo();
        render::GraphicsSceneNode::setupTypeInfo();
        render::GraphicsSkeleton::setupTypeInfo();
        render::GraphicsSystem::setupTypeInfo();
        render::GraphicsSystem::StateListener::setupTypeInfo();
        render::GraphicsWindow::setupTypeInfo();
        render::IAnimationController::setupTypeInfo();
        render::IAnimationControllerListener::setupTypeInfo();
        render::IAnimationState::setupTypeInfo();
        render::IAnimationStateController::setupTypeInfo();
        render::IAnimationTextureControl::setupTypeInfo();
        render::IBillboard::setupTypeInfo();
        render::IBillboardSet::setupTypeInfo();
        render::IComputeShader::setupTypeInfo();
        render::IDebug::setupTypeInfo();
        render::IDebugCircle::setupTypeInfo();
        render::IDebugLine::setupTypeInfo();
        render::IDebugText::setupTypeInfo();
        render::IDecalCursor::setupTypeInfo();
        render::IDynamicLines::setupTypeInfo();
        render::IDynamicMesh::setupTypeInfo();
        render::IFont::setupTypeInfo();
        render::IFontManager::setupTypeInfo();
        render::IFrustum::setupTypeInfo();
        render::IGraphicsBone::setupTypeInfo();
        render::IGraphicsCamera::setupTypeInfo();
        render::IGraphicsCubemap::setupTypeInfo();
        render::IGraphicsDeferredShading::setupTypeInfo();
        render::IGraphicsLight::setupTypeInfo();
        render::IGraphicsMesh::setupTypeInfo();
        render::IGraphicsNode::setupTypeInfo();
        render::IGraphicsObject::setupTypeInfo();
        render::IGraphicsScene::setupTypeInfo();
        render::IGraphicsSceneNode::setupTypeInfo();
        render::IGraphicsSkeleton::setupTypeInfo();
        render::IGraphicsSystem::setupTypeInfo();
        render::IGraphicsTerrain::setupTypeInfo();
        render::IGraphicsWater::setupTypeInfo();
        render::IGraphicsWindow::setupTypeInfo();
        render::IGraphicsWindowEvent::setupTypeInfo();
        render::IGraphicsWindowListener::setupTypeInfo();
        render::ILightmap::setupTypeInfo();
        render::ILightmapper::setupTypeInfo();
        render::IMaterial::setupTypeInfo();
        render::IMaterialManager::setupTypeInfo();
        render::IMaterialNode::setupTypeInfo();
        render::IMaterialNodeAnimatedTexture::setupTypeInfo();
        render::IMaterialPass::setupTypeInfo();
        render::IMaterialShader::setupTypeInfo();
        render::IMaterialTechnique::setupTypeInfo();
        render::IMaterialTexture::setupTypeInfo();
        render::IMeshConverter::setupTypeInfo();
        render::IOverlay::setupTypeInfo();
        render::IOverlayElement::setupTypeInfo();
        render::IOverlayElementContainer::setupTypeInfo();
        render::IOverlayElementText::setupTypeInfo();
        render::IOverlayElementVector::setupTypeInfo();
        render::IOverlayManager::setupTypeInfo();
        render::IParticle::setupTypeInfo();
        render::IParticleAffector::setupTypeInfo();
        render::IParticleEmitter::setupTypeInfo();
        render::IParticleManager::setupTypeInfo();
        render::IParticleNode::setupTypeInfo();
        render::IParticleRenderer::setupTypeInfo();
        render::IParticleSystem::setupTypeInfo();
        render::IParticleTechnique::setupTypeInfo();
        render::IRenderer::setupTypeInfo();
        render::IRenderer2::setupTypeInfo();
        render::IRenderer3::setupTypeInfo();
        render::IRenderTarget::setupTypeInfo();
        render::IRenderTexture::setupTypeInfo();
        render::IShader::setupTypeInfo();
        render::IShaderManager::setupTypeInfo();
        render::ISky::setupTypeInfo();
        render::ISkybox::setupTypeInfo();
        render::ISkyboxCube::setupTypeInfo();
        render::ISkyboxPlane::setupTypeInfo();
        render::ISkySphere::setupTypeInfo();
        render::ISprite::setupTypeInfo();
        render::ITerrainBlendMap::setupTypeInfo();
        render::ITexture::setupTypeInfo();
        render::ITextureManager::setupTypeInfo();
        render::IVideo::setupTypeInfo();
        render::IVideoManager::setupTypeInfo();
        render::IVideoMaterial::setupTypeInfo();
        render::IVideoStream::setupTypeInfo();
        render::IVideoTexture::setupTypeInfo();
        render::IViewport::setupTypeInfo();
        render::IVolumeRenderer::setupTypeInfo();

        render::GraphicsSettings::setupTypeInfo();
        render::GraphicsLight::setupTypeInfo();
        render::Material::setupTypeInfo();
        render::Material::MaterialStateListener::setupTypeInfo();
        render::MaterialManager::setupTypeInfo();
        render::MaterialManager::StateListener::setupTypeInfo();
        render::MaterialPass::setupTypeInfo();
        render::MaterialTechnique::setupTypeInfo();
        render::MaterialTexture::setupTypeInfo();
        render::Overlay::setupTypeInfo();
        render::ParticleAffector::setupTypeInfo();
        render::ParticleEmitter::setupTypeInfo();
        render::ParticleManager::setupTypeInfo();
        render::ParticleSystem::setupTypeInfo();
        render::ParticleTechnique::setupTypeInfo();
        render::RenderTexture::setupTypeInfo();
        render::Shader::setupTypeInfo();
        render::Skybox::setupTypeInfo();
        render::SkyboxCube::setupTypeInfo();
        render::SkyboxPlane::setupTypeInfo();
        render::SkySphere::setupTypeInfo();
        render::Terrain::setupTypeInfo();
        render::Texture::setupTypeInfo();
        render::Texture::StateListener::setupTypeInfo();
        render::TextureManager::setupTypeInfo();
        render::TextureManager::StateListener::setupTypeInfo();
        render::Viewport::setupTypeInfo();

        // Scene types.
        scene::AnimatedMaterial::setupTypeInfo();
        scene::Animation::setupTypeInfo();
        scene::Animator::setupTypeInfo();
        scene::AudioEmitter::setupTypeInfo();
        scene::Billboard::setupTypeInfo();
        scene::Billboards::setupTypeInfo();
        scene::Button::setupTypeInfo();
        scene::ButtonDirector::setupTypeInfo();
        scene::Camera::setupTypeInfo();
        scene::CameraController::setupTypeInfo();
        scene::CameraController::EventListener::setupTypeInfo();
        scene::CameraFollow::setupTypeInfo();
        scene::CameraManager::setupTypeInfo();
        scene::CameraTarget::setupTypeInfo();
        scene::CarController::setupTypeInfo();
        scene::CharacterController::setupTypeInfo();
        scene::Collision::setupTypeInfo();
        scene::CollisionBox::setupTypeInfo();
        scene::CollisionMesh::setupTypeInfo();
        scene::CollisionPlane::setupTypeInfo();
        scene::CollisionSphere::setupTypeInfo();
        scene::CollisionTerrain::setupTypeInfo();
        scene::Component::setupTypeInfo();
        scene::Component::ComponentFSMListener::setupTypeInfo();
        scene::ComponentSystem::setupTypeInfo();
        scene::Constraint::setupTypeInfo();
        scene::Cubemap::setupTypeInfo();
        scene::Cutscene::setupTypeInfo();
        scene::CutscenePlayer::setupTypeInfo();
        Director::setupTypeInfo();
        scene::Dropdown::setupTypeInfo();
        scene::Dropdown::Option::setupTypeInfo();
        scene::EditorCameraController::setupTypeInfo();
        scene::FiniteStateMachine::setupTypeInfo();
        scene::FpsCameraController::setupTypeInfo();
        scene::GameActor::setupTypeInfo();
        scene::GameActor::FsmListener::setupTypeInfo();
        scene::GameEditor::setupTypeInfo();
        scene::GameManager::setupTypeInfo();
        scene::GameManager::EventListener::setupTypeInfo();
        scene::GamePrefab::setupTypeInfo();
        scene::GamePrefabManager::setupTypeInfo();
        scene::GameScene::setupTypeInfo();
        scene::GameSceneBuilder::setupTypeInfo();
        scene::GraphicsSettingsDirector::setupTypeInfo();
        scene::GridLayout::setupTypeInfo();
        scene::HorizontalLayout::setupTypeInfo();
        scene::ICameraManager::setupTypeInfo();
        scene::IComponent::setupTypeInfo();
        scene::IComponentEvent::setupTypeInfo();
        scene::IComponentEventListener::setupTypeInfo();
        scene::IComponentSystem::setupTypeInfo();
        scene::IGameActor::setupTypeInfo();
        scene::IGameEditor::setupTypeInfo();
        scene::IGameManager::setupTypeInfo();
        scene::IGamePrefab::setupTypeInfo();
        scene::IGamePrefabManager::setupTypeInfo();
        scene::IGameScene::setupTypeInfo();
        scene::IGameSceneBuilder::setupTypeInfo();
        scene::Image::setupTypeInfo();
        scene::InputField::setupTypeInfo();
        scene::ISubComponent::setupTypeInfo();
        scene::ITransform::setupTypeInfo();
        scene::Layout::setupTypeInfo();
        scene::LayoutContainer::setupTypeInfo();
        scene::LayoutTransform::setupTypeInfo();
        scene::Light::setupTypeInfo();
        scene::LightingDirector::setupTypeInfo();
        scene::LODGroup::setupTypeInfo();
        scene::LODSystem::setupTypeInfo();
        scene::Material::setupTypeInfo();
        scene::Material::MaterialStateListener::setupTypeInfo();
        scene::Material::MaterialStateObjectListener::setupTypeInfo();
        scene::MaterialResourceDirector::setupTypeInfo();
        scene::Mesh::setupTypeInfo();
        scene::MeshRenderer::setupTypeInfo();
        scene::MeshResourceDirector::setupTypeInfo();
        scene::NetworkListener::setupTypeInfo();
        scene::NetworkPlayer::setupTypeInfo();
        scene::NetworkStream::setupTypeInfo();
        scene::NetworkView::setupTypeInfo();
        scene::NetworkView::Listener::setupTypeInfo();
        scene::PanelDirector::setupTypeInfo();
        scene::ParticleSystem::setupTypeInfo();
        scene::Renderer::setupTypeInfo();
        scene::RenderTexture::setupTypeInfo();
        scene::ResourceDirector::setupTypeInfo();
        scene::Rigidbody::setupTypeInfo();
        scene::RigidbodyListener::setupTypeInfo();
        scene::Script::setupTypeInfo();
        scene::ScrollBar::setupTypeInfo();
        scene::ScrollView::setupTypeInfo();
        scene::Skybox::setupTypeInfo();
        scene::Skybox::MaterialSharedListener::setupTypeInfo();
        scene::SkyboxPanorama::setupTypeInfo();
        scene::ProceduralMeshComponent::setupTypeInfo();
        scene::ProceduralVehicle::setupTypeInfo();
        scene::ProceduralRoad::setupTypeInfo();
        scene::ProceduralSky::setupTypeInfo();
        scene::ProceduralSurfaceTexture::setupTypeInfo();

        procedural::IRoadSystem::setupTypeInfo();
        procedural::ISkyAtmosphere::setupTypeInfo();
        procedural::ITextureForge::setupTypeInfo();
        procedural::IVehicleAppearance::setupTypeInfo();
        procedural::IVehicleDamage::setupTypeInfo();
        procedural::IVehicleDynamics::setupTypeInfo();
        procedural::IVehicleEffects::setupTypeInfo();
        procedural::IVehicleGenerator::setupTypeInfo();
        procedural::IVehicleGeometry::setupTypeInfo();
        procedural::IVehiclePhysics::setupTypeInfo();
        procedural::IVehiclePresentation::setupTypeInfo();

        scene::Slider::setupTypeInfo();
        scene::SoundResourceDirector::setupTypeInfo();
        scene::SphericalCameraController::setupTypeInfo();
        scene::SubComponent::setupTypeInfo();
        scene::TabItem::setupTypeInfo();
        scene::TableCell::setupTypeInfo();
        scene::TableLayout::setupTypeInfo();
        scene::TabPage::setupTypeInfo();
        scene::TabView::setupTypeInfo();
        scene::TerrainBlendMap::setupTypeInfo();
        scene::TerrainGrassLayer::setupTypeInfo();
        scene::TerrainLayer::setupTypeInfo();
        scene::TerrainSystem::setupTypeInfo();
        scene::TerrainTreeLayer::setupTypeInfo();
        scene::Text::setupTypeInfo();
        scene::TextDirector::setupTypeInfo();
        scene::TextureResourceDirector::setupTypeInfo();
        scene::ThirdPersonCameraController::setupTypeInfo();
        scene::Thumbnail::setupTypeInfo();
        scene::Toggle::setupTypeInfo();
        scene::ToggleGroup::setupTypeInfo();
        scene::ToolTip::setupTypeInfo();
        scene::Transform::setupTypeInfo();
        scene::UIComponent::setupTypeInfo();
        scene::UIComponent::UIElementListener::setupTypeInfo();
        scene::UiDialogDirector::setupTypeInfo();
        scene::UiDirector::setupTypeInfo();
        scene::UiElementDirector::setupTypeInfo();
        scene::VehicleCameraController::setupTypeInfo();
        scene::VehicleController::setupTypeInfo();
        scene::VerticalLayout::setupTypeInfo();
        scene::VideoPlayer::setupTypeInfo();
        scene::WheelController::setupTypeInfo();

        // UI types.
        ui::IUIAbout::setupTypeInfo();
        ui::IUIApplication::setupTypeInfo();
        ui::IUIButton::setupTypeInfo();
        ui::IUICheckbox::setupTypeInfo();
        ui::IUICollapsingHeader::setupTypeInfo();
        ui::IUIColourPicker::setupTypeInfo();
        ui::IUIDataGrid::setupTypeInfo();
        ui::IUIDial::setupTypeInfo();
        ui::IUIDragSource::setupTypeInfo();
        ui::IUIDropdown::setupTypeInfo();
        ui::IUIDropTarget::setupTypeInfo();
        ui::IUIElement::setupTypeInfo();
        ui::IUIEvent::setupTypeInfo();
        ui::IUIEventWindow::setupTypeInfo();
        ui::IUIFileBrowser::setupTypeInfo();
        ui::IUIFrame::setupTypeInfo();
        ui::IUIGrid::setupTypeInfo();
        ui::IUIHorizontalLayout::setupTypeInfo();
        ui::IUIImage::setupTypeInfo();
        ui::IUIInputManager::setupTypeInfo();
        ui::IUILabelDropdownPair::setupTypeInfo();
        ui::IUILabelSliderPair::setupTypeInfo();
        ui::IUILabelTextInputPair::setupTypeInfo();
        ui::IUILabelTogglePair::setupTypeInfo();
        ui::IUILayoutContainer::setupTypeInfo();
        ui::IUILayoutWindow::setupTypeInfo();
        ui::IUIManager::setupTypeInfo();
        ui::IUIMenu::setupTypeInfo();
        ui::IUIMenubar::setupTypeInfo();
        ui::IUIMenuItem::setupTypeInfo();
        ui::IUIOutputConsole::setupTypeInfo();
        ui::IUIProfilerWindow::setupTypeInfo();
        ui::IUIProfileWindow::setupTypeInfo();
        ui::IUIProgressBar::setupTypeInfo();
        ui::IUIPropertyGrid::setupTypeInfo();
        ui::IUIRenderWindow::setupTypeInfo();
        ui::IUIScrollingText::setupTypeInfo();
        ui::IUISearchBar::setupTypeInfo();
        ui::IUISeparator::setupTypeInfo();
        ui::IUISlider::setupTypeInfo();
        ui::IUISpinner::setupTypeInfo();
        ui::IUITabBar::setupTypeInfo();
        ui::IUITabItem::setupTypeInfo();
        ui::IUITerrainEditor::setupTypeInfo();
        ui::IUIText::setupTypeInfo();
        ui::IUITextEntry::setupTypeInfo();
        ui::IUIToggle::setupTypeInfo();
        ui::IUIToggleGroup::setupTypeInfo();
        ui::IUIToolbar::setupTypeInfo();
        ui::IUITreeCtrl::setupTypeInfo();
        ui::IUITreeNode::setupTypeInfo();
        ui::IUIVector2::setupTypeInfo();
        ui::IUIVector3::setupTypeInfo();
        ui::IUIVector4::setupTypeInfo();
        ui::IUIVerticalLayout::setupTypeInfo();
        ui::IUIWindow::setupTypeInfo();
        ui::UIButton::setupTypeInfo();
        ui::UIDropdown::setupTypeInfo();
        ui::UIHorizontalLayout::setupTypeInfo();
        ui::UIImage::setupTypeInfo();
        ui::UILayout::setupTypeInfo();
        ui::UIManager::setupTypeInfo();
        ui::UIProgressBar::setupTypeInfo();
        ui::UISlider::setupTypeInfo();
        ui::UIText::setupTypeInfo();
        ui::UITextEntry::setupTypeInfo();
        ui::UIToggle::setupTypeInfo();
        ui::UIVerticalLayout::setupTypeInfo();
        ui::UIWindow::setupTypeInfo();

        // Vehicle types.
        vehicle::IAerodymanicsWind::setupTypeInfo();
        vehicle::IAerofoil::setupTypeInfo();
        vehicle::IAircraft::setupTypeInfo();
        vehicle::IAircraftBody::setupTypeInfo();
        vehicle::IAircraftCallback::setupTypeInfo();
        vehicle::IAircraftControlSurface::setupTypeInfo();
        vehicle::IAircraftPlane::setupTypeInfo();
        vehicle::IAircraftPowerUnit::setupTypeInfo();
        vehicle::IAircraftPropeller::setupTypeInfo();
        vehicle::IAircraftPropellerUnit::setupTypeInfo();
        vehicle::IAircraftPropWash::setupTypeInfo();
        vehicle::IAircraftTurbineUnit::setupTypeInfo();
        vehicle::IAircraftWing::setupTypeInfo();
        vehicle::IBatteryPack::setupTypeInfo();
        vehicle::IDifferential::setupTypeInfo();
        vehicle::IDriveTrain::setupTypeInfo();
        vehicle::IDrone::setupTypeInfo();
        vehicle::IElectricMotor::setupTypeInfo();
        vehicle::IESController::setupTypeInfo();
        vehicle::IFourWheelVehicle::setupTypeInfo();
        vehicle::IGearBox::setupTypeInfo();
        vehicle::IGroundEffect::setupTypeInfo();
        vehicle::IHelicopter::setupTypeInfo();
        vehicle::ITruckVehicle::setupTypeInfo();
        vehicle::IVehicle::setupTypeInfo();
        vehicle::IVehicleBody::setupTypeInfo();
        vehicle::IVehicleCallback::setupTypeInfo();
        vehicle::IVehicleComponent::setupTypeInfo();
        vehicle::IVehicleManager::setupTypeInfo();
        vehicle::IVehiclePowerUnit::setupTypeInfo();
        vehicle::IWheelComponent::setupTypeInfo();
        vehicle::Vehicle::setupTypeInfo();
        vehicle::VehicleBody::setupTypeInfo();
        vehicle::VehicleManager::setupTypeInfo();
        vehicle::WheelComponent::setupTypeInfo();

        for( u32 i = 0; i < baseObjectDataPools.size(); ++i )
        {
            // if( ui::IUIElement::typeInfo() == i )
            {
                baseObjectDataPools[i].setNextSize( 32 );
            }
        }

        for( u32 i = 0; i < sharedObjectDataPools.size(); ++i )
        {
            // if( ui::IUIElement::typeInfo() == i )
            {
                sharedObjectDataPools[i].setNextSize( 32 );
            }
        }

        for( u32 i = 0; i < baseObjectDataPools.size(); ++i )
        {
            // if( ui::IUIElement::typeInfo() == i )
            {
                baseObjectDataPools[i].allocateData();
            }
        }

        for( u32 i = 0; i < sharedObjectDataPools.size(); ++i )
        {
            // if( ui::IUIElement::typeInfo() == i )
            {
                sharedObjectDataPools[i].allocateData();
            }
        }
    }

    void TypeManager::unload()
    {
        baseObjectDataPool.clear();
        sharedObjectDataPool.clear();

        types.clear();

        m_names.clear();
        m_labels.clear();
        m_hashes.clear();
        m_typeIndex.clear();
        m_typeGroup.clear();
        m_hashTypes.clear();
        m_baseType.clear();
        m_dataTypes.clear();
        m_numInstances.clear();

        iTypeInfo.clear();
        objectFlags.clear();

        m_objectDataSize = 0;
        m_objectDataIndex = 0;
        m_typeCount = 1;
        m_size = 0;
    }

    auto TypeManager::getName( u32 id ) const -> const c8 *
    {
        WP_ASSERT( id < m_names.size() );
        return m_names[id].c_str();
    }

    auto TypeManager::getLabel( u32 id ) const -> const c8 *
    {
        WP_ASSERT( id < m_labels.size() );
        return m_labels[id].c_str();
    }

    void TypeManager::setLabel( u32 id, const String &label )
    {
        m_labels[id] = label.c_str();
    }

    auto TypeManager::getHash( u32 hash ) const -> hash64
    {
        WP_ASSERT( hash < getSize() );
        return m_hashes[hash];
    }

    auto TypeManager::getBaseType( u32 id ) const -> u32
    {
        WP_ASSERT( id < getSize() );
        return m_baseType[id];
    }

    auto TypeManager::isExactly( u32 a, u32 b ) const -> bool
    {
        WP_ASSERT( a < getSize() );
        WP_ASSERT( b < getSize() );
        return a == b;
    }

    auto TypeManager::isDerived( u32 a, u32 b ) const -> bool
    {
        WP_ASSERT( a < getSize() );
        WP_ASSERT( b < getSize() );

        // Quick check for same type
        if( a == b )
        {
            return true;
        }

        // Skip if b is 0 (root type) - nothing derives from root except itself
        if( b == 0 )
        {
            return false;
        }

        u32 baseType = m_baseType[a];
        while( baseType )
        {
            if( baseType == b )
            {
                return true;
            }

            baseType = m_baseType[baseType];
        }

        return false;
    }

    auto TypeManager::getClassHierarchy( u32 id ) const -> Array<String>
    {
        Array<String> classHierarchy;
        classHierarchy.reserve( 4 );

        auto baseType = id;
        while( baseType )
        {
            auto name = getName( baseType );
            classHierarchy.emplace_back( name );
            baseType = getBaseType( baseType );
        }

        return classHierarchy;
    }

    auto TypeManager::getClassHierarchyId( u32 id ) const -> Array<u32>
    {
        Array<u32> classHierarchy;
        classHierarchy.reserve( 4 );

        auto baseType = id;
        while( baseType )
        {
            classHierarchy.push_back( baseType );
            baseType = getBaseType( baseType );
        }

        return classHierarchy;
    }

    auto TypeManager::getTypeIndex( u32 id ) const -> u32
    {
        WP_ASSERT( id < getSize() );
        return m_typeIndex[id];
    }

    auto TypeManager::getNumInstances( u32 id ) const -> u32
    {
        WP_ASSERT( id < getSize() );
        return m_numInstances[id];
    }

    UUID TypeManager::getUUID( u32 id ) const
    {
        auto name = getName( id );
        return StringUtil::getUUID( name );
    }

    auto TypeManager::getTypeByName( const String &name ) -> u32
    {
        auto it = std::find( m_names.begin(), m_names.end(), name.c_str() );
        if( it != m_names.end() )
        {
            return static_cast<u32>( std::distance( m_names.begin(), it ) );
        }

        return 0;
    }

    auto TypeManager::getNewTypeId( const String &name, u32 baseType ) -> u32
    {
        WP_ASSERT( StringUtil::isNullOrEmpty( name ) == false );

        auto type = getTypeByName( name );
        if( type != 0 )
        {
            return type;
        }

        WP_ASSERT( m_typeCount <= getSize() );
        auto newId = m_typeCount++;
        if( newId >= getSize() )
        {
            auto size = getSize() + WP_TYPE_MANAGER_NUM_TYPES;
            reserve( size );
        }

        WP_ASSERT( newId < getSize() );

        if( !StringUtil::isNullOrEmpty( name ) )
        {
            m_names[newId] = name.c_str();
        }

        WP_ASSERT( m_baseType[newId].load() == 0 );

        m_hashes[newId] = StringUtil::getHash64( name );
        m_typeIndex[newId] = newId;

        WP_ASSERT( m_baseType[newId].load() == 0 );
        m_baseType[newId] = baseType;

        return newId;
    }

    auto TypeManager::getNewTypeId() -> u32
    {
        return m_typeCount++;
    }

    auto TypeManager::getNewTypeIdFromName( const String &name, const String &baseName ) -> u32
    {
        WP_ASSERT( StringUtil::isNullOrEmpty( name ) == false );

        auto type = getTypeByName( name );
        if( type == 0 )
        {
            auto baseType = getTypeByName( baseName );
            return getNewTypeId( name, baseType );
        }

        return type;
    }

    auto TypeManager::getBaseTypes( u32 type ) const -> Array<u32>
    {
        Array<u32> types;
        types.reserve( 12 );

        type = getBaseType( type );
        while( type )
        {
            types.push_back( type );
            type = getBaseType( type );
        }

        return types;
    }

    auto TypeManager::getBaseTypeNames( u32 type ) const -> Array<String>
    {
        Array<String> types;
        types.reserve( 12 );

        type = getBaseType( type );
        while( type )
        {
            auto name = getName( type );
            types.emplace_back( name );
            type = getBaseType( type );
        }

        return types;
    }

    auto TypeManager::getDerivedTypes( u32 type ) const -> Array<u32>
    {
        Array<u32> types;
        types.reserve( 12 );

        for( size_t i = 0; i < m_typeCount; ++i )
        {
            auto currentType = static_cast<u32>( i );
            if( isDerived( currentType, type ) )
            {
                types.push_back( currentType );
            }
        }

        return types;
    }

    void TypeManager::getDerivedTypes( u32 type, u32 *buffer, u32 &bufferSize, u32 maxBufferSize ) const
    {
        bufferSize = 0;
        for( size_t i = 0; i < m_typeCount; ++i )
        {
            auto currentType = static_cast<u32>( i );
            if( isDerived( currentType, type ) )
            {
                if( bufferSize < maxBufferSize )  // Ensure we don't exceed the buffer size
                {
                    buffer[bufferSize++] = currentType;
                }
                else
                {
                    break;  // Stop if we've filled the buffer
                }
            }
        }
    }

    Array<String> TypeManager::getDerivedTypeNames( u32 type ) const
    {
        Array<String> types;
        types.reserve( 12 );

        for( size_t i = 0; i < m_typeCount; ++i )
        {
            auto currentType = static_cast<u32>( i );
            if( isDerived( currentType, type ) )
            {
                auto name = getName( currentType );
                types.push_back( name );
            }
        }

        return types;
    }

    auto TypeManager::getTypeGroup( u32 id ) const -> u32
    {
        if( id < getSize() )
        {
            if( m_typeGroup[id].load() == 0 )
            {
                m_typeGroup[id] = calculateGroupIndex( id );
            }

            return m_typeGroup[id];
        }

        return 0;
    }

    u32 TypeManager::getIdFromName( const String &name ) const
    {
        for( size_t i = 0; i < m_names.size(); ++i )
        {
            const auto &currentName = m_names[i];
            if( StringUtil::isEqual( currentName.c_str(), name ) )
            {
                return (u32)i;
            }
        }

        return 0;
    }

    u32 TypeManager::getIdFromHash( hash64 hash ) const
    {
        for( size_t i = 0; i < m_hashes.size(); ++i )
        {
            auto currentHash = m_hashes[i].load();
            if( currentHash == hash )
            {
                return (u32)i;
            }
        }

        return 0;
    }

    void TypeManager::initialiseMemoryPools()
    {
        String::preallocate_character_pool( 32, 1024 );
        String::preallocate_character_pool( 64, 1024 );
        String::preallocate_character_pool( 128, 1024 );
        String::preallocate_character_pool( 256, 1024 );
        String::preallocate_character_pool( 512, 512 );
        String::preallocate_character_pool( 1024, 256 );

        StringW::preallocate_character_pool( 256, 1024 );
        StringW::preallocate_character_pool( 512, 512 );
        StringW::preallocate_character_pool( 1024, 256 );
    }

    auto TypeManager::getTotalNumTypes() -> u32
    {
        return m_typeCount;
    }

    auto TypeManager::getDataType( u32 id ) const -> u32
    {
        WP_ASSERT( id < getSize() );
        return m_dataTypes[id];
    }

    void TypeManager::setDataType( u32 id, u32 dataType )
    {
        WP_ASSERT( id < getSize() );
        m_dataTypes[id] = dataType;
    }

    auto TypeManager::calculateGroupIndex( u32 typeInfo ) const -> u32
    {
        auto index = TypeGroups::Application;

        if( isDerived( typeInfo, IJob::typeInfo() ) )
        {
            index = TypeGroups::Jobs;
        }
        else if( isDerived( typeInfo, render::IGraphicsSceneNode::typeInfo() ) )
        {
            index = TypeGroups::RenderNodes;
        }
        else if( isDerived( typeInfo, render::IGraphicsObject::typeInfo() ) )
        {
            index = TypeGroups::RenderObjects;
        }
        else if( isDerived( typeInfo, render::ITexture::typeInfo() ) )
        {
            index = TypeGroups::RenderTextures;
        }
        else if( isDerived( typeInfo, render::IMaterialNode::typeInfo() ) )
        {
            index = TypeGroups::RenderMaterialNodes;
        }
        else if( isDerived( typeInfo, render::IMaterial::typeInfo() ) )
        {
            index = TypeGroups::RenderMaterials;
        }
        else if( isDerived( typeInfo, IFactory::typeInfo() ) )
        {
            index = TypeGroups::Factories;
        }
        else if( isDerived( typeInfo, IArchive::typeInfo() ) )
        {
            index = TypeGroups::IOArchive;
        }
        else if( isDerived( typeInfo, IStream::typeInfo() ) )
        {
            index = TypeGroups::IOStream;
        }
        else if( isDerived( typeInfo, IFolderExplorer::typeInfo() ) )
        {
            index = TypeGroups::IO;
        }
        else if( isDerived( typeInfo, core::ApplicationManager::typeInfo() ) ||
                 isDerived( typeInfo, render::IGraphicsSystem::typeInfo() ) ||
                 isDerived( typeInfo, IFactoryManager::typeInfo() ) ||
                 isDerived( typeInfo, IFSMManager::typeInfo() ) ||
                 isDerived( typeInfo, ILogManager::typeInfo() ) ||
                 isDerived( typeInfo, IStateManager::typeInfo() ) ||
                 isDerived( typeInfo, IScriptManager::typeInfo() ) ||
                 isDerived( typeInfo, scene::IGameManager::typeInfo() ) ||
                 isDerived( typeInfo, scene::IGameScene::typeInfo() ) ||
                 isDerived( typeInfo, ITaskManager::typeInfo() ) ||
                 isDerived( typeInfo, IThreadPool::typeInfo() ) ||
                 isDerived( typeInfo, ITimer::typeInfo() ) ||
                 isDerived( typeInfo, ui::IUIManager::typeInfo() ) )
        {
            index = TypeGroups::System;
        }
        else if( isDerived( typeInfo, scene::IGameActor::typeInfo() ) )
        {
            index = TypeGroups::Actor;
        }
        else if( isDerived( typeInfo, scene::IComponent::typeInfo() ) )
        {
            index = TypeGroups::Component;
        }
        else if( isDerived( typeInfo, IFSM::typeInfo() ) )
        {
            index = TypeGroups::FSM;
        }
        else if( isDerived( typeInfo, IFSMListener::typeInfo() ) )
        {
            index = TypeGroups::FSMListener;
        }
        else if( isDerived( typeInfo, ui::IUIRenderWindow::typeInfo() ) )
        {
            index = TypeGroups::UIRenderWindows;
        }
        else if( isDerived( typeInfo, ui::IUIWindow::typeInfo() ) )
        {
            index = TypeGroups::UIGraphicsWindows;
        }
        else if( isDerived( typeInfo, ui::IUITreeNode::typeInfo() ) )
        {
            index = TypeGroups::UITreeNodes;
        }
        else if( isDerived( typeInfo, ui::IUIText::typeInfo() ) )
        {
            index = TypeGroups::UIText;
        }
        else if( isDerived( typeInfo, ui::IUIElement::typeInfo() ) )
        {
            index = TypeGroups::UI;
        }
        else if( isDerived( typeInfo, IStateContext::typeInfo() ) )
        {
            index = TypeGroups::State;
        }
        else if( isDerived( typeInfo, IStateQueue::typeInfo() ) )
        {
            index = TypeGroups::StateQueues;
        }
        else if( isDerived( typeInfo, IStateMessage::typeInfo() ) )
        {
            index = TypeGroups::StateMessages;
        }
        else if( isDerived( typeInfo, IStateListener::typeInfo() ) )
        {
            index = TypeGroups::StateListeners;
        }
        else if( isDerived( typeInfo, IBuildDirector::typeInfo() ) )
        {
            index = TypeGroups::Directors;
        }
        else if( isDerived( typeInfo, IResource::typeInfo() ) )
        {
            index = TypeGroups::Resources;
        }

        return static_cast<u32>( index );
    }

    auto TypeManager::getSize() const -> u32
    {
        return m_size;
    }

    void TypeManager::reserve( u32 size )
    {
        if( m_size != size )
        {
            m_size = size;

            baseObjectDataPools.resize( size );
            sharedObjectDataPools.resize( size );

            m_names.resize( size );
            m_labels.resize( size );
            m_hashes.resize( size );
            m_typeIndex.resize( size );
            m_typeGroup.resize( size );
            m_baseType.resize( size );
            m_dataTypes.resize( size );
            m_numInstances.resize( size );
        }
    }

    void TypeManager::setInstance( TypeManager *typeManager )
    {
        instance_ = typeManager;
    }

    u32 TypeManager::getObjectId()
    {
        ScopedLock lock( &m_mutex );

        /*
        auto count = 0;
        while( count++ < 100 )
        {
            for( ; m_objectDataIndex < loadingStates.size(); ++m_objectDataIndex )
            {
                auto &i = m_objectDataIndex;
                const auto &loadingState = loadingStates[i];
                if( loadingState == LoadingState::None )
                {
                    iTypeInfo[i] = 0;
                    references[i] = 1;
                    weakReferences[i] = 0;
                    loadingStates[i] = LoadingState::Allocated;
                    objectFlags[i] = OBJECT_FLAG_ALIVE | OBJECT_FLAG_GARBAGE_COLLECTED;

                    scriptData[i] = nullptr;
                    sharedObjectListener[i] = nullptr;
                    sharedEventListeners[i] =
                        workphone::make_shared<ConcurrentArray<SmartPtr<IEventListener>>>();

                    return (u32)i;
                }
            }

            m_objectDataIndex = 0;

            auto growSize = m_objectDataSize;
            resizeObjectData( growSize );
        }
        */

        return 0;
    }

    void TypeManager::resizeObjectData( u32 growSize )
    {
        ScopedLock lock( &m_mutex );

        auto newSize = m_objectDataSize + growSize;

        iTypeInfo.resize( newSize );
        objectFlags.resize( newSize );

        for( size_t i = m_objectDataSize; i < growSize; ++i )
        {
            iTypeInfo[i] = 0;
            objectFlags[i] = OBJECT_FLAG_ALIVE | OBJECT_FLAG_GARBAGE_COLLECTED;
        }

        m_objectDataSize = newSize;
    }

    void TypeManager::registerType( u32 id, SharedPtr<reflection::Type> type )
    {
        types[id] = type;
    }

    SharedPtr<reflection::Type> TypeManager::getType( u32 id )
    {
        auto it = types.find( id );
        return it != types.end() ? it->second : nullptr;
    }

}  // namespace workphone
