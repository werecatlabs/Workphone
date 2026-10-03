#include <GameEditorPCH.hpp>
#include "RemoveSelectionCmd.hpp"
#include <editor/EditorManager.hpp>
#include <ui/UIManager.hpp>
#include <FBCore/FBCoreHeaders.hpp>
#include <FBApplication/FBApplicationHeaders.hpp>



namespace fb
{
	namespace editor
	{



		RemoveSelectionCmd::RemoveSelectionCmd()
		{

		}

		RemoveSelectionCmd::~RemoveSelectionCmd()
		{

		}

		void RemoveSelectionCmd::undo()
		{
			auto applicationManager = IApplicationManager::instance();
			auto factoryManager = applicationManager->getFactoryManager();

			auto sceneManager = applicationManager->getSceneManager();
			auto scene = sceneManager->getCurrentScene();

			for (auto selectionData : m_actorData)
			{
				auto actor = factoryManager->make_ptr<CActor>();
				
				auto data = selectionData->getActorData();
				actor->fromData(data);

				auto parent = selectionData->getParent();
				if (parent)
				{
					parent->addChild(actor);
				}
				else
				{
					scene->addActor(actor);
					scene->registerAllUpdates(actor);
				}

				selectionData->setActor(actor);
			}

			auto editorManager = EditorManager::getSingletonPtr();
			auto uiManager = editorManager->getUI();
			uiManager->rebuildSceneTree();
		}

		void RemoveSelectionCmd::redo()
		{
			auto applicationManager = IApplicationManager::instance();
			auto sceneManager = applicationManager->getSceneManager();
			auto scene = sceneManager->getCurrentScene();

			for (auto selectionData : m_actorData)
			{
				auto actor = selectionData->getActor();
				if (actor)
				{
					scene->removeActor(actor);
					auto parent = actor->getParent();
					if (parent)
					{
						parent->removeChild(actor);
						selectionData->setParent(parent);
					}

					auto data = actor->toData();
					if (data)
					{
						selectionData->setActorData(data);
					}

					actor->unload(0);
				}
			}

			auto editorManager = EditorManager::getSingletonPtr();
			auto uiManager = editorManager->getUI();
			uiManager->rebuildSceneTree();
		}

		void RemoveSelectionCmd::execute()
		{
			auto applicationManager = IApplicationManager::instance();
			auto selectionManager = applicationManager->getSelectionManager();

			auto sceneManager = applicationManager->getSceneManager();
			auto scene = sceneManager->getCurrentScene();

			auto selection = selectionManager->getSelection();
			for (auto selected : selection)
			{
				auto selectionData = fb::make_ptr<ActorData>();

				//auto actor = fb::dynamic_pointer_cast<IActor>(selected);
				//if (actor)
				//{
				//	scene->removeActor(actor);
				//	auto parent = actor->getParent();
				//	if (parent)
				//	{
				//		parent->removeChild(actor);
				//		selectionData->setParent(parent);
				//	}

				//	auto data = actor->toData();
				//	if (data)
				//	{
				//		selectionData->setActorData(data);
				//	}

				//	actor->unload(0);
				//	selectionData->setActor(actor);

				//	m_actorData.push_back(selectionData);
				//}
			}

			auto editorManager = EditorManager::getSingletonPtr();
			auto uiManager = editorManager->getUI();
			uiManager->rebuildSceneTree();
		}

		RemoveSelectionCmd::ActorData::ActorData()
		{

		}

		RemoveSelectionCmd::ActorData::~ActorData()
		{

		}

		SmartPtr<IActor> RemoveSelectionCmd::ActorData::getParent() const
		{
			return m_parent;
		}

		void RemoveSelectionCmd::ActorData::setParent(SmartPtr<IActor> val)
		{
			m_parent = val;
		}

		SmartPtr<IActor> RemoveSelectionCmd::ActorData::getActor() const
		{
			return m_actor;
		}

		void RemoveSelectionCmd::ActorData::setActor(SmartPtr<IActor> val)
		{
			m_actor = val;
		}

		SmartPtr<IData> RemoveSelectionCmd::ActorData::getActorData() const
		{
			return m_actorData;
		}

		void RemoveSelectionCmd::ActorData::setActorData(SmartPtr<IData> val)
		{
			m_actorData = val;
		}



	} // end namespace editor
} // end namespace fb


