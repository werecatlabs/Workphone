#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageDefault.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageDefault, StateMessage );

    StateMessageDefault::StateMessageDefault() = default;

    StateMessageDefault::~StateMessageDefault() = default;

    auto StateMessageDefault::setProperty( hash_type id, const String &value ) -> s32
    {
        m_stringMap[id] = value;
        return 0;
    }

    auto StateMessageDefault::getProperty( hash_type id, String &value ) const -> s32
    {
        auto it = m_stringMap.find( id );
        if( it != m_stringMap.end() )
        {
            value = it->second;
        }

        return 0;
    }

    auto StateMessageDefault::setProperty( hash_type id, const Parameter &param ) -> s32
    {
        m_paramMap[id] = param;
        return 0;
    }

    auto StateMessageDefault::setProperty( hash_type id, const Parameters &params ) -> s32
    {
        m_paramListMap[id] = params;
        return 0;
    }

    auto StateMessageDefault::getProperty( hash_type id, Parameter &param ) const -> s32
    {
        auto it = m_paramMap.find( id );
        if( it != m_paramMap.end() )
        {
            param = it->second;
        }

        return 0;
    }

    auto StateMessageDefault::getProperty( hash_type id, Parameters &params ) const -> s32
    {
        auto it = m_paramListMap.find( id );
        if( it != m_paramListMap.end() )
        {
            params = it->second;
        }

        return 0;
    }

    void StateMessageDefault::setSubjectId( u32 id )
    {
        m_subjectId = id;
    }

    auto StateMessageDefault::getSubjectId() const -> u32
    {
        return m_subjectId;
    }
}  // namespace workphone
