#include <EditorPCH.hpp>
#include <commands/AddNewScriptCmd.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <Workphone/Workphone.hpp>
#include <cstdio>
#include <fstream>

using std::endl;
using std::ofstream;

namespace workphone::editor
{

    WP_CLASS_REGISTER_DERIVED( workphone::editor, AddNewScriptCmd, Command );

    AddNewScriptCmd::AddNewScriptCmd() = default;

    AddNewScriptCmd::~AddNewScriptCmd() = default;

    AddNewScriptCmd::AddNewScriptCmd( const Properties &properties )
    {
        m_properties = properties;

        m_properties.getPropertyValue( "filePath", m_filePath );
        m_properties.getPropertyValue( "fileName", m_fileName );
    }

    void AddNewScriptCmd::redo()
    {
        execute();
    }

    void AddNewScriptCmd::execute()
    {
        // auto filePath = m_filePath + String("/") + m_fileName;

        // std::ofstream scriptStream;
        // scriptStream.open( m_filePath.c_str() );
        // scriptStream.close();

        // EditorManager* appRoot = EditorManager::getSingletonPtr();
        // SmartPtr<Project> project = appRoot->getProject();

        // m_scriptTemplate = SmartPtr<ScriptTemplate>(new ScriptTemplate);
        // m_scriptTemplate->setProperties(m_properties);

        // auto applicationManager = core::IApplicationManager::instance();
        // auto& fileSystem = applicationManager->getFileSystem();
        // auto stream = fileSystem->open(m_scriptTemplate->getFileName(),
        // m_scriptTemplate->getFilePath()); if(stream)
        //{
        //	String data = stream->getAsString();
        //	m_scriptTemplate->setData(data);
        // }

        // project->addScriptTemplate(m_scriptTemplate);

        ScriptGenerator scriptGenerator;

        auto path = getPath();
        auto fileName = getFileName();
        scriptGenerator.createScript( LanguageType::LUA, path );

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto fileSystem = applicationManager ? applicationManager->getFileSystemPtr() : nullptr )
        {
            fileSystem->refreshPath( Path::getFilePath( path ), true );
        }
    }

    void AddNewScriptCmd::undo()
    {
        auto filePath = getPath();
        if( StringUtil::isNullOrEmpty( filePath ) )
        {
            m_properties.getPropertyValue( "filePath", filePath );
        }

        if( remove( filePath.c_str() ) != 0 )
        {
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto fileSystem = applicationManager ? applicationManager->getFileSystemPtr() : nullptr )
        {
            fileSystem->refreshPath( Path::getFilePath( filePath ), true );
        }
    }

    String AddNewScriptCmd::getPath() const
    {
        return m_filePath;
    }

    void AddNewScriptCmd::setPath( const String &filePath )
    {
        m_filePath = filePath;
    }

    String AddNewScriptCmd::getFileName() const
    {
        return m_fileName;
    }

    void AddNewScriptCmd::setFileName( const String &fileName )
    {
        m_fileName = fileName;
    }
}  // namespace workphone::editor
