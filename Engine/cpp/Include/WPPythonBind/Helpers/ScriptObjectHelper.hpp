#ifndef ScriptObjectHelper_h__
#define ScriptObjectHelper_h__

#include "WPPythonBind/WPPythonBindPrerequisites.hpp"
#include <Workphone/Workphone.hpp>
#include "WPPythonBind/WPScriptError.hpp"

namespace fb
{

    class ScriptObjectHelper
    {
    public:
        
        static scene::IActor *cast_to_entity( IObject *obj )
        {
            return (scene::IActor *)obj;
        }

        
        static SmartPtr<ISharedObject> _getStateObject( IObject *obj )
        {
            return nullptr;  //obj->getStateObject();
        }

        
        //static FSMContainerPtr _getFSMContainer(IObject* obj)
        //{
        //	return obj->getFSMs();
        //}

        
        static void _initialise( IObject *object, const char *className )
        {
            //object->initialise( String( className ) );
        }

        
        template <class T>
        static void _initialiseTemplate( IObject *object, SmartPtr<T> objectTemplate )
        {
            //object->initialise( objectTemplate );
        }

        
        //template<class T>
        //static s32 _initialiseTemplateProperties(IScriptObject* object, SmartPtr<T> objectTemplate, PropertiesBinding* propertiesBinding)
        //{
        //	return object->initialise(objectTemplate, propertiesBinding->getProperties());
        //}

        static SmartPtr<ISharedObject> getUserDataMap( IObject *object, u32 id )
        {
            return (IObject *)object->getUserData( id );
        }

        static void setUserDataMap( IObject *object, u32 id, IObject *userData )
        {
            object->setUserData( id, (void *)userData );
        }

        static void setObject( SmartPtr<ISharedObject> obj, const String &objectName,
                               SmartPtr<ISharedObject> scriptObj );

        static void setObjectFromHash( SmartPtr<ISharedObject> obj, python_Integer hash,
                                       SmartPtr<ISharedObject> scriptObj );
    };

    template <class T>
    class ScriptObjectFunc
    {
    public:
        static void setPropertyAsBool( T *obj, const char *propertyName, bool value );
        static void setPropertyAsBoolHash( T *obj, python_Integer hashId, bool value );
        static bool getPropertyAsBool( T *obj, const char *propertyName );
        static bool getPropertyAsBoolHash( T *obj, python_Integer hashId );

        static void setPropertyAsNumberHash( T *obj, python_Integer hash, f32 value );
        static python_Number getPropertyAsNumberHash( T *obj, python_Integer hash );
        static void setPropertyAsNumber( T *obj, const char *propertyName, f32 value );
        static f32 getPropertyAsNumber( T *obj, const char *propertyName );

        static String getPropertyAsString( T *obj, const char *propertyName );
        static python_Integer getPropertyAsHash( T *obj, const char *propertyName );

        static void setObject( SmartPtr<T> obj, const String &objectName,
                               SmartPtr<ISharedObject> scriptObj )
        {
            u32 id = StringUtil::getHash( objectName );
            setObjectFromHash( obj, id, scriptObj );
        }

        static void setObjectFromHash( SmartPtr<T> obj, python_Integer hash,
                                       SmartPtr<ISharedObject> scriptObj )
        {
            s32 retValue = obj->setObject( hash, scriptObj );
        }

        static Parameter getObject( T *obj, const char *objectName );
        static Parameter getObjectFromHash( T *obj, python_Integer hash );
        static SmartPtr<scene::IActor> getObjectAsEntity( T *obj, const char *objectName );
        static SmartPtr<scene::IActor> getObjectAsEntityFromHash( T *obj, python_Integer hash );
        static SmartPtr<ISharedObject> getScriptObject( const char *type );
        static SmartPtr<ISharedObject> getScriptObjectHash( python_Integer hash );

        static void ScriptObject_setProperty( T *obj, const char *propertyName, const char *value );
        static void ScriptObject_setPropertyHash( T *obj, python_Integer hash, const char *value );

        static void scriptObjectUpdate( T *obj, s32 task, time_interval t, time_interval dt );

        static SmartPtr<IFSM> getFSMByName( T *obj, const char *value )
        {
            hash32 hash = StringUtil::getHash( value );
            return getFSM( obj, hash );
        }

        static SmartPtr<IFSM> getFSM( T *obj, python_Integer hash )
        {
            return nullptr;
        }

        static Parameter callObjectFunction( T *obj, const char *functionName );
        static Parameter callObjectFunctionHash( T *obj, python_Integer hashId );

        static Parameter callObjectFunctionAny( T *obj, const char *functionName, Parameter arg0 )
        {
            hash32 hash = StringUtil::getHash( functionName );
            return callObjectFunctionAnyHash( obj, hash, arg0 );
        }

        static Parameter callObjectFunctionAnyHash( T *obj, python_Integer hash, Parameter arg0 )
        {
            Parameters params;
            params.resize( 1 );
            params[0] = arg0;

            Parameters results;
            results.resize( 1 );

            s32 retValue = obj->getReceiver()->callFunction( hash, params, results );

            return results[0];
        }

        static Parameter callObjectFunctionAny2( T *obj, const char *functionName, Parameter arg0,
                                                 Parameter arg1 )
        {
            hash32 hash = StringUtil::getHash( functionName );
            return callObjectFunctionAny2Hash( obj, hash, arg0, arg1 );
        }

        static Parameter callObjectFunctionAny2Hash( T *obj, python_Integer hash, Parameter arg0,
                                                     Parameter arg1 )
        {
            Parameters params;
            params.resize( 2 );
            params[0] = arg0;
            params[1] = arg1;

            Parameters results;
            results.resize( 1 );

            s32 retValue = obj->getReceiver()->callFunction( hash, params, results );

            return results[0];
        }

