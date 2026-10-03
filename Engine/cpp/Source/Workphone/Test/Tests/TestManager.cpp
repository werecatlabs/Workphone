#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Test/Tests/TestManager.hpp"
#include "Workphone/Test/Tests/Test.hpp"
#include <Workphone/Memory/PointerUtil.hpp>
#include <cassert>

namespace workphone
{
    namespace
    {
        class CountingTest : public Test
        {
        public:
            CountingTest( int &runCount, TestManager *manager = nullptr, bool stopManager = false ) :
                m_runCount( runCount ),
                m_manager( manager ),
                m_stopManager( stopManager )
            {
            }

            void run() override
            {
                ++m_runCount;

                if( m_stopManager && m_manager )
                {
                    m_manager->setRunning( false );
                }
            }

        private:
            int &m_runCount;
            TestManager *m_manager = nullptr;
            bool m_stopManager = false;
        };
    }  // namespace

    TestManager::TestManager() : m_bIsRunning( false )
    {
    }

    TestManager::~TestManager()
    {
    }

    void TestManager::update()
    {
        if( !isRunning() )
        {
            return;
        }

        for( auto test : m_tests )
        {
            if( test->isEnabled() )
            {
                test->run();
            }

            if( !isRunning() )
            {
                break;
            }
        }
    }

    bool TestManager::isRunning() const
    {
        return m_bIsRunning;
    }

    void TestManager::setRunning( bool running )
    {
        m_bIsRunning = running;
    }

    Array<SmartPtr<Test>> TestManager::getTests() const
    {
        return m_tests.snapshot();
    }

    void TestManager::setTests( const Array<SmartPtr<Test>> &tests )
    {
        m_tests = { tests.begin(), tests.end() };
    }

    void TestManager::run()
    {
        int firstRunCount = 0;
        int disabledRunCount = 0;
        int stoppedRunCount = 0;

        auto first = workphone::make_ptr<CountingTest>( firstRunCount );
        auto disabled = workphone::make_ptr<CountingTest>( disabledRunCount );
        auto stopper = workphone::make_ptr<CountingTest>( stoppedRunCount, this, true );
        auto afterStop = workphone::make_ptr<CountingTest>( stoppedRunCount );

        first->setEnabled( true );
        disabled->setEnabled( false );
        stopper->setEnabled( true );
        afterStop->setEnabled( true );

        setTests( { first, disabled, stopper, afterStop } );
        setRunning( true );
        update();

        assert( firstRunCount == 1 );
        assert( disabledRunCount == 0 );
        assert( stoppedRunCount == 1 );
        assert( !isRunning() );

        update();
        assert( firstRunCount == 1 );
        assert( disabledRunCount == 0 );
        assert( stoppedRunCount == 1 );
    }
}  // namespace workphone
