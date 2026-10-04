#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Memory/IObject.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/Memory.hpp>
#include <Workphone/Memory/BaseObjectData.hpp>
#include <Workphone/Memory/SharedObjectTracker.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Handle.hpp>
#include <Workphone/System/DebugUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <boost/pool/singleton_pool.hpp>
#include <utility>

#if WP_ENABLE_MEMORY_TRACKER
#    include <Workphone/Memory/MemoryTracker.hpp>
#endif

namespace workphone
{

    WP_CLASS_REGISTER( workphone, IObject );

    Handle *IObject::getHandle()
    {
        return &m_objectData->m_handle;
    }

    const Handle *IObject::getHandle() const
    {
        return &m_objectData->m_handle;
    }

    bool IObject::isDerivedType( u32 type, u32 baseType )
    {
        auto typeManager = TypeManager::instance();
        WP_ASSERT( typeManager );
        return typeManager->isDerived( type, baseType );
    }

    bool IObject::isExactlyType( u32 type, u32 otherType )
    {
        auto typeManager = TypeManager::instance();
        WP_ASSERT( typeManager );
        return typeManager->isExactly( type, otherType );
    }

    IObject::IObject()
    {
        auto typeManager = TypeManager::instance();
        WP_ASSERT( typeManager );

        m_objectData = new( typeManager->baseObjectDataPool.allocate_object() ) BaseObjectData;
    }

    IObject::IObject( u32 typeId ) : m_poolTypeId( typeId )
    {
        auto typeManager = TypeManager::instance();
        WP_ASSERT( typeManager );

        if( m_poolTypeId != 0 )
        {
            m_objectData =
                new( typeManager->baseObjectDataPools[m_poolTypeId].allocate_object() ) BaseObjectData;
        }
        else
        {
            m_objectData = new( typeManager->baseObjectDataPool.allocate_object() ) BaseObjectData;
        }
    }

    IObject::~IObject()
    {
        auto typeManager = TypeManager::instance();
        if( typeManager )
        {
            if( auto data = m_objectData.load() )
            {
                WP_DELETE data->m_userData;
                data->m_userData = nullptr;

                data->~BaseObjectData();

                if( m_poolTypeId != 0 )
                {
                    typeManager->baseObjectDataPools[m_poolTypeId].free_object( data );
                }
                else
                {
                    typeManager->baseObjectDataPool.free_object( data );
                }

                m_objectData = nullptr;
                m_poolTypeId = 0;
            }
        }
    }

    void IObject::preUpdate()
    {
    }

    void IObject::update()
    {
    }

    void IObject::postUpdate()
    {
    }

    hash_type IObject::getId() const
    {
        return m_objectData->m_handle.getId();
    }

    void IObject::setId( hash_type id )
    {
        m_objectData->m_handle.setId( id );
    }

    const c8 *IObject::getNamePtr() const
    {
        return m_objectData->m_objectName.c_str();
    }

    String IObject::getName() const
    {
        return { m_objectData->m_objectName.c_str() };
    }

    void IObject::setName( const String &name )
    {
        m_objectData->m_objectName = name.c_str();
    }

    void IObject::setUserData( hash_type id, void *userData )
    {
        if( m_objectData )
        {
            if( !m_objectData->m_userData )
            {
                m_objectData->m_userData = WP_NEW UnorderedMap<hash_type, void *>();
            }

            ( *m_objectData->m_userData )[id] = userData;
        }
    }

    void IObject::setUserData( void *data )
    {
        if( m_objectData )
        {
            m_objectData->m_pUserData = data;
        }
    }

    auto IObject::isValid() const -> bool
    {
        return true;
    }

    auto IObject::getCreatorData() const -> void *
    {
        if( m_objectData )
        {
            return m_objectData->m_creatorData;
        }

        return nullptr;
    }

    void IObject::setCreatorData( void *data )
    {
        if( m_objectData )
        {
            m_objectData->m_creatorData = data;
        }
    }

    auto IObject::getFactoryData() const -> hash_type
    {
        if( m_objectData )
        {
            return m_objectData->m_factoryData;
        }

        return 0;
    }

    void IObject::setFactoryData( hash_type data )
    {
        if( m_objectData )
        {
            m_objectData->m_factoryData = data;
        }
    }

    auto IObject::toString() const -> String
    {
        return {};
    }

    auto IObject::getUserData( hash_type id ) const -> void *
    {
        if( m_objectData && m_objectData->m_userData )
        {
            auto it = m_objectData->m_userData->find( id );
            if( it != m_objectData->m_userData->end() )
            {
                return it->second;
            }
        }

        return nullptr;
    }

