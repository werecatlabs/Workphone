#ifndef IScriptInvoker_h__
#define IScriptInvoker_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Parameter.hpp>

namespace workphone
{

    /** This class is used to call script functions. */
    class WPCore_API IScriptInvoker : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IScriptInvoker() override;

        /** The object that owns the invoker. */
        virtual ISharedObject *getOwnerPtr() const = 0;

        /** The object that owns the invoker. */
        virtual ISharedObject *getOwner() const = 0;

        /** The object that owns the invoker. */
        virtual void setOwner( ISharedObject *owner ) = 0;

        /** Calls an object member. */
        virtual void callObjectMember( const String &functionName ) = 0;

        /** Calls an object member. */
        virtual void callObjectMember( const String &functionName, const Parameters &params ) = 0;

        /** Calls an object member. */
        virtual void callObjectMember( const String &functionName, const Parameters &params,
                                       Parameters &results ) = 0;

        /** Calls a script event with the given hash id. */
        virtual void event( hash_type hash ) = 0;

        /** Calls a script event with the given hash id. */
        virtual void event( hash_type hash, const Parameters &params ) = 0;

        /** Calls a script event with the given hash id. */
        virtual void event( hash_type hash, const Parameters &params, Parameters &results ) = 0;

        /** Used to know if the invoker has an event. */
        virtual bool hasEvent( hash_type hash ) const = 0;

        /** Sets a script event with the given hash id. */
        virtual void setEventFunction( hash_type hash, SmartPtr<IEvent> event ) = 0;

        /** Gets a script event with a given hash id. */
        virtual SmartPtr<IEvent> getEventFunction( hash_type hash ) const = 0;

        /** Gets the number of events. */
        virtual u32 getNumEvents() const = 0;

        //
        // Helper functions
        //

        /** Calls a script function to set parameter. */
        virtual void set( hash_type hash, const Parameter &param ) = 0;

        /** Calls a script function to set parameter. */
        virtual void set( const String &id, const Parameter &param ) = 0;

        /** Calls a script function to get parameter. */
        virtual Parameter get( hash_type hash ) = 0;

        /** Calls a script function to get parameter. */
        virtual Parameter get( const String &id ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IScriptInvoker_h__
