#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/FlagsStateData.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/BitUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, FlagsStateData, StateData );

    FlagsStateData::FlagsStateData()
    {
    }

    FlagsStateData::~FlagsStateData()
    {
    }

    u32 FlagsStateData::getFlags() const
    {
        return m_flags;
    }

    void FlagsStateData::setFlags( u32 flags )
    {
        m_flags = flags;
    }

    void FlagsStateData::setFlag( u32 flag, bool value )
    {
        m_flags = BitUtil::setFlagValue( static_cast<u32>( m_flags ), flag, value );
    }

    bool FlagsStateData::getFlag( u32 flag ) const
    {
        return BitUtil::getFlagValue( static_cast<u32>( m_flags ), flag );
    }

}  // namespace workphone
