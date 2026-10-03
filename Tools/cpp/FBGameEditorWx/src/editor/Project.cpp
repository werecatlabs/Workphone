#include <GameEditorPCH.hpp>
#include <editor/Project.hpp>
#include <editor/EditorManager.hpp>
#include <FBApplication/Script/ScriptGenerator.hpp>
#include <FBApplication/Project/ProjectManager.hpp>
#include <ui/UIManager.hpp>
#include <FBCore/Base/DataUtil.hpp>
#include <FBCore/Memory/CData.hpp>
#include <FBData/FBData.hpp>
#include <FBData/XMLUtil.hpp>

#include <FBCore/Interface/IApplicationManager.hpp>
#include <FBCore/Interface/Actor/ISceneManager.hpp>
#include <FBCore/Interface/Actor/IScene.hpp>
#include <FBCore/Interface/IO/IFileSystem.hpp>
#include <FBCore/Interface/System/IProcessManager.hpp>
#include <fstream>
#include <wx/wx.hpp>
#include <tinyxml.hpp>

namespace fb
{
    namespace editor
    {

        const String Project::DEFAULT_PROJECT_NAME = "Project";
        const String Project::DEFAULT_MEDIA_PATH = "../Media/";
        const String Project::DEFAULT_SCRIPTS_PATH = "../Media/Scripts/";
        const String Project::DEFAULT_ENTITIES_PATH = "../Media/Scripts/Entities/";
        const String Project::DEFAULT_VERSION = "1.0.0";

        //--------------------------------------------
        Project::Project() :
            m_label( StringUtil::EmptyString ),
            m_projectDirectory( StringUtil::EmptyString ),
            m_projectPath( StringUtil::EmptyString ),
            m_applicationFilePath( StringUtil::EmptyString ),
            m_scriptsPath( StringUtil::EmptyString ),
            m_entitiesPath( StringUtil::EmptyString ),
            m_version( StringUtil::EmptyString )
        {
            try
            {
                m_label = DEFAULT_PROJECT_NAME;
                m_scriptsPath = DEFAULT_SCRIPTS_PATH;
                m_entitiesPath = DEFAULT_ENTITIES_PATH;
                m_version = DEFAULT_VERSION;

                setCameraManagerTemplate( SmartPtr<CameraManagerTemplate>( new CameraManagerTemplate ) );

                setResourcesetTemplate( SmartPtr<ResourceSetTemplate>( new ResourceSetTemplate ) );

                setApplicationTemplate( SmartPtr<ApplicationTemplate>( new ApplicationTemplate ) );
                addCoreComponents();

                addDefaultTasks();
                addDefaultScript();
            }
            catch( std::exception &e )
            {
                wxMessageBox( e.what() );
            }
        }

        //--------------------------------------------
        Project::~Project()
        {
        }

        //--------------------------------------------
        void Project::load( const String &filePath )
        {
            auto applicationManager = IApplicationManager::instance();
            auto fileSystem = applicationManager->getFileSystem();

            auto projectPath = StringUtil::cleanupPath( filePath );
            auto path = Path::getFilePath( projectPath );

            auto projectDirectory = Path::getFilePath( projectPath );
            projectDirectory = StringUtil::replaceAll( projectDirectory, "\\", "/" );
            setProjectDirectory( projectDirectory );

            try
            {
                auto pData = fb::make_ptr<CData<data::project_data>>();
                auto data = pData->getDataAsType<data::project_data>();

                auto dataStr = fileSystem->readAllText( projectPath );
                DataUtil::parse( dataStr, data );

                fromData( pData );

                compile();
            }
            catch( std::exception &e )
            {
                wxMessageBox( e.what() );
            }
            catch( ... )
            {
                wxMessageBox( "Unknown exception." );
            }
        }

