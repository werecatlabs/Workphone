#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Input/GameInputMap.hpp>
#include <Workphone/Input/InputActionData.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/Input/IGameInput.hpp>

namespace workphone
{
    GameInputMap::GameInputMap() = default;

    namespace
    {
        SmartPtr<InputActionData> cloneInputActionData( const SmartPtr<IInputAction> &action )
        {
            auto data = workphone::dynamic_pointer_cast<InputActionData>( action );
            if( !data )
            {
                return nullptr;
            }

            auto clone = workphone::make_ptr<InputActionData>();
            clone->setPrimaryAction( data->getPrimaryAction() );
            clone->setSecondaryAction( data->getSecondaryAction() );
            clone->setActionId( data->getActionId() );
            clone->setPrimaryName( data->getPrimaryName() );
            clone->setSecondaryName( data->getSecondaryName() );
            return clone;
        }
    }  // namespace

    GameInputMap::GameInputMap( const GameInputMap &other )
    {
        for( const auto &pair : other.m_keyboardMap )
        {
            if( auto clone = cloneInputActionData( pair.second ) )
            {
                m_keyboardMap[pair.first] = clone;
            }
        }

        for( const auto &pair : other.m_joystickMap )
        {
            if( auto clone = cloneInputActionData( pair.second ) )
            {
                m_joystickMap[pair.first] = clone;
            }
        }
    }

    GameInputMap::~GameInputMap() = default;

    void GameInputMap::setKeyboardAction( u32 id, const String &key0, const String &key1 )
    {
        auto key0Hash = StringUtil::getHash( key0 );
        auto key1Hash = StringUtil::getHash( key1 );
        auto inputAction = workphone::make_ptr<InputActionData>( key0Hash, key1Hash, id );
        inputAction->setPrimaryName( key0 );
        inputAction->setSecondaryName( key1 );
        setKeyboardAction( id, inputAction );
    }

    void GameInputMap::setJoystickAction( u32 id, u32 button0, u32 button1 )
    {
        auto inputAction = workphone::make_ptr<InputActionData>( button0, button1, id );
        setJoystickAction( id, inputAction );
    }

    void GameInputMap::setKeyboardAction( u32 id, const SmartPtr<IInputAction> &actionData )
    {
        m_keyboardMap[id] = actionData;
    }

    void GameInputMap::setJoystickAction( u32 id, const SmartPtr<IInputAction> &actionData )
    {
        m_joystickMap[id] = actionData;
    }

    auto GameInputMap::getActionFromKey( u32 key ) const -> u32
    {
        const auto &keyboardMap = getKeyboardMap();
        auto keyActionIt = keyboardMap.begin();
        for( ; keyActionIt != keyboardMap.end(); ++keyActionIt )
        {
            const SmartPtr<IInputAction> &actionData = keyActionIt->second;
            if( actionData->getPrimaryAction() == key || actionData->getSecondaryAction() == key )
            {
                return (u32)actionData->getActionId();
            }
        }

        return IGameInput::UNASSIGNED;
    }

    void GameInputMap::getKeyboardAction( u32 id, String &key0, String &key1 )
    {
        auto it = m_keyboardMap.find( id );
        if( it != m_keyboardMap.end() )
        {
            if( auto data = workphone::dynamic_pointer_cast<InputActionData>( it->second ) )
            {
                key0 = data->getPrimaryName();
                key1 = data->getSecondaryName();
                return;
            }
        }

        key0.clear();
        key1.clear();
    }

    void GameInputMap::getJoystickAction( u32 id, u32 &button0, u32 &button1 )
    {
        auto it = m_joystickMap.find( id );
        if( it != m_joystickMap.end() )
        {
            if( auto data = workphone::dynamic_pointer_cast<InputActionData>( it->second ) )
            {
                button0 = static_cast<u32>( data->getPrimaryAction() );
                button1 = static_cast<u32>( data->getSecondaryAction() );
                return;
            }
        }

        button0 = IGameInput::UNASSIGNED;
        button1 = IGameInput::UNASSIGNED;
    }

    auto GameInputMap::getActionFromButton( u32 button ) const -> u32
    {
        const auto &joystickMap = getJoystickMap();
        auto joyIt = joystickMap.begin();
        for( ; joyIt != joystickMap.end(); ++joyIt )
        {
            const SmartPtr<IInputAction> &actionData = joyIt->second;
            if( actionData->getPrimaryAction() == button || actionData->getSecondaryAction() == button )
            {
                return (u32)actionData->getActionId();
            }
        }

        return IGameInput::UNASSIGNED;
    }

    auto GameInputMap::getKeyboardMap() const -> const Map<u32, SmartPtr<IInputAction>> &
    {
        return m_keyboardMap;
    }

    auto GameInputMap::getJoystickMap() const -> const Map<u32, SmartPtr<IInputAction>> &
    {
        return m_joystickMap;
    }

    auto GameInputMap::getInputActionData( u32 button, SmartPtr<IInputAction> &data ) -> bool
    {
        const auto &joystickMap = getJoystickMap();
        auto joyIt = joystickMap.begin();
        for( ; joyIt != joystickMap.end(); ++joyIt )
        {
            const SmartPtr<IInputAction> &actionData = joyIt->second;

            bool buttonState0 = false;
            bool buttonState1 = false;

            if( actionData->getPrimaryAction() == button || actionData->getSecondaryAction() == button )
            {
                data = actionData;
                return true;
            }
        }

        return false;
    }
}  // namespace workphone
