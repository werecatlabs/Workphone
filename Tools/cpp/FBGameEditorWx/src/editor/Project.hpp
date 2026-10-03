#ifndef Project_h__
#define Project_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Interface/System/IEditableObject.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Base/Map.hpp>



namespace fb
{	
	namespace editor
	{
		
	
	
		//--------------------------------------------
		class Project : public CSharedObject<IEditableObject>
		{
		public:
			static const String DEFAULT_PROJECT_NAME;
			static const String DEFAULT_MEDIA_PATH;
			static const String DEFAULT_SCRIPTS_PATH;
			static const String DEFAULT_ENTITIES_PATH;
			static const String DEFAULT_VERSION;
	
			Project();	
			~Project();
	
			const String& getEditableType() const;
			
			void load(const String& filePath);
			void save(const String& filePath);
	
			String getLabel() const;
			void setLabel(const String& val);
	
			String getProjectDirectory() const;
			void setProjectDirectory(const String& val);

			String getProjectFilePath() const;
			void setProjectPath(String val);
	
			String getWorkingDirectory() const;
			void setWorkingDirectory(const String& val);
	
			String getApplicationFilePath() const;
			void setApplicationFilePath(const String& val);
	
			SmartPtr<ApplicationTemplate> getApplicationTemplate() const;
			void setApplicationTemplate(SmartPtr<ApplicationTemplate> val);
	
			String getEntitiesPath() const;
			void setEntitiesPath(const String& val);
	
			Array<String> getPaths() const;
			void setPaths(Array<String> val);
			void addPath(const String& val);
	
			Array<SmartPtr<EntityTemplate>> getEntityTemplates() const;
			void setEntityTemplates(const Array<SmartPtr<EntityTemplate>>& val);
			void addEntityTemplate(SmartPtr<EntityTemplate> entityTemplate);
			bool isExistingEntity(const String& val);
			SmartPtr<EntityTemplate> getEntity(const String& val);
			void removeEntity(const String& val);
	
			Array<SmartPtr<GUITemplate>> getGuiTemplates() const;
			void setGuiTemplates(const Array<SmartPtr<GUITemplate>>& val);
			void addGuiTemplate(SmartPtr<GUITemplate> val);
	
			SmartPtr<ScriptTemplate> getScriptTemplate(const String& name) const;
			Array<SmartPtr<ScriptTemplate>> getScriptTemplates() const;
			void setScriptTemplates(const Array<SmartPtr<ScriptTemplate>>& val);
			void addScriptTemplate(SmartPtr<ScriptTemplate> scriptTemplate);
			bool removeScriptTemplate(SmartPtr<ScriptTemplate> scriptTemplate);
	
			Array<SmartPtr<SceneTemplate>> getSceneTemplates() const;
			void setSceneTemplates(const Array<SmartPtr<SceneTemplate>>& val);
			void addSceneTemplate(SmartPtr<SceneTemplate> val);

			Array<SmartPtr<VehicleTemplate>> getVehicles() const;
			void setVehicles(const Array<SmartPtr<VehicleTemplate>>& val);
			void addVehicle(SmartPtr<VehicleTemplate> val);

			Array<SmartPtr<MaterialTemplate>> getMaterials() const;
			void setMaterials(const Array<SmartPtr<MaterialTemplate>>& val);
			void addMaterialTemplate(SmartPtr<MaterialTemplate> materialTemplate);

			Array<SmartPtr<ParticleSystemTemplate>> getParticleSystems() const;
			void setParticleSystems(const Array<SmartPtr<ParticleSystemTemplate>>& val);
			void addParticleSystem(SmartPtr<ParticleSystemTemplate> val);
	
			SmartPtr<EntityTemplate> getSelectedEntityTemplate() const;
			void setSelectedEntityTemplate(SmartPtr<EntityTemplate> val);

