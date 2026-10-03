#ifndef IScriptData_h__
#define IScriptData_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** An interface for an object to store data used by the script system.
     */
    class WPCore_API IScriptData : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IScriptData() override;

        /** Gets the script object that owns the data.
        @return The script object instance.
        */
        virtual SmartPtr<ISharedObject> getOwner() const = 0;

        /** Sets the script object that owns the data.
        @param object The script object instance.
        */
        virtual void setOwner( SmartPtr<ISharedObject> object ) = 0;

        /** Gets object data. */
        virtual void *getObjectData() const = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IScriptData_h__
