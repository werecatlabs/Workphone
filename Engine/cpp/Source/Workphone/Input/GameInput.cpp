#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Input/GameInput.hpp>
#include <Workphone/Interface/Input/IGameInputMap.hpp>
#include <Workphone/Input/GameInputMap.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, GameInput, IGameInput );

    GameInput::GameInput() : m_gameInputMap( workphone::make_ptr<GameInputMap>() )
    {
        m_keyboardInputEnabled = true;
        m_inputReady = true;
        m_dongleReady = false;
    }

    GameInput::~GameInput() = default;

    bool GameInput::isAssigned() const
    {
        return m_inputReady;
    }

    SmartPtr<IGameInputMap> GameInput::getGameInputMap() const
    {
        return m_gameInputMap;
    }

    void GameInput::setPlayerIndex( u32 playerIndex )
    {
        m_playerIndex = playerIndex;
    }

    u32 GameInput::getPlayerIndex() const
    {
        return m_playerIndex;
    }

    u32 GameInput::getJoystickId() const
    {
        return m_joystickId;
    }

    void GameInput::setJoystickId( u32 joystickId )
    {
        m_joystickId = joystickId;
    }

    bool GameInput::isDongleReady() const
    {
        return m_dongleReady;
    }

    void GameInput::setDongleReady( bool isReady )
    {
        m_dongleReady = isReady;
    }

    bool GameInput::isInputReady() const
    {
        return m_inputReady;
    }

    void GameInput::setInputReady( bool isReady )
    {
        m_inputReady = isReady;
    }

    bool GameInput::isKeyboardInputEnabled() const
    {
        return m_keyboardInputEnabled;
    }

    void GameInput::setKeyboardInputEnabled( bool isEnabled )
    {
        m_keyboardInputEnabled = isEnabled;
    }

}  // namespace workphone
