#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/WheelController.hpp>
#include <Workphone/Interface/Vehicle/IVehicleManager.hpp>
#include <Workphone/Interface/Vehicle/IWheelComponent.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, WheelController, Component );

    // Define static const String members
    const String WheelController::massFractionStr = "massFraction";
    const String WheelController::radiusStr = "radius";
    const String WheelController::wheelDampingStr = "wheelDamping";
    const String WheelController::suspensionDistanceStr = "suspensionDistance";
    const String WheelController::springRateStr = "springRate";
    const String WheelController::suspensionDamperStr = "suspensionDamper";
    const String WheelController::targetPositionStr = "targetPosition";
    const String WheelController::forwardExtremumSlipStr = "forwardExtremumSlip";
    const String WheelController::forwardExtrememValueStr = "forwardExtrememValue";
    const String WheelController::forwardAsymptoteSlipStr = "forwardAsymptoteSlip";
    const String WheelController::forwardAsymptoteValueStr = "forwardAsymptoteValue";
    const String WheelController::forwardStiffnessStr = "forwardStiffness";
    const String WheelController::sidewaysExtremumSlipStr = "sidewaysExtremumSlip";
    const String WheelController::sidewaysExtrememValueStr = "sidewaysExtrememValue";
    const String WheelController::sidewaysAsymptoteSlipStr = "sidewaysAsymptoteSlip";
    const String WheelController::sidewaysAsymptoteValueStr = "sidewaysAsymptoteValue";
    const String WheelController::sidewaysStiffnessStr = "sidewaysStiffness";
    const String WheelController::isSteeringWheelStr = "isSteeringWheel";
    const String WheelController::tireModelStr = "tireModel";
    const String WheelController::resetStr = "Reset";

    WheelController::WheelController() = default;

    WheelController::~WheelController() = default;

    void WheelController::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        Component::load( data );
        setLoadingState( LoadingState::Loaded );
    }

    void WheelController::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );
                m_wheelController = nullptr;
                Component::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto WheelController::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = workphone::make_ptr<Properties>();

        properties->setProperty( massFractionStr, m_massFraction );
        properties->setProperty( radiusStr, m_radius );
        properties->setProperty( wheelDampingStr, m_wheelDamping );
        properties->setProperty( suspensionDistanceStr, m_suspensionDistance );
        properties->setProperty( springRateStr, m_springRate );
        properties->setProperty( suspensionDamperStr, m_suspensionDamper );
        properties->setProperty( targetPositionStr, m_targetPosition );
        properties->setProperty( forwardExtremumSlipStr, m_forwardExtremumSlip );
        properties->setProperty( forwardExtrememValueStr, m_forwardExtrememValue );
        properties->setProperty( forwardAsymptoteSlipStr, m_forwardAsymptoteSlip );
        properties->setProperty( forwardAsymptoteValueStr, m_forwardAsymptoteValue );
        properties->setProperty( forwardStiffnessStr, m_forwardStiffness );
        properties->setProperty( sidewaysExtremumSlipStr, m_sidewaysExtremumSlip );
        properties->setProperty( sidewaysExtrememValueStr, m_sidewaysExtrememValue );
        properties->setProperty( sidewaysAsymptoteSlipStr, m_sidewaysAsymptoteSlip );
        properties->setProperty( sidewaysAsymptoteValueStr, m_sidewaysAsymptoteValue );
        properties->setProperty( sidewaysStiffnessStr, m_sidewaysStiffness );
        properties->setProperty( isSteeringWheelStr, m_isSteeringWheel );
        properties->setProperty( tireModelStr, (s32)m_tireModel );

        properties->setButtonPressed( resetStr, false );

        return properties;
    }

    void WheelController::setProperties( SmartPtr<Properties> properties )
    {
        properties->getPropertyValue( massFractionStr, m_massFraction );
        properties->getPropertyValue( radiusStr, m_radius );
        properties->getPropertyValue( wheelDampingStr, m_wheelDamping );
        properties->getPropertyValue( suspensionDistanceStr, m_suspensionDistance );
        properties->getPropertyValue( springRateStr, m_springRate );
        properties->getPropertyValue( suspensionDamperStr, m_suspensionDamper );
        properties->getPropertyValue( targetPositionStr, m_targetPosition );
        properties->getPropertyValue( forwardExtremumSlipStr, m_forwardExtremumSlip );
        properties->getPropertyValue( forwardExtrememValueStr, m_forwardExtrememValue );
        properties->getPropertyValue( forwardAsymptoteSlipStr, m_forwardAsymptoteSlip );
        properties->getPropertyValue( forwardAsymptoteValueStr, m_forwardAsymptoteValue );
        properties->getPropertyValue( forwardStiffnessStr, m_forwardStiffness );
        properties->getPropertyValue( sidewaysExtremumSlipStr, m_sidewaysExtremumSlip );
        properties->getPropertyValue( sidewaysExtrememValueStr, m_sidewaysExtrememValue );
        properties->getPropertyValue( sidewaysAsymptoteSlipStr, m_sidewaysAsymptoteSlip );
        properties->getPropertyValue( sidewaysAsymptoteValueStr, m_sidewaysAsymptoteValue );
        properties->getPropertyValue( sidewaysStiffnessStr, m_sidewaysStiffness );
        properties->getPropertyValue( isSteeringWheelStr, m_isSteeringWheel );
        s32 tireModel = static_cast<s32>( m_tireModel );
        if( properties->getPropertyValue( tireModelStr, tireModel ) )
        {
            m_tireModel = static_cast<TireModel>( tireModel );
        }

        if( properties->isButtonPressed( resetStr ) )
        {
            m_radius = 0.29;
            m_suspensionDistance = 0.1;
            m_wheelDamping = 12000;
        }

        //if( m_wheelController )
        //{
        //    m_wheelController->setTireModel( m_tireModel );
        //}
    }

    auto WheelController::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto objects = Array<SmartPtr<ISharedObject>>();
        objects.reserve( 5 );

        objects.emplace_back( m_wheelController );

        return objects;
    }

    auto WheelController::getWheelController() const -> SmartPtr<vehicle::IWheelComponent>
    {
        return m_wheelController;
    }

    void WheelController::setWheelController( SmartPtr<vehicle::IWheelComponent> wheelController )
    {
        m_wheelController = wheelController;
    }

    auto WheelController::getMassFraction() const -> f32
    {
        return m_massFraction;
    }

    void WheelController::setMassFraction( f32 massFraction )
    {
        m_massFraction = massFraction;
    }

    auto WheelController::getRadius() const -> f32
    {
        return m_radius;
    }

    void WheelController::setRadius( f32 radius )
    {
        m_radius = radius;
    }

    auto WheelController::getWheelDamping() const -> f32
    {
        return m_wheelDamping;
    }

    void WheelController::setWheelDamping( f32 wheelDamping )
    {
        m_wheelDamping = wheelDamping;
    }

    auto WheelController::getSuspensionDistance() const -> f32
    {
        return m_suspensionDistance;
    }

    void WheelController::setSuspensionDistance( f32 suspensionDistance )
    {
        m_suspensionDistance = suspensionDistance;
    }

    void WheelController::reset()
    {
        m_massFraction = 0.05f;
        m_radius = 0.29f;
        m_wheelDamping = 9000.0f;
        m_suspensionDistance = 0.1f;
        m_springRate = 1.0f;
        m_suspensionDamper = 1.0f;
        m_targetPosition = 0.0f;
        m_forwardExtremumSlip = 10.0f;
        m_forwardExtrememValue = 10.0f;
        m_forwardAsymptoteSlip = 10.0f;
        m_forwardAsymptoteValue = 10.0f;
        m_forwardStiffness = 10.0f;
        m_sidewaysExtremumSlip = 1.0f;
        m_sidewaysExtrememValue = 1.0f;
        m_sidewaysAsymptoteSlip = 1.0f;
        m_sidewaysAsymptoteValue = 1.0f;
        m_sidewaysStiffness = 1.0f;
    }

    TireModel WheelController::getTireModel() const
    {
        if( m_wheelController )
        {
            return m_wheelController->getTireModel();
        }

        return m_tireModel;
    }

    void WheelController::setTireModel( TireModel tireModel )
    {
        m_tireModel = tireModel;

        if( m_wheelController )
        {
            m_wheelController->setTireModel( tireModel );
        }
    }

}  // namespace workphone::scene
