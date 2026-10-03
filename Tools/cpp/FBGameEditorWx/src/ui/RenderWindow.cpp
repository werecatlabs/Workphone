#include <GameEditorPCH.hpp>
#include "RenderWindow.hpp"
#include <FBCore/Base/LogManager.hpp>
#include <FBCore/Interface/IApplicationManager.hpp>
#include <FBCore/Interface/Graphics/IGraphicsSystem.hpp>
#include <FBCore/Interface/Graphics/ISceneManager.hpp>
#include <editor/EditorManager.hpp>
#include <ui/UIManager.hpp>


namespace fb
{
	namespace editor
	{



		//---------------------------------------------------------------------
		RenderWindow::RenderWindow(wxWindow* parent, wxWindowID id)
			: wxScrolledWindow(parent, id)
		{
			try
			{
				auto baseSizer = new wxBoxSizer(wxHORIZONTAL);
				SetSizer(baseSizer);

				auto editorManager = EditorManager::getSingletonPtr();
				auto ui = editorManager->getUI();

				auto applicationManager = IApplicationManager::instance();
				auto graphicsSystem = applicationManager->getGraphicsSystem();

				auto sceneManagerName = String("ViewSM");
				auto sceneManager = graphicsSystem->getSceneManager(sceneManagerName);
				FB_ASSERT(sceneManager);

				auto viewWindow = new ui::wxViewWindow(this, 0, 0, 400, 400);
				ui->addWindow(viewWindow);
				m_viewWindow = viewWindow;
				baseSizer->Add(viewWindow, 1, wxEXPAND);

				m_window = viewWindow->getWindow();
				FB_ASSERT(m_window);

				if (m_window)
				{
					m_camera = sceneManager->addCamera("DefaultWindowCamera");
					FB_ASSERT(m_camera);

					m_camera->setNearClipDistance(0.0001f);
					m_camera->setFarClipDistance(10000.0f);

					m_cameraNode = sceneManager->getRootSceneNode()->addChildSceneNode();
					FB_ASSERT(m_cameraNode);

					m_cameraNode->attachObject(m_camera);
					//m_cameraNode->setPosition(Vector3F::UNIT_Z * 250.0f);

					m_viewport = m_window->addViewport(0, m_camera);
					FB_ASSERT(m_viewport);

					m_viewport->setBackgroundColour(ColourF(0.02, 0.85, 0.75, 1.0));

					m_window->setAutoUpdated(false);
				}
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
				wxMessageBox(e.what());
			}
		}



		//---------------------------------------------------------------------
		RenderWindow::~RenderWindow()
		{
		}


		//---------------------------------------------------------------------
		void RenderWindow::initialiseRenderWindows()
		{
			try
			{
				auto applicationManager = IApplicationManager::instance();
				auto editorManager = EditorManager::getSingletonPtr();
				auto uiManager = editorManager->getUI();

				auto graphicsSystem = applicationManager->getGraphicsSystem();
				auto sceneManager = graphicsSystem->getSceneManager(String("ViewSM"));

				auto viewWindow = new ui::wxViewWindow(this, 0, 0, 400, 400);
				uiManager->addWindow(viewWindow);
				m_viewWindow = viewWindow;
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
				wxMessageBox(e.what());
			}
		}



		ui::wxViewWindow* RenderWindow::getWxWindow() const
		{
			return m_viewWindow;
		}

		SmartPtr<render::IWindow> RenderWindow::getWindow() const
		{
			return m_window;
		}

		void RenderWindow::setCameraNode(SmartPtr<render::ISceneNode> val)
		{
			m_cameraNode = val;
		}

		SmartPtr<render::ISceneNode> RenderWindow::getCameraNode() const
		{
			return m_cameraNode;
		}

		void RenderWindow::setCamera(SmartPtr<render::ICamera> val)
		{
			m_camera = val;
		}

		SmartPtr<render::ICamera> RenderWindow::getCamera() const
		{
			return m_camera;
		}

		void RenderWindow::setWindow(SmartPtr<render::IWindow> val)
		{
			m_window = val;
		}




	} // end namespace editor
} // end namespace fb


