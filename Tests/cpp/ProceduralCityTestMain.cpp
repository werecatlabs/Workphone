// The production suite is the same source compiled by UnitTests, with no plugin/device startup.
#define BOOST_TEST_MODULE WPProceduralCityProduction
#include <boost/test/included/unit_test.hpp>
#include <Workphone/Memory/TypeManager.hpp>

struct CityTestRuntime
{
    workphone::TypeManager types;

    CityTestRuntime()
    {
        types.load();
    }

    ~CityTestRuntime()
    {
        types.unload();
    }
};

BOOST_GLOBAL_FIXTURE( CityTestRuntime );
