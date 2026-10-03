namespace fb
{

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsBool( T *obj, const char *propertyName, bool value )
    {
        hash32 hash = StringUtil::getHash( propertyName );
        setPropertyAsBoolHash( obj, *reinterpret_cast<python_Integer *>( &hash ), value );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsBoolHash( T *obj, python_Integer hash, bool value )
    {
        Parameter param;
        param.setBool( value ? TRUE : FALSE );

        auto receiver = obj->getReceiver();
        if( receiver )
        {
            s32 retValue = receiver->setProperty( *reinterpret_cast<u32 *>( &hash ), param );
        }
    }

    template <class T>
    bool ScriptObjectFunc<T>::getPropertyAsBool( T *obj, const char *propertyName )
    {
        hash32 hash = StringUtil::getHash( propertyName );
        return getPropertyAsBoolHash( obj, *reinterpret_cast<python_Integer *>( &hash ) );
    }

    template <class T>
    bool ScriptObjectFunc<T>::getPropertyAsBoolHash( T *obj, python_Integer hash )
    {
        auto receiver = obj->getReceiver();
        if( receiver )
        {
            Parameter value;
            s32 retValue = receiver->getProperty( *reinterpret_cast<u32 *>( &hash ), value );

            return value.data.bData;
        }

        return false;
    }

    template <class T>
    void ScriptObjectFunc<T>::scriptObjectUpdate( T *obj, s32 task, time_interval t, time_interval dt )
    {
        obj->update( task, t, dt );
    }

    template <class T>
    Parameter ScriptObjectFunc<T>::callObjectFunction( T *obj, const char *functionName )
    {
        hash32 hash = StringUtil::getHash( functionName );
        return callObjectFunctionHash( obj, *reinterpret_cast<python_Integer *>( &hash ) );
    }

    template <class T>
    Parameter ScriptObjectFunc<T>::callObjectFunctionHash( T *obj, python_Integer hash )
    {
        auto receiver = obj->getReceiver();
        if( receiver )
        {
            s32 retValue =
                receiver->callFunction( *reinterpret_cast<u32 *>( &hash ), Parameters(), Parameters() );
        }
        else
        {
        }

        return Parameter::VOID_PARAM;
    }

    template <class T>
    Parameter ScriptObjectFunc<T>::callObjectFunctionParams( T *obj, const char *functionName,
                                                             const Parameters &params )
    {
        hash32 hash = StringUtil::getHash( functionName );
        return callObjectFunctionParamsHash( obj, *reinterpret_cast<python_Integer *>( &hash ), params );
    }

    template <class T>
    Parameter ScriptObjectFunc<T>::callObjectFunctionParamsHash( T *obj, python_Integer hash,
                                                                 const Parameters &params )
    {
        Parameters results;
        s32 retValue =
            obj->getReceiver()->callFunction( *reinterpret_cast<u32 *>( &hash ), params, results );

        return results.size() == 0 ? Parameter::VOID_PARAM : results[0];
    }

    template <class T>
    void ScriptObjectFunc<T>::callObjectFunctionParamsResults( T *obj, const char *functionName,
                                                               const Parameters &params,
                                                               Parameters &results )
    {
        hash32 hash = StringUtil::getHash( functionName );
        return callObjectFunctionParamsResultsHash( obj, *reinterpret_cast<python_Integer *>( &hash ),
                                                    params, results );
    }

    template <class T>
    void ScriptObjectFunc<T>::callObjectFunctionParamsResultsHash( T *obj, python_Integer hash,
                                                                   const Parameters &params,
                                                                   Parameters &results )
    {
        s32 retValue =
            obj->getReceiver()->callFunction( *reinterpret_cast<u32 *>( &hash ), params, results );
    }

    template <class T>
    Parameter ScriptObjectFunc<T>::callObjectFunctionObj( T *obj, const char *functionName,
                                                          const Properties &properties )
    {
        hash32 hash = StringUtil::getHash( functionName );
        return callObjectFunctionObjHash( obj, *reinterpret_cast<python_Integer *>( &hash ),
                                          properties );
    }

    template <class T>
    Parameter ScriptObjectFunc<T>::callObjectFunctionObjHash( T *obj, python_Integer hash,
                                                              const Properties &properties )
    {
        //auto object = const_cast<ScriptProperties*>(&properties);

        //Parameters results;
        //s32 retValue = obj->getReceiver()->callFunction(*reinterpret_cast<u32*>(&hash), object, results);
        //
        //return results.size() == 0 ? Parameter::VOID_PARAM : results[0];

        return Parameter();
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsVector2iHash( T *obj, python_Integer hash,
                                                         const Vector2I &value )
    {
        Parameters params;
        params.resize( 2 );
        params[0].data.iData = value.X();
        params[1].data.iData = value.Y();
        s32 retValue = obj->getReceiver()->setProperty( *reinterpret_cast<u32 *>( &hash ), params );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsVector2i( T *obj, const char *propertyName,
                                                     const Vector2I &value )
    {
        hash32 hash = StringUtil::getHash( propertyName );
        setPropertyAsVector2iHash( obj, *reinterpret_cast<python_Integer *>( &hash ), value );
    }

    template <class T>
    Vector2I ScriptObjectFunc<T>::getPropertyAsVector2iHash( T *obj, python_Integer hash )
    {
        Parameters params;
        s32 retValue = obj->getReceiver()->getProperty( *reinterpret_cast<u32 *>( &hash ), params );

        if( params.size() == 2 )
            return Vector2I( params[0].data.iData, params[1].data.iData );

        FB_LOG_MESSAGE( "LuaScriptMgr", "Could not get property: Vector2" );
        return Vector2I::ZERO;
    }

    template <class T>
    Vector2I ScriptObjectFunc<T>::getPropertyAsVector2i( T *obj, const char *propertyName )
    {
        hash32 hash = StringUtil::getHash( propertyName );
        return getPropertyAsVector2iHash( obj, *reinterpret_cast<python_Integer *>( &hash ) );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsVector2Hash( T *obj, python_Integer hash,
                                                        const Vector2F &value )
    {
        Parameters params;
        params.resize( 2 );
        params[0].data.fData = value.X();
        params[1].data.fData = value.Y();
        s32 retValue = obj->getReceiver()->setProperty( *reinterpret_cast<u32 *>( &hash ), params );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsVector2( T *obj, const char *propertyName,
                                                    const Vector2F &value )
    {
        hash32 hash = StringUtil::getHash( propertyName );
        setPropertyAsVector2Hash( obj, *reinterpret_cast<python_Integer *>( &hash ), value );
    }

    template <class T>
    Vector2F ScriptObjectFunc<T>::getPropertyAsVector2Hash( T *obj, python_Integer hash )
    {
        Parameters params;
        s32 retValue = obj->getReceiver()->getProperty( *reinterpret_cast<u32 *>( &hash ), params );

        if( params.size() == 2 )
            return Vector2F( params[0].data.fData, params[1].data.fData );

        FB_LOG_MESSAGE( "LuaScriptMgr", "Could not get property: Vector2" );
        return Vector2F::ZERO;
    }

    template <class T>
    Vector2F ScriptObjectFunc<T>::getPropertyAsVector2( T *obj, const char *propertyName )
    {
        hash32 hash = StringUtil::getHash( propertyName );
        return getPropertyAsVector2Hash( obj, *reinterpret_cast<python_Integer *>( &hash ) );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsVector3( T *obj, const char *propertyName, Vector3F &value )
    {
        hash32 hash = StringUtil::getHash( propertyName );
        setPropertyAsVector3Hash( obj, *reinterpret_cast<python_Integer *>( &hash ), value );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsVector3Hash( T *obj, python_Integer hash, Vector3F &value )
    {
        s32 retVal = obj->getReceiver()->setProperty( *reinterpret_cast<u32 *>( &hash ),
                                                      static_cast<void *>( &value ) );

        if( retVal != 0 )
        {
            Parameters params;
            params.resize( 3 );
            params[0].data.fData = value.X();
            params[1].data.fData = value.Y();
            params[2].data.fData = value.Z();

            retVal = obj->getReceiver()->setProperty( *reinterpret_cast<u32 *>( &hash ), params );
        }

        PYTHON_SCRIPT_OBJ_CODE( retVal );
    }

    template <class T>
    Vector3F ScriptObjectFunc<T>::getPropertyAsVector3( T *obj, const char *propertyName )
    {
        hash32 hash = StringUtil::getHash( propertyName );
        return getPropertyAsVector3Hash( obj, *reinterpret_cast<python_Integer *>( &hash ) );
    }

    template <class T>
    Vector3F ScriptObjectFunc<T>::getPropertyAsVector3Hash( T *obj, python_Integer hash )
    {
        Vector3F vec;
        s32 retValue = obj->getReceiver()->getProperty( *reinterpret_cast<u32 *>( &hash ), &vec );
        if( retValue == 0 )
            return vec;

        Parameters params;
        retValue = obj->getReceiver()->getProperty( *reinterpret_cast<u32 *>( &hash ), params );
        if( retValue == 0 )
            return Vector3F( params[0].data.fData, params[1].data.fData, params[2].data.fData );

        FB_LOG_MESSAGE( "LuaScriptMgr", "Could not get property: vector3" );
        return Vector3F::ZERO;
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsQuaternion( T *obj, const char *propertyName,
                                                       const QuaternionF &value )
    {
        hash32 hash = StringUtil::getHash( propertyName );

        Parameters params;
        params.resize( 4 );
        params[0].data.fData = value.W();
        params[1].data.fData = value.X();
        params[2].data.fData = value.Y();
        params[3].data.fData = value.Z();
        s32 retValue = obj->getReceiver()->setProperty( hash, params );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsQuaternionHash( T *obj, python_Integer hash,
                                                           const QuaternionF &value )
    {
        Parameters params;
        params.resize( 4 );
        params[0].data.fData = value.W();
        params[1].data.fData = value.X();
        params[2].data.fData = value.Y();
        params[3].data.fData = value.Z();
        s32 retValue = obj->getReceiver()->setProperty( *reinterpret_cast<u32 *>( &hash ), params );
    }

    template <class T>
    QuaternionF ScriptObjectFunc<T>::getPropertyAsQuaternion( T *obj, const char *propertyName )
    {
        hash32 hash = StringUtil::getHash( propertyName );

        Parameters params;
        s32 retValue = obj->getReceiver()->getProperty( hash, params );

        if( params.size() == 4 )
            return QuaternionF( (f32)params[0].data.fData, (f32)params[1].data.fData,
                                (f32)params[2].data.fData, (f32)params[3].data.fData );

        return QuaternionF::IDENTITY;
    }

    template <class T>
    QuaternionF ScriptObjectFunc<T>::getPropertyAsQuaternionHash( T *obj, python_Integer hash )
    {
        Parameters params;
        s32 retValue = obj->getReceiver()->getProperty( *reinterpret_cast<u32 *>( &hash ), params );

        if( params.size() == 4 )
            return QuaternionF( (f32)params[0].data.fData, (f32)params[1].data.fData,
                                (f32)params[2].data.fData, (f32)params[3].data.fData );

        return QuaternionF::IDENTITY;
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsColour( T *obj, const char *propertyName,
                                                   const ColourF &color )
    {
        hash32 hash = StringUtil::getHash( propertyName );
        s32 retValue = obj->getReceiver()->setProperty( hash, (void *)&color );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsColourHash( T *obj, python_Integer hash,
                                                       const ColourF &color )
    {
        s32 retValue =
            obj->getReceiver()->setProperty( *reinterpret_cast<u32 *>( &hash ), (void *)&color );
    }

    template <class T>
    ColourF ScriptObjectFunc<T>::getPropertyAsColour( T *obj, const char *propertyName )
    {
        hash32 hash = StringUtil::getHash( propertyName );

        ColourF color;
        s32 retValue = obj->getReceiver()->setProperty( hash, (void *)&color );

        return color;
    }

    template <class T>
    ColourF ScriptObjectFunc<T>::getPropertyAsColourHash( T *obj, python_Integer hash )
    {
        ColourF color;
        s32 retValue =
            obj->getReceiver()->setProperty( *reinterpret_cast<u32 *>( &hash ), (void *)&color );

        return color;
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsInt( T *obj, const char *propertyName, python_Integer value )
    {
        hash32 hash = StringUtil::getHash( propertyName );
        setPropertyAsIntHash( obj, *reinterpret_cast<python_Integer *>( &hash ), value );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsIntHash( T *obj, python_Integer hash, python_Integer value )
    {
        auto receiver = obj->getReceiver();
        if( receiver )
        {
            s32 retValue =
                obj->getReceiver()->setProperty( *reinterpret_cast<u32 *>( &hash ), Parameter( value ) );
        }
        else
        {
        }
    }

    template <class T>
    python_Integer ScriptObjectFunc<T>::getPropertyAsInt( T *obj, const char *propertyName )
    {
        hash32 hash = StringUtil::getHash( propertyName );
        return getPropertyAsIntHash( obj, *reinterpret_cast<python_Integer *>( &hash ) );
    }

    template <class T>
    python_Integer ScriptObjectFunc<T>::getPropertyAsIntHash( T *obj, python_Integer hash )
    {
        auto receiver = obj->getReceiver();
        if( receiver )
        {
            Parameter value;
            s32 retValue = obj->getReceiver()->getProperty( *reinterpret_cast<u32 *>( &hash ), value );

            return value.data.iData;
        }

        return 0;
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsUInt( T *obj, const char *propertyName, python_Number value )
    {
        hash32 hash = StringUtil::getHash( propertyName );
        setPropertyAsUIntHash( obj, *reinterpret_cast<python_Integer *>( &hash ), value );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsUIntHash( T *obj, python_Integer hash, python_Number value )
    {
        auto receiver = obj->getReceiver();
        if( receiver )
        {
            s32 retValue =
                receiver->setProperty( *reinterpret_cast<u32 *>( &hash ), Parameter( (u32)value ) );
        }
        else
        {
        }
    }

    template <class T>
    python_Integer ScriptObjectFunc<T>::getPropertyAsUInt( T *obj, const char *propertyName )
    {
        hash32 hash = StringUtil::getHash( propertyName );
        return getPropertyAsUIntHash( obj, *reinterpret_cast<python_Integer *>( &hash ) );
    }

    template <class T>
    python_Integer ScriptObjectFunc<T>::getPropertyAsUIntHash( T *obj, python_Integer hash )
    {
        auto receiver = obj->getReceiver();
        if( receiver )
        {
            Parameter value;
            s32 retValue = receiver->getProperty( *reinterpret_cast<u32 *>( &hash ), value );

            return value.data.iData;
        }

        return 0;
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsNumberHash( T *obj, python_Integer hash, f32 value )
    {
        auto receiver = obj->getReceiver();
        if( receiver )
        {
            s32 retValue =
                receiver->setProperty( *reinterpret_cast<u32 *>( &hash ), Parameter( value ) );
        }
        else
        {
        }
    }

    template <class T>
    python_Number ScriptObjectFunc<T>::getPropertyAsNumberHash( T *obj, python_Integer hash )
    {
        auto receiver = obj->getReceiver();
        if( receiver )
        {
            Parameter value;
            s32 retValue = obj->getReceiver()->getProperty( *reinterpret_cast<u32 *>( &hash ), value );

            return python_Number( value.data.fData );
        }

        return python_Number( 0.0 );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsNumber( T *obj, const char *propertyName, f32 value )
    {
        hash32 hash = StringUtil::getHash( propertyName );
        setPropertyAsNumberHash( obj, *reinterpret_cast<python_Integer *>( &hash ), value );
    }

    template <class T>
    f32 ScriptObjectFunc<T>::getPropertyAsNumber( T *obj, const char *propertyName )
    {
        hash32 hash = StringUtil::getHash( propertyName );
        return getPropertyAsNumberHash( obj, *reinterpret_cast<python_Integer *>( &hash ) );
    }

    template <class T>
    String ScriptObjectFunc<T>::getPropertyAsString( T *obj, const char *propertyName )
    {
        hash32 hash = StringUtil::getHash( propertyName );

        String value;

        s32 retValue = obj->getReceiver()->getProperty( hash, value );

        return value;
    }

    template <class T>
    python_Integer ScriptObjectFunc<T>::getPropertyAsHash( T *obj, const char *propertyName )
    {
        hash32 hash = StringUtil::getHash( propertyName );

        String value;

        s32 retValue = obj->getReceiver()->getProperty( hash, value );

        if( retValue != 0 )
        {
            int halt = 0;
            halt = 0;
        }

        u32 hashValue = StringUtil::getHash( value );
        return hashValue;
    }

    template <class T>
    Parameter ScriptObjectFunc<T>::getObject( T *obj, const char *objectName )
    {
        hash32 hash = StringUtil::getHash( objectName );
        return getObjectFromHash( obj, *reinterpret_cast<python_Integer *>( &hash ) );
    }

    template <class T>
    Parameter ScriptObjectFunc<T>::getObjectFromHash( T *obj, python_Integer hash )
    {
        SmartPtr<ISharedObject> object;
        obj->getObject( *reinterpret_cast<u32 *>( &hash ), object );

        if( object )
        {
            Parameter param;
            param.setPtr( object.get() );
            return param;
        }

        Parameter param;
        param.setPtr( nullptr );
        return param;
    }

    template <class T>
    SmartPtr<scene::IActor> ScriptObjectFunc<T>::getObjectAsEntity( T *obj, const char *objectName )
    {
        hash32 hash = StringUtil::getHash( objectName );

        SmartPtr<ISharedObject> object;
        obj->getObject( hash, object );

        if( object )
            return object;

        return nullptr;
    }

    template <class T>
    SmartPtr<scene::IActor> ScriptObjectFunc<T>::getObjectAsEntityFromHash( T *obj, python_Integer hash )
    {
        SmartPtr<ISharedObject> object;
        obj->getObject( *reinterpret_cast<u32 *>( &hash ), object );

        if( object )
            return object;

        return nullptr;
    }

    template <class T>
    SmartPtr<ISharedObject> ScriptObjectFunc<T>::getScriptObject( const char *type )
    {
        hash32 hash = StringUtil::getHash( type );
        return getScriptObjectHash( *reinterpret_cast<python_Integer *>( &hash ) );
    }

    template <class T>
    SmartPtr<ISharedObject> ScriptObjectFunc<T>::getScriptObjectHash( python_Integer hash )
    {
        //ObjectPtr& scriptObject = Engine::getSingletonPtr()->getScriptObject(*reinterpret_cast<u32*>(&hash));
        //if(scriptObject)
        //	return scriptObject.getPtr();

        return nullptr;
    }

    template <class T>
    void ScriptObjectFunc<T>::ScriptObject_setProperty( T *obj, const char *propertyName,
                                                        const char *value )
    {
        hash32 hash = StringUtil::getHash( propertyName );
        s32 retValue = obj->getReceiver()->setProperty( hash, String( value ) );
    }

    template <class T>
    void ScriptObjectFunc<T>::ScriptObject_setPropertyHash( T *obj, python_Integer hash,
                                                            const char *value )
    {
        s32 retValue =
            obj->getReceiver()->setProperty( *reinterpret_cast<u32 *>( &hash ), String( value ) );
    }

    template <class T>
    Parameters ScriptObjectFunc<T>::getPropertyAsArray( T *obj, const char *propertyName )
    {
        hash32 hash = StringUtil::getHash( propertyName );
        return getPropertyAsBoolHash( obj, *reinterpret_cast<python_Integer *>( &hash ) );
    }

    template <class T>
    Parameters ScriptObjectFunc<T>::getPropertyAsArrayHash( T *obj, python_Integer hash )
    {
        Parameters params;
        s32 retValue = obj->getReceiver()->getProperty( *reinterpret_cast<u32 *>( &hash ), params );

        return params;
    }

}  // end namespace fb
