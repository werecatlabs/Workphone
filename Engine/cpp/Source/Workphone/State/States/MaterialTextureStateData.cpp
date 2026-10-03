#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/MaterialTextureStateData.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, MaterialTextureStateData, StateData );

    MaterialTextureStateData::MaterialTextureStateData() :
        StateData( MaterialTextureStateData::typeInfo() )
    {
    }

    MaterialTextureStateData::~MaterialTextureStateData() = default;

}  // namespace workphone
