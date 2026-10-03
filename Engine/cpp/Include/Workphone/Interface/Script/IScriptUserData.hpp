#ifndef IScriptUserData_h__
#define IScriptUserData_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** Script user data interface. */
    class WPCore_API IScriptUserData : public ISharedObject
    {
    public:
        /** Destructor. */
        ~IScriptUserData() override;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IScriptUserData_h__
