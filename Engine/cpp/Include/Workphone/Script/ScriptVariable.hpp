#ifndef ScriptVariable_h__
#define ScriptVariable_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Script/IScriptVariable.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    /** ScriptVariable class. */
    class WPCore_API ScriptVariable : public IScriptVariable
    {
    public:
        /** Constructor. */
        ScriptVariable();

        /** Destructor. */
        ~ScriptVariable() override;

        /** Get the type of the variable. */
        ParameterType getType() const override;

        /** Set the type of the variable.
         * @param type The type of the variable.
         */
        void setType( ParameterType type ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /** The type of the variable. */
        ParameterType m_type;
    };

}  // namespace workphone

#endif  // ScriptVariable_h__
