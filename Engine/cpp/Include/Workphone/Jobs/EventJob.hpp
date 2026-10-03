#ifndef EventJob_h__
#define EventJob_h__

#include <Workphone/WorkphoneEnums.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/Interface/System/IEvent.hpp>
#include <Workphone/Core/Parameter.hpp>

namespace workphone
{

    /**
     * @file EventJob.hpp
     * @brief Job used to dispatch an IEvent to a target object or owner.
     *
     * The EventJob packages an event together with optional sender, target object,
     * owner context and arguments so it can be executed later by a job system.
     *
     * Thread-safety:
     * - Public setters/getters are lightweight and the internal state uses atomic
     *   smart pointers and atomic primitives so instances can be prepared on one
     *   thread and executed on another.
     *
     * Usage:
     * - Set the event (or event type/value and arguments), sender and target object,
     *   then enqueue this job with the job system. The job's execute() will dispatch
     *   the event to the configured recipient.
     *
     * @see Job, IEvent
     */
    class WPCore_API EventJob : public Job
    {
    public:
        /**
         * @brief Construct a new EventJob.
         *
         * Initializes members to default values. The job may be configured via
         * the provided setters prior to execution.
         */
        EventJob();

        /**
         * @brief Destroy the EventJob.
         *
         * Releases any held references. Destructor is virtual via override.
         */
        ~EventJob() override;

        /**
         * @brief Load the job with shared object data.
         * @param data Shared object data used to configure the job (may be null).
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unload the job, releasing all held object references.
         *
         * Clears m_event, m_object, m_sender and m_owner before delegating to
         * Job::unload(). This ensures no dangling AtomicSmartPtr references remain
         * when the job is destroyed by the factory pool during shutdown.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Execute the job.
         *
         * Implementation should dispatch the configured event to the target object
         * or owner using the stored sender, event, event type/value and arguments.
         *
         * This overrides Job::execute().
         */
        void execute() override;

        /**
         * @brief Get the target object for the event.
         * @return SmartPtr<ISharedObject> The object that will receive the event (may be null).
         */
        SmartPtr<ISharedObject> getObject() const;

        /**
         * @brief Set the target object for the event.
         * @param object The object that should receive the event.
         */
        void setObject( SmartPtr<ISharedObject> object );

        /**
         * @brief Get the sender of the event.
         * @return SmartPtr<ISharedObject> The object that originated the event (may be null).
         */
        SmartPtr<ISharedObject> getSender() const;

        /**
         * @brief Set the sender of the event.
         * @param sender The object that originated the event.
         */
        void setSender( SmartPtr<ISharedObject> sender );

        /**
         * @brief Get the owner state context for the event.
         * @return IStateContext* Pointer to the owner context (may be null/expired).
         */
        IStateContext *getOwner() const;

        /**
         * @brief Set the owner state context for the event.
         * @param owner The owner context that should receive the event or be used during dispatch.
         */
        void setOwner( IStateContext *owner );

        /**
         * @brief Get the event type.
         * @return EventType The event type enum.
         */
        EventType getEventType() const;

        /**
         * @brief Set the event type.
         * @param eventType The event type to dispatch when no full IEvent is provided.
         */
        void setEventType( EventType eventType );

        /**
         * @brief Get the numeric event value (hash).
         * @return hash_type The event value (often used as an identifier or parameter).
         */
        hash_type getEventValue() const;

        /**
         * @brief Set the numeric event value (hash).
         * @param eventValue Numeric value associated with the event.
         */
        void setEventValue( hash_type eventValue );

        /**
         * @brief Get the argument list for the event.
         * @return Array<Parameter> Copy of the arguments array (may be empty).
         */
        Array<Parameter> getArguments() const;

        /**
         * @brief Set the argument list for the event.
         * @param arguments The arguments to attach to the event.
         */
        void setArguments( const Array<Parameter> &arguments );

        /**
         * @brief Get the IEvent instance to dispatch.
         *
         * If an IEvent instance is provided it will typically take precedence
         * over eventType/eventValue when dispatching.
         *
         * @return SmartPtr<IEvent> The event instance (may be null).
         */
        SmartPtr<IEvent> getEvent() const;

        /**
         * @brief Set the IEvent instance to dispatch.
         * @param event The event instance to dispatch when the job runs.
         */
        void setEvent( SmartPtr<IEvent> event );

        WP_CLASS_REGISTER_DECL;

    private:
        /** @brief Event instance to dispatch. May be null. */
        AtomicSmartPtr<IEvent> m_event;

        /** @brief Target object that should receive the event. */
        AtomicSmartPtr<ISharedObject> m_object;

        /** @brief Sender/originator of the event. */
        AtomicSmartPtr<ISharedObject> m_sender;

        /** @brief Weak reference to the owner state context for the event. */
        AtomicSmartPtr<IStateContext> m_owner;

        /** @brief Event type used when no full IEvent is provided. Defaults to Object. */
        AtomicValue<EventType> m_eventType = EventType::Object;

        /** @brief Numeric event value (hash/identifier). */
        atomic_s64 m_eventValue = 0;

        /** @brief Optional arguments passed with the event. */
        ConcurrentArray<Parameter> m_arguments;
    };

}  // namespace workphone

#endif  // EventJob_h__
