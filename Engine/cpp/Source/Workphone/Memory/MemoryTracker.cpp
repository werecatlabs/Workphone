#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Memory/MemoryTracker.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Thread/ScopedLock.hpp>
#include <Workphone/WorkphoneConfig.hpp>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

#if WP_ENABLE_TRACE
#    include <boost/stacktrace.hpp>
#endif

#ifdef WP_PLATFORM_WIN32
#    include <windows.h>
#    define FBOutputCString( str ) ::OutputDebugStringA( str )
#    define FBOutputWString( str ) ::OutputDebugStringW( str )
#else
#    define FBOutputCString( str ) std::cerr << str
#    define FBOutputWString( str ) std::cerr << str
#endif

namespace workphone
{
#if WP_ENABLE_MEMORY_TRACKER

    MemoryTracker::MemoryTracker() :
        m_fileName( "MemoryLeaks.log" ),
        m_dumpToStdOut( true ),
        m_totalAllocations( 0 ),
        m_recordEnable( true )
    {
    }

    MemoryTracker::~MemoryTracker()
    {
        reportLeaks();
    }

    bool MemoryTracker::getRecordEnable() const
    {
        return m_recordEnable;
    }

    MemoryTracker &MemoryTracker::get()
    {
        static MemoryTracker tracker;
        return tracker;
    }

    void MemoryTracker::recordAlloc( void *ptr, u32 sz, unsigned int pool, const char *file, u32 ln,
                                     const char *func )
    {
        if( m_recordEnable )
        {
            if( !ptr )
            {
                return;
            }

            ScopedLock allocationsLock( &m_allocations, true );
            ScopedLock poolsLock( &m_allocationsByPool, true );

            auto existing = m_allocations.find( ptr );
            WP_ASSERT( existing == m_allocations.end() &&
                       "Double allocation with same address - "
                       "this probably means you have a mismatched allocation / deallocation style" );
            if( existing != m_allocations.end() )
            {
                const auto existingPool = existing->second.pool;
                const auto existingBytes = static_cast<u32>( existing->second.bytes );
                if( existingPool < m_allocationsByPool.size() )
                {
                    m_allocationsByPool[existingPool] -= existingBytes;
                }

                m_totalAllocations -= existingBytes;
                m_allocations.erase( existing );
            }

            m_allocations[ptr] = Alloc( sz, pool, file, ln, func );
            if( pool >= m_allocationsByPool.size() )
                m_allocationsByPool.resize( pool + 1, 0 );
            m_allocationsByPool[pool] += sz;
            m_totalAllocations += sz;
        }
    }

    void MemoryTracker::recordDealloc( void *ptr )
    {
        if( m_recordEnable )
        {
            // deal cleanly with null pointers
            if( !ptr )
            {
                return;
            }

            ScopedLock allocationsLock( &m_allocations, true );
            ScopedLock poolsLock( &m_allocationsByPool, true );

            auto i = m_allocations.find( ptr );
            WP_ASSERT( i != m_allocations.end() &&
                       "Unable to locate allocation unit - "
                       "this probably means you have a mismatched allocation / deallocation style, "
                       "check if you're are using OGRE_ALLOC_T / OGRE_FREE and OGRE_NEW_T / "
                       "OGRE_DELETE_T consistently" );
            if( i == m_allocations.end() )
            {
                return;
            }

            // update category stats
            const auto pool = i->second.pool;
            const auto bytes = static_cast<u32>( i->second.bytes );
            WP_ASSERT( pool < m_allocationsByPool.size() );
            if( pool < m_allocationsByPool.size() )
            {
                m_allocationsByPool[pool] -= bytes;
            }

            // global stats
            m_totalAllocations -= bytes;
            m_allocations.erase( i );
        }
    }

    void MemoryTracker::setRecordEnable( bool recordEnable )
    {
        m_recordEnable = recordEnable;
    }

    String MemoryTracker::getReportFileName() const
    {
        return m_fileName;
    }

    void MemoryTracker::setReportToStdOut( bool rep )
    {
        m_dumpToStdOut = rep;
    }

    bool MemoryTracker::getReportToStdOut() const
    {
        return m_dumpToStdOut;
    }

    u32 MemoryTracker::getTotalMemoryAllocated() const
    {
        return m_totalAllocations;
    }

    u32 MemoryTracker::getMemoryAllocatedForPool( u32 pool ) const
    {
        ScopedLock poolsLock( &m_allocationsByPool, false );

        if( pool >= m_allocationsByPool.size() )
        {
            return 0;
        }

        return m_allocationsByPool[pool];
    }

    void MemoryTracker::reportLeaks()
    {
        if( m_recordEnable )
        {
            ScopedLock allocationsLock( &m_allocations, false );

            std::stringstream os;

            if( m_allocations.empty() )
            {
                os << "Memory: No memory leaks" << std::endl;
            }
            else
            {
                os << "Memory: Detected memory leaks !!! " << std::endl;
                os << "Memory: (" << m_allocations.size() << ") Allocation(s) with total "
                   << m_totalAllocations << " bytes." << std::endl;
                os << "Memory: Dumping allocations -> " << std::endl;

                for( auto i = m_allocations.begin(); i != m_allocations.end(); ++i )
                {
                    os << std::endl;

                    const Alloc &alloc = i->second;
                    if( !alloc.filename.empty() )
                        os << alloc.filename;
                    else
                        os << "(unknown source):";

                    os << "(" << alloc.line << ") : {" << alloc.bytes << " bytes}"
                       << " function: " << alloc.function << std::endl;
                    os << std::endl;
                }
                os << std::endl;
            }

            if( m_dumpToStdOut )
            {
                std::cout << os.str();
            }

            std::ofstream of;

            auto fileName = getReportFileName();
            of.open( fileName.c_str() );
            of << os.str();
            of.close();

            FBOutputCString( os.str().c_str() );
        }
    }

    void MemoryTracker::setReportFileName( const String &name )
    {
        m_fileName = name;
    }

    MemoryTracker::Alloc::Alloc( u32 sz, u32 p, const c8 *file, u32 ln, const c8 *func ) :
        bytes( sz ),
        pool( p ),
        line( ln )
    {
        if( file )
        {
            filename = file;
        }

        if( func )
        {
            function = func;
        }
    }

    MemoryTracker::Alloc::Alloc() : bytes( 0 ), line( 0 )
    {
    }

#endif
}  // namespace workphone
