#ifndef __EditorManager_h__
#define __EditorManager_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Interface/System/IEditorManager.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Base/HashMap.hpp>



namespace fb
{
	namespace editor
	{



		//--------------------------------------------
		class EditorManager : public CSharedObject<IEditorManager>
		{
		public:
			EditorManager();
			~EditorManager();

			void unload(SmartPtr<ISharedObject> data) override;

			void update(time_interval t, time_interval dt);

			/** Import the assets. */
			void importAssets() override;

			/** Get the project path.
			@return A string with the project path.
			*/
			virtual String getProjectPath() const;

			/** Set the project path.
			@param path A string to the project path.
			*/
			virtual void setProjectPath(const String& path);

			/** Get the cache path.
			@return A string with the cache path.
			*/
			String getCachePath() const override;

			/** Set the cache path.
			@param path A string to the cache path.
			*/
			void setCachePath(const String& path) override;

			EditorScriptManagerPtr getScriptManager() const;
			void setScriptManager(EditorScriptManagerPtr val);

			MessageManagerPtr getMessageManager() const;
			void setMessageManager(MessageManagerPtr val);

			ProjectManagerPtr getProjectManager() const;
			void setProjectManager(ProjectManagerPtr val);

			ComponentTemplateMgrPtr getComponentTemplateMgr() const;
			void setComponentTemplateMgr(ComponentTemplateMgrPtr val);

			ProjectPtr getProject() const;
			void setProject(ProjectPtr val);

			//LuaEditConfigPtr getLuaEditConfig() const;
			//void setLuaEditConfig(LuaEditConfigPtr val);

			SmartPtr<UIManager> getUI() const;
			void setGUIManager(SmartPtr<UIManager> val);

			SmartPtr<render::IDecalCursor> getDecalCursor() const;
			void setDecalCursor(SmartPtr<render::IDecalCursor> val);

			SmartPtr<render::ITerrain> getTerrain() const;
			void setTerrain(SmartPtr<render::ITerrain> val);

			//TerrainManagerPtr getTerrainManager() const;
			//void setTerrainManager(TerrainManagerPtr val);

			//RiverManagerPtr getRiverManager() const;
			//void setRiverManager(RiverManagerPtr val);

			//FoliageManagerPtr getFoliageManager() const;
			//void setFoliageManager(FoliageManagerPtr val);

			SmartPtr<render::IWater> getWater() const;
			void setWater(SmartPtr<render::IWater> val);

			bool getEditTerrain() const;
			void setEditTerrain(bool val);

			bool getEditFoliage() const;
			void setEditFoliage(bool val);

			bool getFileSaved() const;
			void setFileSaved(bool val);

			void setCurrentSceneView(u32 sceneViewId);

			SceneViewManagerPtr getSceneViewManager() const;
			void setSceneViewManager(SceneViewManagerPtr val);

			SmartPtr<render::IParticleSystem> getParticleSystem() const;
			void setParticleSystem(SmartPtr<render::IParticleSystem> val);

			SmartPtr<MapTemplate> getSelectedMap() const;
			void setSelectedMap(SmartPtr<MapTemplate> val);

			SmartPtr<procedural::ICityGenerator>& getCityGenerator();
			const SmartPtr<procedural::ICityGenerator>& getCityGenerator() const;
			void setCityGenerator(SmartPtr<procedural::ICityGenerator> val);

			void previewAsset(const String& path);

			SmartPtr<TranslateManipulator> getTranslateManipulator() const;
			void setTranslateManipulator(SmartPtr<TranslateManipulator> val);

			SmartPtr<RotateManipulator> getRotateManipulator() const;
			void setRotateManipulator(SmartPtr<RotateManipulator> val);
			
			SmartPtr<ScaleManipulator> getScaleManipulator() const;
			void setScaleManipulator(SmartPtr<ScaleManipulator> val);

			static EditorManager* getSingletonPtr();

			void startPlaying();
			void pausePlaying();
			void stopPlaying();

		protected:
			/// A pointer to the singleton
			static EditorManager* m_singleton;

			/// The project path.
			String m_projectPath;

			/// The cache path.
			String m_cachePath;

			//RoadManagerPtr m_roadManager;

			/// 
			SmartPtr<render::IParticleSystem> m_particleSystem;

			/// 
			EditorScriptManagerPtr m_scriptManager;

			/// 
			MessageManagerPtr m_messageManager;

			/// 
			ProjectManagerPtr m_projectManager;

			/// 
			ComponentTemplateMgrPtr m_componentTemplateMgr;

			/// 
			ProjectPtr m_project;

			/// 
			//LuaEditConfigPtr m_luaEditConfig;

			/// 
			SmartPtr<UIManager> m_guiMananger;

			/// 
			SmartPtr<render::IDecalCursor> m_decalCursor;

			/// 
			SmartPtr<render::ITerrain> m_terrain;

			/// 
			//TerrainManagerPtr m_terrainManager;

			/// 
			SmartPtr<render::IWater> m_water;

			/// 
			//FoliageManagerPtr m_foliageManager;

			/// 
			//RiverManagerPtr m_riverManager;

			/// 
			SceneViewManagerPtr m_sceneViewManager;

			/// 
			SmartPtr<MapTemplate> m_selectedMap;

			/// 
			bool m_editTerrain;

			/// 
			bool m_editFoliage;

			/// 
			bool m_fileSaved;

			/// 
			atomic_bool m_enablePhysics;

			/// 
			EditorGrid* m_editorGrid = nullptr;

			SmartPtr<procedural::ICityGenerator> m_cityGenerator;

			/// the translate gizmo
			SmartPtr<TranslateManipulator> m_translateManipulator;

			/// the rotate gizmo
			SmartPtr<RotateManipulator> m_rotateManipulator;

			/// the scale gizmo
			SmartPtr<ScaleManipulator> m_scaleManipulator;
		};



	} // end namespace editor
} // end namespace fb



#endif // AppRoot_h__