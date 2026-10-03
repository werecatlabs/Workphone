#include <GameEditorPCH.hpp>
#include <editor/ProjectManager.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <editor/EditorTypes.hpp>

#include <FBApplication/Script/ScriptGenerator.hpp>
#include <FBApplication/Actor/CActor.hpp>
#include <FBApplication/Components/CarController.hpp>
#include <FBApplication/Components/CameraComponent.hpp>
#include <FBApplication/Components/CollisionBox.hpp>
#include <FBApplication/Components/LightComponent.hpp>
#include <FBApplication/Components/MeshRenderer.hpp>
#include <FBApplication/Components/MaterialComponent.hpp>
#include <FBApplication/Components/MeshComponent.hpp>
#include <FBApplication/Components/ProceduralScene.hpp>
#include <FBApplication/Components/Rigidbody.hpp>
#include <FBApplication/Components/SkyboxComponent.hpp>
#include <FBApplication/Components/TerrainRenderer.hpp>
#include <FBApplication/Components/WheelController.hpp>
#include <FBObjectTemplates/EntityTemplate.hpp>
#include <FBObjectTemplates/EventTemplate.hpp>
#include <FBObjectTemplates/FSMTemplate.hpp>
#include <FBObjectTemplates/EventTemplateContainer.hpp>
#include <FBCore/Interface/Actor/ITransform.hpp>



namespace fb
{	
	namespace editor
	{
	
	
	
		const String ProjectManager::DEFAULT_INITIALISE_LABEL = "initialise";
		const String ProjectManager::DEFAULT_HANDLE_MSG_LABEL = "handleMessage";
		const String ProjectManager::DEFAULT_SET_FREE_LABEL = "setFree";
		const String ProjectManager::DEFAULT_IS_FREE_LABEL = "isFree";
	
		const String ProjectManager::DEFAULT_INITIALISE_FUNC = "initialise";
		const String ProjectManager::DEFAULT_HANDLE_MSG_FUNC = "handleMessage";
		const String ProjectManager::DEFAULT_SET_FREE_FUNC = "setFree";
		const String ProjectManager::DEFAULT_IS_FREE_FUNC = "isFree";
		const String ProjectManager::DEFAULT_CONSTRUCT_START_FUNC = "OnConstructStart";
		const String ProjectManager::DEFAULT_CONSTRUCT_END_FUNC = "OnConstructEnd";
	
		const String ProjectManager::DEFAULT_INITIALISING_STATE_NAME = "initialising";
		const String ProjectManager::DEFAULT_FREE_STATE_NAME = "free";
		const String ProjectManager::DEFAULT_ACTIVE_STATE_NAME = "active";
	
		const String ProjectManager::DEFAULT_FUNC_LABEL_ENTER_STATE = "Enter State";
		const String ProjectManager::DEFAULT_FUNC_LABEL_UDPATE_STATE = "Update State";
		const String ProjectManager::DEFAULT_FUNC_LABEL_LEAVE_STATE = "Leave State";
		const String ProjectManager::DEFAULT_FUNC_LABEL_CAN_CHANGE_STATE = "Can Change State";
	
		const String ProjectManager::DEFAULT_FUNC_ENTER_STATE = "enterState";
		const String ProjectManager::DEFAULT_FUNC_UDPATE_STATE = "updateState";
		const String ProjectManager::DEFAULT_FUNC_LEAVE_STATE = "leaveState";
		const String ProjectManager::DEFAULT_FUNC_CAN_CHANGE_STATE = "canChangeState";
	
		const String ProjectManager::DEFAULT_FUNC_NAME_ENTER_STATE = "enterState";
		const String ProjectManager::DEFAULT_FUNC_NAME_UDPATE_STATE = "updateState";
		const String ProjectManager::DEFAULT_FUNC_NAME_LEAVE_STATE = "leaveState";
		const String ProjectManager::DEFAULT_FUNC_NAME_CAN_CHANGE_STATE = "canChangeState";
	
	
	
		//--------------------------------------------
		ProjectManager::ProjectManager()
		{
		}
	
	
	
