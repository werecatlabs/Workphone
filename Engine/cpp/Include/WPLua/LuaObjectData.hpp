#ifndef LuaObjectData_h__
#define LuaObjectData_h__

#include <WPLua/WPLuaPrerequisites.hpp>
#include <Workphone/Interface/Script/IScriptData.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <luabind/luabind.hpp>

namespace workphone
{

    /**
     * @brief Wrapper for script-backed object data stored in Lua.
     *
     * LuaObjectData implements the IScriptData interface and holds all
     * information required to represent a script object bound to a C++
     * ISharedObject. It stores the luabind::object instance, an optional
     * lua_State pointer (when using a single state), a cached script class
     * descriptor and the owning object as a weak pointer.
     *
     * This class is responsible for creating or populating the associated
     * IScriptClass metadata (createClassData) and for forwarding lifecycle
     * operations (load/unload) invoked by the engine.
     */
    class LuaObjectData : public IScriptData
    {
    public:
        /**
         * @brief Construct a new LuaObjectData
         *
         * Initializes members to sensible defaults. The stored lua_State
         * and luabind::object remain empty until set via setLuaState/
         * setObject or load().
         */
        LuaObjectData();

        /**
         * @brief Destroy the LuaObjectData
         *
         * Releases references to script objects and any held metadata.
         */
        ~LuaObjectData() override;

        /** @copydoc IScriptData::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc IScriptData::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @copydoc IScriptData::setOwner
         *
         * The owner is stored as an AtomicWeakPtr to avoid keeping the
         * shared object alive solely because of its script data.
         */
        void setOwner( SmartPtr<ISharedObject> owner ) override;

        /** @copydoc IScriptData::getOwner */
        SmartPtr<ISharedObject> getOwner() const override;

        /** @copydoc IScriptData::getObjectData */
        void *getObjectData() const override;

        /**
         * @brief Get the script-side class name for the object.
         *
         * Returns a copyable String identifying the Lua class this object
         * is bound to. Useful for logging and reflection.
         */
        String getClassName() const;

        /**
         * @brief Get the script class name as a raw string.
         *
         * Convenience accessor that returns the underlying string data.
         */
        String getClassNameStr() const;

        /** @brief Set the script class name. */
        void setClassName( const String &className );

        /** @brief Get a raw pointer to the cached IScriptClass metadata. */
        IScriptClass *getClassDataPtr() const;

        /** @brief Get the cached IScriptClass metadata as a SmartPtr. */
        SmartPtr<IScriptClass> getClassData() const;

        /** @brief Set or replace the cached IScriptClass metadata. */
        void setClassData( SmartPtr<IScriptClass> classData );

        /**
         * @brief Check whether the script object exposes a member function.
         *
         * @param functionName Name of the function to check for.
         * @return true if the function exists on the object or its class,
         *         false otherwise.
         */
        bool hasMemberFunction( const String &functionName ) const;

        /** @copydoc IScriptData::toString */
        String toString() const override;

        /**
         * @brief Access the underlying luabind::object representing the
         *        script-side instance.
         *
         * Note: the returned reference may be empty if no object has been
         * set.
         */
        luabind::object &getObject();

        /**
         * @brief Store a luabind::object for this script object.
         *
         * The provided object is copied into this instance. Typical usage
         * is by the script binding code when associating a Lua instance
         * with a C++ object.
         */
        void setObject( luabind::object &object );

        /** @brief Get the lua_State pointer associated with this object. */
        lua_State *getLuaState() const;

        /** @brief Set the lua_State pointer associated with this object. */
        void setLuaState( lua_State *state );

        WP_CLASS_REGISTER_DECL;

    protected:
        // Create class data from the script. Has  a list of members and functions.
        void createClassData();

        /** The owner of this data. */
        AtomicWeakPtr<ISharedObject> m_owner;

        SmartPtr<IScriptClass> m_classData;

        lua_State *m_luaState = nullptr;

        /** Object if using a single lua state. */
        luabind::object m_object;

        /** The script class name. */
        FixedString<32> m_className;
    };

}  // namespace workphone

#endif  // LuaObjectData_h__
