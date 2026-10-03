#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/ITestManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ITestManager, ISharedObject );

    ITestManager::~ITestManager() = default;

}  // namespace workphone
