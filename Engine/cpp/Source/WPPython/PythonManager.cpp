#include <WPPython/PythonManager.hpp>
#include <WPPythonBind/Bindings/WPModule.hpp>
#include <WPPython/PythonObjectData.hpp>
#include <Workphone/Base/LogManager.hpp>
#include <Workphone/Base/Path.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/Interface/Script/IScriptBreakpoint.hpp>
#include <Workphone/Interface/Script/IScriptBind.hpp>
#include <pyconfig.hpp>
#include <boost/python.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/python/class.hpp>
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/filesystem.hpp>
#include <Python.hpp>

namespace fb
{
    namespace python = boost::python;

    struct std_string_to_python_str
    {
        static PyObject *convert( const std::string &s )
        {
            return python::incref( python::object( s.c_str() ).ptr() );
        }
    };

    PyObject *PyString_FromString( const String &str )
    {
        return python::incref( python::object( str.c_str() ).ptr() );
    }

    FB_CLASS_REGISTER_DERIVED( fb, PythonManager, CSharedObject<IScriptManager> );

    PythonManager::PythonManager()
    {
    }

    PythonManager::~PythonManager()
    {
        unload( nullptr );
    }

    void PythonManager::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            Py_VerboseFlag = 1;
            Py_OptimizeFlag = 1;
            Py_NoSiteFlag = 1;
            //Py_DontWriteBytecodeFlag = 1;

            initPythonBinding();

            //Py_Initialize();
            Py_InitializeEx( 0 );

            // now time to insert the current working directory into the python path so module search can take advantage
            // this must happen after python has been initialised
            boost::filesystem::path workingDir = boost::filesystem::complete( "./" ).normalize();
            PyObject *sysPath = PySys_GetObject( "path" );
            PyList_Insert( sysPath, 0, PyString_FromString( workingDir.string() ) );

