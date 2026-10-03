#include <GameEditorPCH.hpp>
#include <ui/UIManager.hpp>
#include <editor/FileSelection.hpp>
#include <editor/EditorManager.hpp>
#include <ui/ActorWindow.hpp>
#include <ui/ApplicationFrame.hpp>
#include <ui/ProjectWindow.hpp>
#include <ui/TerrainWindow.hpp>
#include <ui/SceneWindow.hpp>
#include <ui/PropertiesWindow.hpp>
#include <ui/MeshImportWindow.hpp>
#include <ui/FoliageWindow.hpp>
#include <ui/RoadFrame.hpp>
#include <ui/RenderWindow.hpp>
#include <ui/MaterialWindow.hpp>
#include <ui/TextureWindow.hpp>
#include <ui/ObjectWindow.hpp>
#include <FBApplication/FBApplicationHeaders.hpp>
#include <FBCore/FBCoreHeaders.hpp>
#include <wx/aui/framemanager.hpp>
#include <wx/aui/dockart.hpp>
#include <wx/aui/auibook.hpp>
#include <wx/aui/aui.hpp>



namespace fb
{	
	namespace editor
	{
	


		//--------------------------------------------
		UIManager::UIManager()
		{
		}
	


		//--------------------------------------------
		UIManager::~UIManager()
		{	
		}



		//--------------------------------------------
		void UIManager::update(time_interval t, time_interval dt)
		{
			if (m_projectWindow)
			{
				m_projectWindow->update(t, dt);
			}
		}



