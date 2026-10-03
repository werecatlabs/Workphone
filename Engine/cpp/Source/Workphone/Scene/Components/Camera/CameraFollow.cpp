#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Camera/CameraFollow.hpp>
#include <Workphone/Scene/Components/Camera/CameraTarget.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>

namespace workphone::scene
{
    const String CameraFollow::targetStr = String( "target" );
    const String CameraFollow::followObjectStr = String( "followObject" );

    WP_CLASS_REGISTER_DERIVED( workphone::scene, CameraFollow, Component );

    CameraFollow::CameraFollow() = default;

    CameraFollow::~CameraFollow() = default;

    void CameraFollow::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        Component::load( data );
        setLoadingState( LoadingState::Loaded );
    }

    void CameraFollow::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    auto CameraFollow::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = Component::getProperties();
        properties->setPropertyAsType( CameraFollow::targetStr, m_target );
        properties->setPropertyAsType( CameraFollow::followObjectStr, m_followObject );
        return properties;
    }

    void CameraFollow::setProperties( SmartPtr<Properties> properties )
    {
        Component::setProperties( properties );

        properties->getPropertyAsType( CameraFollow::targetStr, m_target );
        properties->getPropertyAsType( CameraFollow::followObjectStr, m_followObject );
    }

    auto CameraFollow::getTarget() const -> SmartPtr<CameraTarget>
    {
        return m_target;
    }

    void CameraFollow::setTarget( SmartPtr<CameraTarget> target )
    {
        m_target = target;
    }

    auto CameraFollow::getFollowObject() const -> SmartPtr<IGameActor>
    {
        return m_followObject;
    }

    void CameraFollow::setFollowObject( SmartPtr<IGameActor> followObject )
    {
        m_followObject = followObject;
    }
}  // namespace workphone::scene