        static Parameter callObjectFunctionAny3( T *obj, const char *functionName, Parameter arg0,
                                                 Parameter arg1, Parameter arg2 )
        {
            hash32 hash = StringUtil::getHash( functionName );
            return callObjectFunctionAny3Hash( obj, hash, arg0, arg1, arg2 );
        }

        static Parameter callObjectFunctionAny3Hash( T *obj, python_Integer hash, Parameter arg0,
                                                     Parameter arg1, Parameter arg2 )
        {
            Parameters params;
            params.resize( 3 );
            params[0] = arg0;
            params[1] = arg1;
            params[2] = arg2;

            Parameters results;
            results.resize( 1 );

            s32 retValue = obj->getReceiver()->callFunction( hash, params, results );

            return results[0];
        }

        static Parameter callObjectFunctionAny4( T *obj, const char *functionName, Parameter arg0,
                                                 Parameter arg1, Parameter arg2, Parameter arg3 )
        {
            hash32 hash = StringUtil::getHash( functionName );
            return callObjectFunctionAny3Hash( obj, hash, arg0, arg1, arg2 );
        }

        static Parameter callObjectFunctionAny4Hash( T *obj, python_Integer hash, Parameter arg0,
                                                     Parameter arg1, Parameter arg2, Parameter arg3 )
        {
            Parameters params;
            params.resize( 4 );
            params[0] = arg0;
            params[1] = arg1;
            params[2] = arg2;
            params[3] = arg3;

            Parameters results;
            results.resize( 1 );

            s32 retValue = obj->getReceiver()->callFunction( hash, params, results );

            return results[0];
        }

        static Parameter callObjectFunctionParams( T *obj, const char *functionName,
                                                   const Parameters &params );
        static Parameter callObjectFunctionParamsHash( T *obj, python_Integer hashId,
                                                       const Parameters &params );

        static void callObjectFunctionParamsResults( T *obj, const char *functionName,
                                                     const Parameters &params, Parameters &results );
        static void callObjectFunctionParamsResultsHash( T *obj, python_Integer hashId,
                                                         const Parameters &params, Parameters &results );

        static Parameter callObjectFunctionObj( T *obj, const char *functionName,
                                                const Properties &properties );
        static Parameter callObjectFunctionObjHash( T *obj, python_Integer hashId,
                                                    const Properties &properties );

        static void setPropertyAsVector2iHash( T *obj, python_Integer hash, const Vector2I &value );
        static void setPropertyAsVector2i( T *obj, const char *propertyName, const Vector2I &value );
        static Vector2I getPropertyAsVector2iHash( T *obj, python_Integer hashId );
        static Vector2I getPropertyAsVector2i( T *obj, const char *propertyName );

        static void setPropertyAsVector2Hash( T *obj, python_Integer hash, const Vector2F &value );
        static void setPropertyAsVector2( T *obj, const char *propertyName, const Vector2F &value );
        static Vector2F getPropertyAsVector2Hash( T *obj, python_Integer hashId );
        static Vector2F getPropertyAsVector2( T *obj, const char *propertyName );

        static void setPropertyAsVector3( T *obj, const char *propertyName, Vector3F &value );
        static void setPropertyAsVector3Hash( T *obj, python_Integer hash, Vector3F &value );
        static Vector3F getPropertyAsVector3( T *obj, const char *propertyName );
        static Vector3F getPropertyAsVector3Hash( T *obj, python_Integer hash );

        static void setPropertyAsQuaternion( T *obj, const char *propertyName,
                                             const QuaternionF &value );
        static void setPropertyAsQuaternionHash( T *obj, python_Integer hash, const QuaternionF &value );
        static QuaternionF getPropertyAsQuaternion( T *obj, const char *propertyName );
        static QuaternionF getPropertyAsQuaternionHash( T *obj, python_Integer hash );

        static void setPropertyAsColour( T *obj, const char *propertyName, const ColourF &color );
        static void setPropertyAsColourHash( T *obj, python_Integer hash, const ColourF &color );
        static ColourF getPropertyAsColour( T *obj, const char *propertyName );
        static ColourF getPropertyAsColourHash( T *obj, python_Integer hash );

        static void setPropertyAsInt( T *obj, const char *propertyName, python_Integer value );
        static void setPropertyAsIntHash( T *obj, python_Integer hash, python_Integer value );
        static python_Integer getPropertyAsInt( T *obj, const char *propertyName );
        static python_Integer getPropertyAsIntHash( T *obj, python_Integer hash );

        static void setPropertyAsUInt( T *obj, const char *propertyName, python_Number value );
        static void setPropertyAsUIntHash( T *obj, python_Integer hash, python_Number value );
        static python_Integer getPropertyAsUInt( T *obj, const char *propertyName );
        static python_Integer getPropertyAsUIntHash( T *obj, python_Integer hash );

        static Parameters getPropertyAsArray( T *obj, const char *propertyName );
        static Parameters getPropertyAsArrayHash( T *obj, python_Integer hashId );

        static void setCallback( T *obj, const char *callbackName, const char *functionName )
        {
            //ScriptEventPtr scriptEvent( new ScriptEvent, true );
            //scriptEvent->setFunction( functionName );
            //obj->getInvoker()->setEventFunction( StringUtil::getHash( callbackName ), scriptEvent );
        }
    };

}  // end namespace fb

#include "ScriptObjectHelper.inl"

#endif  // ScriptObjectHelper_h__
