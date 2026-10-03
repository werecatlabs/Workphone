#include <GameEditorPCH.hpp>
#include <jobs/JobRendererSetup.hpp>
#include <FBState/Messages/StateMessage.hpp>
#include <FBCore/Interface/IApplicationManager.hpp>
#include <FBCore/Interface/Graphics/IGraphicsSystemConfiguration.hpp>
#include <FBCore/Interface/Graphics/ISceneManager.hpp>
#include <FBCore/Interface/System/IStateManager.hpp>
#include <FBCore/Memory/PointerUtil.hpp>
#include <wx/msgdlg.hpp>

#if FB_GRAPHICS_SYSTEM_OGRENEXT
#include <FBGraphicsOgreNext/FBGraphicsOgre.hpp>
#elif FB_GRAPHICS_SYSTEM_OGRE
#include <FBGraphicsOgre/FBGraphicsOgre.hpp>
#else
#include <FBGraphicsD3D11/FBGraphicsD3D11.hpp>
#endif



namespace fb
{
	namespace editor
	{


		JobRendererSetup::JobRendererSetup()
		{

		}

		JobRendererSetup::~JobRendererSetup()
		{

		}

		void JobRendererSetup::execute()
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);
			
			auto graphicsSystem = applicationManager->getGraphicsSystem();
			FB_ASSERT(graphicsSystem);

			graphicsSystem->load(nullptr);
			
			auto configuration = graphicsSystem->createConfiguration();
			FB_ASSERT(configuration);

			//setupResources();

			configuration->setCreateWindow(false);
			graphicsSystem->configure(configuration);

			//graphicsSystem->loadResources();

			//chooseSceneManager();

			//Ogre::ResourceGroupManager::getSingletonPtr()->initialiseAllResourceGroups();
			//Ogre::MaterialManager::getSingletonPtr()->initialise();

			static const auto RENDERER_READY_HASH = StringUtil::getHash("RendererReady");
			auto stateManager = applicationManager->getStateManager();
			
			auto stateMessage = fb::make_ptr<StateMessage>();
			stateMessage->setType(RENDERER_READY_HASH);

			stateManager->sendMessage(Thread::Task::Render, stateMessage);
		}

		void JobRendererSetup::chooseSceneManager()
		{
			try
			{
				auto applicationManager = IApplicationManager::instance();
				auto graphicsSystem = applicationManager->getGraphicsSystem();

				auto smgr = graphicsSystem->addSceneManager("DefaultSceneManager", "ViewSM");
				smgr->setAmbientLight(ColourF::White);

				//SmartPtr<IWindow> window = graphicsSystem->getDefaultWindow();
				//if ( window )
				//{
				//	SmartPtr<ICamera> camera = smgr->addCamera("DefaultWindowCamera");
				//	SmartPtr<ISceneNode> cameraNode = smgr->getRootSceneNode()->addChildSceneNode();
				//	cameraNode->attachObject(camera);
				//	cameraNode->setPosition(Vector3F::UNIT_Z * 250.0f);
				//	SmartPtr<IViewport> viewport = window->addViewport(0, camera);
				//	viewport->setBackgroundColour(ColourF(0.02, 0.85, 0.75, 1.0));
				//}

				//SmartPtr<render::ISceneManager> particleSceneManager = graphicsSystem->addSceneManager("DefaultSceneManager", "ParticleSceneManager");
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
				wxMessageBox(e.what());
			}
		}

	} // end namespace editor	
} // end namespace fb

