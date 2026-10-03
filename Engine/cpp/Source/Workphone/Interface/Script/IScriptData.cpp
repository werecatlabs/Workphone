#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Script/IScriptData.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IScriptData, ISharedObject );

    IScriptData::~IScriptData() = default;

}  // namespace workphone
