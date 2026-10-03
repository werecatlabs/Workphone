#ifndef __DebugCheck_h__
#define __DebugCheck_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Memory/RawPtr.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/List.hpp>
#include <Workphone/Core/Map.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{
    /** Used to check race conditions and heap corruption. */
    class WPCore_API DebugTrace : public ISharedObject
    {
    public:
        class DebugCheckFunction
        {
        public:
            DebugCheckFunction();
            explicit DebugCheckFunction( DebugTrace *debugCheck );
            ~DebugCheckFunction();

            RawPtr<DebugTrace> m_debugCheck;
        };

        explicit DebugTrace( bool bCheckHeap = false, bool bCheckGrowth = false );
        ~DebugTrace() override;

        bool getEnableConsoleOutput() const;
        void setEnableConsoleOutput( bool value );

        size_t getVirtualMemUsed() const;
        void setVirtualMemUsed( size_t value );

        size_t getPhysMemUsed() const;
        void setPhysMemUsed( size_t value );

        bool getCheckHeap() const;
        void setCheckHeap( bool value );

        bool getCheckGrowth() const;
        void setCheckGrowth( bool value );

        void handleFunctionStart();
        void handleFunctionEnd();

        String getSourceFile() const;
        void setSourceFile( const String &value );

        String getFunctionName() const;
        void setFunctionName( const String &value );

        s32 getLineNumber() const;
        void setLineNumber( s32 value );

        void writeTrace();

    protected:
        static void addDebugObject( RawPtr<DebugTrace> pDebugCheck );
        static void removeDebugObject( RawPtr<DebugTrace> pDebugCheck );

        bool m_bCheckHeap = false;
        bool m_bCheckGrowth = false;
        bool m_bEnableConsoleOutput = false;
        size_t m_virtualMemUsed = 0;
        size_t m_physMemUsed = 0;
        atomic_s32 m_currentTask = 0;
        String m_sourceFile;
        String m_functionName;
        s32 m_lineNumber = 0;
        f64 m_startTime = 0.0;
        f64 m_endTime = 0.0;

        static Map<String, List<RawPtr<DebugTrace>>> m_debugObjects;
        static RecursiveMutex m_debugObjectsMutex;
    };
}  // namespace workphone

#if WP_ENABLE_DEBUG_TRACE
#    if WP_ENABLE_HEAP_DEBUG
#        define WP_DEBUG_TRACE                          \
            DebugTrace debugCheck( true, false );       \
            debugCheck.setSourceFile( __FILE__ );       \
            debugCheck.setFunctionName( __FUNCTION__ ); \
            debugCheck.setLineNumber( __LINE__ );       \
            debugCheck.writeTrace();
#    else
#        define WP_DEBUG_TRACE                          \
            DebugTrace debugCheck( false, false );      \
            debugCheck.setSourceFile( __FILE__ );       \
            debugCheck.setFunctionName( __FUNCTION__ ); \
            debugCheck.setLineNumber( __LINE__ );       \
            debugCheck.writeTrace();
#    endif
#else
#    define WP_DEBUG_TRACE
#    define WP_DEBUG_TRACE_FUNCTION
#endif

#endif  // DebugCheck_h__
