#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Test/Tests/TestRandomClicking.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <cassert>
#include <random>

namespace workphone
{
    TestRandomClicking::TestRandomClicking() = default;

    TestRandomClicking::~TestRandomClicking() = default;

    void TestRandomClicking::run()
    {
        constexpr int width = 1920;
        constexpr int height = 1080;
        constexpr int sampleCount = 128;

        std::mt19937 randomEngine( 42 );
        std::uniform_int_distribution<int> xDistribution( 0, width - 1 );
        std::uniform_int_distribution<int> yDistribution( 0, height - 1 );

        for( int i = 0; i < sampleCount; ++i )
        {
            const auto click = Vector2I( xDistribution( randomEngine ), yDistribution( randomEngine ) );
            assert( click.X() >= 0 );
            assert( click.X() < width );
            assert( click.Y() >= 0 );
            assert( click.Y() < height );
        }
    }
}  // namespace workphone
