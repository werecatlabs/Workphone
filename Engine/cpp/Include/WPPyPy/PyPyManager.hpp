#ifndef PyPyManager_h__
#define PyPyManager_h__

#include "Workphone/Interface/Script/IScriptManager.hpp"

namespace fb
{

    //-------------------------------------------------------------------------------------------------------------------------------
    class PyPyManager : public IScriptManager
    {
    public:
        PyPyManager();
        ~PyPyManager();

        void update( s32 task, time_interval t, time_interval dt );

        void loadScript( const String &filename );

        void callFunction( const String &functionName );
        void callFunction( const String &functionName, const Parameters &parameters );
        void callFunction( const String &functionName, const Parameters &parameters,
                           Parameters &results );

        s32 callMember( const String &className, const String &functionName );
        s32 callMember( const String &className, const String &functionName,
                        const Parameters &parameters );
        s32 callMember( const String &className, const String &functionName,
                        const Parameters &parameters, Parameters &results );

        void callObjectMember( IObject *object, const String &functionName );
        void callObjectMember( IObject *object, const String &functionName,
                               const Parameters &parameters );
        void callObjectMember( IObject *object, const String &functionName, const Parameters &parameters,
                               Parameters &results );

        void createObject( const String &className, SmartPtr<IObject> object );

        void *createInstance( const String &className );

        void destroyInstance( void *instance );

        void destroyObject( SmartPtr<IObject> object );

        void setTaskId( u32 taskId );

        u32 getTaskId() const;

        void reloadScripts();

        String getDebugInfo();

        bool getDelayedCreation() const;

        void setDelayedCreation( bool delayedCreation );

        bool isDebugEnabled() const;

        void setDebugEnabled( bool enable );

        void removeBreakpoint( SmartPtr<IScriptBreakpoint> breakpoint );

        void addBreakpoint( SmartPtr<IScriptBreakpoint> breakpoint );

        Array<SmartPtr<IScriptBreakpoint>> getBreakpoints() const;

        void _getObject( void **object );

    protected:
        FB_MUTEX( ScriptMutex );
    };

}  // end namespace fb

#endif  // PyPyManager_h__