    auto IObject::getUserData() const -> void *
    {
        if( m_objectData )
        {
            return m_objectData->m_pUserData;
        }

        return nullptr;
    }

    auto IObject::derived( u32 type ) const -> bool
    {
        auto typeManager = TypeManager::instance();

        auto typeInfo = getTypeInfo();
        return typeManager->isDerived( typeInfo, type );
    }

    auto IObject::exactly( u32 type ) const -> bool
    {
        auto typeManager = TypeManager::instance();

        auto typeInfo = getTypeInfo();
        return typeManager->isExactly( typeInfo, type );
    }

    void IObject::setObjectFlags( u8 flags )
    {
        m_objectFlags = flags;
    }

    void IObject::setObjectFlag( u8 flag, bool value )
    {
        if( value )
        {
            m_objectFlags = m_objectFlags | flag;
        }
        else
        {
            m_objectFlags = m_objectFlags & ~flag;
        }
    }

#if WP_USE_CUSTOM_NEW_DELETE

    auto IObject::operator new( size_Num sz ) -> void *
    {
        void *ptr = nullptr;

#    if WP_OBJECT_ALIGNED_ALLOC
        ptr = Memory::ScalableAlignedMalloc( sz, WP_ALIGNMENT );
#    else
        ptr = Memory::malloc( sz );
#    endif

#    if WP_ENABLE_MEMORY_TRACKER
#        if WP_ARCH_TYPE == WP_ARCHITECTURE_64
#            if WP_ENABLE_TRACE
        const auto stack = DebugUtil::getStackTrace();
        MemoryTracker::get().recordAlloc( ptr, sz, 0, stack.c_str() );
#            else
        MemoryTracker::get().recordAlloc( ptr, sz, 0, "" );
#            endif
#        else
        MemoryTracker::get().recordAlloc( ptr, sz, 0, "" );
#        endif
#    endif

        return ptr;
    }

    auto IObject::operator new( size_Num sz, void *ptr ) -> void *
    {
        (void)sz;
        return ptr;
    }

    auto IObject::operator new( size_Num sz, [[maybe_unused]] const c8 *file, [[maybe_unused]] s32 line,
                                [[maybe_unused]] const c8 *func ) -> void *
    {
        void *ptr = nullptr;

#    if WP_OBJECT_ALIGNED_ALLOC
        ptr = Memory::ScalableAlignedMalloc( sz, WP_ALIGNMENT );
#    else
        ptr = Memory::malloc( sz );
#    endif

#    if WP_ENABLE_MEMORY_TRACKER
#        if WP_ARCH_TYPE == WP_ARCHITECTURE_64
#            if WP_ENABLE_TRACE
        const auto stack = DebugUtil::getStackTrace();
        MemoryTracker::get().recordAlloc( ptr, sz, 0, file, line, stack.c_str() );
#            else
        MemoryTracker::get().recordAlloc( ptr, sz, 0, file, line, "" );
#            endif
#        else
        MemoryTracker::get().recordAlloc( ptr, sz, 0, file, line, "" );
#        endif
#    endif

        return ptr;
    }

    auto IObject::operator new( size_Num sz, void *ptr, [[maybe_unused]] const c8 *file,
                                [[maybe_unused]] s32 line, [[maybe_unused]] const c8 *func ) -> void *
    {
        (void)sz;

#    if WP_ENABLE_MEMORY_TRACKER
#        if WP_ARCH_TYPE == WP_ARCHITECTURE_64
#            if WP_ENABLE_TRACE
        const auto stack = DebugUtil::getStackTrace();
        MemoryTracker::get().recordAlloc( ptr, sz, 0, file, line, stack.c_str() );
#            else
        MemoryTracker::get().recordAlloc( ptr, sz, 0, file, line, "" );
#            endif
#        else
        MemoryTracker::get().recordAlloc( ptr, sz, 0, file, line, "" );
#        endif
#    endif

        return ptr;
    }

    void *IObject::operator new( size_Num sz, std::align_val_t alignment, s32 line, const c8 *file,
                                 s32 func )
    {
        void *ptr = nullptr;

#    if WP_OBJECT_ALIGNED_ALLOC
        ptr = Memory::ScalableAlignedMalloc( sz, WP_ALIGNMENT );
#    else
        ptr = Memory::malloc( sz );
#    endif

#    if WP_ENABLE_MEMORY_TRACKER
#        if WP_ARCH_TYPE == WP_ARCHITECTURE_64
#            if WP_ENABLE_TRACE
        const auto stack = DebugUtil::getStackTrace();
        MemoryTracker::get().recordAlloc( ptr, sz, 0, file, line, stack.c_str() );
#            else
        MemoryTracker::get().recordAlloc( ptr, sz, 0, file, line, "" );
#            endif
#        else
        MemoryTracker::get().recordAlloc( ptr, sz, 0, file, line, "" );
#        endif
#    endif

        return ptr;
    }