		//--------------------------------------------
		ProjectManager::~ProjectManager()
		{
		}



		//--------------------------------------------
		s32 ProjectManager::createNewEntity( const String& type, bool useDefaults /*= false*/ )
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto editorManager = EditorManager::getSingletonPtr();
			auto project = editorManager->getProject();
						
			auto sceneManager = applicationManager->getSceneManager();
			auto scene = sceneManager->getCurrentScene();

			auto actor = fb::make_ptr<CActor>();			
			
			auto name = String("Actor");
			actor->setName(name);
			
			//auto id = StringUtil::getHash(name);
			//actor->setId(id);

			//auto uuid = StringUtil::getUUID();
			//actor->setUUID(uuid);

			// todo remove 
			// for test the renderer
			auto meshRenderer = actor->addComponent<component::MeshRenderer>();

			scene->addActor(actor);
			scene->registerAllUpdates(actor);

			auto uniqueId = 0;// StringUtil::getHash(uuid);
			return uniqueId;
		}
	
	

		//--------------------------------------------
		void ProjectManager::setDefaultFSMStates( SmartPtr<FSMTemplate> entityFSM )
		{
			entityFSM->addState(DEFAULT_INITIALISING_STATE_NAME);
			entityFSM->addState(DEFAULT_FREE_STATE_NAME);
			entityFSM->addState(DEFAULT_ACTIVE_STATE_NAME);
		}
	
	
	
		//--------------------------------------------
		void ProjectManager::setDefaultEvents( SmartPtr<FSMTemplate> entityFSM )
		{
			SmartPtr<EventTemplate> enterStateEvent(new EventTemplate);
			SmartPtr<EventTemplate> updateStateEvent(new EventTemplate);
			SmartPtr<EventTemplate> leaveStateEvent(new EventTemplate);
			SmartPtr<EventTemplate> canChangeStateEvent(new EventTemplate);
	
			enterStateEvent->setLabel(DEFAULT_FUNC_LABEL_ENTER_STATE);
			updateStateEvent->setLabel(DEFAULT_FUNC_LABEL_UDPATE_STATE);
			leaveStateEvent->setLabel(DEFAULT_FUNC_LABEL_LEAVE_STATE);
			canChangeStateEvent->setLabel(DEFAULT_FUNC_LABEL_CAN_CHANGE_STATE);
	
			enterStateEvent->setType(DEFAULT_FUNC_ENTER_STATE);
			updateStateEvent->setType(DEFAULT_FUNC_UDPATE_STATE);
			leaveStateEvent->setType(DEFAULT_FUNC_LEAVE_STATE);
			canChangeStateEvent->setType(DEFAULT_FUNC_CAN_CHANGE_STATE);
	
			enterStateEvent->setFunction(DEFAULT_FUNC_NAME_ENTER_STATE);
			updateStateEvent->setFunction(DEFAULT_FUNC_NAME_UDPATE_STATE);
			leaveStateEvent->setFunction(DEFAULT_FUNC_NAME_LEAVE_STATE);
			canChangeStateEvent->setFunction(DEFAULT_FUNC_NAME_CAN_CHANGE_STATE);
	
			enterStateEvent->addParameter(EditorTypes::DEFAULT_ENTITY_PARAM_NAME);
			enterStateEvent->addParameter(EditorTypes::DEFAULT_PARAMS_PARAM_NAME);
	
			updateStateEvent->addParameter(EditorTypes::DEFAULT_ENTITY_PARAM_NAME);
			updateStateEvent->addParameter(EditorTypes::DEFAULT_PARAMS_PARAM_NAME);
	
			leaveStateEvent->addParameter(EditorTypes::DEFAULT_ENTITY_PARAM_NAME);
			leaveStateEvent->addParameter(EditorTypes::DEFAULT_PARAMS_PARAM_NAME);
	
			canChangeStateEvent->addParameter(EditorTypes::DEFAULT_ENTITY_PARAM_NAME);
			canChangeStateEvent->addParameter(EditorTypes::DEFAULT_PARAMS_PARAM_NAME);
			canChangeStateEvent->addParameter(EditorTypes::DEFAULT_RESULTS_PARAM_NAME);
	
			entityFSM->addEventTemplate(enterStateEvent);
			entityFSM->addEventTemplate(updateStateEvent);
			entityFSM->addEventTemplate(leaveStateEvent);
			entityFSM->addEventTemplate(canChangeStateEvent);
		}
	
	
	
