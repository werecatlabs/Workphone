#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Camera/CameraTarget.hpp>

namespace workphone::scene
{
    const String CameraTarget::offsetPositionStr = String( "offsetPosition" );
    const String CameraTarget::offsetRotationStr = String( "offsetRotation" );

    WP_CLASS_REGISTER_DERIVED( workphone::scene, CameraTarget, Component );

    CameraTarget::CameraTarget() = default;

    CameraTarget::~CameraTarget() = default;

    void CameraTarget::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        Component::load( data );
        setLoadingState( LoadingState::Loaded );
    }

    void CameraTarget::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    auto CameraTarget::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = Component::getProperties();
        properties->setProperty( CameraTarget::offsetPositionStr, m_offsetPosition );
        properties->setProperty( CameraTarget::offsetRotationStr, m_offsetRotation );
        return properties;
    }

    void CameraTarget::setProperties( SmartPtr<Properties> properties )
    {
        Component::setProperties( properties );

        properties->getPropertyValue( CameraTarget::offsetPositionStr, m_offsetPosition );
        properties->getPropertyValue( CameraTarget::offsetRotationStr, m_offsetRotation );
    }

    auto CameraTarget::getOffsetPosition() const -> Vector3<real_Num>
    {
        return m_offsetPosition;
    }

    void CameraTarget::setOffsetPosition( const Vector3<real_Num> &offsetPosition )
    {
        m_offsetPosition = offsetPosition;
    }

    auto CameraTarget::getOffsetRotation() const -> Quaternion<real_Num>
    {
        return m_offsetRotation;
    }

    void CameraTarget::setOffsetRotation( const Quaternion<real_Num> &offsetRotation )
    {
        m_offsetRotation = offsetRotation;
    }
}  // namespace workphone::scene
