#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Input/InputFunction.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, InputFunction, ISharedObject );

    InputFunction::InputFunction() :
        m_isAxis( false ),
        m_isButton( true ),
        m_isReversed( false ),
        m_mapId( -1 )
    {
        m_channelValue = 0;
        m_prevChannelValue = 0;
        m_timeChanged = 0;
    }

    InputFunction::~InputFunction()
    {
    }

    String InputFunction::getFunction() const
    {
        return m_function;
    }

    void InputFunction::setFunction( const String &functionName )
    {
        m_function = functionName;

        auto functionHash = StringUtil::getHash( functionName );
        setFunctionHash( functionHash );
    }

    bool InputFunction::isAxis() const
    {
        return m_isAxis;
    }

    void InputFunction::setAxis( bool isAxisInput )
    {
        m_isAxis = isAxisInput;
    }

    bool InputFunction::getButton() const
    {
        return m_isButton;
    }

    void InputFunction::setButton( bool isButtonInput )
    {
        m_isButton = isButtonInput;
    }

    void InputFunction::setMapId( s32 mapId )
    {
        m_mapId = mapId;
    }

    s32 InputFunction::getMapId() const
    {
        return m_mapId;
    }

    hash_type InputFunction::getFunctionHash() const
    {
        return m_functionHash;
    }

    void InputFunction::setFunctionHash( hash_type hashValue )
    {
        m_functionHash = hashValue;
    }

    bool InputFunction::isReversed() const
    {
        return m_isReversed;
    }

    void InputFunction::setReversed( bool isReversedInput )
    {
        m_isReversed = isReversedInput;
    }

    f32 InputFunction::getChannelValue() const
    {
        return m_channelValue;
    }

    void InputFunction::setChannelValue( f32 channelValue )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            m_prevChannelValue = m_channelValue;
            m_channelValue = channelValue;
            return;
        }

        auto timer = applicationManager->getTimerPtr();
        if( !MathF::equals( m_channelValue, channelValue ) )
        {
            m_timeChanged = timer ? timer->now() : 0.0;
            m_prevChannelValue = m_channelValue;
            m_channelValue = channelValue;
        }
    }

    f32 InputFunction::getPrevChannelValue() const
    {
        return m_prevChannelValue;
    }

    void InputFunction::setPrevChannelValue( f32 previousChannelValue )
    {
        m_prevChannelValue = previousChannelValue;
    }

    time_interval InputFunction::timeSinceChange() const
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            return 0.0;
        }

        auto timer = applicationManager->getTimerPtr();
        if( !timer )
        {
            return 0.0;
        }

        return timer->now() - m_timeChanged;
    }

}  // namespace workphone
