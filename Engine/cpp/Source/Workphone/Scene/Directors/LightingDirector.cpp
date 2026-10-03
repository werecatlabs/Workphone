#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Directors/LightingDirector.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, LightingDirector, Director );

    const String LightingDirector::ambientColourStr = String( "ambientColour" );
    const String LightingDirector::upperHemisphereStr = String( "upperHemisphere" );
    const String LightingDirector::lowerHemisphereStr = String( "lowerHemisphere" );
    const String LightingDirector::hemisphereDirStr = String( "hemisphereDir" );
    const String LightingDirector::envmapScaleStr = String( "envmapScale" );

    LightingDirector::LightingDirector() = default;
    LightingDirector::~LightingDirector() = default;

    SmartPtr<Properties> LightingDirector::getProperties() const
    {
        auto properties = ResourceDirector::getProperties();
        if( !properties )
        {
            return nullptr;
        }

        properties->setProperty( ambientColourStr, m_ambientColour );
        properties->setProperty( upperHemisphereStr, m_upperHemisphere );
        properties->setProperty( lowerHemisphereStr, m_lowerHemisphere );
        properties->setProperty( hemisphereDirStr, m_hemisphereDir );
        properties->setProperty( envmapScaleStr, m_envmapScale );

        return properties;
    }

    void LightingDirector::setProperties( SmartPtr<Properties> properties )
    {
        ResourceDirector::setProperties( properties );
        if( !properties )
        {
            return;
        }

        properties->getPropertyValue( ambientColourStr, m_ambientColour );
        properties->getPropertyValue( upperHemisphereStr, m_upperHemisphere );
        properties->getPropertyValue( lowerHemisphereStr, m_lowerHemisphere );
        properties->getPropertyValue( hemisphereDirStr, m_hemisphereDir );
        properties->getPropertyValue( envmapScaleStr, m_envmapScale );
    }

    void LightingDirector::setEnvmapScale( f32 envmapScale )
    {
        m_envmapScale = envmapScale;
    }

    f32 LightingDirector::getEnvmapScale() const
    {
        return m_envmapScale;
    }

    void LightingDirector::setHemisphereDir( Vector3<real_Num> hemisphereDir )
    {
        m_hemisphereDir = hemisphereDir;
    }

    Vector3<real_Num> LightingDirector::getHemisphereDir() const
    {
        return m_hemisphereDir;
    }

    void LightingDirector::setUpperHemisphere( const ColourF &upperHemisphere )
    {
        m_upperHemisphere = upperHemisphere;
    }

    ColourF LightingDirector::getUpperHemisphere() const
    {
        return m_upperHemisphere;
    }

    void LightingDirector::setAmbientColour( const ColourF &ambientColour )
    {
        m_ambientColour = ambientColour;
    }

    ColourF LightingDirector::getAmbientColour() const
    {
        return m_ambientColour;
    }

    void LightingDirector::setLowerHemisphere( const ColourF &lowerHemisphere )
    {
        m_lowerHemisphere = lowerHemisphere;
    }

    ColourF LightingDirector::getLowerHemisphere() const
    {
        return m_lowerHemisphere;
    }
}  // namespace workphone::scene
