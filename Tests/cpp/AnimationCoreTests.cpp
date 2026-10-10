#define BOOST_TEST_MODULE WPAnimationCoreTests
#include <Workphone/WorkphonePrerequisites.hpp>
#include <boost/test/included/unit_test.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>

struct AnimationRuntimeFixture
{
    workphone::TypeManager types;
    workphone::SmartPtr<workphone::core::ApplicationManager> application;
    AnimationRuntimeFixture()
    {
        types.load();
        workphone::TypeManager::setInstance( &types );
        application = workphone::make_ptr<workphone::core::ApplicationManager>();
        workphone::core::IApplicationManager::setInstance( application );
        application->setFactoryManager( workphone::make_ptr<workphone::FactoryManager>() );
    }
    ~AnimationRuntimeFixture()
    {
        application->setFactoryManager( nullptr );
        workphone::core::IApplicationManager::setInstance( nullptr );
        application = nullptr;
        workphone::TypeManager::setInstance( nullptr );
        types.unload();
    }
};

BOOST_GLOBAL_FIXTURE( AnimationRuntimeFixture );
