#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Test/Tests/TestComponent.hpp"
#include <Workphone/Workphone.hpp>

#include <cassert>

namespace workphone
{
    TestComponent::TestComponent()
    {
    }

    TestComponent::~TestComponent()
    {
    }

    void TestComponent::run()
    {
        testBaseComponent();
    }

    void TestComponent::testBaseComponent()
    {
        TestComponent component;

        assert( !component.isEnabled() );

        component.setEnabled( true );
        assert( component.isEnabled() );

        component.setEnabled( false );
        assert( !component.isEnabled() );
    }
}  // namespace workphone
