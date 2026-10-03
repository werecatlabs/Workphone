#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/SceneNodeStateData.hpp>
#include <Workphone/Math/MathUtil.hpp>
#include <Workphone/Memory/Memory.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, SceneNodeStateData, StateData );

    SceneNodeStateData::SceneNodeStateData() : StateData( SceneNodeStateData::typeInfo() )
    {
        constexpr auto size = sizeof( SceneNodeStateData );
    }

    SceneNodeStateData::SceneNodeStateData( const SceneNodeStateData &state )
    {
        this->lookAt = state.lookAt;
    }

    auto SceneNodeStateData::operator!=( SceneNodeStateData *other ) const -> bool
    {
        return Memory::Memcmp( this, other, sizeof( SceneNodeStateData ) ) != 0;
    }

    auto SceneNodeStateData::operator==( SceneNodeStateData *other ) const -> bool
    {
        return Memory::Memcmp( this, other, sizeof( SceneNodeStateData ) ) == 0;
    }

    auto SceneNodeStateData::clone() const -> SmartPtr<IState>
    {
        auto state = workphone::make_ptr<SceneNodeStateData>();

        state->lookAt = lookAt;

        return state;
    }

}  // namespace workphone
