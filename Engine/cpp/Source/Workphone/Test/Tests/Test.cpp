#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Test/Tests/Test.hpp>

namespace workphone
{
    Test::Test() : m_isEnabled( false )
    {
    }

    Test::~Test()
    {
    }

    void Test::run()
    {
    }

    void Test::report()
    {
    }

    bool Test::isEnabled() const
    {
        return m_isEnabled;
    }

    void Test::setEnabled( bool enabled )
    {
        m_isEnabled = enabled;
    }

}  // namespace workphone
