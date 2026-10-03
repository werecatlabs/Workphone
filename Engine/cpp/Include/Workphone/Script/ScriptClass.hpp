#ifndef ScriptClass_h__
#define ScriptClass_h__

#include <Workphone/Interface/Script/IScriptClass.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    /** ScriptClass implementation. */
    class WPCore_API ScriptClass : public IScriptClass
    {
    public:
        /** Constructor. */
        ScriptClass();

        /** Destructor. */
        ~ScriptClass() override;

        /** Get the class name. */
        String getClassName() const override;

        /** Set the class name. */
        void setClassName( const String &className ) override;

        /** Get the namespace names. */
        Array<String> getNamespaceNames() const override;

        /** Set the namespace names. */
        void setNamespaceNames( const Array<String> &namespaceNames ) override;

        /** Get the functions. */
        Array<SmartPtr<IScriptFunction>> getFunctions() const override;

        /** Set the functions. */
        void setFunctions( const Array<SmartPtr<IScriptFunction>> &functions ) override;

        /** Get the parent classes. */
        Array<String> getParentClasses() const override;

        /** Set the parent classes. */
        void setParentClasses( const Array<String> &parentClasses ) override;

        /** Get the header includes. */
        Array<String> getHeaderIncludes() const override;

        /** Set the header includes. */
        void setHeaderIncludes( const Array<String> &headers ) override;

        /** Get the source includes. */
        Array<String> getSourceIncludes() const override;

        /** Set the source includes. */
        void setSourceIncludes( const Array<String> &headers ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /** Class name. */
        FixedString<WP_MAX_CLASSNAME> m_className;

        /** Namespace names. */
        Array<String> m_namespaceNames;

        /** Parent classes. */
        Array<String> m_parentClasses;

        /** Header includes. */
        Array<String> m_headerIncludes;

        /** Source includes. */
        Array<String> m_sourceIncludes;

        /** Functions. */
        Array<SmartPtr<IScriptFunction>> m_functions;
    };
}  // namespace workphone

#endif  // ScriptClass_h__