        //--------------------------------------------
        void Project::save( const String &filePath )
        {
            try
            {
                FB_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );

                auto applicationManager = IApplicationManager::instance();
                FB_ASSERT( applicationManager );

                auto fileSystem = applicationManager->getFileSystem();
                FB_ASSERT( fileSystem );

                auto path = Path::getFilePath( filePath );
                setProjectDirectory( path );

                auto pData = toData();
                auto data = pData->getDataAsType<data::project_data>();
                auto dataStr = DataUtil::toString( data, true );

                fileSystem->writeAllText( filePath, dataStr );
            }
            catch( std::exception &e )
            {
                wxMessageBox( e.what() );
            }
            catch( ... )
            {
                wxMessageBox( "Unknown exception." );
            }
        }

        //--------------------------------------------
        String Project::getLabel() const
        {
            return m_label;
        }

        //--------------------------------------------
        void Project::setLabel( const String &val )
        {
            m_label = val;
        }

        //--------------------------------------------
        String Project::getProjectDirectory() const
        {
            return m_projectDirectory;
        }

        //--------------------------------------------
        void Project::setProjectDirectory( const String &val )
        {
            m_projectDirectory = val;
        }

        //--------------------------------------------
        String Project::getWorkingDirectory() const
        {
            return m_projectDirectory;
        }

        //--------------------------------------------
        void Project::setWorkingDirectory( const String &val )
        {
            m_projectDirectory = val;
        }

        //--------------------------------------------
        String Project::getApplicationFilePath() const
        {
            return m_applicationFilePath;
        }

        //--------------------------------------------
        void Project::setApplicationFilePath( const String &val )
        {
            m_applicationFilePath = val;
        }

        //--------------------------------------------
        SmartPtr<ApplicationTemplate> Project::getApplicationTemplate() const
        {
            return m_applicationTemplate;
        }

        //--------------------------------------------
        void Project::setApplicationTemplate( SmartPtr<ApplicationTemplate> val )
        {
            m_applicationTemplate = val;
        }

        //--------------------------------------------
        String Project::getEntitiesPath() const
        {
            return m_entitiesPath;
        }

        //--------------------------------------------
        void Project::setEntitiesPath( const String &val )
        {
            m_entitiesPath = val;
        }

        //--------------------------------------------
        Array<String> Project::getPaths() const
        {
            return m_paths;
        }

        //--------------------------------------------
        void Project::setPaths( Array<String> val )
        {
            m_paths = val;
        }

        //--------------------------------------------
        void Project::addPath( const String &val )
        {
            m_paths.push_back( val );
        }

        //--------------------------------------------
        void Project::saveProject( TiXmlElement *root )
        {
            // auto applicationManager = IApplicationManager::instance();
            // SmartPtr<IFileSystem>& fileSystem = engine->getFileSystem();

            // TiXmlElement* propertiesElem = new TiXmlElement("properties");
            // root->LinkEndChild(propertiesElem);

            // Properties propertyGroup;
            // getProperties(propertyGroup);
            // XMLUtil::writeProperties(propertyGroup, propertiesElem);

            // saveApplication( root );
            // saveEntities(root);
            //
            // Array<SmartPtr<TemplateFilter>> templateFilters = getFilters();
            // TemplateXMLWriter::saveTemplateFilters(templateFilters, root);

            // Array<SmartPtr<ScriptTemplate>> scriptTemplates = getScriptTemplates();

            //// make paths relative
            // String projectPath = getProjectDirectory();

            // for(u32 i=0; i<scriptTemplates.size(); ++i)
            //{
            //	SmartPtr<ScriptTemplate>& scriptTemplate = scriptTemplates[i];
            //	String filePath = scriptTemplate->getFilePath();

            //	if(StringUtil::isPathRelative(projectPath, filePath))
            //	{
            //		scriptTemplate->setFilePath(filePath);
            //	}
            //	else
            //	{
            //		String relativePath = Path::getRelativePath(projectPath, filePath);
            //		scriptTemplate->setFilePath(relativePath);
            //	}
            //}

            // TemplateXMLWriter::writeScriptResources( root, scriptTemplates );
        }

        //--------------------------------------------
        void Project::saveApplication( TiXmlElement *root )
        {
            // String appTemplateFileName = m_applicationTemplate->getAppName() + String(".application");
            // TemplateXMLWriter::save(m_projectDirectory + String("/") + appTemplateFileName,
            // m_applicationTemplate);
        }

