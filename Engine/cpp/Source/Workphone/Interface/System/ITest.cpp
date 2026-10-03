#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/ITest.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ITest, ISharedObject );

    ITest::~ITest() = default;

}  // namespace workphone
