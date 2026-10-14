#ifndef WORKPHONE_UIUTIL_TEST_SUPPORT_HPP
#define WORKPHONE_UIUTIL_TEST_SUPPORT_HPP

#include "TestGuard.hpp"
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Scene/Systems/UI/LayoutTransformSystem.hpp>

// UnitTests::setupGame installs managers directly; Application::load normally installs this
// system. Keep these factory tests on the same state allocation path as the application.
struct UiUtilTestGuard : workphone::TestGuard
{
    UiUtilTestGuard()
    {
        system = workphone::make_ptr<workphone::scene::LayoutTransformSystem>();
        system->load( nullptr );
        sceneManager->addSystem( workphone::scene::LayoutTransform::typeInfo(), system );
    }

    ~UiUtilTestGuard()
    {
        sceneManager->clear();
        sceneManager->removeSystem( workphone::scene::LayoutTransform::typeInfo() );
        system->unload( nullptr );
    }

    workphone::SmartPtr<workphone::scene::LayoutTransformSystem> system;
};

#endif