        //--------------------------------------------
        void Project::saveEntities( TiXmlElement *root )
        {
            // TiXmlElement* entitiesElem = new TiXmlElement("entities");
            // root->LinkEndChild(entitiesElem);

            // String projectPath = getProjectDirectory();

            // Array<SmartPtr<EntityTemplate>> entityTemplates = getEntityTemplates();
            // for(u32 i=0; i<entityTemplates.size(); ++i)
            //{
            //	SmartPtr<EntityTemplate> entityTemplate = entityTemplates[i];

            //	String filePath = entityTemplate->getPath();

            //	if(StringUtil::isPathRelative(projectPath, filePath))
            //	{
            //		entityTemplate->setPath(filePath);
            //	}
            //	else
            //	{
            //		String relativePath = Path::getRelativePath(projectPath, filePath);
            //		entityTemplate->setPath(relativePath);
            //	}

            //	String fullTemplatePath = Path::getFullPath(projectPath, entityTemplate->getPath());
            //	TemplateXMLWriter::save(fullTemplatePath + String("/") + entityTemplate->getFileName(),
            // entityTemplate);

            //	TiXmlElement* entityElem = new TiXmlElement("entity");
            //	entitiesElem->LinkEndChild(entityElem);

            //	TiXmlElement* propertiesElem = new TiXmlElement("properties");
            //	entityElem->LinkEndChild(propertiesElem);

            //	Properties properties;
            //	entityTemplate->getProperties(properties);
            //	XMLUtil::writeProperties(properties, propertiesElem);
            //}
        }

        //--------------------------------------------
        void Project::saveGuiLayouts( TiXmlElement *root )
        {
            TiXmlElement *layoutsElem = new TiXmlElement( "layouts" );
            root->LinkEndChild( layoutsElem );

            Array<SmartPtr<GUITemplate>> guiTemplates = getGuiTemplates();
            for( u32 i = 0; i < guiTemplates.size(); ++i )
            {
                SmartPtr<GUITemplate> guiTemplate = guiTemplates[i];

                TiXmlElement *layoutElem = new TiXmlElement( "layout" );
                layoutsElem->LinkEndChild( layoutElem );

                TiXmlElement *propertiesElem = new TiXmlElement( "properties" );
                layoutElem->LinkEndChild( propertiesElem );

                Properties properties;
                guiTemplate->getProperties( properties );
                XMLUtil::writeProperties( properties, propertiesElem );
            }
        }

        //--------------------------------------------
        void Project::saveScripts()
        {
            using std::endl;
            using std::ofstream;

            Array<SmartPtr<ScriptTemplate>> scriptTemplates = m_applicationTemplate->getScripts();
            for( u32 i = 0; i < scriptTemplates.size(); ++i )
            {
                SmartPtr<ScriptTemplate> &scriptTemplate = scriptTemplates[i];
                String fileName = scriptTemplate->getFileName();
                String data = scriptTemplate->getData();

                ofstream stream;
                stream.open( ( m_projectDirectory + String( "/" ) + fileName ).c_str() );

                stream << data.c_str() << endl;
            }
        }

        //--------------------------------------------
        void Project::addEntityTemplate( SmartPtr<EntityTemplate> entityTemplate )
        {
            if( !isExistingEntity( entityTemplate->getName() ) )
                m_entityTemplates.push_back( entityTemplate );
            else
            {
                String text = "An entity with this name already exist - Entity name: ";
                text += entityTemplate->getName();

                wxMessageDialog *dial = new wxMessageDialog( NULL, text.c_str(), wxT( "Duplicate" ),
                                                             wxOK | wxICON_INFORMATION );
                dial->ShowModal();
            }
        }

        //--------------------------------------------
        bool Project::isExistingEntity( const String &val )
        {
            Array<SmartPtr<EntityTemplate>> entitytemplates = getEntityTemplates();
            for( u32 i = 0; i < entitytemplates.size(); ++i )
            {
                SmartPtr<EntityTemplate> entitytemplate = entitytemplates[i];

                if( entitytemplate->getName() == val )
                    return true;
            }

            return false;
        }

