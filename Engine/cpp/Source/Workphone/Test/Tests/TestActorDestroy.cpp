#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Test/Tests/TestActorDestroy.hpp"
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <cassert>

namespace workphone
{
    TestActorDestroy::TestActorDestroy()
    {
    }

    TestActorDestroy::~TestActorDestroy()
    {
    }

    void TestActorDestroy::run()
    {
        SmartPtr<scene::GameActor> actor = workphone::make_ptr<scene::GameActor>();
        assert( actor );

        actor->setEnabled( true );
        actor->setVisible( true );
        assert( actor->isEnabled() );
        assert( actor->isVisible() );

        actor->setEnabled( false );
        actor->setVisible( false );
        assert( !actor->isEnabled() );
        assert( !actor->isVisible() );

        actor->unload( nullptr );
        actor = nullptr;
        assert( !actor );
    }
}  // namespace workphone
