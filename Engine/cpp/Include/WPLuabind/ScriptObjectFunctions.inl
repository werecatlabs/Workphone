namespace workphone
{

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsBool( T *obj, const char *propertyName, bool value )
    {
        auto hash = StringUtil::getHash( propertyName );
        setPropertyAsBoolHash( obj, *reinterpret_cast<lua_Integer *>( &hash ), value );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsBoolHash( T *obj, lua_Integer hash, bool value )
    {
        Parameter param;
        param.setBool( value ? true : false );

        auto receiver = obj->getReceiver();
        if( receiver )
        {
            s32 retValue = receiver->setProperty( *reinterpret_cast<hash32 *>( &hash ), param );
        }
    }

    template <class T>
    bool ScriptObjectFunc<T>::getPropertyAsBool( T *obj, const char *propertyName )
    {
        auto hash = StringUtil::getHash( propertyName );
        return getPropertyAsBoolHash( obj, *reinterpret_cast<lua_Integer *>( &hash ) );
    }

    template <class T>
    bool ScriptObjectFunc<T>::getPropertyAsBoolHash( T *obj, lua_Integer hash )
    {
        auto receiver = obj->getReceiver();
        if( receiver )
        {
            Parameter value;
            s32 retValue = receiver->getProperty( *reinterpret_cast<hash32 *>( &hash ), value );

            return value.data.bData;
        }

        return false;
    }

    template <class T>
    void ScriptObjectFunc<T>::scriptObjectUpdate( T *obj, const s32 &task, const time_interval &t,
                                                  const time_interval &dt )
    {
        obj->update( task, t, dt );
    }

    template <class T>
    Parameter ScriptObjectFunc<T>::callObjectFunction( T *obj, const char *functionName )
    {
        auto hash = StringUtil::getHash( functionName );
        return callObjectFunctionHash( obj, *reinterpret_cast<lua_Integer *>( &hash ) );
    }

    template <class T>
    Parameter ScriptObjectFunc<T>::callObjectFunctionHash( T *obj, lua_Integer hash )
    {
        auto receiver = obj->getReceiver();
        if( receiver )
        {
            //s32 retValue = receiver->callFunction(*reinterpret_cast<hash32*>(&hash), Parameters(), Parameters());
        }

        return Parameter::VOID_PARAM;
    }

    template <class T>
    Parameter ScriptObjectFunc<T>::callObjectFunctionParams( T *obj, const char *functionName,
                                                             const Parameters &params )
    {
        auto hash = StringUtil::getHash( functionName );
        return callObjectFunctionParamsHash( obj, *reinterpret_cast<lua_Integer *>( &hash ), params );
    }

    template <class T>
    Parameter ScriptObjectFunc<T>::callObjectFunctionParamsHash( T *obj, lua_Integer hash,
                                                                 const Parameters &params )
    {
        Parameters results;
        s32 retValue =
            obj->getReceiver()->callFunction( *reinterpret_cast<hash32 *>( &hash ), params, results );

        return results.size() == 0 ? Parameter::VOID_PARAM : results[0];
    }

    template <class T>
    void ScriptObjectFunc<T>::callObjectFunctionParamsResults( T *obj, const char *functionName,
                                                               const Parameters &params,
                                                               Parameters &results )
    {
        auto hash = StringUtil::getHash( functionName );
        return callObjectFunctionParamsResultsHash( obj, *reinterpret_cast<lua_Integer *>( &hash ),
                                                    params, results );
    }

    template <class T>
    void ScriptObjectFunc<T>::callObjectFunctionParamsResultsHash( T *obj, lua_Integer hash,
                                                                   const Parameters &params,
                                                                   Parameters &results )
    {
        s32 retValue =
            obj->getReceiver()->callFunction( *reinterpret_cast<hash32 *>( &hash ), params, results );
    }

    //
    //template <class T>
    //Parameter ScriptObjectFunc<T>::callObjectFunctionObj(T* obj, const char* functionName, const ScriptProperties& properties)
    //{
    //	auto hash = StringUtil::getHash(functionName);
    //	return callObjectFunctionObjHash(obj, *reinterpret_cast<lua_Integer*>(&hash), properties);
    //}

    //
    //template <class T>
    //Parameter ScriptObjectFunc<T>::callObjectFunctionObjHash(T* obj, lua_Integer hash, const ScriptProperties& properties)
    //{
    //	SmartPtr<IScriptObject> object(const_cast<ScriptProperties*>(&properties));
    //
    //	Parameters results;
    //	s32 retValue = obj->getReceiver()->callFunction(*reinterpret_cast<hash32*>(&hash), object, results);
    //
    //	return results.size() == 0 ? Parameter::VOID_PARAM : results[0];
    //}

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsVector2iHash( T *obj, lua_Integer hash,
                                                         const Vector2I &value )
    {
        Parameters params;
        params.resize( 2 );
        params[0].data.iData = value.X();
        params[1].data.iData = value.Y();
        s32 retValue = obj->getReceiver()->setProperty( *reinterpret_cast<hash32 *>( &hash ), params );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsVector2i( T *obj, const char *propertyName,
                                                     const Vector2I &value )
    {
        auto hash = StringUtil::getHash( propertyName );
        setPropertyAsVector2iHash( obj, *reinterpret_cast<lua_Integer *>( &hash ), value );
    }

    template <class T>
    Vector2I ScriptObjectFunc<T>::getPropertyAsVector2iHash( T *obj, lua_Integer hash )
    {
        Parameters params;
        s32 retValue = obj->getReceiver()->getProperty( *reinterpret_cast<hash32 *>( &hash ), params );

        if( params.size() == 2 )
            return Vector2I( params[0].data.iData, params[1].data.iData );

        WP_LOG_INFO( "Could not get property: Vector2" );
        return Vector2I::ZERO;
    }

    template <class T>
    Vector2I ScriptObjectFunc<T>::getPropertyAsVector2i( T *obj, const char *propertyName )
    {
        auto hash = StringUtil::getHash( propertyName );
        return getPropertyAsVector2iHash( obj, *reinterpret_cast<lua_Integer *>( &hash ) );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsVector2Hash( T *obj, lua_Integer hash, const Vector2F &value )
    {
        auto receiver = obj->getReceiver();
        if( receiver )
        {
            Parameters params;
            params.resize( 2 );
            params[0].data.fData = value.X();
            params[1].data.fData = value.Y();
            obj->getReceiver()->setProperty( *reinterpret_cast<hash32 *>( &hash ), params );
        }
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsVector2( T *obj, const char *propertyName,
                                                    const Vector2F &value )
    {
        auto hash = StringUtil::getHash( propertyName );
        setPropertyAsVector2Hash( obj, *reinterpret_cast<lua_Integer *>( &hash ), value );
    }

    template <class T>
    Vector2F ScriptObjectFunc<T>::getPropertyAsVector2Hash( T *obj, lua_Integer hash )
    {
        Parameters params;

        auto receiver = obj->getReceiver();
        if( !receiver )
        {
            return Vector2F::ZERO;
        }

        obj->getReceiver()->getProperty( *reinterpret_cast<hash32 *>( &hash ), params );
        if( params.size() == 2 )
            return Vector2F( params[0].data.fData, params[1].data.fData );

        WP_LOG_INFO( "Could not get property: Vector2" );
        return Vector2F::ZERO;
    }

    template <class T>
    Vector2F ScriptObjectFunc<T>::getPropertyAsVector2( T *obj, const char *propertyName )
    {
        auto hash = StringUtil::getHash( propertyName );
        return getPropertyAsVector2Hash( obj, *reinterpret_cast<lua_Integer *>( &hash ) );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsVector3( T *obj, const char *propertyName, Vector3F &value )
    {
        auto hash = StringUtil::getHash( propertyName );
        setPropertyAsVector3Hash( obj, *reinterpret_cast<lua_Integer *>( &hash ), value );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsVector3Hash( T *obj, lua_Integer hash, Vector3F &value )
    {
        auto receiver = obj->getReceiver();
        if( !receiver )
        {
            WP_EXCEPTION( "Error" );
        }

        s32 retVal = obj->getReceiver()->setProperty( *reinterpret_cast<hash32 *>( &hash ),
                                                      static_cast<void *>( &value ) );

        if( retVal != 0 )
        {
            Parameters params;
            params.resize( 3 );
            params[0].data.fData = value.X();
            params[1].data.fData = value.Y();
            params[2].data.fData = value.Z();

            retVal = obj->getReceiver()->setProperty( *reinterpret_cast<hash32 *>( &hash ), params );
        }
    }

    template <class T>
    Vector3F ScriptObjectFunc<T>::getPropertyAsVector3( T *obj, const char *propertyName )
    {
        auto hash = StringUtil::getHash( propertyName );
        return getPropertyAsVector3Hash( obj, *reinterpret_cast<lua_Integer *>( &hash ) );
    }

    template <class T>
    Vector3F ScriptObjectFunc<T>::getPropertyAsVector3Hash( T *obj, lua_Integer hash )
    {
        Vector3F vec;
        s32 retValue = obj->getReceiver()->getProperty( *reinterpret_cast<hash32 *>( &hash ), &vec );
        if( retValue == 0 )
            return vec;

        Parameters params;
        retValue = obj->getReceiver()->getProperty( *reinterpret_cast<hash32 *>( &hash ), params );
        if( retValue == 0 )
            return Vector3F( params[0].data.fData, params[1].data.fData, params[2].data.fData );

        WP_LOG_INFO( "Could not get property: vector3" );
        return Vector3F::ZERO;
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsQuaternion( T *obj, const char *propertyName,
                                                       const QuaternionF &value )
    {
        auto hash = StringUtil::getHash( propertyName );

        Parameters params;
        params.resize( 4 );
        params[0].data.fData = value.W();
        params[1].data.fData = value.X();
        params[2].data.fData = value.Y();
        params[3].data.fData = value.Z();
        s32 retValue = obj->getReceiver()->setProperty( hash, params );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsQuaternionHash( T *obj, lua_Integer hash,
                                                           const QuaternionF &value )
    {
        Parameters params;
        params.resize( 4 );
        params[0].data.fData = value.W();
        params[1].data.fData = value.X();
        params[2].data.fData = value.Y();
        params[3].data.fData = value.Z();
        s32 retValue = obj->getReceiver()->setProperty( *reinterpret_cast<hash32 *>( &hash ), params );
    }

    template <class T>
    QuaternionF ScriptObjectFunc<T>::getPropertyAsQuaternion( T *obj, const char *propertyName )
    {
        auto hash = StringUtil::getHash( propertyName );

        Parameters params;
        s32 retValue = obj->getReceiver()->getProperty( hash, params );

        if( params.size() == 4 )
            return QuaternionF( (f32)params[0].data.fData, (f32)params[1].data.fData,
                                (f32)params[2].data.fData, (f32)params[3].data.fData );

        return QuaternionF::identity();
    }

    template <class T>
    QuaternionF ScriptObjectFunc<T>::getPropertyAsQuaternionHash( T *obj, lua_Integer hash )
    {
        Parameters params;
        s32 retValue = obj->getReceiver()->getProperty( *reinterpret_cast<hash32 *>( &hash ), params );

        if( params.size() == 4 )
            return QuaternionF( (f32)params[0].data.fData, (f32)params[1].data.fData,
                                (f32)params[2].data.fData, (f32)params[3].data.fData );

        return QuaternionF::identity();
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsColour( T *obj, const char *propertyName,
                                                   const ColourF &color )
    {
        auto hash = StringUtil::getHash( propertyName );
        s32 retValue = obj->getReceiver()->setProperty( hash, (void *)&color );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsColourHash( T *obj, lua_Integer hash, const ColourF &color )
    {
        s32 retValue =
            obj->getReceiver()->setProperty( *reinterpret_cast<hash32 *>( &hash ), (void *)&color );
    }

    template <class T>
    ColourF ScriptObjectFunc<T>::getPropertyAsColour( T *obj, const char *propertyName )
    {
        auto hash = StringUtil::getHash( propertyName );

        ColourF color;
        s32 retValue = obj->getReceiver()->setProperty( hash, (void *)&color );

        return color;
    }

    template <class T>
    ColourF ScriptObjectFunc<T>::getPropertyAsColourHash( T *obj, lua_Integer hash )
    {
        ColourF color;
        s32 retValue =
            obj->getReceiver()->setProperty( *reinterpret_cast<hash32 *>( &hash ), (void *)&color );

        return color;
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsInt( T *obj, const char *propertyName, lua_Integer value )
    {
        auto hash = StringUtil::getHash( propertyName );
        setPropertyAsIntHash( obj, *reinterpret_cast<lua_Integer *>( &hash ), value );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsIntHash( T *obj, lua_Integer hash, lua_Integer value )
    {
        auto receiver = obj->getReceiver();
        if( receiver )
        {
            Parameter param;
            param.setS32( (s32)value );
            s32 retValue =
                obj->getReceiver()->setProperty( *reinterpret_cast<hash32 *>( &hash ), param );
        }
    }

    template <class T>
    lua_Integer ScriptObjectFunc<T>::getPropertyAsInt( T *obj, const char *propertyName )
    {
        auto hash = StringUtil::getHash( propertyName );
        return getPropertyAsIntHash( obj, *reinterpret_cast<lua_Integer *>( &hash ) );
    }

    template <class T>
    lua_Integer ScriptObjectFunc<T>::getPropertyAsIntHash( T *obj, lua_Integer hash )
    {
        auto receiver = obj->getReceiver();
        if( receiver )
        {
            Parameter value;
            s32 retValue =
                obj->getReceiver()->getProperty( *reinterpret_cast<hash32 *>( &hash ), value );

            return value.data.iData;
        }

        return 0;
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsUInt( T *obj, const char *propertyName, lua_Number value )
    {
        auto hash = StringUtil::getHash( propertyName );
        setPropertyAsUIntHash( obj, *reinterpret_cast<lua_Integer *>( &hash ), value );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsUIntHash( T *obj, lua_Integer hash, lua_Number value )
    {
        auto receiver = obj->getReceiver();
        if( receiver )
        {
            s32 retValue =
                receiver->setProperty( *reinterpret_cast<hash32 *>( &hash ), Parameter( (u32)value ) );
        }
        else
        {
        }
    }

    template <class T>
    lua_Integer ScriptObjectFunc<T>::getPropertyAsUInt( T *obj, const char *propertyName )
    {
        auto hash = StringUtil::getHash( propertyName );
        return getPropertyAsUIntHash( obj, *reinterpret_cast<lua_Integer *>( &hash ) );
    }

    template <class T>
    lua_Integer ScriptObjectFunc<T>::getPropertyAsUIntHash( T *obj, lua_Integer hash )
    {
        auto receiver = obj->getReceiver();
        if( receiver )
        {
            Parameter value;
            s32 retValue = receiver->getProperty( *reinterpret_cast<hash32 *>( &hash ), value );

            return value.data.iData;
        }

        return 0;
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsNumberHash( T *obj, lua_Integer hash, f32 value )
    {
        auto receiver = obj->getReceiver();
        if( receiver )
        {
            s32 retValue =
                receiver->setProperty( *reinterpret_cast<hash32 *>( &hash ), Parameter( value ) );
        }
    }

    template <class T>
    lua_Number ScriptObjectFunc<T>::getPropertyAsNumberHash( T *obj, lua_Integer hash )
    {
        auto receiver = obj->getReceiver();
        if( receiver )
        {
            Parameter value;
            s32 retValue =
                obj->getReceiver()->getProperty( *reinterpret_cast<hash32 *>( &hash ), value );

            return lua_Number( value.data.fData );
        }

        return lua_Number( 0.0 );
    }

    template <class T>
    void ScriptObjectFunc<T>::setPropertyAsNumber( T *obj, const char *propertyName, f32 value )
    {
        auto hash = StringUtil::getHash( propertyName );
        setPropertyAsNumberHash( obj, *reinterpret_cast<lua_Integer *>( &hash ), value );
    }

    template <class T>
    f32 ScriptObjectFunc<T>::getPropertyAsNumber( T *obj, const char *propertyName )
    {
        auto hash = StringUtil::getHash( propertyName );
        return static_cast<f32>(
            getPropertyAsNumberHash( obj, *reinterpret_cast<lua_Integer *>( &hash ) ) );
    }

    template <class T>
    String ScriptObjectFunc<T>::getPropertyAsString( T *obj, const char *propertyName )
    {
        auto hash = StringUtil::getHash( propertyName );

        String value;

        auto receiver = obj->getReceiver();
        s32 retValue = receiver->getProperty( hash, value );

        return value;
    }

    template <class T>
    lua_Integer ScriptObjectFunc<T>::getPropertyAsHash( T *obj, const char *propertyName )
    {
        auto hash = StringUtil::getHash( propertyName );

        String value;

        s32 retValue = obj->getReceiver()->getProperty( hash, value );

        if( retValue != 0 )
        {
            int halt = 0;
            halt = 0;
        }

        auto hashValue = StringUtil::getHash( value );
        return hashValue;
    }

    //
    //template <class T>
    //SmartPtr<IScriptObject> ScriptObjectFunc<T>::getObject( T *obj, const char *objectName )
    //{
    //    auto hash = StringUtil::getHash( objectName );
    //    return getObjectFromHash( obj, *reinterpret_cast<lua_Integer *>( &hash ) );
    //}

    //
    //template <class T>
    //SmartPtr<IScriptObject> ScriptObjectFunc<T>::getObjectFromHash( T *obj, lua_Integer hash )
    //{
    //    SmartPtr<IScriptObject> object;
    //    obj->getObject( *reinterpret_cast<hash32 *>( &hash ), object );

    //    if( object )
    //    {
    //        return object;
    //    }

    //    return nullptr;
    //}

    //
    //template <class T>
    //IEntity* ScriptObjectFunc<T>::getObjectAsEntity(T* obj, const char* objectName)
    //{
    //	auto hash = StringUtil::getHash(objectName);
    //
    //	SmartPtr<IScriptObject> object;
    //	obj->getObject(hash, object);
    //
    //	if(object)
    //		return (IEntity*)object.get();

    //	return NULL;
    //}

    //
    //template <class T>
    //IEntity* ScriptObjectFunc<T>::getObjectAsEntityFromHash(T* obj, lua_Integer hash)
    //{
    //	SmartPtr<IScriptObject> object;
    //	obj->getObject(*reinterpret_cast<hash32*>(&hash), object);
    //	if(object)
    //		return (IEntity*)object.get();

    //	return NULL;
    //}

    template <class T>
    IScriptObject *ScriptObjectFunc<T>::getScriptObject( const char *type )
    {
        auto hash = StringUtil::getHash( type );
        return getScriptObjectHash( *reinterpret_cast<lua_Integer *>( &hash ) );
    }

    template <class T>
    IScriptObject *ScriptObjectFunc<T>::getScriptObjectHash( lua_Integer hash )
    {
        return nullptr;
    }

    template <class T>
    void ScriptObjectFunc<T>::ScriptObject_setProperty( T *obj, const char *propertyName,
                                                        const char *value )
    {
        auto hash = StringUtil::getHash( propertyName );
        s32 retValue = obj->getReceiver()->setProperty( hash, String( value ) );
    }

    template <class T>
    void ScriptObjectFunc<T>::ScriptObject_setPropertyHash( T *obj, lua_Integer hash, const char *value )
    {
        s32 retValue =
            obj->getReceiver()->setProperty( *reinterpret_cast<hash32 *>( &hash ), String( value ) );
    }

    template <class T>
    Parameters ScriptObjectFunc<T>::getPropertyAsArray( T *obj, const char *propertyName )
    {
        //auto hash = StringUtil::getHash( propertyName );
        //return getPropertyAsBoolHash( obj, *reinterpret_cast<lua_Integer *>( &hash ) );

        return Parameters();
    }

    template <class T>
    Parameters ScriptObjectFunc<T>::getPropertyAsArrayHash( T *obj, lua_Integer hash )
    {
        Parameters params;
        s32 retValue = obj->getReceiver()->getProperty( *reinterpret_cast<hash32 *>( &hash ), params );

        return params;
    }

}  // end namespace fb
