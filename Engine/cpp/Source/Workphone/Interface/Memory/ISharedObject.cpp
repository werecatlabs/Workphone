#include <Workphone/WorkphonePCH.hpp>
#include <limits>
#include <Workphone/Memory/ObjectObserver.hpp>
#include <mutex>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Memory/ISharedObjectListener.hpp>
#include <Workphone/Interface/System/IEditorManager.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/Handle.hpp>
#include <Workphone/Core/Pool.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Memory/BaseObjectData.hpp>
#include <Workphone/Memory/SharedObjectData.hpp>
#include <Workphone/Memory/Memory.hpp>
#include <Workphone/Memory/SharedObjectTracker.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <Workphone/Thread/Thread.hpp>

#if WP_TRACK_REFERENCES
#    include <Workphone/Memory/SharedObjectTracker.hpp>
#endif

namespace workphone
{
    namespace
    {
        struct ObservedLifetimes
        {
            using Tokens = SourceBudgetMap<const ISharedObject *, std::shared_ptr<const void>>;
            Tokens tokens;
        };

        struct ObserverRegistryState
        {
            std::mutex mutex;
            SourceBudgetUnique<ObservedLifetimes> registry;
        };

        ObserverRegistryState &observerRegistryState() noexcept
        {
            // Static synchronization survives late factory destruction without a
            // permanent heap allocation. All dynamic registry/token storage is
            // lazy and admitted; expiration must never construct that storage.
            alignas(ObserverRegistryState) static unsigned char storage[sizeof(ObserverRegistryState)];
            static auto state = new(storage) ObserverRegistryState;
            return *state;
        }
    }  // namespace

    std::weak_ptr<const void> observeObjectLifetime( ISharedObject *object )
    {
        if( !object || !object->isAlive() || object->getReferences() <= 0 )
        {
            return {};
        }
        auto &state = observerRegistryState();
        std::lock_guard<std::mutex> lock(state.mutex);
        if(state.registry)
        {
            auto found = state.registry->tokens.find(object);
            if(found != state.registry->tokens.end()) return found->second;
            auto token = std::allocate_shared<const int>(SourceBudgetAllocator<const int>{}, 0);
            state.registry->tokens.emplace(object, token);
            return token;
        }

        auto registry = makeSourceBudgetUnique<ObservedLifetimes>();
        auto token = std::allocate_shared<const int>(SourceBudgetAllocator<const int>{}, 0);
        registry->tokens.emplace(object, token);
        state.registry = std::move(registry);
        return token;
    }

    void expireObjectLifetime( const ISharedObject *object ) noexcept
    {
        auto &state = observerRegistryState();
        SourceBudgetUnique<ObservedLifetimes> retired;
        ObservedLifetimes::Tokens::node_type expired;
        {
            std::lock_guard<std::mutex> lock(state.mutex);
            if(!state.registry) return;
            expired = state.registry->tokens.extract(object);
            if(state.registry->tokens.empty()) retired = std::move(state.registry);
        }
        // Retire nodes/control storage outside the registry lock. Expired weak
        // observers keep their token control allocation charged until released.
    }

    namespace
    {
        constexpr s32 destroyingReferenceCount = std::numeric_limits<s32>::min();

        bool compareExchangeReferences( atomic_s32 &references, s32 &expected, s32 desired )
        {
#if WP_USE_STD_ATOMIC
            return references.compare_exchange_strong( expected, desired );
#else
            return references.compareExchange( expected, desired );
#endif
        }

        bool releaseReferenceForDestroy( atomic_s32 &references )
        {
            auto currentReferences = references.load();

            for( ;; )
            {
                WP_ASSERT( currentReferences > 0 );
                if( currentReferences <= 0 )
                {
                    return false;
                }

                const auto desiredReferences =
                    currentReferences == 1 ? destroyingReferenceCount : currentReferences - 1;
                auto expectedReferences = currentReferences;
                if( compareExchangeReferences( references, expectedReferences, desiredReferences ) )
                {
                    return currentReferences == 1;
                }

                currentReferences = expectedReferences;
            }
        }

    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone, ISharedObject, IObject );

