#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Input/GameInput.hpp>
#include <Workphone/Interface/Input/IGameInputMap.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, GameInput, IGameInput );

    GameInput::GameInput() = default;

    GameInput::~GameInput() = default;

    bool GameInput::isAssigned() const
    {
        return false;
    }

    SmartPtr<IGameInputMap> GameInput::getGameInputMap() const
    {
        return nullptr;
    }

    void GameInput::setPlayerIndex( u32 playerIndex )
    {
    }

    u32 GameInput::getPlayerIndex() const
    {
        return 0;
    }

    u32 GameInput::getJoystickId() const
    {
        return 0;
    }

    void GameInput::setJoystickId( u32 joystickId )
    {
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
