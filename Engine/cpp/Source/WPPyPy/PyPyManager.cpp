#include "WPPyPy/PyPyManager.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/python.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/python/class.hpp>
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>

namespace python = boost::python;

namespace fb
{

    PyPyManager::PyPyManager()
    {
    }

    PyPyManager::~PyPyManager()
    {
    }

    void PyPyManager::update( s32 task, time_interval t, time_interval dt )
    {
    }

    void PyPyManager::loadScript( const String &filePath )
    {
        FB_LOCK_MUTEX( ScriptMutex );

        python::object main;
        python::object global;
        python::object file;

        try
        {
            // Retrieve the main module
            main = python::import( "__main__" );

            // Retrieve the main module's namespace
            //global = python::object( main.attr( "__dict__" ) );

            file = python::exec_file( filePath.c_str(), global, global );

            //Ogre::LogManager* logMgr = Ogre::LogManager::getSingletonPtr();
            //Ogre::Log* scriptLog = logMgr->getLog(DEFAULT_SCRIPT_LOG_NAME);
            //scriptLog->logMessage(std::string("Loaded file: ") + filePath);
        }
        catch( boost::python::error_already_set & )
        {
            //if (PyErr_Occurred())
            {
                //std::string msg = handle_pyerror();
                //printf(msg.c_str());

                //Ogre::LogManager* logMgr = Ogre::LogManager::getSingletonPtr();
                //Ogre::Log* scriptLog = logMgr->getLog(DEFAULT_SCRIPT_LOG_NAME);
                //scriptLog->logMessage(msg, Ogre::LML_CRITICAL);
            }

            //PyErr_Clear();
        }
    }

    void PyPyManager::callFunction( const String &functionName )
    {
    }

    void PyPyManager::callFunction( const String &functionName, const Parameters &parameters )
    {
    }

    void PyPyManager::callFunction( const String &functionName, const Parameters &parameters,
                                    Parameters &results )
    {
    }

    s32 PyPyManager::callMember( const String &className, const String &functionName )
    {
        return 0;
    }

    s32 PyPyManager::callMember( const String &className, const String &functionName,
                                     const Parameters &parameters )
    {
        return 0;
    }

    s32 PyPyManager::callMember( const String &className, const String &functionName,
                                     const Parameters &parameters, Parameters &results )
    {
        return 0;
    }

    void PyPyManager::callObjectMember( IObject *object, const String &functionName )
    {
    }

    void PyPyManager::callObjectMember( IObject *object, const String &functionName,
                                        const Parameters &parameters )
    {
    }

    void PyPyManager::callObjectMember( IObject *object, const String &functionName,
                                        const Parameters &parameters, Parameters &results )
    {
    }

    void PyPyManager::createObject( const String &className, SmartPtr<IObject> object )
    {
    }

    void *PyPyManager::createInstance( const String &className )
    {
        return nullptr;
    }

    void PyPyManager::destroyInstance( void *instance )
    {
    }

    void PyPyManager::destroyObject( SmartPtr<IObject> object )
    {
    }

    void PyPyManager::setTaskId( u32 taskId )
    {
    }

    u32 PyPyManager::getTaskId() const
    {
        return 0;
    }

    void PyPyManager::reloadScripts()
    {
    }

    String PyPyManager::getDebugInfo()
    {
        return "";
    }

    bool PyPyManager::getDelayedCreation() const
    {
        return false;
    }

    void PyPyManager::setDelayedCreation( bool delayedCreation )
    {
    }

    bool PyPyManager::isDebugEnabled() const
    {
        return false;
    }

    void PyPyManager::setDebugEnabled( bool enable )
    {
    }

    void PyPyManager::removeBreakpoint( SmartPtr<IScriptBreakpoint> breakpoint )
    {
    }

    void PyPyManager::addBreakpoint( SmartPtr<IScriptBreakpoint> breakpoint )
    {
    }

    Array<SmartPtr<IScriptBreakpoint>> PyPyManager::getBreakpoints() const
    {
        return Array<SmartPtr<IScriptBreakpoint>>();
    }

    void PyPyManager::_getObject( void **object )
    {
    }

}  // end namespace fb