    ISharedObjectListener *ISharedObject::getSharedObjectListener() const
    {
        return m_sharedObjectData->m_sharedObjectListener;
    }

    u32 ISharedObject::getEventTaskFlags() const
    {
        return m_sharedObjectData->m_eventTaskFlags;
    }

    ISharedObject *ISharedObject::getScriptDataPtr() const
    {
        return m_sharedObjectData->m_scriptData.get();
    }

    SmartPtr<ISharedObject> ISharedObject::getScriptData() const
    {
        auto p = m_sharedObjectData->m_scriptData.load();
        return p.lock();
    }

    u32 ISharedObject::getNumListeners() const
    {
        auto listeners = getObjectListenerSnapshot();
        return listeners ? static_cast<u32>(listeners->size()) : 0;
    }

    ISharedObject::ScopedLoadstateWait::ScopedLoadstateWait( ISharedObject *object ) : m_object( object )
    {
        if( m_object )
        {
            while( m_object->isLoadLocked() )
            {
                Thread::yield();
            }
        }
    }

    const Array<String> ISharedObject::loadingStateNames = { "None",     "Allocated", "Unallocated",
                                                             "QueuedGC", "Unloading", "Unloaded",
                                                             "Loading",  "Loaded",    "LoadingQueued",
                                                             "Error" };

    const String ISharedObject::loadedStr = String( "loaded" );
    const String ISharedObject::loadingStateStr = String( "loadingState" );
    const String ISharedObject::referencesStr = String( "references" );
    const String ISharedObject::weakReferencesStr = String( "weakReferences" );
    const String ISharedObject::typeNameStr = String( "ISharedObjectTypeName" );

    ISharedObject::ISharedObject() : IObject()
    {
#if !WP_FINAL
        constexpr auto basesize = sizeof( IObject );
        constexpr auto size = sizeof( ISharedObject );
        constexpr auto baseObjectData = sizeof( BaseObjectData );
        constexpr auto sharedObjectData = sizeof( SharedObjectData );
        constexpr auto handleSize = sizeof( Handle );
#endif

        auto typeManager = TypeManager::instance();

        m_sharedObjectData = new( typeManager->sharedObjectDataPool.allocate_object() ) SharedObjectData;

#if WP_TRACK_REFERENCES
        setObjectFlag( OBJECT_FLAG_TRACK_REFERENCES, false );

        auto &objectTracker = SharedObjectTracker::instance();
        m_referenceData = objectTracker.getObjectData( this );
#endif
    }

    ISharedObject::ISharedObject( u32 typeId ) : IObject( typeId )
    {
        auto typeManager = TypeManager::instance();
        WP_ASSERT( typeManager );

        if( m_poolTypeId != 0 )
        {
            m_sharedObjectData =
                new( typeManager->sharedObjectDataPools[m_poolTypeId].allocate_object() )
                    SharedObjectData;
        }
        else
        {
            m_sharedObjectData =
                new( typeManager->sharedObjectDataPool.allocate_object() ) SharedObjectData;
        }

#if WP_TRACK_REFERENCES
        setObjectFlag( OBJECT_FLAG_TRACK_REFERENCES, false );

        auto &objectTracker = SharedObjectTracker::instance();
        m_referenceData = objectTracker.getObjectData( this );
#endif
    }

    ISharedObject::~ISharedObject()
    {
        expireObjectLifetime( this );
        //WP_ASSERT( m_references == 0 );
        //WP_ASSERT( m_weakReferences == 0 );

        // Factories may destroy pooled objects directly during factory teardown,
        // bypassing destroySharedObject(). Keep weak pointers from treating that
        // still-addressable pool storage as a live object while their owners are
        // being torn down later.
        setObjectFlag( OBJECT_FLAG_ALIVE, false );

        m_loadingState = LoadingState::None;

        auto typeManager = TypeManager::instance();
        if( typeManager )
        {
            if( auto data = m_sharedObjectData.load() )
            {
                data->~SharedObjectData();

                if( m_poolTypeId != 0 )
                {
                    typeManager->sharedObjectDataPools[m_poolTypeId].free_object( data );
                }
                else
                {
                    typeManager->sharedObjectDataPool.free_object( data );
                }

                m_sharedObjectData = nullptr;
            }
        }
    }

