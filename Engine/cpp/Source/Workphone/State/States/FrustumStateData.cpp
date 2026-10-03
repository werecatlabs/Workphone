#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/FrustumStateData.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, FrustumStateData, StateData );

    FrustumStateData::FrustumStateData() : StateData( FrustumStateData::typeInfo() )
    {
    }

    FrustumStateData::~FrustumStateData()
    {
    }

}  // namespace workphone
