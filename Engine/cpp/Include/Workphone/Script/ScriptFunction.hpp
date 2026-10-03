#ifndef ScriptFunction_h__
#define ScriptFunction_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Script/IScriptFunction.hpp>

namespace workphone
{

    /** Script function implementation. */
    class WPCore_API ScriptFunction : public IScriptFunction
    {
    public:
        /** Constructor. */
        ScriptFunction();

        /** Destructor. */
        ~ScriptFunction() override;

        /** Get the class name. */
        String getClassName() const override;

        /** Set the class name.
         * @param className The class name.
         */
        void setClassName( const String &className ) override;

        /** Get the function name. */
        String getFunctionName() const override;

        /** Set the function name.
         * @param functionName The function name.
         */
        void setFunctionName( const String &functionName ) override;

        /** Get the return type. */
        ParameterType getReturnType() const override;

        /** Set the return type.
         * @param returnType The return type.
         */
        void setReturnType( ParameterType returnType ) override;

        /** Get the arguments. */
        Array<SmartPtr<IScriptVariable>> getArguments() const override;

        /** Set the arguments.
         * @param arguments The arguments.
         */
        void setArguments( const Array<SmartPtr<IScriptVariable>> &arguments ) override;

        /** Get to know if the function is a constructor. */
        bool isConstructor() const override;

        /** Set the function as a constructor.
         * @param constructor True if the function is a constructor.
         */
        void setConstructor( bool constructor ) override;

        /** Get to know if the function is a destructor. */
        bool isDestructor() const override;

        /** Set the function as a destructor.
         * @param destructor True if the function is a destructor.
         */
        void setDestructor( bool destructor ) override;

        /** Get the function body in C++ format. */
        String getFunctionBodyCPP() const;

        WP_CLASS_REGISTER_DECL;

    protected:
        /** The function is a constructor. */
        bool m_isConstructor = false;

        /** The function is a destructor. */
        bool m_isDestructor = false;

        /** The return type. */
        ParameterType m_returnType;

        /** The class name. */
        FixedString<WP_MAX_CLASSNAME> m_className;

        /** The function name. */
        FixedString<WP_MAX_FUNCNAME> m_functionName;

        /** The arguments. */
        Array<SmartPtr<IScriptVariable>> m_arguments;
    };
}  // namespace workphone

#endif  // ScriptFunction_h__
