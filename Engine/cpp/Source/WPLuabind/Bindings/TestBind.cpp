#include <WPLuabind/WPLuabindPCH.hpp>
#include <WPLuabind/Bindings/SystemBind.hpp>
#include <WPLuabind/ScriptObjectFunctions.hpp>
#include <WPLuabind/Helpers/EngineHelper.hpp>
#include <WPLuabind/Helpers/FileSystemHelper.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <WPLuabind/ParamConverter.hpp>
#include <Workphone/Workphone.hpp>
#include <luabind/luabind.hpp>
#include <boost/core/noncopyable.hpp>

namespace workphone
{

    void bindTest( lua_State *L )
    {
        using namespace luabind;

        module( L )[class_<ITest, ISharedObject, SmartPtr<ITest>>( "ITest" )
                        .def( "run", &ITest::run )
                        .def( "report", &ITest::report )
                        .def( "isEnabled", &ITest::isEnabled )
                        .def( "setEnabled", &ITest::setEnabled )
                        .scope[def( "typeInfo", ITest::typeInfo )]];

        module( L )[class_<ITestManager, ISharedObject, SmartPtr<ITestManager>>( "ITestManager" )
                        .def( "start", &ITestManager::start )
                        .def( "stop", &ITestManager::stop )
                        .def( "isRunning", &ITestManager::isRunning )
                        .def( "setIsRunning", &ITestManager::setIsRunning )
                        .def( "getTests", &ITestManager::getTests )
                        .def( "setTests", &ITestManager::setTests )
                        .scope[def( "typeInfo", ITestManager::typeInfo )]];
    }

} // namespace workphone
