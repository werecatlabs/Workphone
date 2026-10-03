#include <GameEditorPCH.hpp>
#include <commands/AddNewScriptCmd.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <FBApplication/Script/ScriptGenerator.hpp>
#include <FBObjectTemplates/ScriptTemplate.hpp>
#include <FBCore/Interface/IApplicationManager.hpp>
#include <FBCore/FBCore.hpp>
#include <stdio.hpp>
#include <fstream>



using std::ofstream;
using std::endl;




namespace fb
{	
	namespace editor
	{

		

		//--------------------------------------------
		AddNewScriptCmd::AddNewScriptCmd()
		{

		}



		//--------------------------------------------
		AddNewScriptCmd::AddNewScriptCmd(const Properties& properties)
		{
			m_properties = properties;

			m_properties.getPropertyValue("filePath", m_filePath);
			m_properties.getPropertyValue("fileName", m_fileName);
		}



		//--------------------------------------------
		AddNewScriptCmd::~AddNewScriptCmd()
		{
		}



		//--------------------------------------------
		void AddNewScriptCmd::redo()
		{
		}



		//--------------------------------------------
		void AddNewScriptCmd::execute()
		{
			//auto filePath = m_filePath + String("/") + m_fileName;

			//std::ofstream scriptStream;
			//scriptStream.open( m_filePath.c_str() );
			//scriptStream.close();

			//EditorManager* appRoot = EditorManager::getSingletonPtr();
			//ProjectPtr project = appRoot->getProject();
		
			//m_scriptTemplate = SmartPtr<ScriptTemplate>(new ScriptTemplate);		
			//m_scriptTemplate->setProperties(m_properties);
	
			//auto applicationManager = IApplicationManager::instance();
			//auto& fileSystem = applicationManager->getFileSystem();
			//auto stream = fileSystem->open(m_scriptTemplate->getFileName(), m_scriptTemplate->getFilePath());
			//if(stream)
			//{
			//	String data = stream->getAsString();
			//	m_scriptTemplate->setData(data);
			//}

			//project->addScriptTemplate(m_scriptTemplate);

			ScriptGenerator scriptGenerator;

			auto path = getPath();
			auto fileName = getFileName();
			scriptGenerator.createCPlusPlusScript(path, fileName);
		}



		//--------------------------------------------
		void AddNewScriptCmd::undo()
		{
			String filePath;
			m_properties.getPropertyValue("filePath", filePath);

			if( remove( filePath.c_str() ) != 0 )
			{
				
			}  
		}



		//--------------------------------------------
		SmartPtr<ScriptTemplate> AddNewScriptCmd::getScriptTemplate() const
		{
			return m_scriptTemplate;
		}



		//--------------------------------------------
		void AddNewScriptCmd::setScriptTemplate( SmartPtr<ScriptTemplate> val )
		{
			m_scriptTemplate = val;
		}



		//--------------------------------------------
		String AddNewScriptCmd::getPath() const
		{
			return m_filePath;
		}



		//--------------------------------------------
		void AddNewScriptCmd::setPath( const String& val )
		{
			m_filePath = val;
		}



		//--------------------------------------------
		String AddNewScriptCmd::getFileName() const
		{
			return m_fileName;
		}



		//--------------------------------------------
		void AddNewScriptCmd::setFileName(const String& val)
		{
			m_fileName = val;
		}



	} // end namespace editor	
} // end namespace fb