        //--------------------------------------------
        void Project::removeEntity( const String &val )
        {
            ////Array<SmartPtr<EntityTemplate>> entitytemplates = getEntityTemplates();
            // for(u32 i=0; i<m_entityTemplates.size(); ++i)
            //{
            //	SmartPtr<EntityTemplate> entitytemplate = m_entityTemplates[i];

            //	if(entitytemplate->getName() == val)
            //	{
            //		m_entityTemplates.erase_element_index(i);
            //		return;
            //	}
            //}
        }

        //--------------------------------------------
        SmartPtr<EntityTemplate> Project::getEntity( const String &val )
        {
            Array<SmartPtr<EntityTemplate>> entitytemplates = getEntityTemplates();
            for( u32 i = 0; i < entitytemplates.size(); ++i )
            {
                SmartPtr<EntityTemplate> entitytemplate = entitytemplates[i];

                if( entitytemplate->getName() == val )
                    return entitytemplate;
            }

            return nullptr;
        }

        //--------------------------------------------
        void Project::setProperties( const Properties &properties )
        {
            // properties.getPropertyValue("name", m_label);
            // properties.getPropertyValue("application", m_applicationFilePath);

            // m_mediaPaths.clear();

            // String value;
            // properties.getPropertyValue("resourcePaths", value);
            // StringUtil::parseArray(value, m_mediaPaths);
            // for(u32 i=0; i<m_mediaPaths.size(); ++i)
            //{
            //	String mediaPath = m_mediaPaths[i];

            //	if(!StringUtil::isPathRelative(m_projectDirectory, mediaPath))
            //		m_mediaPaths[i] = Path::getRelativePath( m_projectDirectory, mediaPath );
            //}

            // auto applicationManager = IApplicationManager::instance();
            // SmartPtr<IFileSystem> fileSystem = engine->getFileSystem();
            // for(u32 i=0; i<m_mediaPaths.size(); ++i)
            //{
            //	try
            //	{
            //		String fullPath = Path::getFullPath(m_projectDirectory, m_mediaPaths[i]);
            //		fileSystem->addFileArchive(fullPath, true, true, FB_ARCH_TYPE_FOLDER);
            //	}
            //	catch (std::exception& e)
            //	{
            //		LOG_MESSAGE("Application", "Error creating relative path.");
            //	}
            //	catch (...)
            //	{
            //		LOG_MESSAGE("Application", "Error creating relative path.");
            //	}
            // }

            // SmartPtr<IGraphicsSystem> gfxSystem = engine->getGraphicsSystem();
            // gfxSystem->loadResources();

            properties.getPropertyValue( "scriptsPath", m_scriptsPath );
            properties.getPropertyValue( "entitiesPath", m_entitiesPath );
            properties.getPropertyValue( "version", m_version );
        }

        //--------------------------------------------
        void Project::getProperties( Properties &properties ) const
        {
            properties.addProperty( Property( "name", m_label, "Name", "string" ) );
            properties.addProperty(
                Property( "application", m_applicationFilePath, "Application", "file" ) );

            String value = StringUtil::toString( m_mediaPaths );
            properties.addProperty( Property( "resourcePaths", value, "Resource Paths", "folders" ) );

            // properties.addProperty(Property("scriptsPath", m_scriptsPath, "Scripts Path", "string"));
            // properties.addProperty(Property("entitiesPath", m_entitiesPath, "Entities Path",
            // "string"));
            properties.addProperty( Property( "version", m_version, "Version", "string" ) );
        }

        //--------------------------------------------
        const String &Project::getEditableType() const
        {
            static String editableType = "Project";
            return editableType;
        }

