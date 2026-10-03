#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Script/ScriptFunction.hpp>
#include <Workphone/Script/ScriptVariable.hpp>
#include <Workphone/Core/ParameterUtil.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ScriptFunction, IScriptFunction );

    ScriptFunction::ScriptFunction() = default;

    ScriptFunction::~ScriptFunction() = default;

    auto ScriptFunction::getClassName() const -> String
    {
        return m_className;
    }

    void ScriptFunction::setClassName( const String &className )
    {
        m_className = className;
    }

    auto ScriptFunction::getFunctionName() const -> String
    {
        return m_functionName;
    }

    void ScriptFunction::setFunctionName( const String &functionName )
    {
        m_functionName = functionName;
    }

    auto ScriptFunction::getReturnType() const -> ParameterType
    {
        return m_returnType;
    }

    void ScriptFunction::setReturnType( ParameterType returnType )
    {
        m_returnType = returnType;
    }

    auto ScriptFunction::getArguments() const -> Array<SmartPtr<IScriptVariable>>
    {
        return m_arguments;
    }

    void ScriptFunction::setArguments( const Array<SmartPtr<IScriptVariable>> &arguments )
    {
        m_arguments = arguments;
    }

    auto ScriptFunction::isConstructor() const -> bool
    {
        return m_isConstructor;
    }

    void ScriptFunction::setConstructor( bool constructor )
    {
        m_isConstructor = constructor;
    }

    auto ScriptFunction::isDestructor() const -> bool
    {
        return m_isDestructor;
    }

    void ScriptFunction::setDestructor( bool destructor )
    {
        m_isDestructor = destructor;
    }

    auto ScriptFunction::getFunctionBodyCPP() const -> String
    {
        if( m_returnType != ParameterType::PARAM_TYPE_VOID )
        {
            return "return " + ParameterUtil::getParamTypeAsString( m_returnType ) + "();";
        }

        return "";
    }
}  // namespace workphone
