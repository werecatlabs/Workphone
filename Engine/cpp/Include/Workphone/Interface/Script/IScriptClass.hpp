#ifndef IScriptClass_h__
#define IScriptClass_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @brief The IScriptClass class is an abstract base class for a script class.
     *
     * It inherits from the ISharedObject class and provides functions to retrieve and
     * set information about the script class, such as its name, namespaces, parent classes,
     * functions, and header/source file includes.
     */
    class WPCore_API IScriptClass : public ISharedObject
    {
    public:
        /**
         * @brief Destructor.
         */
        ~IScriptClass() override;

        /**
         * @brief Gets the name of the script class.
         * @return A string containing the class name.
         */
        virtual String getClassName() const = 0;

        /**
         * @brief Sets the name of the script class.
         * @param className A string containing the class name.
         */
        virtual void setClassName( const String &className ) = 0;

        /**
         * @brief Gets the namespaces of the script class.
         * @return An array of strings containing the namespace names.
         */
        virtual Array<String> getNamespaceNames() const = 0;

        /**
         * @brief Sets the namespaces of the script class.
         * @param namespaceNames An array of strings containing the namespace names.
         */
        virtual void setNamespaceNames( const Array<String> &namespaceNames ) = 0;

        /**
         * @brief Gets the functions of the script class.
         * @return An array of smart pointers to IScriptFunction objects.
         */
        virtual Array<SmartPtr<IScriptFunction>> getFunctions() const = 0;

        /**
         * @brief Sets the functions of the script class.
         * @param functions An array of smart pointers to IScriptFunction objects.
         */
        virtual void setFunctions( const Array<SmartPtr<IScriptFunction>> &functions ) = 0;

        /**
         * @brief Gets the parent classes of the script class.
         * @return An array of strings containing the parent class names.
         */
        virtual Array<String> getParentClasses() const = 0;

        /**
         * @brief Sets the parent classes of the script class.
         * @param parentClasses An array of strings containing the parent class names.
         */
        virtual void setParentClasses( const Array<String> &parentClasses ) = 0;

        /**
         * @brief Gets the header file includes of the script class.
         * @return An array of strings containing the header file names to be included.
         */
        virtual Array<String> getHeaderIncludes() const = 0;

        /**
         * @brief Sets the header file includes of the script class.
         * @param headers An array of strings containing the header file names to be included.
         */
        virtual void setHeaderIncludes( const Array<String> &headers ) = 0;

        /**
         * @brief Gets the source file includes of the script class.
         * @return An array of strings containing the source file names to be included.
         */
        virtual Array<String> getSourceIncludes() const = 0;

        /**
         * @brief Sets the source file includes of the script class.
         * @param headers An array of strings containing the source file names to be included.
         */
        virtual void setSourceIncludes( const Array<String> &headers ) = 0;

        /**
         * @brief Macro used to register the class with the script system.
         *
         * This macro is used to register the class with the script system. It should be
         * placed in the class definition file.
         */
        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IScriptClass_h__
