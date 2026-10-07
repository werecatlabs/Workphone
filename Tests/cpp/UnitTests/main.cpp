#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include "UnitTestsFixture.hpp"
#include <cstdlib>
#include <memory>

#ifdef WP_PLATFORM_WIN32
#    define _CRTDBG_MAP_ALLOC
#    include <stdlib.h>
#    include <crtdbg.h>
#    include <dbghelp.h>
#    pragma comment( lib, "dbghelp.lib" )
#endif

#ifdef min
#    undef min
#endif

#ifdef max
#    undef max
#endif

#ifdef nil
#    undef nil
//#define nil 0
#endif

#define BOOST_TEST_ALTERNATIVE_INIT_API
#define BOOST_TEST_NO_MAIN
//#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE WP_Tests
#include <boost/test/included/unit_test.hpp>

using namespace workphone;

#include <iostream>
#include <boost/mpl/string.hpp>

struct LogToFile
{
    LogToFile()
    {
        std::string logFileName( boost::unit_test::framework::master_test_suite().p_name );
        logFileName.append( ".log" );
        logFile.open( logFileName.c_str() );
        boost::unit_test::unit_test_log.set_stream( logFile );
    }

    ~LogToFile()
    {
        boost::unit_test::unit_test_log.test_finish();
        logFile.close();
        boost::unit_test::unit_test_log.set_stream( std::cout );
    }

    std::ofstream logFile;
};

BOOST_GLOBAL_FIXTURE( LogToFile );

template <class a_mpl_string>
struct A
{
    static const char *string;
};

template <class a_mpl_string>
const char *A<a_mpl_string>::string{ boost::mpl::c_str<a_mpl_string>::value };  // boost compatible

// typedef A< MPL_STRING("any string as template argument") > a_string_type;

struct verify;

class tester
{
public:
    static int livecount;
    const tester *self;

    tester() : self( this )
    {
        ++livecount;
    }

    tester( const tester & ) : self( this )
    {
        ++livecount;
    }

    ~tester()
    {
        assert( self == this );
        --livecount;
    }

    tester &operator=( const tester &b )
    {
        assert( self == this && b.self == &b );
        return *this;
    }

    void cfunction() const
    {
        assert( self == this );
    }

    void mfunction()
    {
        assert( self == this );
    }
};

int tester::livecount = 0;

struct verify
{
    ~verify()
    {
        assert( tester::livecount == 0 );
    }
} verifier;

namespace
{
    std::unique_ptr<TypeManager> g_unitTestTypeManager;

    void configureDebugCrtForUnitTests()
    {
#ifdef WP_PLATFORM_WIN32
        _putenv_s( "BOOST_TEST_DETECT_MEMORY_LEAK", "0" );

        auto flags = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
        flags |= _CRTDBG_ALLOC_MEM_DF;
        if( const char *leakCheck = std::getenv( "WP_TEST_CRT_LEAK_CHECK" );
            leakCheck && leakCheck[0] == '1' )
        {
            flags |= _CRTDBG_LEAK_CHECK_DF;
        }
        _CrtSetDbgFlag( flags );
        _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG );
        _CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDERR );
        _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG );
        _CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDERR );
        _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG );
        _CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR );
