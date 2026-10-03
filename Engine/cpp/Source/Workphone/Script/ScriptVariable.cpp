#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Script/ScriptVariable.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ScriptVariable, IScriptVariable );

    ScriptVariable::ScriptVariable() = default;

    ScriptVariable::~ScriptVariable() = default;

    auto ScriptVariable::getType() const -> ParameterType
    {
        return m_type;
    }

    void ScriptVariable::setType( ParameterType type )
    {
        m_type = type;
    }

}  // namespace workphone
