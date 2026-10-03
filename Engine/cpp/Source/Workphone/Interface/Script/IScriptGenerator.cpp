#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Script/IScriptGenerator.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IScriptGenerator, ISharedObject );

    IScriptGenerator::~IScriptGenerator() = default;

}  // namespace workphone