        //--------------------------------------------
        void Project::rescanProject()
        {
            SmartPtr<IFileSystem> fileSystem = IApplicationManager::instance()->getFileSystem();

            // Array<String> applicationFileNames;
            // fileSystem->getFileNamesWithExtension(".application", applicationFileNames);
            // Set<String> uniqueApplicationFileNames = CoreUtil::createSet(applicationFileNames);
            // if(uniqueApplicationFileNames.size() > 1)
            //{
            //	MessageBoxUtil::show("More than one application found. Please select your application.");
            //	//todo add file dialog
            // }
            // else
            //{
            //	if(!uniqueApplicationFileNames.empty())
            //	{
            //		const String& fileName = uniqueApplicationFileNames[0];
            //		TemplateXMLLoader::loadApplication(fileName, m_applicationTemplate);
            //	}
            // }

            // Array<String> entityFileNames;
            // fileSystem->getFileNamesWithExtension(".entity", entityFileNames);
            // Set<String> uniqueEntityFileNames = CoreUtil::createSet(entityFileNames);
            // for(u32 i=0; i<uniqueEntityFileNames.size(); ++i)
            //{
            //	const String& fileName = uniqueEntityFileNames[i];

            //	SmartPtr<EntityTemplate> entityTemplate(new EntityTemplate);
            //	TemplateXMLLoader loader;
            //	loader.load(fileName, entityTemplate);
            //	addEntityTemplate(entityTemplate);

            //	String fileDir = fileSystem->getFileDir(fileName);
            //	String relPath = Path::getRelativePath(m_projectDirectory, fileDir);

            //	String filePath = Path::getFilePath(relPath);
            //	entityTemplate->setPath(filePath);
            //	entityTemplate->setFileName(fileName);
            //}

            // Array<String> guiFileNames;
            // fileSystem->getFileNamesWithExtension(".gui", guiFileNames);
            // Set<String> uniqueGUIFileNames = CoreUtil::createSet(guiFileNames);
            // for(u32 i=0; i<uniqueGUIFileNames.size(); ++i)
            //{
            //	const String& fileName = uniqueGUIFileNames[i];

            //	SmartPtr<GUITemplate> guiTemplate(new GUITemplate);
            //	TemplateXMLLoader loader;
            //	loader.loadGUI(fileName, guiTemplate);
            //	addGuiTemplate(guiTemplate);

            //	String fileDir = fileSystem->getFileDir(fileName);
            //	String relPath = Path::getRelativePath(m_projectDirectory, fileDir);
            //	guiTemplate->setFilePath(relPath);
            //}

            addCoreComponents();
        }

        //--------------------------------------------
        void Project::addCoreComponents()
        {
            // Array<SmartPtr<ComponentTemplate>> components =
            // ApplicationManager::getSingletonPtr()->getComponentTemplateMgr()->getComponents(); for(u32
            // i=0; i<components.size(); ++i)
            //{
            //	SmartPtr<ComponentTemplate> componentTemplate = components[i]->clone();
            //	if(componentTemplate->hasTag("system"))
            //	{
            //		componentTemplate->setName(componentTemplate->getType());
            //		m_applicationTemplate->getComponents()->addComponent(componentTemplate);
            //	}
            // }
        }

        //--------------------------------------------
        void Project::addDefaultTasks()
        {
            // add default tasks
            TaskTemplatePtr gfxTask( new TaskTemplate );
            gfxTask->setLabel( "Render" );
            m_applicationTemplate->addTask( gfxTask );

            TaskTemplatePtr gameLogicTask( new TaskTemplate );
            gameLogicTask->setLabel( "GameLogic" );
            m_applicationTemplate->addTask( gameLogicTask );

            TaskTemplatePtr physicsTask( new TaskTemplate );
            physicsTask->setLabel( "Physics" );
            m_applicationTemplate->addTask( physicsTask );

            TaskTemplatePtr aiTask( new TaskTemplate );
            aiTask->setLabel( "Ai" );
            m_applicationTemplate->addTask( aiTask );

            TaskTemplatePtr sceneCull( new TaskTemplate );
            sceneCull->setLabel( "SceneCull" );
            m_applicationTemplate->addTask( sceneCull );

            TaskTemplatePtr sceneSort( new TaskTemplate );
            sceneSort->setLabel( "SceneSort" );
            m_applicationTemplate->addTask( sceneSort );
        }

        //--------------------------------------------
        void Project::addDefaultScript()
        {
            SmartPtr<ScriptTemplate> scriptTemplate( new ScriptTemplate );
            scriptTemplate->setFileName( "App.lua" );

            ScriptGenerator scriptGenerator;
            String data;  // = scriptGenerator.createScript(m_applicationTemplate, scriptTemplate);
            scriptTemplate->setData( data );

            m_applicationTemplate->addScript( scriptTemplate );
        }

