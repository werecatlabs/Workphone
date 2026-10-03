#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Input/AxisData.hpp>

namespace workphone
{
    AxisData::AxisData() = default;

    AxisData::~AxisData() = default;

    f32 AxisData::getOffset() const noexcept
    {
        return m_offset;
    }

    f32 AxisData::getMultiplier() const noexcept
    {
        return m_multiplier;
    }

    f32 AxisData::getLowMultiplier() const noexcept
    {
        return m_lowMultiplier;
    }

    f32 AxisData::getHighMultiplier() const noexcept
    {
        return m_highMultiplier;
    }

    bool AxisData::isReversed() const noexcept
    {
        return m_isReversed;
    }

    void AxisData::setReversed( bool reversed ) noexcept
    {
        m_isReversed = reversed;
    }

    s32 AxisData::getAxisIndex() const noexcept
    {
        return m_axisIndex;
    }

    void AxisData::setAxisIndex( s32 axisIndex ) noexcept
    {
        m_axisIndex = axisIndex;
    }

    s32 AxisData::getDeviceMap() const noexcept
    {
        return m_deviceMap;
    }

    void AxisData::setDeviceMap( s32 deviceMap ) noexcept
    {
        m_deviceMap = deviceMap;
    }

    hash_type AxisData::getFunctionHash() const noexcept
    {
        return m_functionHash;
    }

    void AxisData::setFunctionHash( hash_type functionHash ) noexcept
    {
        m_functionHash = functionHash;
    }

    String AxisData::getFunction() const
    {
        return m_function.str().c_str();
    }

    void AxisData::setFunction( const String &functionName )
    {
        m_function = functionName.c_str();
    }

    void AxisData::setOffset( f32 offset ) noexcept
    {
        m_offset = offset;
    }

    void AxisData::setLowMultiplier( f32 lowMultiplier ) noexcept
    {
        m_lowMultiplier = lowMultiplier;
    }

    void AxisData::setHighMultiplier( f32 highMultiplier ) noexcept
    {
        m_highMultiplier = highMultiplier;
    }

    void AxisData::setMultiplier( f32 multiplier ) noexcept
    {
        m_multiplier = multiplier;
    }
}  // namespace workphone
