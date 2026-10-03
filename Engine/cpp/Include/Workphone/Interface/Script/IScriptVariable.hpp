#ifndef IScriptVariable_h__
#define IScriptVariable_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @brief The IScriptVariable class is an abstract base class for a script variable.
     *
     * It inherits from the ISharedObject class and provides functions to retrieve and
     * set information about the script variable, such as its type and name.
     */
    class WPCore_API IScriptVariable : public ISharedObject
    {
    public:
        /**
         * @brief Destructor.
         */
        ~IScriptVariable() override;

        /**
         * @brief Gets the type of the script variable.
         * @return The type of the script variable.
         */
        virtual ParameterType getType() const = 0;

        /**
         * @brief Sets the type of the script variable.
         * @param type The type of the script variable.
         */
        virtual void setType( ParameterType type ) = 0;

        /**
         * @brief Macro used to register the class with the script system.
         *
         * This macro is used to register the class with the script system. It should be
         * placed in the class definition file.
         */
        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IScriptVariable_h__
