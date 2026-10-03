#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Test/Tests/TestSystemSettings.hpp"
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/Resolution.hpp>
#include <cassert>
#include <filesystem>

namespace workphone
{
    TestSystemSettings::TestSystemSettings()
    {
    }

    TestSystemSettings::~TestSystemSettings()
    {
    }

    void TestSystemSettings::run()
    {
        testSystemSettings();
        testQuickSettings();
    }

    void TestSystemSettings::testSystemSettings()
    {
        Resolution resolution;
        assert( resolution.size == Vector2I( 0, 0 ) );
        assert( resolution.depth == 32 );

        resolution.size = Vector2I( 1920, 1080 );
        resolution.depth = 24;

        assert( resolution.size == Vector2I( 1920, 1080 ) );
        assert( resolution.depth == 24 );
    }

    void TestSystemSettings::testQuickSettings()
    {
        const auto workingDirectory = Path::getWorkingDirectory();
        assert( !workingDirectory.empty() );

        const auto currentPath = std::filesystem::current_path();
        assert( std::filesystem::exists( currentPath ) );
        assert( std::filesystem::is_directory( currentPath ) );
    }
}  // namespace workphone
