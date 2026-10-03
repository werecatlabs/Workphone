#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Input/GameInputState.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, GameInputState, IGameInputState );

    GameInputState::GameInputState() = default;

    GameInputState::~GameInputState() = default;

    auto GameInputState::getEventType() const -> hash32
    {
        return m_eventType;
    }

    void GameInputState::setEventType( hash32 eventType )
    {
        m_eventType = eventType;
    }

    auto GameInputState::getAction() const -> hash32
    {
        return m_action;
    }

    void GameInputState::setAction( hash32 action )
    {
        m_action = action;
    }
}  // namespace workphone
