#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/SkyStateData.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, SkyStateData, StateData );

    SkyStateData::SkyStateData() : StateData( SkyStateData::typeInfo() )
    {
    }

    SkyStateData::~SkyStateData() = default;

}  // namespace workphone
