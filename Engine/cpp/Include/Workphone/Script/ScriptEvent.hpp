#ifndef ScriptEvent_h__
#define ScriptEvent_h__

#include <Workphone/Interface/Script/IScriptEvent.hpp>

namespace workphone
{

    /** A class used to store info for a scripted event. */
    class WPCore_API ScriptEvent : public IScriptEvent
    {
    public:
        /** Constructor. */
        ScriptEvent();

        /** Constructor. */
        explicit ScriptEvent( const String &function );

        /** Destructor. */
        ~ScriptEvent() override;

        /** Gets the type of event. */
        hash_type getEventType() const;

        /** Sets the type of event. */
        void setEventType( hash_type type );

        /** Gets the class name. */
        String getClassName() const override;

        /** Sets the class name. */
        void setClassName( const String &className ) override;

        /** Gets the function name. */
        String getFunction() const override;

        /** sets the function name. */
        void setFunction( const String &function ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// The type of event.
        hash_type m_hashType = 0;

        /// The name of the script class.
        FixedString<WP_MAX_CLASSNAME> m_className;

        /// The name of the function.
        FixedString<WP_MAX_FUNCNAME> m_function;
    };
}  // namespace workphone

#endif  // ScriptEvent_h__