        //--------------------------------------------
        SmartPtr<ScriptTemplate> Project::getScriptTemplate( const String &name ) const
        {
            for( u32 i = 0; i < m_scriptTemplates.size(); ++i )
            {
                SmartPtr<ScriptTemplate> scriptTemplate = m_scriptTemplates[i];
                if( scriptTemplate->getFileName() == ( name ) )
                {
                    return scriptTemplate;
                }
            }

            return nullptr;
        }

        //--------------------------------------------
        Array<SmartPtr<ScriptTemplate>> Project::getScriptTemplates() const
        {
            return m_scriptTemplates;
        }

        //--------------------------------------------
        void Project::setScriptTemplates( const Array<SmartPtr<ScriptTemplate>> &val )
        {
            m_scriptTemplates = val;
        }

        //--------------------------------------------
        void Project::addScriptTemplate( SmartPtr<ScriptTemplate> scriptTemplate )
        {
            m_scriptTemplates.push_back( scriptTemplate );
        }

        //--------------------------------------------
        bool Project::removeScriptTemplate( SmartPtr<ScriptTemplate> scriptTemplate )
        {
            // return m_scriptTemplates.erase_element(scriptTemplate);
            return false;
        }

        //--------------------------------------------
        void Project::addFilter( SmartPtr<TemplateFilter> filter )
        {
            auto it = m_templateFilters.find( filter->getName() );
            if( it == m_templateFilters.end() )
            {
                m_templateFilters[filter->getName()] = filter;
            }
        }

        //--------------------------------------------
        void Project::removeFilter( SmartPtr<TemplateFilter> filter )
        {
            m_templateFilters.erase( filter->getName() );
        }

        //--------------------------------------------
        Array<SmartPtr<TemplateFilter>> Project::getFilters() const
        {
            Array<SmartPtr<TemplateFilter>> templates;

            auto it = m_templateFilters.begin();
            for( ; it != m_templateFilters.end(); ++it )
            {
                templates.push_back( it->second );
            }

            return templates;
        }

        //--------------------------------------------
        SmartPtr<TemplateFilter> Project::findFilter( const String &name ) const
        {
            auto it = m_templateFilters.find( name );
            if( it != m_templateFilters.end() )
            {
                return it->second;
            }

            return nullptr;
        }

        //--------------------------------------------
        Array<SmartPtr<SceneTemplate>> Project::getSceneTemplates() const
        {
            return m_sceneTemplates;
        }

        /*
        Array<SmartPtr<SceneTemplate>> Project::getSceneTemplates() const
        {
            Array<SmartPtr<SceneTemplate>> sceneTemplates;

            SceneTemplates::const_iterator it = m_sceneTemplates.begin();
            for(; it!=m_sceneTemplates.end(); ++it)
            {
                sceneTemplates.push_back(it->second);
            }

            return sceneTemplates;
        }

        */

        //--------------------------------------------
        void Project::setSceneTemplates( const Array<SmartPtr<SceneTemplate>> &val )
        {
            m_sceneTemplates = val;
        }

        //--------------------------------------------
        void Project::addSceneTemplate( SmartPtr<SceneTemplate> val )
        {
            m_sceneTemplates.push_back( val );
        }

        //--------------------------------------------
        SmartPtr<EntityTemplate> Project::getSelectedEntityTemplate() const
        {
            return m_selectedEntityTemplate;
        }

        //--------------------------------------------
        Array<SmartPtr<EntityTemplate>> Project::getEntityTemplates() const
        {
            return m_entityTemplates;
        }

        //--------------------------------------------
        void Project::setEntityTemplates( const Array<SmartPtr<EntityTemplate>> &val )
        {
            m_entityTemplates = val;
        }

        //--------------------------------------------
        Array<SmartPtr<GUITemplate>> Project::getGuiTemplates() const
        {
            return m_guiTemplates;
        }

        //--------------------------------------------
        void Project::setGuiTemplates( const Array<SmartPtr<GUITemplate>> &val )
        {
            m_guiTemplates = val;
        }

