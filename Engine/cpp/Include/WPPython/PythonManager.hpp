#ifndef PythonManager_h__
#define PythonManager_h__

#include <WPPython/WPPythonPrerequisites.hpp>
#include <Workphone/Interface/Script/IScriptManager.hpp>
#include <Workphone/Memory/CSharedObject.hpp>

namespace fb
{
    class PythonManager : public CSharedObject<IScriptManager>
    {
    public:
        PythonManager();
        ~PythonManager() override;

        /** @copydoc ISharedObject::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISharedObject::unload */
        void reload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISharedObject::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        void update();

        void loadScript( const String &filename ) override;
        void loadScripts( const Array<String> &scripts ) override;

        void callFunction( const String &functionName ) override;

        void callFunction( const String &functionName, const Parameters &parameters ) override;

        void callFunction( const String &functionName, const Parameters &parameters,
                           Parameters &results ) override;

        void handleError();

        s32 callMember( const String &className, const String &functionName ) override;

        s32 callMember( const String &className, const String &functionName,
                        const Parameters &parameters ) override;

        s32 callMember( const String &className, const String &functionName,
                        const Parameters &parameters, Parameters &results ) override;

        void callObjectMember( SmartPtr<ISharedObject> object, const String &functionName ) override;

        void callObjectMember( SmartPtr<ISharedObject> object, const String &functionName,
                               const Parameters &parameters ) override;

        void callObjectMember( SmartPtr<ISharedObject> object, const String &functionName,
                               const Parameters &parameters, Parameters &results ) override;

        void createObject( const String &className, SmartPtr<ISharedObject> object ) override;

        void *createInstance( const String &className ) override;

        void destroyInstance( void *instance ) override;

        void destroyObject( SmartPtr<ISharedObject> object ) override;

        void setTaskId( u32 taskId );

        u32 getTaskId() const;

        void reloadScripts() override;

        String getDebugInfo() override;

        bool getDelayedCreation() const override;

        void setDelayedCreation( bool delayedCreation ) override;

        bool isDebugEnabled() const override;

        void setDebugEnabled( bool enable ) override;

        void removeBreakpoint( SmartPtr<IScriptBreakpoint> breakpoint ) override;

        void addBreakpoint( SmartPtr<IScriptBreakpoint> breakpoint ) override;

        Array<SmartPtr<IScriptBreakpoint>> getBreakpoints() const override;

        void _getObject( void **object ) override;

        void garbageCollect() override;

        void registerClass( void *ptr ) override;

        Array<String> getScripts() const;

        void setScripts( const Array<String> &scripts );

        bool getError() const;

        void setError( bool error );

        FB_CLASS_REGISTER_DECL;

    protected:
        bool createPythonInstance( PythonObjectDataPtr objectData );

        void addScriptBinding( SmartPtr<IScriptBind> scriptBinding ) override;

        void removeScriptBinding( SmartPtr<IScriptBind> scriptBinding ) override;

        ///
        atomic_bool m_bReload;

        ///
        u32 m_taskId;

        /// The scripts that were loaded
        Array<String> m_scripts;

        ///
        Array<PythonObjectDataPtr> m_objectData;

        ///
        Array<PythonObjectDataPtr> m_creationList;

        /// The script functions
        Array<SmartPtr<IScriptBind>> m_functions;

        /// Instances
        Array<boost::python::object *> m_instances;

        ///
        atomic_u32 m_state;

        FB_MUTEX( ScriptMutex );

        //
        // Value for debugging
        //

        ///
        String m_curClass;

        ///
        String m_curFunction;

        ///
        bool m_enableFullDebug;

        bool m_delayedCreation;

        bool m_isDebugEnabled;

        /// A flag use to know if the scripts have an error.
        bool m_error = false;
    };

}  // end namespace fb

#endif  // PythonManager_h__
