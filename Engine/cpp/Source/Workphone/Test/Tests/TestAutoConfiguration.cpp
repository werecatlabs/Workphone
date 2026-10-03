#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Test/Tests/TestAutoConfiguration.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>

namespace workphone
{
    TestAutoConfiguration::TestAutoConfiguration()
    {
    }

    TestAutoConfiguration::~TestAutoConfiguration()
    {
    }

    void TestAutoConfiguration::run()
    {
        const auto currentPath = std::filesystem::current_path();
        const auto tempPath = std::filesystem::temp_directory_path();

        const auto pathEnv = std::getenv( "PATH" );

        assert( std::filesystem::exists( currentPath ) );
        assert( std::filesystem::is_directory( currentPath ) );
        assert( std::filesystem::exists( tempPath ) );
        assert( pathEnv != nullptr );
        assert( std::string( pathEnv ).empty() == false );

        m_isConfigured = true;
        m_report = "Auto configuration test passed.";
    }

    void TestAutoConfiguration::report()
    {
        assert( m_isConfigured );
        assert( !m_report.empty() );
    }
}  // namespace workphone
