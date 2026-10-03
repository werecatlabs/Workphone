#ifndef TestManager_h__
#define TestManager_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Test/Tests/Test.hpp>

namespace workphone
{

    class WPCore_API TestManager : public ISharedObject
    {
    public:
        TestManager();
        ~TestManager() override;

        void update() override;

        void run();

        bool isRunning() const;
        void setRunning( bool running );

        Array<SmartPtr<Test>> getTests() const;
        void setTests( const Array<SmartPtr<Test>> &tests );

    protected:
        ConcurrentArray<SmartPtr<Test>> m_tests;
        atomic_bool m_bIsRunning;
    };
}  // namespace workphone

#endif  // TestManager_h__