            // initbinding
            //exec_test();
        }
        catch( std::exception &e )
        {
            FB_LOG_EXCEPTION( e );
        }
    }

    void PythonManager::reload( SmartPtr<ISharedObject> data )
    {
        try
        {
            unload( nullptr );
            load( nullptr );
        }
        catch( std::exception &e )
        {
            FB_LOG_EXCEPTION( e );
        }
    }

    void PythonManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            getScripts().clear();
            m_objectData.clear();
            m_creationList.clear();
            m_functions.clear();

            Py_Finalize();
        }
        catch( std::exception &e )
        {
            FB_LOG_EXCEPTION( e );
        }
    }

    void PythonManager::update()
    {
    }

    void PythonManager::loadScript( const String &filePath )
    {
        try
        {
            FB_LOCK_MUTEX( ScriptMutex );

            auto applicationManager = core::IApplicationManager::instance();
            FB_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            FB_ASSERT( fileSystem );

            if( !fileSystem )
            {
                FB_LOG( "No file system. " );
                return;
            }

            // Retrieve the main module
            python::object main = python::import( "__main__" );

            // Retrieve the main module's namespace
            python::object global( main.attr( "__dict__" ) );

            FileInfo fileInfo;
            if( fileSystem->findFileInfo( filePath, fileInfo, true ) )
            {
                auto stream = fileSystem->open( fileInfo.filePath, true, false, false, false, false );
                if( !stream )
                {
                    stream = fileSystem->open( fileInfo.filePath, true, false, false, true, true );
                }

                if( stream )
                {
                    auto dataStr = stream->getAsString();
                    exec( dataStr.c_str(), global, global );
                }

                FB_LOG( ( String( "Loaded file: " ) + filePath ).c_str() );
            }
        }
        catch( python::error_already_set & )
        {
            handleError();
        }
    }

    void PythonManager::loadScripts( const Array<String> &scripts )
    {
        for( auto &script : scripts )
        {
            loadScript( script );
        }

        setScripts( scripts );
    }

    void PythonManager::callFunction( const String &functionName )
    {
        FB_LOCK_MUTEX( ScriptMutex );

        try
        {
            auto main = python::import( "__main__" );
            auto global = python::object( main.attr( "__dict__" ) );
            python::object func = global[functionName.c_str()];
            boost::python::call<void>( func.ptr() );
        }
        catch( python::error_already_set & )
        {
            handleError();
        }
    }

    void PythonManager::callFunction( const String &functionName, const Parameters &parameters )
    {
        FB_LOCK_MUTEX( ScriptMutex );

        try
        {
            auto main = python::import( "__main__" );
            auto global = python::object( main.attr( "__dict__" ) );
            python::object func = global[functionName.c_str()];
            boost::python::call<void>( func.ptr(), parameters );
        }
        catch( python::error_already_set & )
        {
            handleError();
        }
    }

    void PythonManager::callFunction( const String &functionName, const Parameters &parameters,
                                      Parameters &results )
    {
    }

    s32 PythonManager::callMember( const String &className, const String &functionName )
    {
        return 0;
    }

    s32 PythonManager::callMember( const String &className, const String &functionName,
                                   const Parameters &parameters )
    {
        return 0;
    }

    s32 PythonManager::callMember( const String &className, const String &functionName,
                                   const Parameters &parameters, Parameters &results )
    {
        return 0;
    }

    void PythonManager::callObjectMember( SmartPtr<ISharedObject> object, const String &functionName )
    {
        FB_LOCK_MUTEX( ScriptMutex );

        int returnValue = 0;  //default value

        PythonObjectDataPtr data = object->getScriptData();
        if( data )
        {
            if( m_enableFullDebug )
            {
                m_curClass = data->getClassName();
                m_curFunction = functionName;
            }

            python::object &luaObject = data->getObject();
            if( !luaObject )
            {
                if( !createPythonInstance( data ) )
                {
                    auto msg = String( "Object not found: " );
                    FB_LOG( msg );

                    //errorObjectNotFound();

                    returnValue = 0;
                }
            }

            if( luaObject )
            {
                try
                {
                    boost::python::call_method<void>( luaObject.ptr(), functionName.c_str() );
                }
                catch( python::error_already_set & )
                {
                    handleError();
                    returnValue = 0;
                }
            }
        }
        else
        {
            auto msg = String( "Object not found: " );
            FB_LOG( msg );

            //errorObjectNotFound();

            returnValue = 0;
        }
    }

    void PythonManager::callObjectMember( SmartPtr<ISharedObject> object, const String &functionName,
                                          const Parameters &parameters )
    {
        FB_LOCK_MUTEX( ScriptMutex );

        int returnValue = 0;  //default value

        PythonObjectDataPtr data = object->getScriptData();
        if( data )
        {
            if( m_enableFullDebug )
            {
                m_curClass = data->getClassName();
                m_curFunction = functionName;
            }

            python::object &luaObject = data->getObject();
            if( !luaObject )
            {
                if( !createPythonInstance( data ) )
                {
                    auto msg = String( "Object not found: " );
                    FB_LOG( msg );

                    //errorObjectNotFound();

                    returnValue = 0;
                }
            }

            if( luaObject )
            {
                try
                {
                    boost::python::call_method<void>( luaObject.ptr(), functionName.c_str(),
                                                      parameters );
                }
                catch( python::error_already_set & )
                {
                    handleError();
                    returnValue = 0;
                }
            }
        }
        else
        {
            auto msg = String( "Object not found: " );
            FB_LOG( msg );

            //errorObjectNotFound();

            returnValue = 0;
        }
    }

    void PythonManager::callObjectMember( SmartPtr<ISharedObject> object, const String &functionName,
                                          const Parameters &parameters, Parameters &results )
    {
        FB_LOCK_MUTEX( ScriptMutex );

        int returnValue = 0;  //default value

        PythonObjectDataPtr data = object->getScriptData();
        if( data )
        {
            if( m_enableFullDebug )
            {
                m_curClass = data->getClassName();
                m_curFunction = functionName;
            }

            python::object &luaObject = data->getObject();
            if( !luaObject )
            {
                if( !createPythonInstance( data ) )
                {
                    auto msg = String( "Object not found: " );
                    FB_LOG( msg );

                    //errorObjectNotFound();

                    returnValue = 0;
                }
            }

            if( luaObject )
            {
                try
                {
                    boost::python::call_method<void>( luaObject.ptr(), functionName.c_str(), parameters,
                                                      boost::ref( results ) );
                }
                catch( python::error_already_set & )
                {
                    handleError();
                    returnValue = 0;
                }
            }
        }
        else
        {
            auto msg = String( "Object not found: " );
            FB_LOG( msg );

            //errorObjectNotFound();

            returnValue = 0;
        }
    }

    void PythonManager::createObject( const String &className, SmartPtr<ISharedObject> object )
    {
        FB_LOCK_MUTEX( ScriptMutex );

        try
        {
            Parameter objectParam;
            objectParam.setPtr( object.get() );

            PythonObjectDataPtr objectData = object->getScriptData();
            if( !objectData )
            {
                objectData = fb::make_ptr<PythonObjectData>();
                objectData->setClassName( className );
                objectData->setOwner( object.get() );
                object->setScriptData( objectData );

                m_objectData.push_back( objectData );

                if( m_delayedCreation )
                {
                    m_creationList.push_back( objectData );
                }
                else
                {
                    createPythonInstance( objectData );
                }
            }
            else
            {
                m_objectData.push_back( objectData );

                if( m_delayedCreation )
                {
                    m_creationList.push_back( objectData );
                }
                else
                {
                    createPythonInstance( objectData );
                }
            }
        }
        catch( std::exception &e )
        {
            String msg = String( "Error : " ) + String( e.what() );
            FB_LOG( msg );
        }
    }

    bool PythonManager::createPythonInstance( PythonObjectDataPtr objectData )
    {
        FB_LOCK_MUTEX( ScriptMutex );

        if( objectData )
        {
            auto className = objectData->getClassName();
            if( StringUtil::isNullOrEmpty( className ) )
            {
                return false;
            }

            try
            {
                python::object main = python::import( "__main__" );

                // Retrieve the main module's namespace
                python::object global( main.attr( "__dict__" ) );

                if( global )
                {
                    auto pythonClass = global[className.c_str()];

                    if( pythonClass )
                    {
                        auto &currentLuaObject = objectData->getObject();
                        if( !currentLuaObject )
                        {
                            auto owner = objectData->getOwner();
                            currentLuaObject = pythonClass();
                            return true;
                        }
                    }
                }

                return false;
            }
            catch( python::error_already_set & )
            {
                handleError();
            }
        }

        return false;
    }

    void *PythonManager::createInstance( const String &className )
    {
        try
        {
            FB_LOCK_MUTEX( ScriptMutex );

            if( StringUtil::isNullOrEmpty( className ) )
            {
                return nullptr;
            }

            python::object main = python::import( "__main__" );

            // Retrieve the main module's namespace
            python::object global( main.attr( "__dict__" ) );

            if( global )
            {
                auto pythonClass = global[className.c_str()];
                return new python::object( pythonClass() );
            }
        }
        catch( python::error_already_set & )
        {
            handleError();
        }

        return nullptr;
    }

    void PythonManager::destroyInstance( void *instance )
    {
    }

    void PythonManager::destroyObject( SmartPtr<ISharedObject> object )
    {
        if( object )
        {
            auto pScriptData = object->getScriptData();
            auto objectData = fb::static_pointer_cast<PythonObjectData>( pScriptData );
            if( objectData )
            {
                auto &rObject = objectData->getObject();
                if( rObject )
                {
                    *rObject = python::object();
                }
            }

            object->setScriptData( nullptr );
        }
    }

    void PythonManager::setTaskId( u32 taskId )
    {
    }

    u32 PythonManager::getTaskId() const
    {
        return static_cast<u32>( Thread::Task::Application );
    }

    void PythonManager::reloadScripts()
    {
        //Py_Finalize();
        setError( false );

        load( nullptr );

        for( size_t i = 0; i < m_scripts.size(); ++i )
        {
            loadScript( m_scripts[i] );
        }
    }

    String PythonManager::getDebugInfo()
    {
        return "error";  // todo workaround

        FB_LOCK_MUTEX( ScriptMutex );

        using namespace boost::python;
        using namespace boost;

        PyObject *exc, *exceptionValue, *tb;
        object formatted_list, formatted;
        PyErr_Fetch( &exc, &exceptionValue, &tb );
        handle<> hexc( exc ), hval( allow_null( exceptionValue ) ), htb( allow_null( tb ) );

        object traceback( import( "traceback" ) );
        if( !tb )
        {
            object format_exception_only( traceback.attr( "format_exception_only" ) );
            formatted_list = format_exception_only( hexc, hval );
        }
        else
        {
            if( auto format_exception = traceback.attr( "format_exception" ) )
            {
                formatted_list = format_exception( hexc, hval, htb );
            }
        }

        formatted = str( "\n" ).join( formatted_list );

        auto errorStr = extract<std::string>( formatted );
        return errorStr;
    }

    bool PythonManager::getDelayedCreation() const
    {
        return false;
    }

    void PythonManager::setDelayedCreation( bool delayedCreation )
    {
    }

    bool PythonManager::isDebugEnabled() const
    {
        return false;
    }

    void PythonManager::setDebugEnabled( bool enable )
    {
    }

    void PythonManager::removeBreakpoint( SmartPtr<IScriptBreakpoint> breakpoint )
    {
    }

    void PythonManager::addBreakpoint( SmartPtr<IScriptBreakpoint> breakpoint )
    {
    }

    Array<SmartPtr<IScriptBreakpoint>> PythonManager::getBreakpoints() const
    {
        return Array<SmartPtr<IScriptBreakpoint>>();
    }

    void PythonManager::_getObject( void **object )
    {
    }

    void PythonManager::garbageCollect()
    {
    }

    void PythonManager::registerClass( void *ptr )
    {
    }

    Array<String> PythonManager::getScripts() const
    {
        return m_scripts;
    }

    void PythonManager::setScripts( const Array<String> &scripts )
    {
        m_scripts = scripts;
    }

    bool PythonManager::getError() const
    {
        return m_error;
    }

    void PythonManager::setError( bool error )
    {
        m_error = error;
    }

    void PythonManager::handleError()
    {
        if( PyErr_Occurred() )
        {
            setError( true );

            auto msg = getDebugInfo();
            FB_LOG_ERROR( msg.c_str() );
        }

        PyErr_Clear();
    }

    void PythonManager::addScriptBinding( SmartPtr<IScriptBind> scriptBinding )
    {
    }

    void PythonManager::removeScriptBinding( SmartPtr<IScriptBind> scriptBinding )
    {
    }
}  // end namespace fb
