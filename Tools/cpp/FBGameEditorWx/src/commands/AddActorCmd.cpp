#include <GameEditorPCH.hpp>
#include <commands/AddActorCmd.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <editor/ProjectManager.hpp>
#include <ui/UIManager.hpp>
#include <FBObjectTemplates/EntityTemplate.hpp>
#include <FBObjectTemplates/EventTemplate.hpp>
#include <FBObjectTemplates/EventTemplateContainer.hpp>
#include <FBObjectTemplates/FSMTemplateContainer.hpp>
#include <FBApplication/Actor/CActor.hpp>
#include <FBApplication/Components/MaterialComponent.hpp>
#include <FBApplication/Components/ProceduralScene.hpp>
#include <FBApplication/Components/Road.hpp>
#include <FBApplication/Components/RoadNode.hpp>
#include <FBApplication/Components/TerrainRenderer.hpp>
#include <FBApplication/Components/CollisionTerrain.hpp>
#include <FBApplication/UI/Components/ButtonComponent.hpp>
#include <FBApplication/UI/Components/CanvasComponent.hpp>
#include <FBApplication/UI/Components/CanvasTransform.hpp>
#include <FBApplication/UI/Components/ImageComponent.hpp>
#include <FBApplication/UI/Components/TextComponent.hpp>



namespace fb
{
	namespace editor
	{



		//--------------------------------------------
		AddActorCmd::AddActorCmd()
		{
		}
		
		
		
