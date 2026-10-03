#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Script/IScriptInvoker.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IScriptInvoker, ISharedObject );

    IScriptInvoker::~IScriptInvoker() = default;

}  // namespace workphone
