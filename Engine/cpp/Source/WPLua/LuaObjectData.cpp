#include <WPLua/LuaObjectData.hpp>
#include <WPLua/LuaManager.hpp>
#include <Workphone/Workphone.hpp>
#include <luabind/detail/class_rep.hpp>
#include <luabind/detail/object_rep.hpp>
#include <luabind/detail/stack_utils.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, LuaObjectData, IScriptData );

    LuaObjectData::LuaObjectData() = default;

    LuaObjectData::~LuaObjectData() = default;

    void LuaObjectData::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        createClassData();
        setLoadingState( LoadingState::Loaded );
    }

    void LuaObjectData::unload( SmartPtr<ISharedObject> data )
    {
        using namespace luabind;

        setLoadingState( LoadingState::Unloading );

        if( auto owner = getOwner() )
        {
            owner->setScriptData( nullptr );
        }

        m_owner = nullptr;
        // Release registry references while their state is still alive, even if
        // external callers retain this ScriptData past manager shutdown.
        m_object = luabind::object();
        m_classData = nullptr;
        m_luaState = nullptr;

        setLoadingState( LoadingState::Unloaded );
    }

    void LuaObjectData::setOwner( SmartPtr<ISharedObject> owner )
    {
        m_owner = owner;
    }

    SmartPtr<ISharedObject> LuaObjectData::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void *LuaObjectData::getObjectData() const
    {
        return (void *)m_object;
    }

    String LuaObjectData::getClassName() const
    {
        return String( m_className.c_str() );
    }

    String LuaObjectData::getClassNameStr() const
    {
        return String( m_className.begin(), m_className.end() );
    }

    void LuaObjectData::setClassName( const String &className )
    {
        m_className = className.c_str();
    }

    IScriptClass *LuaObjectData::getClassDataPtr() const
    {
        return m_classData.get();
    }

    SmartPtr<IScriptClass> LuaObjectData::getClassData() const
    {
        return m_classData;
    }

    void LuaObjectData::setClassData( SmartPtr<IScriptClass> classData )
    {
        m_classData = classData;
    }

    bool LuaObjectData::hasMemberFunction( const String &functionName ) const
    {
        auto classData = getClassData();
        if( classData )
        {
            auto functions = classData->getFunctions();
            for( auto function : functions )
            {
                if( function->getFunctionName() == functionName )
                {
                    return true;
                }
            }
        }

        return false;
    }

    String LuaObjectData::toString() const
    {
        std::stringstream stream;
        stream << "m_className: " << m_className.c_str() << std::endl;
        return stream.str().c_str();
    }

    luabind::object &LuaObjectData::getObject()
    {
        return m_object;
    }

    void LuaObjectData::setObject( luabind::object &object )
    {
        m_object = object;
    }

    lua_State *LuaObjectData::getLuaState() const
    {
        return m_luaState;
    }

    void LuaObjectData::setLuaState( lua_State *state )
    {
        m_luaState = state;
    }

    void LuaObjectData::createClassData()
    {
        using namespace luabind;

        auto classData = make_ptr<ScriptClass>();
        classData->load( nullptr );
        classData->setClassName( getClassName() );

        auto L = getLuaState();
        if( L )
        {
            m_object.push( L );

            luabind::detail::class_rep *crep = nullptr;

            if( luabind::detail::is_class_rep( L, -1 ) )
            {
                crep = static_cast<luabind::detail::class_rep *>( lua_touserdata( L, -1 ) );
                lua_pop( L, 1 );
            }
            else
            {
                luabind::detail::object_rep *obj = luabind::detail::get_instance( L, -1 );
                lua_pop( L, 1 );

                if( obj )
                {
                    crep = obj->crep();
                }
            }

            if( crep )
            {
                crep->get_table( L );
                object table( from_stack( L, -1 ) );

                Array<SmartPtr<IScriptFunction>> functions;

                for( iterator i( table ), e; i != e; ++i )
                {
                    if( type( *i ) != LUA_TFUNCTION )
                        continue;

                    object member( *i );
                    member.push( L );
                    luabind::detail::stack_pop pop( L, 1 );

                    if( lua_tocfunction( L, -1 ) == &luabind::detail::property_tag )
                        continue;

                    auto key = i.key();
                    auto functionName = object_cast<std::string>( key );

                    auto scriptFunction = make_ptr<ScriptFunction>();
                    scriptFunction->setFunctionName( functionName.c_str() );
                    scriptFunction->setClassName( getClassName() );

                    functions.push_back( scriptFunction );
                }

                classData->setFunctions( functions );

                lua_pop( L, 1 );
            }
        }

        m_classData = classData;
    }

}  // namespace workphone