		//--------------------------------------------
		AddActorCmd::~AddActorCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void AddActorCmd::undo()
		{
			auto applicationManager = IApplicationManager::instance();
			auto editorManager = EditorManager::getSingletonPtr();
			auto projectManager = editorManager->getProjectManager();
			auto uiManager = editorManager->getUI();

			auto sceneManager = applicationManager->getSceneManager();
			auto scene = sceneManager->getCurrentScene();
		
			auto actor = getActor();
			if (actor)
			{
				scene->unregisterAll(actor);
				scene->removeActor(actor);
			}

			uiManager->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void AddActorCmd::redo()
		{
			auto applicationManager = IApplicationManager::instance();
			auto editorManager = EditorManager::getSingletonPtr();
			auto projectManager = editorManager->getProjectManager();
			auto uiManager = editorManager->getUI();
			auto project = editorManager->getProject();

			auto sceneManager = applicationManager->getSceneManager();
			auto scene = sceneManager->getCurrentScene();

			auto actor = createActor();
			if (actor)
			{
				scene->addActor(actor);
				scene->registerAllUpdates(actor);
			}

			setActor(actor);

			uiManager->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void AddActorCmd::execute()
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto editorManager = EditorManager::getSingletonPtr();
			FB_ASSERT(editorManager);

			auto projectManager = editorManager->getProjectManager();
			FB_ASSERT(projectManager);

			auto uiManager = editorManager->getUI();
			FB_ASSERT(uiManager);

			auto project = editorManager->getProject();
			FB_ASSERT(project);

			auto selectionManager = applicationManager->getSelectionManager();
			FB_ASSERT(selectionManager);

			auto selection = selectionManager->getSelection();

			SmartPtr<IActor> parent;

			if (!selection.empty())
			{
				parent = selection.back();
			}

			auto sceneManager = applicationManager->getSceneManager();
			FB_ASSERT(sceneManager);

			auto scene = sceneManager->getCurrentScene();
			FB_ASSERT(scene);

			auto actor = createActor();
			FB_ASSERT(actor);

			if (actor)
			{
				if (parent)
				{
					parent->addChild(actor);

					auto root = parent->getSceneRoot();
					if (root)
					{
						root->hierarchyChanged();
					}
				}
				else
				{
					scene->addActor(actor);
					scene->registerUpdates(Thread::Task::Application, actor);
				}
			}

			setActor(actor);

			//if (applicationManager->isPlaying())
			//{
			//	actor->start();
			//}
			//else
			//{
			//	actor->edit();
			//}

			uiManager->rebuildSceneTree();
		}
		
		

		//--------------------------------------------
		SmartPtr<IActor> AddActorCmd::createActor()
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto sceneManager = applicationManager->getSceneManager();
			FB_ASSERT(sceneManager);

			auto actor = sceneManager->createActor();

			auto actorType = getActorType();
			switch (actorType)
			{
			case AddActorCmd::ActorType::Button:
			{
				auto name = String("Button");
				actor->setName(name);

				auto canvasTransform = actor->addComponent<component::CanvasTransform>();
				if (canvasTransform)
				{
					auto size = Vector2F(300, 100);
					canvasTransform->setSize(size);
				}

				actor->addComponent<component::ImageComponent>();
				actor->addComponent<component::ButtonComponent>();

				auto material = actor->addComponent<component::MaterialComponent>();
				if (material)
				{
					//material->setMainTexturePath("Rounded Filled 256px.png");
					//material->setTint(ColourF(0.0f, 0.5f, 0.0f, 1.0f));
				}

				auto actorText = sceneManager->createActor();
				actor->addChild(actorText);

				actorText->addComponent<component::CanvasTransform>();
				auto text = actorText->addComponent<component::TextComponent>();
				text->setText("Button");

				auto textName = String("Text");
				actorText->setName(textName);
			}
			break;
			case AddActorCmd::ActorType::Canvas:
			{
				auto name = String("Canvas");
				actor->setName(name);

				actor->addComponent<component::CanvasComponent>();
			}
			break;
			case AddActorCmd::ActorType::Panel:
			{
				auto name = String("Panel");
				actor->setName(name);

				auto canvasTransform = actor->addComponent<component::CanvasTransform>();
				if (canvasTransform)
				{
					auto size = Vector2F(1920, 1080);
					canvasTransform->setSize(size);
				}

				actor->addComponent<component::ImageComponent>();

				auto material = actor->addComponent<component::MaterialComponent>();
				if (material)
				{
					//material->setMainTexturePath("BumpyMetal.jpg");
					//material->setTint(ColourF(0.5f, 0.5f, 0.5f, 1.0f));
				}
			}
			break;
			case AddActorCmd::ActorType::ProceduralScene:
			{
				auto name = String("ProceduralScene");
				actor->setName(name);

				auto proceduralScene = actor->addComponent<component::ProceduralScene>();
				FB_ASSERT(proceduralScene);
			}
			break;
			case AddActorCmd::ActorType::Text:
			{
				auto name = String("Text");
				actor->setName(name);

				auto canvasTransform = actor->addComponent<component::CanvasTransform>();
				if (canvasTransform)
				{
					auto size = Vector2F(300, 100);
					canvasTransform->setSize(size);
				}

				actor->addComponent<component::TextComponent>();
			}
			break;
			case AddActorCmd::ActorType::Terrain:
			{
				auto name = String("Terrain");
				actor->setName(name);

				auto terrain = actor->addComponent<component::TerrainRenderer>();
				actor->addComponent<component::CollisionTerrain>();
			}
			break;
			default:
			{
				auto name = String("Actor");
				actor->setName(name);
			}
			};

			return actor;
		}



		//--------------------------------------------
		String AddActorCmd::getCommandId() const
		{
			return "AddEntityCmd";
		}
			


		//--------------------------------------------
		SmartPtr<IActor> AddActorCmd::getActor() const
		{
			return m_actor;
		}



		//--------------------------------------------
		void AddActorCmd::setActor(SmartPtr<IActor> val)
		{
			m_actor = val;
		}



		SmartPtr<IActor> AddActorCmd::getParent() const
		{
			return m_parent;
		}



		void AddActorCmd::setParent(SmartPtr<IActor> val)
		{
			m_parent = val;
		}


		AddActorCmd::ActorType AddActorCmd::getActorType() const
		{
			return m_actorType;
		}


		void AddActorCmd::setActorType(AddActorCmd::ActorType val)
		{
			m_actorType = val;
		}



	} // end namespace editor	
} // end namespace fb


