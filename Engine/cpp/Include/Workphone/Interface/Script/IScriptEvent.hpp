#ifndef IScriptEvent_h__
#define IScriptEvent_h__

#include <Workphone/Interface/System/IEvent.hpp>

namespace workphone
{

    /** A class used to store info for a scripted event. */
    class WPCore_API IScriptEvent : public IEvent
    {
    public:
        /** Destructor. */
        ~IScriptEvent() override;

        /** Gets the class name. */
        virtual String getClassName() const = 0;

        /** Sets the class name. */
        virtual void setClassName( const String &className ) = 0;

        /** Gets the function name. */
        virtual String getFunction() const = 0;

        /** Sets the function name. */
        virtual void setFunction( const String &function ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IScriptEvent_h__
