#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/Factory.hpp>
#include <Workphone/Memory/Memory.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Memory/MemoryTracker.hpp>
#include <Workphone/Memory/SharedObjectData.hpp>
#include <Workphone/System/DebugUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, Factory, IFactory );
    WP_CLASS_REGISTER_DERIVED( workphone, Factory::Listener, ISharedObjectListener );

    Factory::Factory()
    {
#if WP_TRACK_REFERENCES
        setObjectFlag( OBJECT_FLAG_TRACK_REFERENCES, true );
#endif
    }

    Factory::~Factory() = default;

    void Factory::load( SmartPtr<ISharedObject> data )
    {
        auto listener = new Listener();
        listener->setFactory( this );
        m_listener = listener;
    }

    void Factory::unload( SmartPtr<ISharedObject> data )
    {
        freePoolData();

        if( m_listener )
        {
            m_listener->unload( nullptr );
            delete m_listener;
            m_listener = nullptr;
        }

        m_tags.clear();
    }

    u32 Factory::getGrowSize() const
    {
        return m_nextSize;
    }

    void Factory::setGrowSize( u32 size )
    {
        m_nextSize = size;
    }

    void Factory::allocatePoolData()
    {
    }

    void Factory::freePoolData()
    {
    }

    void *Factory::allocateMemory()
    {
        auto *ptr = Memory::ScalableAlignedMalloc( m_objectSize, WP_ALIGNMENT );
        if( !ptr )
        {
            std::terminate();
        }

#if WP_ENABLE_MEMORY_TRACKER
        const char *file = __FILE__;
        int line = __LINE__;
        const char *func = __FUNCTION__;

        const auto stack = DebugUtil::getStackTrace();
        MemoryTracker::get().recordAlloc( ptr, m_objectSize, 0, file, line, stack.c_str() );
#endif

        return ptr;
    }

    void Factory::freeMemory( void *ptr )
    {
        WP_ASSERT( ptr );
        if( !ptr )
        {
            return;
        }

        Memory::ScalableAlignedFree( ptr );

#if WP_ENABLE_MEMORY_TRACKER
        MemoryTracker::get().recordDealloc( ptr );
#endif
    }

    void *Factory::createObject()
    {
        auto *ptr = Memory::ScalableAlignedMalloc( m_objectSize, WP_ALIGNMENT );
        if( !ptr )
        {
            std::terminate();
        }

#if WP_ENABLE_MEMORY_TRACKER
        const char *file = __FILE__;
        int line = __LINE__;
        const char *func = __FUNCTION__;

        const auto stack = DebugUtil::getStackTrace();
        MemoryTracker::get().recordAlloc( ptr, m_objectSize, 0, file, line, stack.c_str() );
#endif

        return ptr;
    }

    void Factory::freeObject( void *object )
    {
        WP_ASSERT( object );
        if( !object )
        {
            return;
        }

        Memory::ScalableAlignedFree( object );

#if WP_ENABLE_MEMORY_TRACKER
        MemoryTracker::get().recordDealloc( object );
#endif
    }

    u32 Factory::getObjectSize() const
    {
        return m_objectSize;
    }

    void Factory::setObjectSize( u32 objectSize )
    {
        m_objectSize = objectSize;
    }

    ISharedObjectListener *Factory::getListener() const
    {
        return m_listener;
    }

    bool Factory::isObjectDerivedFromByInfo( u32 typeInfo ) const
    {
        if( typeInfo != 0 )
        {
            auto objectTypeInfo = getObjectTypeId();
            WP_ASSERT( objectTypeInfo != 0 );

            auto typeManager = TypeManager::instance();
            WP_ASSERT( typeManager );

            return typeManager->isDerived( objectTypeInfo, typeInfo );
        }

        return false;
    }

    const c8 *Factory::getTypeNamePtr() const
    {
        return m_typeName.c_str();
    }

    String Factory::getTypeName() const
    {
        return m_typeName;
    }

    void Factory::setTypeName( const String &typeName )
    {
        m_typeName = typeName;
    }

    void *Factory::createArray( u32 numElements )
    {
        if( numElements == 0 )
        {
            return nullptr;
        }

        auto totalSize = m_objectSize * numElements;
        auto *ptr = Memory::ScalableAlignedMalloc( totalSize, WP_ALIGNMENT );
        if( !ptr )
        {
            std::terminate();
        }

#if WP_ENABLE_MEMORY_TRACKER
        const char *file = __FILE__;
        int line = __LINE__;
        const char *func = __FUNCTION__;

        const auto stack = DebugUtil::getStackTrace();
        MemoryTracker::get().recordAlloc( ptr, totalSize, 0, file, line, stack.c_str() );
#endif

        return ptr;
    }

    const c8 *Factory::getObjectTypeNamePtr() const
    {
        return m_objectTypeName.c_str();
    }

    String Factory::getObjectTypeName() const
    {
        return m_objectTypeName;
    }

    void Factory::setObjectTypeName( const String &type )
    {
        m_objectTypeName = type;
    }

    hash_type Factory::getObjectTypeHash() const
    {
        return m_objectTypeHash;
    }

    void Factory::setObjectTypeHash( hash_type hash )
    {
        m_objectTypeHash = hash;
    }

    u32 Factory::getObjectTypeId() const
    {
        return m_objectTypeId;
    }

    void Factory::setObjectTypeId( u32 objectTypeId )
    {
        m_objectTypeId = objectTypeId;
    }

    s32 Factory::getMemoryUsed() const
    {
        // Calculate memory based on the number of instances and object size
        auto instances = m_instances.snapshot();
        return static_cast<s32>( instances.size() * m_objectSize );
    }

    Array<SmartPtr<ISharedObject>> Factory::getInstanceObjects() const
    {
        Array<SmartPtr<ISharedObject>> result;

        auto instances = m_instances.snapshot();
        result.reserve( instances.size() );

        for( auto instance : instances )
        {
            if( instance )
            {
                // Cast the raw pointer to ISharedObject and wrap it in a SmartPtr
                auto sharedObject = static_cast<ISharedObject *>( instance );
                result.push_back( sharedObject );
            }
        }

        return result;
    }

    Array<String> Factory::getTags() const
    {
        return m_tags.snapshot();
    }

    void Factory::setTags( const Array<String> &tags )
    {
        m_tags = { tags.begin(), tags.end() };
    }

    bool Factory::hasPool() const
    {
        return false;
    }

    SmartPtr<IFactoryManager> Factory::getFactoryManager() const
    {
        auto p = m_factoryManager.load();
        return p.lock();
    }

    void Factory::setFactoryManager( SmartPtr<IFactoryManager> factoryManager )
    {
        m_factoryManager = factoryManager;
    }

    void Factory::lock()
    {
        auto factoryManager = getFactoryManagerPtr();
        factoryManager->lock();
    }

    bool Factory::try_lock()
    {
        auto factoryManager = getFactoryManagerPtr();
        return factoryManager->try_lock();
    }

    void Factory::unlock()
    {
        auto factoryManager = getFactoryManagerPtr();
        factoryManager->unlock();
    }

    Factory::Listener::Listener() = default;

    Factory::Listener::Listener( Factory *factory ) : m_factory( factory )
    {
    }

    Factory::Listener::~Listener() = default;

    void Factory::Listener::unload( SmartPtr<ISharedObject> data )
    {
        m_factory = nullptr;
    }

    bool Factory::Listener::destroy( void *ptr )
    {
        //std::fprintf( stderr, "TRACE Factory::Listener::destroy this=%p object=%p factory-weak=%p\n",
        //              this, ptr, m_factory.get() );
        if( auto factory = getFactory() )
        {
            const auto objectType = factory->getObjectTypeName();
            //std::fprintf( stderr, "TRACE Factory::Listener::destroy locked factory=%p type=%s\n",
            //              factory.get(), objectType.c_str() );
            factory->freeObject( ptr );
            //std::fprintf( stderr, "TRACE Factory::Listener::destroy freeObject returned\n" );
            return true;
        }

        return false;
    }

    SmartPtr<Factory> Factory::Listener::getFactory() const
    {
        auto p = m_factory.load();
        return p.lock();
    }

    void Factory::Listener::setFactory( SmartPtr<Factory> factory )
    {
#if !WP_FINAL
        if( factory )
        {
            auto typeinfo = factory->getTypeInfo();
            auto typeManager = TypeManager::instance();
            if( auto typeName = typeManager->getName( typeinfo ) )
            {
                auto str = getDebugStr() + String( " " ) + typeName;
                setDebugStr( str );
            }
        }
#endif

        m_factory = factory;
    }
}  // namespace workphone