    auto ISharedObject::addWeakReference( void *address, const c8 *file, u32 line, const c8 *func )
        -> s32
    {
#if WP_TRACK_REFERENCES
#    if WP_TRACK_WEAK_REFERENCES
        auto &objectTracker = SharedObjectTracker::instance();
        objectTracker.addReference( this, address, file, line, func );
#    endif
#endif

        return ++m_weakReferences;
    }

    auto ISharedObject::removeWeakReference() -> bool
    {
#if WP_TRACK_REFERENCES
#    if WP_TRACK_WEAK_REFERENCES
        auto address = (void *)this;
        const c8 *file = __FILE__;
        const u32 line = __LINE__;
        const c8 *func = __FUNCTION__;

        auto &objectTracker = SharedObjectTracker::instance();
        objectTracker.removeReference( this, address, file, line, func );
#    endif
#endif

        // Decrement the weak reference count and return true if the count is now 0.
        return --m_weakReferences == 0;
    }

    auto ISharedObject::removeWeakReference( void *address, const c8 *file, u32 line, const c8 *func )
        -> bool
    {
#if WP_TRACK_REFERENCES
#    if WP_TRACK_WEAK_REFERENCES
        // If reference tracking is enabled, remove the weak reference tracking information.
        auto &objectTracker = SharedObjectTracker::instance();
        objectTracker.removeReference( this, address, file, line, func );
#    endif
#endif

        // Decrement the weak reference count and return true if the count is now 0.
        return --m_weakReferences == 0;
    }

    auto ISharedObject::removeReference( void *address, const c8 *file, const u32 line, const c8 *func )
        -> bool
    {
#if WP_TRACK_REFERENCES
        auto &objectTracker = SharedObjectTracker::instance();
        objectTracker.removeReference( this, address, file, line, func );
#endif

        if( releaseReferenceForDestroy( m_references ) )
        {
            destroySharedObject();
            return true;
        }

        return false;
    }

    void ISharedObject::destroySharedObject()
    {
        expireObjectLifetime( this );
        WP_ASSERT( m_references <= 0 );
        //WP_ASSERT( m_weakReferences <= 0 );

        setObjectFlag( OBJECT_FLAG_ALIVE, false );

        bool destroyed = false;

        if( auto sharedObjectData = m_sharedObjectData.load() )
        {
            auto sharedObjectListener = sharedObjectData->m_sharedObjectListener.load();
            if( sharedObjectListener )
            {
                sharedObjectData->m_sharedObjectListener = nullptr;
                destroyed = sharedObjectListener->destroy( this );
            }
        }

        if( !destroyed )
        {
            // Use a qualified (non-virtual) call to bypass virtual dispatch.
            // destroySharedObject() may be invoked from within a destructor chain
            // (e.g. via AtomicSmartPtr cleanup in FactoryTemplate::unload), at which
            // point the vtable has been wound back to IObject — a class that does not
            // declare isPoolElement(). A virtual call would read past the end of
            // IObject::vftable and jump to a garbage address, causing an access violation.
            // ISharedObject::isPoolElement() is a force-inlined flag read and is always safe.
            if( !ISharedObject::isPoolElement() )
            {
                delete this;
            }
        }
    }

    auto ISharedObject::addReference( void *address, const c8 *file, const u32 line, const c8 *func )
        -> s32
    {
#if WP_TRACK_REFERENCES
        // WP_ASSERT( isGarbageCollected() && references > 0 );
        WP_ASSERT( m_references < 1e10 );

#    if WP_TRACK_STRONG_REFERENCES
        auto &objectTracker = SharedObjectTracker::instance();
        objectTracker.addReference( this, address, file, line, func );
#    endif

        return ++m_references;
#else
        return ++m_references;
#endif
    }

