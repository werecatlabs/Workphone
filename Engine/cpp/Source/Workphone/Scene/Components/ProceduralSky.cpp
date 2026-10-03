#include <Workphone/WorkphonePCH.hpp>
#include "ProceduralProperties.hpp"
#include <Workphone/Scene/Components/ProceduralSky.hpp>
#include <Workphone/Scene/Components/Light.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <algorithm>
#include <cmath>
namespace workphone::scene
{
    ProceduralSky::ProceduralSky() = default;
    ProceduralSky::~ProceduralSky() = default;

    WP_CLASS_REGISTER_DERIVED( workphone::scene, ProceduralSky, Component );
    void ProceduralSky::load( SmartPtr<ISharedObject> data )
    {
        Component::load( data );
        m_dirty = true;
        // Apply global scene settings after actor/component deserialization.
        setLoadingState( LoadingState::Loaded );
    }
    void ProceduralSky::update()
    {
        if( Thread::getCurrentTask() != TaskId::Application || !isEnabled() )
            return;
        if( m_dirty && getActor() )
            regenerate();
        // A Light may create its render node after the component FSM enters Edit/Play.
        if( m_applyLighting && m_light )
        {
            if( auto node = m_light->getSceneNode() )
            {
                const auto direction = m_results.sunAltitude >= 0 ? m_results.sunDir : m_results.moonDir;
                const auto orientation = Quaternion<real_Num>::getRotationTo(
                    Vector3<real_Num>( 0, 0, -1 ), -direction );
                if( node->getOrientation() != orientation )
                    node->setOrientation( orientation );
            }
        }
    }
    bool ProceduralSky::regenerate()
    {
        m_dirty = false;
        auto app = core::IApplicationManager::instancePtr();
        if( !m_service && app && app->getFactoryManager() )
            m_service = app->getFactoryManager()->createObjectFromType<procedural::ISkyAtmosphere>(
                "ISkyAtmosphere" );
        if( !m_service )
        {
            m_error = "Load the WPProcedural plugin to generate sky parameters";
            return false;
        }
        try
        {
            m_service->setState( m_state );
            m_service->update();
            m_results = m_service->getResults();
            if( app && app->getGraphicsSystem() )
            {
                auto scene = app->getGraphicsSystem()->getGraphicsScene();
                if( scene && m_applyLighting )
                    scene->setAmbientLight( m_results.ambientColour );
                if( scene && m_applyFog )
                    scene->setFog( render::IGraphicsScene::FOG_EXP2, m_results.fogColour, m_results.fogDensity );
            }
            if( m_applyLighting )
                if( auto actor = getActor() )
                {
                    if( !m_light )
                    {
                        m_light = actor->getComponent<Light>();
                        m_ownsLight = !m_light;
                        if( !m_light )
                            m_light = actor->addComponent<Light>();
                    }
                    m_light->setLightType( LightTypes::LT_DIRECTIONAL );
                    m_light->setDiffuseColour( m_results.keyColour );
                    m_light->setIntensity( m_results.keyIntensity );
                    m_light->setState( getState() );
                    if( auto node = m_light->getSceneNode() )
                    {
                        const auto direction =
                            m_results.sunAltitude >= 0 ? m_results.sunDir : m_results.moonDir;
                        node->setOrientation( Quaternion<real_Num>::getRotationTo(
                            Vector3<real_Num>( 0, 0, -1 ), -direction ) );
                    }
                }
            m_error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            m_error = e.what();
            return false;
        }
    }
    void ProceduralSky::unload( SmartPtr<ISharedObject> data )
    {
        if( m_ownsLight && m_light )
        {
            m_light->unload( nullptr );
            if( auto actor = getActor() )
                actor->removeComponentInstance( m_light );
        }
        m_light = nullptr;
        m_service = nullptr;
        m_ownsLight = false;
        m_dirty = true;
        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }
    SmartPtr<Properties> ProceduralSky::getProperties() const
    {
        auto p = Component::getProperties();
        p->setProperty( "Time Of Day", m_state.timeOfDay );
        p->setProperty( "Day Of Year", m_state.dayOfYear );
        p->setProperty( "Latitude", m_state.latitude );
        p->setProperty( "Turbidity", m_state.turbidity );
        p->setProperty( "Sun Intensity", m_state.sunIntensity );
        procedural_properties::setEnum( p, "Weather", static_cast<s32>( m_state.weather ),
                                        { "Clear", "Partly Cloudy", "Overcast", "Foggy", "Stormy" } );
        p->setProperty( "Apply Lighting", m_applyLighting );
        p->setProperty( "Apply Fog", m_applyFog );
        p->setButtonPressed( "Regenerate" );
        p->setProperty( "Generation Error", m_error );
        p->getPropertyObject( "Generation Error" ).setReadOnly( true );
        return p;
    }
    void ProceduralSky::setProperties( SmartPtr<Properties> p )
    {
        if( !p )
            return;
        Component::setProperties( p );
        p->getPropertyValue( "Time Of Day", m_state.timeOfDay );
        p->getPropertyValue( "Day Of Year", m_state.dayOfYear );
        p->getPropertyValue( "Latitude", m_state.latitude );
        p->getPropertyValue( "Turbidity", m_state.turbidity );
        p->getPropertyValue( "Sun Intensity", m_state.sunIntensity );
        // Reject nonfinite values before the astronomical calculation.
        auto bounded = []( real_Num v, real_Num lo, real_Num hi, real_Num fallback ) {
            return std::isfinite( v ) ? std::clamp( v, lo, hi ) : fallback;
        };
        m_state.timeOfDay = bounded( m_state.timeOfDay, 0, 24, 14 );
        m_state.dayOfYear = bounded( m_state.dayOfYear, 0, 365, 172 );
        m_state.latitude = bounded( m_state.latitude, -90, 90, 32 );
        m_state.turbidity = bounded( m_state.turbidity, 1, 20, 2 );
        m_state.sunIntensity = bounded( m_state.sunIntensity, 0, 100, 1.5f );
        s32 weather = procedural_properties::getEnum(
            p, "Weather", static_cast<s32>( m_state.weather ),
            { "Clear", "Partly Cloudy", "Overcast", "Foggy", "Stormy" } );
        m_state.weather = static_cast<procedural::Weather>( std::clamp( weather, 0, 4 ) );
        p->getPropertyValue( "Apply Lighting", m_applyLighting );
        p->getPropertyValue( "Apply Fog", m_applyFog );
        m_dirty = true;
    }
}  // namespace workphone::scene
