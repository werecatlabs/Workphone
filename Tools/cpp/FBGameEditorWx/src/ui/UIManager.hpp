#ifndef __UIManager_h__
#define __UIManager_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>



namespace fb
{	
	namespace editor
	{



		//--------------------------------------------
		class UIManager : public CSharedObject<ISharedObject>
		{
		public:
			UIManager();
			~UIManager();

			void update(time_interval t, time_interval dt);

			/** Returns a file path. */
			String saveEntity(const String& fileName);

			/** Returns a file path. */
			String saveScript(const String& fileName);

			SceneWindow* getSceneWindow() const;
			void setSceneWindow(SceneWindow* val);

			ActorWindow* getActorWindow() const;
			void setActorWindow(ActorWindow* val);

			TerrainWindow* getTerrainWindow() const;
			void setTerrainWindow(TerrainWindow* val);

			PropertiesWindow* getPropertiesWindow() const;
			void setPropertiesWindow(PropertiesWindow* val);

			ApplicationFrame* getApplicationFrame() const;
			void setApplicationFrame(ApplicationFrame* val);
	
			void rebuildSceneTree();
			void rebuildActorTree();

			void updateSelection();
			void updateActorSelection();
			void updateComponentSelection();

			void showComponentEditWindow();
			void hideComponentEditWindows() const;

			wxAuiManager* getAui() const;
			void setAui(wxAuiManager* val);

			MeshImportWindow* getMeshImportWindow() const;
			void setMeshImportWindow(MeshImportWindow* val);

			FoliageWindow* getFoliageWindow() const;
			void setFoliageWindow(FoliageWindow* val);

			RoadFrame* getRoadWindow() const;
			void setRoadWindow(RoadFrame* val);

			HoudiniWindow* getHoudiniWindow() const;
			void setHoudiniWindow(HoudiniWindow* val);

			RenderWindow* getRenderWindow() const;
			void setRenderWindow(RenderWindow* val);

			RenderWindow* getGameWindow() const;
			void setGameWindow(RenderWindow* val);

			FileWindow* getFileWindow() const;
			void setFileWindow(FileWindow* val);

			ProjectWindow* getProjectWindow() const;
			void setProjectWindow(ProjectWindow* val);

			Array<ui::wxViewWindow*> getWindows() const;
			void setWindows(const Array<ui::wxViewWindow*>& val);
			void addWindow(ui::wxViewWindow* window);

			TextureWindow* getTextureWindow() const;
			void setTextureWindow(TextureWindow* textureWindow);

			MaterialWindow* getMaterialWindow() const;
			void setMaterialWindow(MaterialWindow* materialWindow);

			SmartPtr<ObjectWindow> getObjectWindow() const;
			void setObjectWindow(SmartPtr<ObjectWindow> objectWindow);

		protected:
			wxAuiManager* m_aui = nullptr;

			wxAuiNotebook* m_bookProject = nullptr;
			wxAuiNotebook* m_bookProps = nullptr;
			wxAuiNotebook* m_bookComponents = nullptr;
			wxAuiNotebook* m_bookMain = nullptr;
			wxAuiNotebook* m_bookAnimation = nullptr;
			wxAuiNotebook* m_bookFiles = nullptr;

			/// 
			RenderWindow* m_renderWindow = nullptr;

			RenderWindow* m_gameWindow = nullptr;

			FileWindow* m_fileWindow = nullptr;

			ActorWindow* m_actorWindow = nullptr;
			SceneWindow* m_sceneWindow = nullptr;
			TerrainWindow* m_terrainWindow = nullptr;
			ProjectWindow* m_projectWindow = nullptr;
			PropertiesWindow* m_propertiesWindow = nullptr;
			ApplicationFrame* m_appFrame = nullptr;
			MeshImportWindow* m_meshImportWindow = nullptr;
			FoliageWindow* m_foliageWindow = nullptr;
			RoadFrame* m_roadWindow = nullptr;
			HoudiniWindow* m_houdiniWindow = nullptr;
			TextureWindow* m_textureWindow = nullptr;
			MaterialWindow* m_materialWindow = nullptr;

			SmartPtr<ObjectWindow> m_objectWindow;

			/// 
			Array<ui::wxViewWindow*> m_windows;
		};
		
	
	
	} // end namespace editor
} // end namespace fb



#endif // GUIManager_h__


