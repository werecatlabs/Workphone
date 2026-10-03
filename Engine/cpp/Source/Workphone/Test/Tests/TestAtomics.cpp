#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Test/Tests/TestAtomics.hpp"

#include <atomic>
#include <cassert>
#include <thread>
#include <vector>

namespace workphone
{
    TestAtomics::TestAtomics()
    {
    }

    TestAtomics::~TestAtomics()
    {
    }

    void TestAtomics::run()
    {
        std::atomic<int> counter{ 0 };

        const int threadCount = 8;
        const int incrementsPerThread = 100000;

        std::vector<std::thread> threads;
        threads.reserve( threadCount );

        for( int i = 0; i < threadCount; ++i )
        {
            threads.emplace_back( [&counter, incrementsPerThread]() {
                for( int j = 0; j < incrementsPerThread; ++j )
                {
                    ++counter;
                }
            } );
        }

        for( auto &thread : threads )
        {
            thread.join();
        }

        assert( counter == threadCount * incrementsPerThread );
    }
}  // namespace workphone