    auto IObject::operator new[]( size_Num sz, [[maybe_unused]] const c8 *file,
                                  [[maybe_unused]] s32 line, [[maybe_unused]] const c8 *func ) -> void *
    {
        void *ptr = nullptr;

#    if WP_OBJECT_ALIGNED_ALLOC
        ptr = Memory::ScalableAlignedMalloc( sz, WP_ALIGNMENT );
#    else
        ptr = Memory::malloc( sz );
#    endif

#    if WP_ENABLE_MEMORY_TRACKER
        MemoryTracker::get().recordAlloc( ptr, sz, 0, file, line );
#    endif

        return ptr;
    }

    auto IObject::operator new[]( size_Num sz ) -> void *
    {
        void *ptr = nullptr;

#    if WP_OBJECT_ALIGNED_ALLOC
        ptr = Memory::ScalableAlignedMalloc( sz, WP_ALIGNMENT );
#    else
        ptr = Memory::malloc( sz );
#    endif

#    if WP_ENABLE_MEMORY_TRACKER
#        if WP_ARCH_TYPE == WP_ARCHITECTURE_64
#            if WP_ENABLE_TRACE
        const auto stack = DebugUtil::getStackTrace();
        MemoryTracker::get().recordAlloc( ptr, sz, 0, stack.c_str() );
#            else
        MemoryTracker::get().recordAlloc( ptr, sz, 0, "" );
#            endif
#        else
        MemoryTracker::get().recordAlloc( ptr, sz, 0, "" );
#        endif
#    endif
        return ptr;
    }

    void IObject::operator delete( void *ptr )
    {
#    if WP_OBJECT_ALIGNED_ALLOC
        Memory::ScalableAlignedFree( ptr );
#    else
        Memory::free( ptr );
#    endif

#    if WP_ENABLE_MEMORY_TRACKER
        MemoryTracker::get().recordDealloc( ptr );
#    endif
    }

    void IObject::operator delete( void *ptr, void * )
    {
#    if WP_OBJECT_ALIGNED_ALLOC
        Memory::ScalableAlignedFree( ptr );
#    else
        Memory::free( ptr );
#    endif

#    if WP_ENABLE_MEMORY_TRACKER
        MemoryTracker::get().recordDealloc( ptr );
#    endif
    }

    void IObject::operator delete[]( void *ptr )
    {
#    if WP_OBJECT_ALIGNED_ALLOC
        Memory::ScalableAlignedFree( ptr );
#    else
        Memory::free( ptr );
#    endif

#    if WP_ENABLE_MEMORY_TRACKER
        MemoryTracker::get().recordDealloc( ptr );
#    endif
    }

    void IObject::operator delete( void *ptr, [[maybe_unused]] const c8 *file, [[maybe_unused]] s32 line,
                                   [[maybe_unused]] const c8 *func ) noexcept
    {
#    if WP_OBJECT_ALIGNED_ALLOC
        Memory::ScalableAlignedFree( ptr );
#    else
        Memory::free( ptr );
#    endif

#    if WP_ENABLE_MEMORY_TRACKER
        MemoryTracker::get().recordDealloc( ptr );
#    endif
    }

    void IObject::operator delete( void *ptr, void *, const c8 *file, s32 line, const c8 *func )
    {
#    if WP_OBJECT_ALIGNED_ALLOC
        Memory::ScalableAlignedFree( ptr );
#    else
        Memory::free( ptr );
#    endif

#    if WP_ENABLE_MEMORY_TRACKER
        MemoryTracker::get().recordDealloc( ptr );
#    endif
    }

    void IObject::operator delete[]( void *ptr, const c8 *file, s32 line, const c8 *func ) noexcept
    {
#    if WP_OBJECT_ALIGNED_ALLOC
        Memory::ScalableAlignedFree( ptr );
#    else
        Memory::free( ptr );
#    endif

#    if WP_ENABLE_MEMORY_TRACKER
        MemoryTracker::get().recordDealloc( ptr );
#    endif
    }
#endif

#if !WP_FINAL
    String IObject::getDebugStr() const
    {
        return m_objectData->m_debugStr;
    }

    void IObject::setDebugStr( const String &debugStr )
    {
        m_objectData->m_debugStr = debugStr;
    }
#endif

}  // namespace workphone