		//--------------------------------------------
		void ProjectManager::setEntityEventDefaults(SmartPtr<EntityTemplate> entityTemplate)
		{
			SmartPtr<EventTemplate> initialiseEvent(new EventTemplate);
			initialiseEvent->setLabel(DEFAULT_INITIALISE_FUNC);
			initialiseEvent->setType(DEFAULT_INITIALISE_FUNC);
			initialiseEvent->setFunction(DEFAULT_INITIALISE_FUNC);
			initialiseEvent->addParameter(EditorTypes::DEFAULT_PARAMS_PARAM_NAME);
			entityTemplate->getEventTemplateContainer()->addEventTemplate(initialiseEvent);
	
			SmartPtr<EventTemplate> handleMsgEvent(new EventTemplate);
			handleMsgEvent->setLabel(DEFAULT_HANDLE_MSG_LABEL);
			handleMsgEvent->setType(DEFAULT_HANDLE_MSG_LABEL);
			handleMsgEvent->setFunction(DEFAULT_HANDLE_MSG_LABEL);
			handleMsgEvent->addParameter(EditorTypes::DEFAULT_PARAMS_PARAM_NAME);
			entityTemplate->getEventTemplateContainer()->addEventTemplate(handleMsgEvent);
	
			SmartPtr<EventTemplate> setFreeEvent(new EventTemplate);
			setFreeEvent->setLabel(DEFAULT_SET_FREE_LABEL);
			setFreeEvent->setType(DEFAULT_SET_FREE_LABEL);
			setFreeEvent->setFunction(DEFAULT_SET_FREE_LABEL);
			setFreeEvent->addParameter(EditorTypes::DEFAULT_PARAMS_PARAM_NAME);
			entityTemplate->getEventTemplateContainer()->addEventTemplate(setFreeEvent);
	
			SmartPtr<EventTemplate> isFreeEvent(new EventTemplate);
			isFreeEvent->setLabel(DEFAULT_IS_FREE_LABEL);
			isFreeEvent->setType(DEFAULT_IS_FREE_LABEL);
			isFreeEvent->setFunction(DEFAULT_IS_FREE_LABEL);
			isFreeEvent->addParameter(EditorTypes::DEFAULT_PARAMS_PARAM_NAME);
			isFreeEvent->addParameter(EditorTypes::DEFAULT_RESULTS_PARAM_NAME);
			entityTemplate->getEventTemplateContainer()->addEventTemplate(isFreeEvent);
	
			SmartPtr<EventTemplate> constructStartEvent(new EventTemplate);
			constructStartEvent->setLabel(DEFAULT_CONSTRUCT_START_FUNC);
			constructStartEvent->setType(DEFAULT_CONSTRUCT_START_FUNC);
			constructStartEvent->setFunction(DEFAULT_CONSTRUCT_START_FUNC);
			constructStartEvent->addParameter(EditorTypes::DEFAULT_PARAMS_PARAM_NAME);
			entityTemplate->getEventTemplateContainer()->addEventTemplate(constructStartEvent);
	
			SmartPtr<EventTemplate> constructEndEvent(new EventTemplate);
			constructEndEvent->setLabel(DEFAULT_CONSTRUCT_END_FUNC);
			constructEndEvent->setType(DEFAULT_CONSTRUCT_END_FUNC);
			constructEndEvent->setFunction(DEFAULT_CONSTRUCT_END_FUNC);
			constructEndEvent->addParameter(EditorTypes::DEFAULT_PARAMS_PARAM_NAME);
			entityTemplate->getEventTemplateContainer()->addEventTemplate(constructEndEvent);
		}
	
	
	
	} // end namespace editor
} // end namespace fb