        //--------------------------------------------
        void Project::addGuiTemplate( SmartPtr<GUITemplate> val )
        {
            m_guiTemplates.push_back( val );
        }

        //--------------------------------------------
        void Project::setSelectedEntityTemplate( SmartPtr<EntityTemplate> val )
        {
            m_selectedEntityTemplate = val;
        }

        //--------------------------------------------
        SmartPtr<ResourceSetTemplate> Project::getResourceSetTemplate() const
        {
            return m_resourcesetTemplate;
        }

        //--------------------------------------------
        void Project::setResourcesetTemplate( SmartPtr<ResourceSetTemplate> val )
        {
            m_resourcesetTemplate = val;
        }

        //--------------------------------------------
        Array<SmartPtr<VehicleTemplate>> Project::getVehicles() const
        {
            return m_vehicles;
        }

        //--------------------------------------------
        void Project::setVehicles( const Array<SmartPtr<VehicleTemplate>> &val )
        {
            m_vehicles = val;
        }

        //--------------------------------------------
        void Project::addVehicle( SmartPtr<VehicleTemplate> val )
        {
            m_vehicles.push_back( val );
        }

        //--------------------------------------------
        void Project::addMapTempate( SmartPtr<MapTemplate> mapTemplate )
        {
            m_mapTemplates[mapTemplate->getName()] = mapTemplate;
        }

        //--------------------------------------------
        Array<SmartPtr<MapTemplate>> Project::getMapTemplates() const
        {
            Array<SmartPtr<MapTemplate>> mapTemplates;
            auto it = m_mapTemplates.begin();
            for( ; it != m_mapTemplates.end(); ++it )
            {
                mapTemplates.push_back( it->second );
            }

            return mapTemplates;
        }

        //--------------------------------------------
        Array<SmartPtr<MaterialTemplate>> Project::getMaterials() const
        {
            return m_materials;
        }

        //--------------------------------------------
        void Project::setMaterials( const Array<SmartPtr<MaterialTemplate>> &val )
        {
            m_materials = val;
        }

        //--------------------------------------------
        SmartPtr<CameraManagerTemplate> Project::getCameraManagerTemplate() const
        {
            return m_cameraManagerTemplate;
        }

        //--------------------------------------------
        void Project::setCameraManagerTemplate( SmartPtr<CameraManagerTemplate> val )
        {
            m_cameraManagerTemplate = val;
        }

        //--------------------------------------------
        void Project::addMaterialTemplate( SmartPtr<MaterialTemplate> materialTemplate )
        {
            m_materials.push_back( materialTemplate );
        }

        //--------------------------------------------
        void Project::setDirty( bool dirty )
        {
        }

        //--------------------------------------------
        String Project::getSelectedProjectPath() const
        {
            return m_selectedProjectPath;
        }

        //--------------------------------------------
        void Project::setSelectedProjectPath( const String &val )
        {
            m_selectedProjectPath = val;
        }

        //--------------------------------------------
        void Project::applyDefaults()
        {
        }

        //--------------------------------------------
        void Project::addEditableListener( IEditableListener *listener )
        {
        }

        //--------------------------------------------
        bool Project::removeEditableListener( IEditableListener *listener )
        {
            return false;
        }

        //--------------------------------------------
        SmartPtr<IEditableObject> Project::clone() const
        {
            return nullptr;
        }

        //--------------------------------------------
        SmartPtr<ISharedObject> Project::getOwner() const
        {
            return nullptr;
        }

        //--------------------------------------------
        void Project::getOwner( SmartPtr<ISharedObject> owner )
        {
        }

        //--------------------------------------------
        String Project::getCurrentScenePath() const
        {
            return m_currentScenePath;
        }

        //--------------------------------------------
        void Project::setCurrentScenePath( const String &val )
        {
            m_currentScenePath = val;
        }

        //--------------------------------------------
        SmartPtr<IData> Project::toData() const
        {
            auto pData = fb::make_ptr<CData<data::project_data>>();
            auto data = pData->getDataAsType<data::project_data>();

            data->projectVersion = "1.0.0";
            data->uuid = m_uuid;
            data->productName = m_productName;
            data->companyName = m_companyName;
            data->currentScenePath = getCurrentScenePath();

            return pData;
        }

