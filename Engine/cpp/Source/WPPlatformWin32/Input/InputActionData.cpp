#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Input/InputActionData.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, InputActionData, IInputAction );

    InputActionData::InputActionData() = default;

    InputActionData::InputActionData( u32 first, u32 second, u32 actionId ) :
        m_primary( first ),
        m_secondary( second ),
        m_actionId( actionId )
    {
    }

    auto InputActionData::getPrimaryAction() const -> hash_type
    {
        return m_primary;
    }

    void InputActionData::setPrimaryAction( hash_type primary )
    {
        m_primary = primary;
    }

    auto InputActionData::getSecondaryAction() const -> hash_type
    {
        return m_secondary;
    }

    void InputActionData::setSecondaryAction( hash_type secondary )
    {
        m_secondary = secondary;
    }

    auto InputActionData::getActionId() const -> hash_type
    {
        return m_actionId;
    }

    void InputActionData::setActionId( hash_type actionId )
    {
        m_actionId = actionId;
    }

    String InputActionData::getPrimaryName() const
    {
        return m_primaryName;
    }

    void InputActionData::setPrimaryName( const String &name )
    {
        m_primaryName = name;
    }

    String InputActionData::getSecondaryName() const
    {
        return m_secondaryName;
    }

    void InputActionData::setSecondaryName( const String &name )
    {
        m_secondaryName = name;
    }
}  // namespace workphone
