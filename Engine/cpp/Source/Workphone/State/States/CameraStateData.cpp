#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/CameraStateData.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, CameraStateData, StateData );

    CameraStateData::CameraStateData() : StateData( CameraStateData::typeInfo() )
    {
    }

    CameraStateData::~CameraStateData()
    {
    }

}  // namespace workphone