    bool ISharedObject::removeReference()
    {
#if WP_TRACK_REFERENCES
#    if WP_TRACK_STRONG_REFERENCES
        auto address = (void *)this;
        const c8 *file = __FILE__;
        const u32 line = __LINE__;
        const c8 *func = __FUNCTION__;

        auto &objectTracker = SharedObjectTracker::instance();
        objectTracker.removeReference( this, address, file, line, func );
#    endif
#endif

        if( releaseReferenceForDestroy( m_references ) )
        {
            destroySharedObject();
            return true;
        }

        return false;
    }

    void ISharedObject::load( SmartPtr<ISharedObject> data )
    {
    }

    void ISharedObject::reload( SmartPtr<ISharedObject> data )
    {
    }

    void ISharedObject::unload( SmartPtr<ISharedObject> data )
    {
        WP_ASSERT( isLoadLocked() == false );
        setScriptData( nullptr );
    }

    void ISharedObject::setLoadingState( LoadingState state )
    {
        auto triggerEvents = getObjectFlags();
        if( ( triggerEvents & OBJECT_FLAG_TRIGGER_EVENTS ) != 0 &&
            ( triggerEvents & OBJECT_FLAG_LOADING_EVENTS ) != 0 )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( applicationManager )
            {
                if( applicationManager->isLoaded() )
                {
                    auto oldState = getLoadingState();
                    if( oldState != state )
                    {
                        m_loadingState = state;

                        auto args = makeSourceBudgetSnapshot<Parameter>();
                        args.reserve( 3 );
                        args.resize( 3 );

                        args[0] = Parameter( static_cast<s32>( oldState ) );
                        args[1] = Parameter( static_cast<s32>( state ) );
                        args[2] = Parameter( (void *)this );

                        SmartPtr<ISharedObject> pThis;
                        if( isAlive() )
                        {
                            pThis = getSharedFromThis<ISharedObject>();
                        }

                        auto eventTaskFlags = getEventTaskFlags();

                        applicationManager->triggerEvent( EventType::Loading,
                                                          IEvent::loadingStateChanged, args, pThis,
                                                          pThis, nullptr, false, eventTaskFlags );
                    }
                }
                else
                {
                    m_loadingState = state;
                }
            }
            else
            {
                m_loadingState = state;
            }
        }
        else
        {
            m_loadingState = state;
        }
    }

    auto ISharedObject::isThreadSafe() const -> bool
    {
        return false;
    }

    void ISharedObject::setPoolElement( bool poolElement )
    {
        if( poolElement )
        {
            m_objectFlags = m_objectFlags | OBJECT_FLAG_POOL_ELEMENT;
        }
        else
        {
            m_objectFlags = m_objectFlags & ~OBJECT_FLAG_POOL_ELEMENT;
        }
    }

    auto ISharedObject::toData() const -> SmartPtr<ISharedObject>
    {
        auto properties = getProperties();

        auto typeManager = TypeManager::instance();
        auto type = getTypeInfo();

        auto pTypeName = typeManager->getName( type );
        auto typeName = pTypeName ? String( pTypeName ) : String();
        properties->setProperty( typeNameStr, typeName );

        return properties;
    }

    void ISharedObject::fromData( SmartPtr<ISharedObject> data )
    {
        if( data )
        {
            if( data->isDerived<Properties>() )
            {
                const auto properties = workphone::static_pointer_cast<Properties>( data );
                setProperties( properties );
            }
        }
        else
        {
            WP_LOG_ERROR( "Data is null in ISharedObject::fromData" );
        }
    }

    String ISharedObject::toString() const
    {
        auto data = toData();
        return DataUtil::toString( data.get(), true, DataFormat::JSON );
    }

    auto ISharedObject::getProperties() const -> SmartPtr<Properties>
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto factoryManager = applicationManager->getFactoryManagerPtr();
        auto editorManager = applicationManager->getEditorManagerPtr();

        auto properties = factoryManager->make_ptr<Properties>();

        if( editorManager )
        {
            if( editorManager->getShowDebug() )
            {
                auto references = getReferences();
                auto weakReferences = getWeakReferences();
                auto loadingState = getLoadingState();
                auto iLoadingState = static_cast<s32>( loadingState );

                properties->setProperty( referencesStr, references );
                properties->setProperty( weakReferencesStr, weakReferences );
                properties->setPropertyAsEnum( loadingStateStr, iLoadingState, loadingStateNames );
            }
        }

        return properties;
    }

    void ISharedObject::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        // Reference counts are runtime ownership data. They may be exposed by
        // getProperties() for diagnostics, but deserializing them can destroy an
        // object while setProperties() is still executing.
        auto iLoadingState = static_cast<s32>( getLoadingState() );
        if( properties->getPropertyValue( loadingStateStr, iLoadingState ) && iLoadingState >= 0 &&
            iLoadingState < static_cast<s32>( LoadingState::Count ) )
        {
            const auto loadingState = static_cast<LoadingState>( iLoadingState );
            if( m_loadingState == loadingState )
            {
                return;
            }

            if( loadingState == LoadingState::Loaded )
            {
                load( nullptr );
            }
            else if( loadingState == LoadingState::Unloaded )
            {
                unload( nullptr );
            }

            m_loadingState = loadingState;
        }
    }

    auto ISharedObject::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        return {};
    }

    auto ISharedObject::getObjectListeners() const -> Array<SmartPtr<IEventListener>>
    {
        auto listeners = getObjectListenerSnapshot();
        return listeners ? *listeners : makeSourceBudgetSnapshot<SmartPtr<IEventListener>>();
    }

    ISharedObject::ObjectListenerSnapshot ISharedObject::getObjectListenerSnapshot() const
    {
        // Serialize acquisition with exclusive-generation edits. Once retained,
        // readers need no lock and prevent that generation from being modified.
        ScopedLock lock(&m_sharedObjectData->m_eventListenerMutex);
        return std::atomic_load(&m_sharedObjectData->m_sharedEventListeners);
    }

    void ISharedObject::getObjectListenersList( SmartPtr<IEventListener> *listeners, u32 *count,
                                                u32 maxCount ) const
    {
        auto sharedListeners = getObjectListenerSnapshot();
        u32 actualCount = sharedListeners ?
            std::min(static_cast<u32>(sharedListeners->size()), maxCount) : 0;

        for( u32 i = 0; i < actualCount; ++i )
        {
            listeners[i] = (*sharedListeners)[i];
        }

        if( count )
        {
            *count = actualCount;
        }
    }

    auto ISharedObject::hasObjectListener( SmartPtr<IEventListener> listener ) const -> bool
    {
        auto listeners = getObjectListenerSnapshot();
        return listeners && std::find(listeners->begin(), listeners->end(), listener) != listeners->end();
    }

    void ISharedObject::addObjectListener( SmartPtr<IEventListener> listener )
    {
        WP_ASSERT( listener != nullptr );

        if(!listener) return;
        ObjectListenerSnapshot previous;
        {
            ScopedLock lock(&m_sharedObjectData->m_eventListenerMutex);
            previous = getObjectListenerSnapshot();
            if(previous && std::find(previous->begin(), previous->end(), listener) != previous->end()) return;
            auto next = previous ? *previous : makeSourceBudgetSnapshot<SmartPtr<IEventListener>>();
            next.push_back(listener);
            ObjectListenerSnapshot published = std::allocate_shared<Array<SmartPtr<IEventListener>>>(
                SourceBudgetAllocator<Array<SmartPtr<IEventListener>>>{}, std::move(next));
            std::atomic_store(&m_sharedObjectData->m_sharedEventListeners, std::move(published));
        }
    }

    void ISharedObject::removeObjectListener( SmartPtr<IEventListener> listener )
    {
        ObjectListenerSnapshot previous;
        {
            ScopedLock lock(&m_sharedObjectData->m_eventListenerMutex);
            previous = getObjectListenerSnapshot();
            if(!previous || std::find(previous->begin(), previous->end(), listener) == previous->end()) return;
            if(previous.use_count() == 2)
            {
                // Only the inventory and this local pin own the generation.
                // There is no retained reader, so removal can reuse capacity
                // at the cap. The allocation was constructed as a mutable Array.
                auto &current = const_cast<Array<SmartPtr<IEventListener>> &>(*previous);
                current.erase(std::remove(current.begin(), current.end(), listener), current.end());
                if(current.empty())
                    std::atomic_store(&m_sharedObjectData->m_sharedEventListeners, ObjectListenerSnapshot{});
                return;
            }
            ObjectListenerSnapshot published;
            if(previous->size() > 1)
            {
                auto next = *previous;
                next.erase(std::remove(next.begin(), next.end(), listener), next.end());
                published = std::allocate_shared<Array<SmartPtr<IEventListener>>>(
                    SourceBudgetAllocator<Array<SmartPtr<IEventListener>>>{}, std::move(next));
            }
            std::atomic_store(&m_sharedObjectData->m_sharedEventListeners, std::move(published));
        }
    }

    void ISharedObject::removeObjectListeners()
    {
        ObjectListenerSnapshot previous;
        {
            ScopedLock lock(&m_sharedObjectData->m_eventListenerMutex);
            previous = std::atomic_exchange(&m_sharedObjectData->m_sharedEventListeners, ObjectListenerSnapshot{});
            m_sharedObjectData->m_sharedEventListener = nullptr;
        }
    }

    auto ISharedObject::findObjectListener( const String &name ) const -> SmartPtr<IEventListener>
    {
        auto listeners = getObjectListenerSnapshot();
        if(!listeners) return nullptr;
        for( auto &listener : *listeners )
        {
            if( listener )
            {
                if( listener->getNamePtr() == name )
                {
                    return listener;
                }
            }
        }

        return nullptr;
    }

    void ISharedObject::setSharedObjectListener( ISharedObjectListener *listener )
    {
        m_sharedObjectData->m_sharedObjectListener = listener;
    }

    void ISharedObject::setGarbageCollected( bool garbageCollected )
    {
        if( garbageCollected )
        {
            m_objectFlags.fetch_or( OBJECT_FLAG_GARBAGE_COLLECTED );
        }
        else
        {
            m_objectFlags.fetch_and( ~OBJECT_FLAG_GARBAGE_COLLECTED );
        }
    }

    void ISharedObject::lock()
    {
    }

    bool ISharedObject::try_lock()
    {
        return false;
    }

    void ISharedObject::unlock()
    {
    }

    void ISharedObject::lock_shared()
    {
    }

    void ISharedObject::unlock_shared()
    {
    }

    void ISharedObject::setScriptData( SmartPtr<ISharedObject> data )
    {
        m_sharedObjectData->m_scriptData = data;
    }

    void ISharedObject::setEventTaskFlags( u32 eventTaskFlags )
    {
        m_sharedObjectData->m_eventTaskFlags = eventTaskFlags;
    }

#if WP_TRACK_REFERENCES
    s32 ISharedObject::addReference()
    {
#    if WP_TRACK_REFERENCES
#        if WP_TRACK_STRONG_REFERENCES
        // WP_ASSERT( isGarbageCollected() && m_references > 0 );
        WP_ASSERT( m_references < 1e10 );

        auto address = (void *)this;
        const c8 *file = __FILE__;
        const u32 line = __LINE__;
        const c8 *func = __FUNCTION__;

        auto &objectTracker = SharedObjectTracker::instance();
        objectTracker.addReference( this, address, file, line, func );

        return ++m_references;
#        else
        return ++m_references;
#        endif
#    else
        return ++m_references;
#    endif
    }

#endif

}  // namespace workphone
