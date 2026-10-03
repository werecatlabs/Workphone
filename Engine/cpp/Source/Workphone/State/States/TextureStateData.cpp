#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/TextureStateData.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, TextureStateData, StateData );

    TextureStateData::TextureStateData() : StateData( TextureStateData::typeInfo() )
    {
    }

    TextureStateData::~TextureStateData() = default;

}  // namespace workphone