#endif
    }

    void cleanupUnitTestTypeManager()
    {
        if( auto typeManager = TypeManager::instance() )
        {
            TypeManager::setInstance( nullptr );
        }
        UnitTests::sTypeManager = nullptr;
        g_unitTestTypeManager.reset();
    }

    void ensureUnitTestTypeManager()
    {
        auto typeManager = TypeManager::instance();
        if( !typeManager )
        {
            g_unitTestTypeManager = std::make_unique<TypeManager>();
            g_unitTestTypeManager->load();
            TypeManager::setInstance( g_unitTestTypeManager.get() );
            UnitTests::sTypeManager = g_unitTestTypeManager.get();
            std::atexit( cleanupUnitTestTypeManager );
        }
    }

    int runUnitTests( boost::unit_test::init_unit_test_func initFunc, int argc, char *argv[] )
    {
        namespace utf = boost::unit_test;

        auto resultCode = boost::exit_success;
        auto frameworkInitialized = false;
        std::unique_ptr<UnitTestsFixture> testFixture;

        try
        {
            utf::framework::init( initFunc, argc, argv );
            frameworkInitialized = true;

            if( utf::runtime_config::get<bool>( utf::runtime_config::btrt_wait_for_debugger ) )
            {
                utf::results_reporter::get_stream() << "Press any key to continue..." << std::endl;
                ( std::getchar )();
                utf::results_reporter::get_stream() << "Continuing..." << std::endl;
            }

            utf::framework::finalize_setup_phase();

            ensureUnitTestTypeManager();
            testFixture = std::make_unique<UnitTestsFixture>();

            utf::framework::run();

            resultCode = !utf::runtime_config::get<bool>( utf::runtime_config::btrt_result_code )
                             ? boost::exit_success
                             : utf::results_collector.results( utf::framework::master_test_suite().p_id )
                                   .result_code();
        }
        catch( utf::framework::nothing_to_test &e )
        {
            resultCode = e.m_result_code;
        }
        catch( utf::framework::internal_error &e )
        {
            utf::results_reporter::get_stream()
                << "Boost.Test framework internal error: " << e.what() << std::endl;
            resultCode = boost::exit_exception_failure;
        }
        catch( utf::framework::setup_error &e )
        {
            utf::results_reporter::get_stream() << "Test setup error: " << e.what() << std::endl;
            resultCode = boost::exit_exception_failure;
        }
        catch( std::logic_error &e )
        {
            utf::results_reporter::get_stream() << "Test setup error: " << e.what() << std::endl;
            resultCode = boost::exit_exception_failure;
        }
        catch( ... )
        {
            utf::results_reporter::get_stream()
                << "Boost.Test framework internal error: unknown reason" << std::endl;
            resultCode = boost::exit_exception_failure;
        }

        // Destroy the fixture before reporting leaks and shutting down the
        // test framework.  Releasing the unique_ptr leaves UnitTests::m_fixture
        // pointing at a live-looking object and skips the engine teardown in
        // UnitTestsFixture::~UnitTestsFixture().
        testFixture.reset();
        std::cerr << "Unit test fixture destroyed" << std::endl;
#if WP_ENABLE_MEMORY_TRACKER
        std::cout << "Reporting memory leaks after unit test teardown." << std::endl;
        MemoryTracker::get().reportLeaks();
#endif

        // The type manager is also exposed through two non-owning globals.
        // Clear those aliases before destroying the owner; otherwise a
        // destructor reached during shutdown can observe a dangling manager.
        TypeManager::setInstance( nullptr );
        UnitTests::sTypeManager = nullptr;
        g_unitTestTypeManager.reset();
        std::cerr << "Unit test type manager destroyed" << std::endl;

        if( frameworkInitialized )
        {
            std::cerr << "Boost.Test shutdown starting" << std::endl;
            try
            {
                utf::framework::shutdown();
                std::cerr << "Boost.Test shutdown complete" << std::endl;
            }
            catch( ... )
            {
                std::cerr << "Boost.Test shutdown exception caught" << std::endl;
            }
        }

        std::cerr << "Unit test runner returning" << std::endl;
        return resultCode;
    }

}  // namespace

BOOST_AUTO_TEST_CASE( create_object )
{
    using namespace workphone;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto obj = std::make_unique<ISharedObject>();
    obj->unload( nullptr );
}

int main( int argc, char *argv[] )
{
    using namespace workphone;

    try
    {
        configureDebugCrtForUnitTests();
        AddVectoredExceptionHandler( 1, []( EXCEPTION_POINTERS *exception ) -> LONG {
            if( exception->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION )
            {
                SymInitialize( GetCurrentProcess(), nullptr, TRUE );
                void *frames[64];
                auto count = CaptureStackBackTrace( 0, 64, frames, nullptr );
                std::cerr << "ACCESS VIOLATION " << exception->ExceptionRecord->ExceptionAddress << std::endl;
                for( unsigned i = 0; i < count; ++i )
                {
                    alignas( SYMBOL_INFO ) char buffer[sizeof( SYMBOL_INFO ) + 1024] = {};
                    auto symbol = reinterpret_cast<SYMBOL_INFO *>( buffer );
                    symbol->SizeOfStruct = sizeof( SYMBOL_INFO );
                    symbol->MaxNameLen = 1024;
                    DWORD64 displacement = 0;
                    if( SymFromAddr( GetCurrentProcess(), reinterpret_cast<DWORD64>( frames[i] ), &displacement, symbol ) )
                        std::cerr << symbol->Name << " + " << displacement << std::endl;
                }
            }
            return EXCEPTION_CONTINUE_SEARCH;
        } );

        // prototype for user's unit test init function
#ifdef BOOST_TEST_ALTERNATIVE_INIT_API
        extern bool init_unit_test();

        boost::unit_test::init_unit_test_func init_func = &init_unit_test;
#else
        extern ::boost::unit_test::test_suite *init_unit_test_suite( int argc, char *argv[] );

        boost::unit_test::init_unit_test_func init_func = &init_unit_test_suite;
#endif

        return runUnitTests( init_func, argc, argv );
    }
    catch( std::exception &e )
    {
        std::cout << e.what() << std::endl;
    }

    return -1;
}