			SmartPtr<ResourceSetTemplate>  getResourceSetTemplate() const;
			void setResourcesetTemplate(SmartPtr<ResourceSetTemplate> val);
	
			void setProperties(const Properties& properties);
			void getProperties(Properties& properties) const;
	
			Array<String> getMediaPaths() const;
			void setMediaPaths(Array<String> val);
	
			void rescanProject();
	
			void addCoreComponents();
	
			void addFilter(SmartPtr<TemplateFilter> filter);
			void removeFilter(SmartPtr<TemplateFilter> filter);
			Array<SmartPtr<TemplateFilter>> getFilters() const;
			SmartPtr<TemplateFilter> findFilter(const String& name) const;

			void addMapTempate(SmartPtr<MapTemplate> mapTemplate);

			Array<SmartPtr<MapTemplate>> getMapTemplates() const;

			SmartPtr<CameraManagerTemplate> getCameraManagerTemplate() const;
			void setCameraManagerTemplate(SmartPtr<CameraManagerTemplate> val);

			bool isDirty() const;
			void setDirty(bool dirty);
	
			String getSelectedProjectPath() const;
			void setSelectedProjectPath(const String& val);

			void applyDefaults() override;

			void addEditableListener(IEditableListener* listener) override;
			bool removeEditableListener(IEditableListener* listener) override;

			SmartPtr<IEditableObject> clone() const override;

			SmartPtr<ISharedObject> getOwner() const override;
			void getOwner(SmartPtr<ISharedObject> owner) override;

			String getCurrentScenePath() const;
			void setCurrentScenePath(const String& val);

			/** Get object data as a structure. */
			SmartPtr<IData> toData() const;

			/** Set object data from a structure. */
			void fromData(SmartPtr<IData> data);

			void compile();

		protected:
			void addDefaultTasks();

			void saveProject( TiXmlElement* root );
	
			void addDefaultScript();
	
			void saveEntities( TiXmlElement* root );
			void saveApplication( TiXmlElement* root );
			void saveGuiLayouts( TiXmlElement* root );
			void saveScripts();

			SmartPtr<ResourceSetTemplate> m_resourcesetTemplate;

			SmartPtr<EntityTemplate> m_selectedEntityTemplate;

			SmartPtr<CameraManagerTemplate> m_cameraManagerTemplate;

			String m_currentScenePath;

			String m_uuid;

			String m_productName;
			String m_companyName;

			/// 
			String m_label;
	
			/// The working directory
			String m_projectDirectory;

			/// 
			String m_projectPath;

			String m_selectedProjectPath;

			/// 
			String m_applicationFilePath;
	
			/// The scripts path.
			String m_scriptsPath;
	
			/// 
			String m_entitiesPath;
	
			/// 
			String m_version;

			/// The media path.
			Array<String> m_mediaPaths;
	
			/// 
			Array<String> m_paths;
	
			SmartPtr<ApplicationTemplate> m_applicationTemplate;
	
			/// 
			Array<SmartPtr<EntityTemplate>> m_entityTemplates;
	
			/// 
			Array<SmartPtr<GUITemplate>>	m_guiTemplates;
	
			/// 
			Array<SmartPtr<ScriptTemplate>> m_scriptTemplates;
	
			/// 
			Array<SmartPtr<SceneTemplate>> m_sceneTemplates;

			/// 
			Array<SmartPtr<VehicleTemplate>> m_vehicles;

			/// 
			Array<SmartPtr<MaterialTemplate>> m_materials;

			/// 
			Array<SmartPtr<ParticleSystemTemplate>> m_particleSystems;

			Map<String, SmartPtr<TemplateFilter>> m_templateFilters;

			//typedef std::map<String, SmartPtr<SceneTemplate>> SceneTemplates;
			//SceneTemplates m_sceneTemplates;
			// 

			Map<String, SmartPtr<MapTemplate>> m_mapTemplates;
		};
	
	
	
	} // end namespace editor	
} // end namespace fb	



#endif // Project_h__