#ifndef ScriptInvokerStandard_h__
#define ScriptInvokerStandard_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Script/IScriptInvoker.hpp>
#include <Workphone/Core/HashMap.hpp>

namespace workphone
{

    /** Implementation of IScriptInvoker the interface. */
    class WPCore_API ScriptInvoker : public IScriptInvoker
    {
    public:
        /** Constructor. */
        ScriptInvoker();

        /** Constructor. */
        explicit ScriptInvoker( SmartPtr<ISharedObject> scriptObject );

        /** Destructor. */
        ~ScriptInvoker() override;

        /** @copydoc IScriptInvoker::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc IScriptInvoker::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc IScriptInvoker::getOwnerPtr */
        ISharedObject *getOwnerPtr() const override;

        /** @copydoc IScriptInvoker::getOwner */
        ISharedObject *getOwner() const override;

        /** @copydoc IScriptInvoker::setOwner */
        void setOwner( ISharedObject *owner ) override;

        /** @copydoc IScriptInvoker::callObjectMember */
        void callObjectMember( const String &functionName ) override;

        /** @copydoc IScriptInvoker::callObjectMember */
        void callObjectMember( const String &functionName, const Parameters &params ) override;

        /** @copydoc IScriptInvoker::callObjectMember */
        void callObjectMember( const String &functionName, const Parameters &params,
                               Parameters &results ) override;

        /** @copydoc IScriptInvoker::event */
        void event( hash_type hash ) override;

        /** @copydoc IScriptInvoker::event */
        void event( hash_type hash, const Parameters &params ) override;

        /** @copydoc IScriptInvoker::event */
        void event( hash_type hash, const Parameters &params, Parameters &results ) override;

        /** @copydoc IScriptInvoker::hasEvent */
        bool hasEvent( hash_type hash ) const override;

        /** @copydoc IScriptInvoker::setEventFunction */
        void setEventFunction( hash_type hash, SmartPtr<IEvent> event ) override;

        /** @copydoc IScriptInvoker::getEventFunction */
        SmartPtr<IEvent> getEventFunction( hash_type hash ) const override;

        /** @copydoc IScriptInvoker::getNumEvents */
        u32 getNumEvents() const override;

        /** @copydoc IScriptInvoker::set */
        void set( hash_type hash, const Parameter &param ) override;

        /** @copydoc IScriptInvoker::set */
        void set( const String &id, const Parameter &param ) override;

        /** @copydoc IScriptInvoker::get */
        Parameter get( hash_type hash ) override;

        /** @copydoc IScriptInvoker::get */
        Parameter get( const String &id ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// The owner object.
        AtomicWeakPtr<ISharedObject> m_object;

        /// A type definition for an event hash map.
        using Events = HashMap<hash_type, SmartPtr<IScriptEvent>>;

        /// Stores the events.
        Events m_events;
    };
}  // namespace workphone

#endif  // ScriptInvokerStandard_h__