        //--------------------------------------------
        void Project::fromData( SmartPtr<IData> data )
        {
            try
            {
                auto applicationManager = IApplicationManager::instance();
                auto editorManager = EditorManager::getSingletonPtr();
                auto uiManager = editorManager->getUI();

                auto projectData = data->getDataAsType<data::project_data>();

                auto currentScenePath = StringUtil::cleanupPath( projectData->currentScenePath );
                setCurrentScenePath( currentScenePath );

                auto sceneManager = applicationManager->getSceneManager();
                auto scene = sceneManager->getCurrentScene();
                if( scene )
                {
                    scene->clear();

                    if( !StringUtil::isNullOrEmpty( currentScenePath ) )
                    {
                        scene->loadScene( currentScenePath );
                    }
                }

                sceneManager->edit();
            }
            catch( std::exception &e )
            {
                FB_LOG_EXCEPTION( e );
            }
        }

        //--------------------------------------------
        void Project::compile()
        {
            auto applicationManager = IApplicationManager::instance();
            auto fileSystem = applicationManager->getFileSystem();
            auto processManager = applicationManager->getProcessManager();

            auto projectManager = SmartPtr<fb::ProjectManager>( new fb::ProjectManager );

            auto workingDirectory = Path::getWorkingDirectory();
            auto projectFolder = applicationManager->getProjectPath();
            auto cacheFolder = applicationManager->getCachePath();
            auto scriptProjectFolder = cacheFolder + "/project";
            auto projectName = Path::getLeaf( projectFolder );

            fileSystem->deleteFilesFromPath( scriptProjectFolder );

            projectManager->generateCMakeProject();

            fileSystem->createDirectories( scriptProjectFolder );
            Path::setWorkingDirectory( scriptProjectFolder );

            auto projectFolderW = StringUtil::toUTF8to16( projectFolder );

            Array<StringW> args;
            args.push_back( L"-T v142" );
            args.push_back( projectFolderW );

            processManager->shellExecute( L"cmake", args );

            auto buildCommandStr =
                L"\"C:/Program Files/Microsoft Visual "
                L"Studio/2022/Preview/Msbuild/Current/Bin/amd64/msbuild.exe\"";

            Array<StringW> buildArgs;
            buildArgs.push_back( StringUtil::toUTF8to16( projectName + ".sln" ) );

            processManager->shellExecute( buildCommandStr, buildArgs );

            Path::setWorkingDirectory( workingDirectory );

            applicationManager->setProjectLibraryName( projectName );

            auto numRetries = 0;
            auto dllPath = applicationManager->getProjectLibraryPath();
            while( !fileSystem->isExistingFile( dllPath ) && numRetries++ > 100 )
            {
                Thread::sleep( 1.0 );
            }

            auto scriptManager = applicationManager->getScriptManager();
            if( scriptManager )
            {
                scriptManager->reload( 0 );
            }
        }

        //--------------------------------------------
        bool Project::isDirty() const
        {
            return false;
        }

        //--------------------------------------------
        void Project::setMediaPaths( Array<String> val )
        {
            m_mediaPaths = val;
        }

        //--------------------------------------------
        Array<String> Project::getMediaPaths() const
        {
            return m_mediaPaths;
        }

        //--------------------------------------------
        void Project::setProjectPath( String val )
        {
            m_projectPath = val;
        }

        //--------------------------------------------
        String Project::getProjectFilePath() const
        {
            return m_projectPath;
        }

        //--------------------------------------------
        void Project::addParticleSystem( SmartPtr<ParticleSystemTemplate> val )
        {
            m_particleSystems.push_back( val );
        }

        //--------------------------------------------
        void Project::setParticleSystems( const Array<SmartPtr<ParticleSystemTemplate>> &val )
        {
            m_particleSystems = val;
        }

        //--------------------------------------------
        Array<SmartPtr<ParticleSystemTemplate>> Project::getParticleSystems() const
        {
            return m_particleSystems;
        }

    }  // end namespace editor
}  // end namespace fb
