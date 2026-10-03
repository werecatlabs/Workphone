#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Script/ScriptEvent.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ScriptEvent, IScriptEvent );

    ScriptEvent::ScriptEvent() = default;

    ScriptEvent::ScriptEvent( const String &function ) : m_function( function )
    {
    }

    ScriptEvent::~ScriptEvent() = default;

    auto ScriptEvent::getEventType() const -> hash_type
    {
        return m_hashType;
    }

    void ScriptEvent::setEventType( hash_type type )
    {
        m_hashType = type;
    }

    auto ScriptEvent::getClassName() const -> String
    {
        return m_className;
    }
    void ScriptEvent::setClassName( const String &className )
    {
        m_className = className;
    }

    auto ScriptEvent::getFunction() const -> String
    {
        return m_function;
    }

    void ScriptEvent::setFunction( const String &function )
    {
        m_function = function;
    }
}  // namespace workphone
