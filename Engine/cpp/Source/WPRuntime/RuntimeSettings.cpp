#include <WPRuntime/WPRuntimePCH.hpp>
#include <WPRuntime/RuntimeSettings.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    RuntimeSettings::RuntimeSettings()
    {
        m_numThreads = 1;
        m_targetFrames = 500;
        m_physicsUpdate = 50;
        m_gameLogicUpdate = 50;
        m_aiUpdate = 30;
        m_trafficUpdate = 15;
        m_scheme = "High";
    }

    RuntimeSettings::~RuntimeSettings() = default;

    SmartPtr<Properties> RuntimeSettings::getProperties() const
    {
        auto properties = workphone::make_ptr<Properties>();
        WP_ASSERT( properties );

        properties->setProperty( "NumThreads", m_numThreads );
        properties->setProperty( "TargetFrames", m_targetFrames );
        properties->setProperty( "PhysicsUpdate", m_physicsUpdate );
        properties->setProperty( "GameLogicUpdate", m_gameLogicUpdate );
        properties->setProperty( "AiUpdate", m_aiUpdate );
        properties->setProperty( "TrafficUpdate", m_trafficUpdate );
        properties->setProperty( "Scheme", m_scheme );

        return properties;
    }

    void RuntimeSettings::setProperties( SmartPtr<Properties> properties )
    {
        WP_ASSERT( properties );
        if( !properties )
        {
            m_properties = nullptr;
            return;
        }

        m_properties = properties;

        properties->getPropertyValue( "NumThreads", m_numThreads );
        properties->getPropertyValue( "TargetFrames", m_targetFrames );
        properties->getPropertyValue( "PhysicsUpdate", m_physicsUpdate );
        properties->getPropertyValue( "GameLogicUpdate", m_gameLogicUpdate );
        properties->getPropertyValue( "AiUpdate", m_aiUpdate );
        properties->getPropertyValue( "TrafficUpdate", m_trafficUpdate );
        properties->getPropertyValue( "Scheme", m_scheme );
    }

    u32 RuntimeSettings::getNumThreads() const
    {
        return m_numThreads;
    }

    void RuntimeSettings::setNumThreads( u32 numThreads )
    {
        m_numThreads = numThreads;
    }

    u32 RuntimeSettings::getTargetFrames() const
    {
        return m_targetFrames;
    }

    void RuntimeSettings::setTargetFrames( u32 targetFrames )
    {
        m_targetFrames = targetFrames;
    }

    u32 RuntimeSettings::getPhysicsUpdate() const
    {
        return m_physicsUpdate;
    }

    void RuntimeSettings::setPhysicsUpdate( u32 physicsUpdate )
    {
        m_physicsUpdate = physicsUpdate;
    }

    u32 RuntimeSettings::getGameLogicUpdate() const
    {
        return m_gameLogicUpdate;
    }

    void RuntimeSettings::setGameLogicUpdate( u32 gameLogicUpdate )
    {
        m_gameLogicUpdate = gameLogicUpdate;
    }

    u32 RuntimeSettings::getAiUpdate() const
    {
        return m_aiUpdate;
    }

    void RuntimeSettings::setAiUpdate( u32 aiUpdate )
    {
        m_aiUpdate = aiUpdate;
    }

    u32 RuntimeSettings::getTrafficUpdate() const
    {
        return m_trafficUpdate;
    }

    void RuntimeSettings::setTrafficUpdate( u32 trafficUpdate )
    {
        m_trafficUpdate = trafficUpdate;
    }

    String RuntimeSettings::getScheme() const
    {
        return m_scheme;
    }

    void RuntimeSettings::setScheme( const String &scheme )
    {
        m_scheme = scheme;
    }
}  // namespace workphone
