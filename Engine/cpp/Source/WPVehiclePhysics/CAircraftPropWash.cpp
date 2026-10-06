#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CAircraftPropWash.hpp>
#include <WPVehiclePhysics/EngineSimple.hpp>
#include <WPVehiclePhysics/CAircraft.hpp>
#include <Workphone/WorkphoneInterface.hpp>

namespace workphone
{
    namespace vehicle
    {
        CAircraftPropWash::CAircraftPropWash() = default;

        CAircraftPropWash::~CAircraftPropWash() = default;

        void CAircraftPropWash::load( SmartPtr<ISharedObject> data )
        {
        }

        void CAircraftPropWash::loadFromData( void *pData )
        {
            // auto data = static_cast<data::aircraft_prop_wash_data *>(pData);
            // m_curve.setPoints( data->throttleMultipliers );

            // m_maxSpeed = data->maxSpeed;
        }

        void CAircraftPropWash::unload( SmartPtr<ISharedObject> data )
        {
        }

        Vector3<real_Num> CAircraftPropWash::getPropWash()
        {
            if( m_propellerUnit )
            {
                return m_propellerUnit->getThrust() * m_strength;
            }

            return Vector3<real_Num>::zero();
        }

        Vector3<real_Num> CAircraftPropWash::getPropWash( s32 section )
        {
            if( m_strength > std::numeric_limits<real_Num>::epsilon() ||
                m_strength < -std::numeric_limits<real_Num>::epsilon() )
            {
                auto index = section / m_parentAircraft->getSectionMultiplier();
                WP_ASSERT( index < m_affectedSections.size() );

                if( index < m_affectedSections.size() )
                {
                    auto affectedSection = m_affectedSections[index];
                    if( affectedSection )
                    {
                        if( m_propellerUnit )
                        {
                            auto throttlePos = static_cast<real_Num>( 0.8 ) -
                                               m_parentAircraft->getChannel( CAircraft::m_thrChannel );
                            auto throttle = throttlePos / ( 0.8 * 2.0 );

                            auto thrust = m_propellerUnit->getThrust();
                            auto throttleCoef = m_curve.interpolate( throttle );

                            auto linearVelocity = m_parentAircraft->getLinearVelocity();

                            auto worldTransform = m_parentAircraft->getWorldTransform();
                            auto localLinearVelocity =
                                worldTransform.inverseTransformVector( linearVelocity );
                            auto attenuation =
                                static_cast<real_Num>( 1.0 ) -
                                Math<real_Num>::clamp01( localLinearVelocity.Z() / m_maxSpeed );

                            return thrust * m_strength * m_sectionMultipliers[index] * throttleCoef *
                                   attenuation;
                        }
                    }
                }
            }

            return Vector3<real_Num>::zero();
        }

        SmartPtr<IAircraftPropellerUnit> CAircraftPropWash::getPropellerUnit() const
        {
            return m_propellerUnit;
        }

        void CAircraftPropWash::setPropellerUnit( SmartPtr<IAircraftPropellerUnit> propellerUnit )
        {
            m_propellerUnit = propellerUnit;
        }

        real_Num CAircraftPropWash::getStrength() const
        {
            return m_strength;
        }

        void CAircraftPropWash::setStrength( real_Num strength )
        {
            m_strength = strength;
        }

        const Array<bool> &CAircraftPropWash::getAffectedSections() const
        {
            return m_affectedSections;
        }

        Array<bool> &CAircraftPropWash::getAffectedSections()
        {
            return m_affectedSections;
        }

        void CAircraftPropWash::setAffectedSections( const Array<bool> &affectedSections )
        {
            m_affectedSections = affectedSections;
        }

        Array<float> &CAircraftPropWash::getSectionMultipliers()
        {
            return m_sectionMultipliers;
        }

        const Array<float> &CAircraftPropWash::getSectionMultipliers() const
        {
            return m_sectionMultipliers;
        }

        void CAircraftPropWash::setSectionMultipliers( const Array<float> &sectionMultipliers )
        {
            m_sectionMultipliers = sectionMultipliers;
        }
    } // namespace vehicle
} // namespace workphone