		//--------------------------------------------
		String UIManager::saveEntity(const String& fileName)
		{
			StringW defaultFileName = StringUtil::getWString(fileName);

			wxFileDialog dialog(m_appFrame,
				_T("Save Entity"),
				wxEmptyString,
				defaultFileName.c_str(),
				_T("Entity (*.entity)|*.entity"),
				wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
	
			dialog.SetFilterIndex(0);
	
			if (dialog.ShowModal() == wxID_OK)
			{
				return String(dialog.GetPath().c_str());
			}

			return StringUtil::EmptyString;
		}
	


		//--------------------------------------------
		String UIManager::saveScript(const String& fileName)
		{
			StringW defaultFileName = StringUtil::getWString(fileName);

			wxFileDialog dialog(m_appFrame,
				_T("Save Script"),
				wxEmptyString,
				defaultFileName.c_str(),
				_T("Lua Script (*.lua)|*.lua"),
				wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
	
			dialog.SetFilterIndex(0);
	
			if (dialog.ShowModal() == wxID_OK)
			{
				return String(dialog.GetPath().c_str());
			}

			return StringUtil::EmptyString;
		}



		//--------------------------------------------
		void UIManager::rebuildSceneTree()
		{
			try
			{
				auto sceneWindow = getSceneWindow();
				if(sceneWindow)
				{
					sceneWindow->saveTreeState();
					sceneWindow->buildTree();
					sceneWindow->restoreTreeState();
				}
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
			}
		}



		//--------------------------------------------
		void UIManager::rebuildActorTree()
		{
			try
			{
				auto actorWindow = getActorWindow();
				if (actorWindow)
				{
					actorWindow->buildTree();
				}
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
			}
		}



		//--------------------------------------------
		SceneWindow* UIManager::getSceneWindow() const
		{
			return m_sceneWindow;
		}



		//--------------------------------------------
		void UIManager::setSceneWindow(SceneWindow* val)
		{
			m_sceneWindow = val;
		}



		//--------------------------------------------
		ActorWindow* UIManager::getActorWindow() const
		{
			return m_actorWindow;
		}



		//--------------------------------------------
		void UIManager::updateSelection()
		{
			auto actorWindow = getActorWindow();
			if (actorWindow)
			{
				actorWindow->updateSelection();
			}

			auto propertiesWindow = getPropertiesWindow();
			if (propertiesWindow)
			{
				propertiesWindow->updateSelection();
			}

			showComponentEditWindow();;
		}



		//--------------------------------------------
		void UIManager::updateActorSelection()
		{
			auto propertiesWindow = getPropertiesWindow();
			FB_ASSERT(propertiesWindow);

			if (propertiesWindow)
			{
				propertiesWindow->updateSelection();
			}
		}



		//--------------------------------------------
		void UIManager::updateComponentSelection()
		{
			auto propertiesWindow = getPropertiesWindow();
			FB_ASSERT(propertiesWindow);

			if (propertiesWindow)
			{
				propertiesWindow->updateSelection();
			}

			auto applicationManager = IApplicationManager::instance();
			auto selectionManager = applicationManager->getSelectionManager();

			auto selection = selectionManager->getSelection();
			for (auto object : selection)
			{
				if (object)
				{
					//auto actor = fb::dynamic_pointer_cast<IActor>(object);
					//auto tranform = fb::dynamic_pointer_cast<ITransform>(object);
					//auto component = fb::dynamic_pointer_cast<component::IComponent>(object);
					//auto resource = fb::dynamic_pointer_cast<IResource>(object);

					//if (resource)
					//{
					//	int stop = 0;
					//	stop = 0;
					//}
				}
			}
		}



		//--------------------------------------------
		PropertiesWindow* UIManager::getPropertiesWindow() const
		{
			return m_propertiesWindow;
		}



		//--------------------------------------------
		void UIManager::setPropertiesWindow(PropertiesWindow* val)
		{
			m_propertiesWindow = val;
		}



		//--------------------------------------------
		ApplicationFrame* UIManager::getApplicationFrame() const
		{
			return m_appFrame;
		}
		


		//--------------------------------------------
		void UIManager::setApplicationFrame( ApplicationFrame* val )
		{
			m_appFrame = val;
		}



		//--------------------------------------------
		void UIManager::setActorWindow(ActorWindow* val)
		{
			m_actorWindow = val;
		}



		//--------------------------------------------
		TerrainWindow* UIManager::getTerrainWindow() const
		{
			return m_terrainWindow;
		}



		//--------------------------------------------
		void UIManager::setTerrainWindow( TerrainWindow* val )
		{
			m_terrainWindow = val;
		}



		//--------------------------------------------
		void UIManager::showComponentEditWindow()
		{

			auto applicationManager = IApplicationManager::instance();
			auto selectionManager = applicationManager->getSelectionManager();

			auto selection = selectionManager->getSelection();
			for (auto object : selection)
			{
				if (object)
				{
					auto actor = fb::dynamic_pointer_cast<IActor>(object);
					auto transform = fb::dynamic_pointer_cast<ITransform>(object);
					auto component = fb::dynamic_pointer_cast<component::IComponent>(object);
					auto resource = fb::dynamic_pointer_cast<IResource>(object);
					auto fileSelection = fb::dynamic_pointer_cast<FileSelection>(object);

					if (component)
					{
						if (component->isExactly<component::MaterialComponent>())
						{
							if (m_materialWindow)
							{
								m_materialWindow->show();
							}
						}
					}
					else if (fileSelection)
					{
						hideComponentEditWindows();

						auto filePath = fileSelection->getFilePath();
						auto ext = Path::getFileExtension(filePath);
						if (ext == ".fbx" || ext == ".FBX")
						{
							if (m_meshImportWindow)
							{
								m_meshImportWindow->show();
							}
						}
					}
				}
			}
		}



		//--------------------------------------------
		void UIManager::hideComponentEditWindows() const
		{
			if (m_terrainWindow)
			{
				m_terrainWindow->hide();
			}

			if (m_meshImportWindow)
			{
				m_meshImportWindow->hide();
			}

			if (m_foliageWindow)
			{
				m_foliageWindow->hide();
			}

			if (m_roadWindow)
			{
				m_roadWindow->hide();
			}

			if (m_textureWindow)
			{
				m_textureWindow->hide();
			}

			if (m_materialWindow)
			{
				m_materialWindow->hide();
			}
		}



		//--------------------------------------------
		wxAuiManager* UIManager::getAui() const
		{
			return m_aui;
		}



		//--------------------------------------------
		void UIManager::setAui(wxAuiManager* val)
		{
			m_aui = val;
		}



		//--------------------------------------------
		MeshImportWindow* UIManager::getMeshImportWindow() const
		{
			return m_meshImportWindow;
		}



		//--------------------------------------------
		void UIManager::setMeshImportWindow(MeshImportWindow* val)
		{
			m_meshImportWindow = val;
		}



		//--------------------------------------------
		FoliageWindow* UIManager::getFoliageWindow() const
		{
			return m_foliageWindow;
		}



		//--------------------------------------------
		void UIManager::setFoliageWindow(FoliageWindow* val)
		{
			m_foliageWindow = val;
		}



		//--------------------------------------------
		RoadFrame* UIManager::getRoadWindow() const
		{
			return m_roadWindow;
		}



		//--------------------------------------------
		void UIManager::setRoadWindow(RoadFrame* val)
		{
			m_roadWindow = val;
		}



		//--------------------------------------------
		HoudiniWindow* UIManager::getHoudiniWindow() const
		{
			return m_houdiniWindow;
		}



		//--------------------------------------------
		void UIManager::setHoudiniWindow(HoudiniWindow* val)
		{
			m_houdiniWindow = val;
		}




		//--------------------------------------------
		RenderWindow* UIManager::getRenderWindow() const
		{
			return m_renderWindow;
		}



		//--------------------------------------------
		void UIManager::setRenderWindow(RenderWindow* val)
		{
			m_renderWindow = val;
		}



		//--------------------------------------------
		RenderWindow* UIManager::getGameWindow() const
		{
			return m_gameWindow;
		}



		//--------------------------------------------
		void UIManager::setGameWindow(RenderWindow* val)
		{
			m_gameWindow = val;
		}



		//--------------------------------------------
		FileWindow* UIManager::getFileWindow() const
		{
			return m_fileWindow;
		}


		//--------------------------------------------
		void UIManager::setFileWindow(FileWindow* val)
		{
			m_fileWindow = val;
		}



		//--------------------------------------------
		ProjectWindow* UIManager::getProjectWindow() const
		{
			return m_projectWindow;
		}



		//--------------------------------------------
		void UIManager::setProjectWindow(ProjectWindow* val)
		{
			m_projectWindow = val;
		}



		//--------------------------------------------
		Array<ui::wxViewWindow*> UIManager::getWindows() const
		{
			return m_windows;
		}



		//--------------------------------------------
		void UIManager::setWindows(const Array<ui::wxViewWindow*>& val)
		{
			m_windows = val;
		}



		//--------------------------------------------
		void UIManager::addWindow(ui::wxViewWindow* window)
		{
			m_windows.push_back(window);
		}

		fb::editor::TextureWindow* UIManager::getTextureWindow() const
		{
			return m_textureWindow;
		}

		void UIManager::setTextureWindow(TextureWindow* textureWindow)
		{
			m_textureWindow = textureWindow;
		}

		MaterialWindow* UIManager::getMaterialWindow() const
		{
			return m_materialWindow;
		}

		void UIManager::setMaterialWindow(MaterialWindow* materialWindow)
		{
			m_materialWindow = materialWindow;
		}

		SmartPtr<ObjectWindow> UIManager::getObjectWindow() const
		{
			return m_objectWindow;
		}

		void UIManager::setObjectWindow(SmartPtr<ObjectWindow> objectWindow)
		{
			m_objectWindow = objectWindow;
		}


	} // end namespace editor
} // end namespace fb


