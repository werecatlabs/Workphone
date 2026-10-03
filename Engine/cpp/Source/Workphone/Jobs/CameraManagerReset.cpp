#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Jobs/CameraManagerReset.hpp>
#include <Workphone/Scene/CameraManager.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, CameraManagerReset, Job );

    CameraManagerReset::CameraManagerReset() = default;

    CameraManagerReset::~CameraManagerReset() = default;

    void CameraManagerReset::execute()
    {
        auto delayTime = getDelayTime();
        if( delayTime > 0.0f )
        {
            Thread::sleep( delayTime );
        }

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto cameraManager = applicationManager->getCameraManager();
        if( cameraManager )
        {
            cameraManager->reset();
        }
    }

    auto CameraManagerReset::getOwner() const -> ISharedObject *
    {
        return m_owner.get();
    }

    void CameraManagerReset::setOwner( ISharedObject *owner )
    {
        m_owner = owner;
    }

    auto CameraManagerReset::getDelayTime() const -> f32
    {
        return m_delayTime;
    }

    void CameraManagerReset::setDelayTime( f32 delayTime )
    {
        m_delayTime = delayTime;
    }

}  // namespace workphone
